#pragma once

#include "parallelpix/sequential/processor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

namespace parallelpix::test::sequential {

inline Image make_image(
    std::uint32_t width,
    std::uint32_t height,
    std::vector<std::uint8_t> pixels,
    const std::filesystem::path& source = "input")
{
    return {
        width,
        height,
        image_channel_count,
        static_cast<std::size_t>(width) * image_channel_count,
        source,
        std::move(pixels),
    };
}

inline Image make_solid_image(
    std::uint32_t width,
    std::uint32_t height,
    std::uint8_t value,
    const std::filesystem::path& source = "input")
{
    return make_image(
        width,
        height,
        std::vector<std::uint8_t>(
            static_cast<std::size_t>(width) * height *
                image_channel_count,
            value),
        source);
}

inline Watermark make_watermark(
    std::uint32_t width,
    std::uint32_t height,
    std::uint8_t color,
    std::uint8_t alpha)
{
    Watermark watermark;
    watermark.color = make_solid_image(
        width,
        height,
        color,
        "watermark");
    watermark.alpha.assign(
        static_cast<std::size_t>(width) * height,
        alpha);
    return watermark;
}

inline bool has_issue(
    const std::vector<ProcessingIssue>& issues,
    ProcessingIssueCode code)
{
    return std::any_of(
        issues.begin(),
        issues.end(),
        [code](const auto& issue) {
            return issue.code == code;
        });
}

inline ProcessingConfig small_config()
{
    ProcessingConfig config;
    config.output_width = 2;
    config.output_height = 2;
    config.brightness_factor = 1.0;
    config.watermark_opacity = 0.0;
    config.watermark_margin = 0;
    return config;
}

}  // namespace parallelpix::test::sequential
