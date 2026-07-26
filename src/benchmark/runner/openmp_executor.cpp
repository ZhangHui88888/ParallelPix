#include "parallelpix/benchmark/backend.hpp"

#include "parallelpix/openmp/processor.hpp"

#include <memory>

namespace parallelpix::benchmark {
namespace {

class OpenMpExecutor final : public IBackendExecutor
{
public:
    [[nodiscard]] m2::Backend backend() const noexcept override
    {
        return m2::Backend::OpenMP;
    }

    BackendExecution execute(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        const m2::ExperimentSpec& experiment,
        const ProgressSink& progress) override
    {
        const auto thread_count =
            experiment.thread_count.value_or(0);
        return {
            openmp::process_batch(
                images,
                watermark,
                config,
                thread_count,
                [&progress](std::size_t processed, double ms_per_image) {
                    if (progress)
                    {
                        progress({processed, ms_per_image});
                    }
                }),
            std::nullopt,
        };
    }
};

}  // namespace

std::unique_ptr<IBackendExecutor> make_openmp_executor()
{
    return std::make_unique<OpenMpExecutor>();
}

}  // namespace parallelpix::benchmark
