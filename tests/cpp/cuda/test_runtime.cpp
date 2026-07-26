#include "parallelpix/cuda/processor.hpp"
#include "runtime/runtime.hpp"
#include "test_support.hpp"

#include <cuda_runtime_api.h>

#include <cstddef>

PP_TEST("CUDA runtime exposes a usable device and reusable allocation")
{
    const auto status =
        parallelpix::cuda::detail::query_device_status();
    PP_REQUIRE(status.available);
    PP_REQUIRE(status.max_grid_z > 0);
    PP_REQUIRE(!status.message.empty());

    parallelpix::cuda::detail::DeviceBuffer buffer;
    buffer.reserve(16);
    PP_REQUIRE(buffer.data() != nullptr);
    PP_REQUIRE(buffer.capacity() >= std::size_t{16});
    const auto* original = buffer.data();
    buffer.reserve(8);
    PP_REQUIRE_EQ(buffer.data(), original);
}

PP_TEST("CUDA runtime allocation failure hook reports memory allocation")
{
    parallelpix::cuda::detail::set_allocation_limit_for_testing(8);
    bool caught = false;
    try
    {
        parallelpix::cuda::detail::DeviceBuffer buffer;
        buffer.reserve(16);
    }
    catch (const parallelpix::cuda::detail::RuntimeError& error)
    {
        caught = error.code() == cudaErrorMemoryAllocation;
    }
    parallelpix::cuda::detail::reset_allocation_limit_for_testing();
    PP_REQUIRE(caught);
}
