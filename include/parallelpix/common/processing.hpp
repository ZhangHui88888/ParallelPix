#pragma once

#include "parallelpix/common/image.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace parallelpix {

struct ProcessingConfig
{
    std::uint32_t output_width = 1024;
    std::uint32_t output_height = 1024;
    double brightness_factor = 1.10;
    double watermark_opacity = 0.35;
    std::uint32_t watermark_margin = 32;
};

struct CropRegion
{
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

enum class ProcessingIssueCode
{
    InvalidConfig,
    EmptyBatch,
    InvalidImage,
    InvalidWatermark,
    WatermarkDoesNotFit,
};

struct ProcessingIssue
{
    ProcessingIssueCode code = ProcessingIssueCode::InvalidConfig;
    std::optional<std::size_t> image_index;
    std::filesystem::path source_path;
    std::string message;
};

struct BatchProcessingResult
{
    std::optional<std::vector<Image>> images;
    std::vector<ProcessingIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return images.has_value();
    }
};

[[nodiscard]] bool is_valid_processing_config(
    const ProcessingConfig& config) noexcept;

[[nodiscard]] std::optional<CropRegion> compute_center_crop(
    std::uint32_t input_width,
    std::uint32_t input_height,
    std::uint32_t output_width,
    std::uint32_t output_height) noexcept;

[[nodiscard]] std::uint8_t round_and_clamp(double value) noexcept;

[[nodiscard]] std::uint8_t sample_bilinear(
    const Image& input,
    const CropRegion& crop,
    std::uint32_t output_x,
    std::uint32_t output_y,
    std::uint32_t output_width,
    std::uint32_t output_height,
    std::uint32_t channel) noexcept;

[[nodiscard]] std::uint8_t apply_brightness(
    std::uint8_t value,
    double brightness_factor) noexcept;

[[nodiscard]] std::uint8_t blend_watermark_channel(
    std::uint8_t base,
    std::uint8_t watermark,
    std::uint8_t alpha,
    double global_opacity) noexcept;

}  // namespace parallelpix
