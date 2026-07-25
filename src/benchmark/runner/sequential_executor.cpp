#include "parallelpix/benchmark/backend.hpp"

#include "parallelpix/sequential/processor.hpp"

#include <memory>

namespace parallelpix::benchmark {
namespace {

class SequentialExecutor final : public IBackendExecutor
{
public:
    [[nodiscard]] m2::Backend backend() const noexcept override
    {
        return m2::Backend::Sequential;
    }

    BackendExecution execute(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        const m2::ExperimentSpec&) override
    {
        return {
            sequential::process_batch(images, watermark, config),
            std::nullopt,
        };
    }
};

}  // namespace

std::unique_ptr<IBackendExecutor> make_sequential_executor()
{
    return std::make_unique<SequentialExecutor>();
}

}  // namespace parallelpix::benchmark
