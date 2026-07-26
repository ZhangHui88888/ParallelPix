#include "parallelpix/common/processing.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix {
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

BatchPreparationResult prepare_processing_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config)
{
    BatchPreparationResult result;
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

    if (result.issues.empty())
    {
        result.crops = std::move(crops);
    }
    return result;
}

}  // namespace parallelpix
