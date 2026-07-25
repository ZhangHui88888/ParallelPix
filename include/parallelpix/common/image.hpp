#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace parallelpix {

inline constexpr std::uint32_t image_channel_count = 3;

struct Image
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = image_channel_count;
    std::size_t stride = 0;
    std::filesystem::path source_path;
    std::vector<std::uint8_t> pixels;
};

struct Watermark
{
    Image color;
    std::vector<std::uint8_t> alpha;
};

inline bool is_valid_image(const Image& image) noexcept
{
    if (image.width == 0 || image.height == 0 ||
        image.channels != image_channel_count)
    {
        return false;
    }

    constexpr auto max_size = std::numeric_limits<std::size_t>::max();
    const auto width = static_cast<std::size_t>(image.width);
    const auto height = static_cast<std::size_t>(image.height);
    const auto channels = static_cast<std::size_t>(image.channels);

    if (width > max_size / channels)
    {
        return false;
    }

    const auto expected_stride = width * channels;
    if (image.stride != expected_stride ||
        height > max_size / expected_stride)
    {
        return false;
    }

    return image.pixels.size() == expected_stride * height;
}

inline bool is_valid_watermark(const Watermark& watermark) noexcept
{
    if (!is_valid_image(watermark.color))
    {
        return false;
    }

    constexpr auto max_size = std::numeric_limits<std::size_t>::max();
    const auto width = static_cast<std::size_t>(watermark.color.width);
    const auto height = static_cast<std::size_t>(watermark.color.height);
    if (height > max_size / width)
    {
        return false;
    }

    return watermark.alpha.size() == width * height;
}

}  // namespace parallelpix
