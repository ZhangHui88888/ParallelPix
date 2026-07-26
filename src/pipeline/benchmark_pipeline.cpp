#include "parallelpix/pipeline/pipeline_factory.hpp"

#include "parallelpix/benchmark/backend.hpp"
#include "parallelpix/benchmark/runner.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace parallelpix::m2 {
namespace {

class BenchmarkPipeline final : public IBenchmarkPipeline
{
public:
    BenchmarkPipeline()
    {
        executors_.push_back(benchmark::make_sequential_executor());
        executors_.push_back(benchmark::make_openmp_executor());
        auto cuda = benchmark::make_cuda_executor();
        if (cuda)
        {
            executors_.push_back(std::move(cuda));
        }
    }

    WorkflowSummary execute(
        const BenchmarkPlan& plan,
        const LogSink& log) override
    {
        std::vector<benchmark::IBackendExecutor*> views;
        views.reserve(executors_.size());
        for (const auto& executor : executors_)
        {
            views.push_back(executor.get());
        }
        return benchmark::run_benchmark_plan(plan, views, log);
    }

private:
    std::vector<std::unique_ptr<benchmark::IBackendExecutor>> executors_;
};

}  // namespace

std::unique_ptr<IBenchmarkPipeline> make_benchmark_pipeline()
{
    return std::make_unique<BenchmarkPipeline>();
}

}  // namespace parallelpix::m2
