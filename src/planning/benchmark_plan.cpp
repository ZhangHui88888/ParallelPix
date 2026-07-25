#include "parallelpix/planning/benchmark_plan.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace parallelpix::m2 {
namespace {

template <typename Value>
std::vector<Value> unique_preserving_order(const std::vector<Value>& values)
{
    std::vector<Value> result;
    for (const auto& value : values)
    {
        if (std::find(result.begin(), result.end(), value) == result.end())
        {
            result.push_back(value);
        }
    }
    return result;
}

bool contains_backend(const std::vector<Backend>& backends, Backend backend)
{
    return std::find(backends.begin(), backends.end(), backend) != backends.end();
}

std::vector<Backend> normalize_backends(const std::vector<Backend>& requested)
{
    const bool has_openmp = contains_backend(requested, Backend::OpenMP);
    const bool has_cuda = contains_backend(requested, Backend::Cuda);
    const bool has_sequential =
        contains_backend(requested, Backend::Sequential) || has_openmp || has_cuda;

    std::vector<Backend> normalized;
    if (has_sequential)
    {
        normalized.push_back(Backend::Sequential);
    }
    if (has_openmp)
    {
        normalized.push_back(Backend::OpenMP);
    }
    if (has_cuda)
    {
        normalized.push_back(Backend::Cuda);
    }
    return normalized;
}

}  // namespace

BenchmarkPlan build_benchmark_plan(const BenchmarkRequest& request)
{
    BenchmarkPlan plan{
        request.input_dir,
        request.output_dir,
        request.watermark_path,
        request.result_csv,
        {},
        request.warmups,
        request.repetitions,
        request.csv_mode,
    };

    const auto backends = normalize_backends(request.backends);
    const auto image_counts = unique_preserving_order(request.image_counts);
    const auto thread_counts = unique_preserving_order(request.thread_counts);
    const auto cuda_batch_sizes = unique_preserving_order(request.cuda_batch_sizes);

    for (const auto backend : backends)
    {
        for (const auto image_count : image_counts)
        {
            if (backend == Backend::Sequential)
            {
                plan.experiments.push_back(
                    {backend, image_count, std::nullopt, std::nullopt});
                continue;
            }

            if (backend == Backend::OpenMP)
            {
                for (const auto thread_count : thread_counts)
                {
                    plan.experiments.push_back(
                        {backend, image_count, thread_count, std::nullopt});
                }
                continue;
            }

            for (const auto batch_size : cuda_batch_sizes)
            {
                plan.experiments.push_back(
                    {backend, image_count, std::nullopt, batch_size});
            }
        }
    }

    return plan;
}

}  // namespace parallelpix::m2
