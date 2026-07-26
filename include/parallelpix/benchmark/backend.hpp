#pragma once

#include "parallelpix/common/processing.hpp"
#include "parallelpix/planning/benchmark_plan.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace parallelpix::benchmark {

struct CudaPhaseTiming
{
    double h2d_ms = 0.0;
    double kernel_ms = 0.0;
    double d2h_ms = 0.0;
};

struct BackendExecution
{
    BatchProcessingResult processing;
    std::optional<CudaPhaseTiming> cuda_phase;
};

struct ProgressSample
{
    std::size_t processed_images = 0;
    double batch_ms_per_image = 0.0;
};

using ProgressSink = std::function<void(const ProgressSample&)>;

class IBackendExecutor
{
public:
    virtual ~IBackendExecutor() = default;

    [[nodiscard]] virtual m2::Backend backend() const noexcept = 0;

    virtual BackendExecution execute(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        const m2::ExperimentSpec& experiment,
        const ProgressSink& progress = {}) = 0;
};

std::unique_ptr<IBackendExecutor> make_sequential_executor();

}  // namespace parallelpix::benchmark
