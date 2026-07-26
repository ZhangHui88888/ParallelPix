#pragma once

#include <cstddef>
#include <cstdint>

namespace parallelpix::cuda::detail {

struct DeviceImageDescriptor
{
    std::uint64_t input_offset = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t stride = 0;
    std::uint32_t crop_x = 0;
    std::uint32_t crop_y = 0;
    std::uint32_t crop_width = 0;
    std::uint32_t crop_height = 0;
};

struct DeviceProcessingConfig
{
    std::uint32_t output_width = 0;
    std::uint32_t output_height = 0;
    double brightness_factor = 1.0;
    double watermark_opacity = 0.0;
    std::uint32_t watermark_width = 0;
    std::uint32_t watermark_height = 0;
    std::uint64_t watermark_stride = 0;
    std::uint32_t watermark_margin = 0;
};

}  // namespace parallelpix::cuda::detail
