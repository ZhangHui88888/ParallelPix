#include "parallelpix/benchmark/backend.hpp"

#include "parallelpix/cuda/processor.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace parallelpix::benchmark {
namespace {

class CudaExecutor final : public IBackendExecutor
{
public:
    [[nodiscard]] m2::Backend backend() const noexcept override
    {
        return m2::Backend::Cuda;
    }

    [[nodiscard]] BackendAvailability availability() const override
    {
        const auto status = processor_.availability();
        return {
            status.available,
            status.available
                ? std::string{}
                : "CUDA backend is unavailable: " + status.message,
        };
    }

    BackendExecution execute(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        const m2::ExperimentSpec& experiment,
        const ProgressSink& progress) override
    {
        if (!experiment.cuda_batch_size ||
            *experiment.cuda_batch_size == 0)
        {
            BatchProcessingResult processing;
            processing.issues.push_back({
                ProcessingIssueCode::InvalidConfig,
                std::nullopt,
                {},
                "CUDA experiment requires a positive batch size.",
            });
            return {
                std::move(processing),
                std::nullopt,
                std::nullopt,
            };
        }

        auto processed = processor_.process_batch(
            images,
            watermark,
            config,
            *experiment.cuda_batch_size,
            [&progress](std::size_t completed, double ms_per_image) {
                if (progress)
                {
                    progress({completed, ms_per_image});
                }
            });

        BackendExecution execution;
        execution.processing = std::move(processed.processing);
        if (processed.phase_timing)
        {
            execution.cuda_phase = CudaPhaseTiming{
                processed.phase_timing->h2d_ms,
                processed.phase_timing->kernel_ms,
                processed.phase_timing->d2h_ms,
            };
        }
        if (processed.effective_batch_size > 0)
        {
            execution.effective_cuda_batch_size =
                processed.effective_batch_size;
        }
        return execution;
    }

private:
    cuda::Processor processor_;
};

}  // namespace

std::unique_ptr<IBackendExecutor> make_cuda_executor()
{
    return std::make_unique<CudaExecutor>();
}

}  // namespace parallelpix::benchmark
