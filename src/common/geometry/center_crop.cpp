#include "parallelpix/common/processing.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace parallelpix {

bool is_valid_processing_config(
    const ProcessingConfig& config) noexcept
{
    if (config.output_width == 0 || config.output_height == 0 ||
        !std::isfinite(config.brightness_factor) ||
        config.brightness_factor <= 0.0 ||
        !std::isfinite(config.watermark_opacity) ||
        config.watermark_opacity < 0.0 ||
        config.watermark_opacity > 1.0)
    {
        return false;
    }

    constexpr auto max_size = std::numeric_limits<std::size_t>::max();
    const auto width = static_cast<std::size_t>(config.output_width);
    const auto height = static_cast<std::size_t>(config.output_height);
    if (width > max_size / image_channel_count)
    {
        return false;
    }

    const auto stride = width * image_channel_count;
    return height <= max_size / stride;
}

std::optional<CropRegion> compute_center_crop(
    std::uint32_t input_width,
    std::uint32_t input_height,
    std::uint32_t output_width,
    std::uint32_t output_height) noexcept
{
    if (input_width == 0 || input_height == 0 ||
        output_width == 0 || output_height == 0)
    {
        return std::nullopt;
    }

    CropRegion crop{0, 0, input_width, input_height};
    const auto input_scaled_width =
        static_cast<std::uint64_t>(input_width) * output_height;
    const auto input_scaled_height =
        static_cast<std::uint64_t>(input_height) * output_width;

    if (input_scaled_width > input_scaled_height)
    {
        const auto computed_width =
            static_cast<std::uint64_t>(input_height) * output_width /
            output_height;
        crop.width = static_cast<std::uint32_t>(
            std::max<std::uint64_t>(1, computed_width));
        crop.x = (input_width - crop.width) / 2;
    }
    else if (input_scaled_width < input_scaled_height)
    {
        const auto computed_height =
            static_cast<std::uint64_t>(input_width) * output_height /
            output_width;
        crop.height = static_cast<std::uint32_t>(
            std::max<std::uint64_t>(1, computed_height));
        crop.y = (input_height - crop.height) / 2;
    }

    return crop;
}

}  // namespace parallelpix
