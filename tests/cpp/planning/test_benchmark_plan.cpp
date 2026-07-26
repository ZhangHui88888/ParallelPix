#include "../test_support.hpp"

#include "parallelpix/planning/benchmark_plan.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace {

using parallelpix::m2::Backend;
using parallelpix::m2::BenchmarkRequest;
using parallelpix::m2::CsvMode;
using parallelpix::m2::ExperimentSpec;
using parallelpix::m2::build_benchmark_plan;

BenchmarkRequest default_request()
{
    return {
        std::filesystem::path("data/images"),
        std::filesystem::path("output"),
        std::filesystem::path("data/watermark.png"),
        std::filesystem::path("results/benchmark.csv"),
        {Backend::Sequential, Backend::OpenMP, Backend::Cuda},
        {10, 50, 100},
        {1, 2, 4, 8},
        {1, 4, 8},
        2,
        5,
        false,
        CsvMode::Append,
    };
}

std::size_t count_backend(
    const std::vector<ExperimentSpec>& experiments,
    Backend backend)
{
    std::size_t count = 0;
    for (const auto& experiment : experiments)
    {
        if (experiment.backend == backend)
        {
            ++count;
        }
    }
    return count;
}

PP_TEST("the M1 default request expands into 24 backend-specific experiments")
{
    const auto plan = build_benchmark_plan(default_request());

    PP_REQUIRE_EQ(plan.experiments.size(), std::size_t(24));
    PP_REQUIRE_EQ(count_backend(plan.experiments, Backend::Sequential), std::size_t(3));
    PP_REQUIRE_EQ(count_backend(plan.experiments, Backend::OpenMP), std::size_t(12));
    PP_REQUIRE_EQ(count_backend(plan.experiments, Backend::Cuda), std::size_t(9));
    PP_REQUIRE_EQ(plan.csv_mode, CsvMode::Append);
    PP_REQUIRE_EQ(plan.warmups, std::uint32_t(2));
    PP_REQUIRE_EQ(plan.repetitions, std::uint32_t(5));
}

PP_TEST("parallel-only input gains a sequential baseline and canonical backend order")
{
    auto request = default_request();
    request.backends = {Backend::Cuda, Backend::OpenMP};
    request.image_counts = {10};
    request.thread_counts = {2};
    request.cuda_batch_sizes = {4};

    const auto plan = build_benchmark_plan(request);

    PP_REQUIRE_EQ(plan.experiments.size(), std::size_t(3));
    PP_REQUIRE_EQ(plan.experiments[0].backend, Backend::Sequential);
    PP_REQUIRE_EQ(plan.experiments[1].backend, Backend::OpenMP);
    PP_REQUIRE_EQ(plan.experiments[2].backend, Backend::Cuda);
}

PP_TEST("numeric matrix values are deduplicated while preserving first occurrence")
{
    auto request = default_request();
    request.image_counts = {50, 10, 50};
    request.thread_counts = {4, 2, 4};
    request.cuda_batch_sizes = {8, 1, 8};

    const auto plan = build_benchmark_plan(request);

    PP_REQUIRE_EQ(plan.experiments.size(), std::size_t(10));
    PP_REQUIRE_EQ(plan.experiments[0].image_count, std::uint32_t(50));
    PP_REQUIRE_EQ(plan.experiments[1].image_count, std::uint32_t(10));

    PP_REQUIRE_EQ(plan.experiments[2].backend, Backend::OpenMP);
    PP_REQUIRE_EQ(plan.experiments[2].image_count, std::uint32_t(50));
    PP_REQUIRE_EQ(plan.experiments[2].thread_count, std::optional<std::uint32_t>(4));
    PP_REQUIRE_EQ(plan.experiments[3].thread_count, std::optional<std::uint32_t>(2));
}

PP_TEST("each experiment exposes only parameters used by its backend")
{
    auto request = default_request();
    request.image_counts = {10};
    request.thread_counts = {2};
    request.cuda_batch_sizes = {4};

    const auto plan = build_benchmark_plan(request);

    PP_REQUIRE(!plan.experiments[0].thread_count.has_value());
    PP_REQUIRE(!plan.experiments[0].cuda_batch_size.has_value());
    PP_REQUIRE_EQ(plan.experiments[1].thread_count, std::optional<std::uint32_t>(2));
    PP_REQUIRE(!plan.experiments[1].cuda_batch_size.has_value());
    PP_REQUIRE(!plan.experiments[2].thread_count.has_value());
    PP_REQUIRE_EQ(plan.experiments[2].cuda_batch_size, std::optional<std::uint32_t>(4));
}

PP_TEST("unused backend lists do not create experiments")
{
    auto request = default_request();
    request.backends = {Backend::Sequential};
    request.image_counts = {10, 50};

    const auto plan = build_benchmark_plan(request);

    PP_REQUIRE_EQ(plan.experiments.size(), std::size_t(2));
    PP_REQUIRE_EQ(count_backend(plan.experiments, Backend::OpenMP), std::size_t(0));
    PP_REQUIRE_EQ(count_backend(plan.experiments, Backend::Cuda), std::size_t(0));
}

}  // namespace
