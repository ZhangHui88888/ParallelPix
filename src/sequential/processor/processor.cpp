#include "parallelpix/sequential/processor.hpp"
#include "process_image.hpp"

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

namespace parallelpix::sequential {

BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const BatchProgressSink& progress)
{
    BatchProcessingResult result;
    auto preparation =
        prepare_processing_batch(images, watermark, config);
    if (!preparation.ok())
    {
        result.issues = std::move(preparation.issues);
        return result;
    }

    std::vector<Image> outputs;
    outputs.reserve(images.size());
    constexpr std::size_t progress_interval = 10;
    double interval_ms = 0.0;
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto started_at = std::chrono::steady_clock::now();
        outputs.push_back(process_image(
            images[index],
            watermark,
            config,
            (*preparation.crops)[index]));
        interval_ms += std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - started_at)
                           .count();
        const auto processed = index + 1;
        if (progress &&
            (processed % progress_interval == 0 || processed == images.size()))
        {
            const auto interval_count = processed % progress_interval == 0
                ? progress_interval
                : processed % progress_interval;
            progress(processed, interval_ms / static_cast<double>(interval_count));
            interval_ms = 0.0;
        }
    }

    result.images = std::move(outputs);
    return result;
}

}  // namespace parallelpix::sequential
