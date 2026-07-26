#include "parallelpix/cuda/processor.hpp"
#include "processing_test_support.hpp"
#include "runtime/runtime.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using parallelpix::test::sequential::has_issue;
using parallelpix::test::sequential::make_solid_image;
using parallelpix::test::sequential::make_watermark;
using parallelpix::test::sequential::small_config;

class AllocationLimitGuard
{
public:
    explicit AllocationLimitGuard(std::size_t bytes)
    {
        parallelpix::cuda::detail::set_allocation_limit_for_testing(bytes);
    }

    ~AllocationLimitGuard()
    {
        parallelpix::cuda::detail::reset_allocation_limit_for_testing();
    }

    AllocationLimitGuard(const AllocationLimitGuard&) = delete;
    AllocationLimitGuard& operator=(const AllocationLimitGuard&) = delete;
};

class RuntimeFaultGuard
{
public:
    explicit RuntimeFaultGuard(
        parallelpix::cuda::detail::RuntimeFault fault)
    {
        parallelpix::cuda::detail::set_runtime_fault_for_testing(fault);
    }

    ~RuntimeFaultGuard()
    {
        parallelpix::cuda::detail::reset_runtime_fault_for_testing();
    }

    RuntimeFaultGuard(const RuntimeFaultGuard&) = delete;
    RuntimeFaultGuard& operator=(const RuntimeFaultGuard&) = delete;
};

}  // namespace

PP_TEST("CUDA processor exposes an injected no device state")
{
    RuntimeFaultGuard fault(
        parallelpix::cuda::detail::RuntimeFault::NoDevice);
    parallelpix::cuda::Processor processor;

    const auto availability = processor.availability();
    PP_REQUIRE(!availability.available);
    PP_REQUIRE(
        availability.message.find("device discovery failure") !=
        std::string::npos);
}

PP_TEST("CUDA processor reports injected API and Kernel failures")
{
    const auto image = make_solid_image(2, 2, 20);
    const auto watermark = make_watermark(1, 1, 10, 255);
    for (const auto fault_type : {
             parallelpix::cuda::detail::RuntimeFault::ApiFailure,
             parallelpix::cuda::detail::RuntimeFault::KernelFailure,
         })
    {
        parallelpix::cuda::Processor processor;
        PP_REQUIRE(processor.availability().available);
        RuntimeFaultGuard fault(fault_type);
        const auto result =
            processor.process_batch({image}, watermark, small_config(), 1);

        PP_REQUIRE(!result.processing.ok());
        PP_REQUIRE(!result.phase_timing.has_value());
        PP_REQUIRE(has_issue(
            result.processing.issues,
            parallelpix::ProcessingIssueCode::BackendFailure));
    }
}

PP_TEST("CUDA processor retries once with half the requested batch")
{
    parallelpix::ProcessingConfig config;
    config.output_width = 8;
    config.output_height = 8;
    config.brightness_factor = 1.0;
    config.watermark_opacity = 0.0;
    config.watermark_margin = 0;
    const auto watermark = make_watermark(1, 1, 0, 0);
    const std::vector<parallelpix::Image> inputs{
        make_solid_image(1, 1, 10, "one"),
        make_solid_image(1, 1, 20, "two"),
        make_solid_image(1, 1, 30, "three"),
        make_solid_image(1, 1, 40, "four"),
    };

    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    AllocationLimitGuard limit(400);
    const auto result =
        processor.process_batch(inputs, watermark, config, 4);

    PP_REQUIRE(result.processing.ok());
    PP_REQUIRE_EQ(result.effective_batch_size, std::uint32_t{2});
    PP_REQUIRE_EQ(result.processing.images->size(), inputs.size());
}

PP_TEST("CUDA processor reports a second allocation failure without metrics")
{
    auto config = small_config();
    config.output_width = 8;
    config.output_height = 8;
    const auto watermark = make_watermark(1, 1, 0, 0);
    const std::vector<parallelpix::Image> inputs{
        make_solid_image(1, 1, 10),
        make_solid_image(1, 1, 20),
    };

    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    AllocationLimitGuard limit(100);
    const auto result =
        processor.process_batch(inputs, watermark, config, 2);

    PP_REQUIRE(!result.processing.ok());
    PP_REQUIRE(!result.phase_timing.has_value());
    PP_REQUIRE_EQ(result.effective_batch_size, std::uint32_t{1});
    PP_REQUIRE(has_issue(
        result.processing.issues,
        parallelpix::ProcessingIssueCode::BackendFailure));
}

PP_TEST("CUDA allocation retry discards progress from the failed attempt")
{
    auto config = small_config();
    config.output_width = 8;
    config.output_height = 8;
    const auto watermark = make_watermark(1, 1, 0, 0);
    std::vector<parallelpix::Image> inputs;
    for (std::uint8_t value = 1; value <= 4; ++value)
    {
        inputs.push_back(make_solid_image(1, 1, value));
    }
    for (std::uint8_t value = 5; value <= 8; ++value)
    {
        inputs.push_back(make_solid_image(10, 9, value));
    }

    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    AllocationLimitGuard limit(800);
    std::vector<std::size_t> progress;
    const auto result = processor.process_batch(
        inputs,
        watermark,
        config,
        4,
        [&](std::size_t completed, double) {
            progress.push_back(completed);
        });

    PP_REQUIRE(result.processing.ok());
    PP_REQUIRE_EQ(result.effective_batch_size, std::uint32_t{2});
    PP_REQUIRE_EQ(
        progress,
        std::vector<std::size_t>({2, 4, 6, 8}));
}
