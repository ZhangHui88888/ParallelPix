#include "parallelpix/common/processing.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace parallelpix {
namespace {

std::uint8_t pixel_at(
    const Image& image,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t channel) noexcept
{
    const auto index =
        static_cast<std::size_t>(y) * image.stride +
        static_cast<std::size_t>(x) * image.channels +
        static_cast<std::size_t>(channel);
    return image.pixels[index];
}

}  // namespace

std::uint8_t sample_bilinear(
    const Image& input,
    const CropRegion& crop,
    std::uint32_t output_x,
    std::uint32_t output_y,
    std::uint32_t output_width,
    std::uint32_t output_height,
    std::uint32_t channel) noexcept
{
    const auto scale_x =
        static_cast<double>(crop.width) / output_width;
    const auto scale_y =
        static_cast<double>(crop.height) / output_height;

    const auto crop_right = static_cast<std::uint64_t>(crop.x) +
        crop.width - 1;
    const auto crop_bottom = static_cast<std::uint64_t>(crop.y) +
        crop.height - 1;
    const auto min_x = static_cast<double>(crop.x);
    const auto min_y = static_cast<double>(crop.y);
    const auto max_x = static_cast<double>(crop_right);
    const auto max_y = static_cast<double>(crop_bottom);

    const auto source_x = std::clamp(
        static_cast<double>(crop.x) +
            (static_cast<double>(output_x) + 0.5) * scale_x - 0.5,
        min_x,
        max_x);
    const auto source_y = std::clamp(
        static_cast<double>(crop.y) +
            (static_cast<double>(output_y) + 0.5) * scale_y - 0.5,
        min_y,
        max_y);

    const auto x0 = static_cast<std::uint32_t>(std::floor(source_x));
    const auto y0 = static_cast<std::uint32_t>(std::floor(source_y));
    const auto x1 = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(
            static_cast<std::uint64_t>(x0) + 1,
            crop_right));
    const auto y1 = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(
            static_cast<std::uint64_t>(y0) + 1,
            crop_bottom));
    const auto weight_x = source_x - x0;
    const auto weight_y = source_y - y0;

    const auto top_left =
        static_cast<double>(pixel_at(input, x0, y0, channel));
    const auto top_right =
        static_cast<double>(pixel_at(input, x1, y0, channel));
    const auto bottom_left =
        static_cast<double>(pixel_at(input, x0, y1, channel));
    const auto bottom_right =
        static_cast<double>(pixel_at(input, x1, y1, channel));

    const auto top =
        top_left + (top_right - top_left) * weight_x;
    const auto bottom =
        bottom_left + (bottom_right - bottom_left) * weight_x;
    return round_and_clamp(top + (bottom - top) * weight_y);
}

}  // namespace parallelpix
