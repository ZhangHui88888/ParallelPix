#include "parallelpix/sequential/processor.hpp"
#include "process_image.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::sequential {
namespace {

ProcessingIssue make_issue(
    ProcessingIssueCode code,
    std::optional<std::size_t> image_index,
    const std::filesystem::path& source_path,
    std::string message)
{
    return {
        code,
        image_index,
        source_path,
        std::move(message),
    };
}

}  // namespace

BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const BatchProgressSink& progress)
{
    BatchProcessingResult result;
    if (!is_valid_processing_config(config))
    {
        result.issues.push_back(make_issue(
            ProcessingIssueCode::InvalidConfig,
            std::nullopt,
            {},
            "Processing configuration is invalid."));
        return result;
    }
    if (images.empty())
    {
        result.issues.push_back(make_issue(
            ProcessingIssueCode::EmptyBatch,
            std::nullopt,
            {},
            "Image batch must contain at least one image."));
        return result;
    }
    if (!is_valid_watermark(watermark))
    {
        result.issues.push_back(make_issue(
            ProcessingIssueCode::InvalidWatermark,
            std::nullopt,
            watermark.color.source_path,
            "Watermark does not satisfy the shared image invariants."));
        return result;
    }
    if (watermark.color.width > config.output_width ||
        config.watermark_margin >
            config.output_width - watermark.color.width ||
        watermark.color.height > config.output_height ||
        config.watermark_margin >
            config.output_height - watermark.color.height)
    {
        result.issues.push_back(make_issue(
            ProcessingIssueCode::WatermarkDoesNotFit,
            std::nullopt,
            watermark.color.source_path,
            "Watermark and its right/bottom margin do not fit the output."));
        return result;
    }

    std::vector<CropRegion> crops;
    crops.reserve(images.size());
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto& image = images[index];
        if (!is_valid_image(image))
        {
            result.issues.push_back(make_issue(
                ProcessingIssueCode::InvalidImage,
                index,
                image.source_path,
                "Input image does not satisfy the shared image invariants."));
            continue;
        }

        const auto crop = compute_center_crop(
            image.width,
            image.height,
            config.output_width,
            config.output_height);
        if (!crop.has_value())
        {
            result.issues.push_back(make_issue(
                ProcessingIssueCode::InvalidImage,
                index,
                image.source_path,
                "A center crop could not be computed for the input image."));
            continue;
        }
        crops.push_back(*crop);
    }

    if (!result.issues.empty())
    {
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
            crops[index]));
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
