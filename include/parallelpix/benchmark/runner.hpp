#pragma once

#include "parallelpix/benchmark/backend.hpp"
#include "parallelpix/controller/controller.hpp"

#include <vector>

namespace parallelpix::benchmark {

m2::WorkflowSummary run_benchmark_plan(
    const m2::BenchmarkPlan& plan,
    const std::vector<IBackendExecutor*>& executors,
    const m2::LogSink& log);

}  // namespace parallelpix::benchmark
