#include "kernels/launch.hpp"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace parallelpix::cuda::detail {
namespace {

__device__ std::uint8_t round_and_clamp_device(double value)
{
    if (!isfinite(value) || value <= 0.0)
    {
        return 0;
    }
    if (value >= 255.0)
    {
        return 255;
    }
    return static_cast<std::uint8_t>(floor(value + 0.5));
}

__device__ std::uint8_t pixel_at(
    const std::uint8_t* input,
    const DeviceImageDescriptor& image,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t channel)
{
    const auto index = image.input_offset +
        static_cast<std::uint64_t>(y) * image.stride +
        static_cast<std::uint64_t>(x) * 3U +
        channel;
    return input[index];
}

__device__ std::uint8_t sample_bilinear_device(
    const std::uint8_t* input,
    const DeviceImageDescriptor& image,
    std::uint32_t output_x,
    std::uint32_t output_y,
    std::uint32_t output_width,
    std::uint32_t output_height,
    std::uint32_t channel)
{
    const auto scale_x =
        static_cast<double>(image.crop_width) / output_width;
    const auto scale_y =
        static_cast<double>(image.crop_height) / output_height;
    const auto crop_right = image.crop_x + image.crop_width - 1U;
    const auto crop_bottom = image.crop_y + image.crop_height - 1U;

    auto source_x = static_cast<double>(image.crop_x) +
        (static_cast<double>(output_x) + 0.5) * scale_x - 0.5;
    auto source_y = static_cast<double>(image.crop_y) +
        (static_cast<double>(output_y) + 0.5) * scale_y - 0.5;
    source_x = fmin(
        fmax(source_x, static_cast<double>(image.crop_x)),
        static_cast<double>(crop_right));
    source_y = fmin(
        fmax(source_y, static_cast<double>(image.crop_y)),
        static_cast<double>(crop_bottom));

    const auto x0 = static_cast<std::uint32_t>(floor(source_x));
    const auto y0 = static_cast<std::uint32_t>(floor(source_y));
    const auto x1 = x0 < crop_right ? x0 + 1U : crop_right;
    const auto y1 = y0 < crop_bottom ? y0 + 1U : crop_bottom;
    const auto weight_x = source_x - x0;
    const auto weight_y = source_y - y0;

    const auto top_left =
        static_cast<double>(pixel_at(input, image, x0, y0, channel));
    const auto top_right =
        static_cast<double>(pixel_at(input, image, x1, y0, channel));
    const auto bottom_left =
        static_cast<double>(pixel_at(input, image, x0, y1, channel));
    const auto bottom_right =
        static_cast<double>(pixel_at(input, image, x1, y1, channel));
    const auto top =
        top_left + (top_right - top_left) * weight_x;
    const auto bottom =
        bottom_left + (bottom_right - bottom_left) * weight_x;
    return round_and_clamp_device(
        top + (bottom - top) * weight_y);
}

__global__ void process_batch_kernel(
    const std::uint8_t* input,
    const DeviceImageDescriptor* descriptors,
    std::uint8_t* output,
    const std::uint8_t* watermark_color,
    const std::uint8_t* watermark_alpha,
    DeviceProcessingConfig config)
{
    const auto x =
        blockIdx.x * blockDim.x + threadIdx.x;
    const auto y =
        blockIdx.y * blockDim.y + threadIdx.y;
    const auto image_index = blockIdx.z;
    if (x >= config.output_width || y >= config.output_height)
    {
        return;
    }

    const auto& image = descriptors[image_index];
    const auto output_stride =
        static_cast<std::uint64_t>(config.output_width) * 3U;
    const auto output_image_bytes =
        output_stride * config.output_height;
    const auto output_index =
        static_cast<std::uint64_t>(image_index) * output_image_bytes +
        static_cast<std::uint64_t>(y) * output_stride +
        static_cast<std::uint64_t>(x) * 3U;

    const auto watermark_origin_x =
        config.output_width - config.watermark_margin -
        config.watermark_width;
    const auto watermark_origin_y =
        config.output_height - config.watermark_margin -
        config.watermark_height;
    const bool inside_watermark =
        x >= watermark_origin_x &&
        x < watermark_origin_x + config.watermark_width &&
        y >= watermark_origin_y &&
        y < watermark_origin_y + config.watermark_height;

    for (std::uint32_t channel = 0; channel < 3U; ++channel)
    {
        const auto resized = sample_bilinear_device(
            input,
            image,
            x,
            y,
            config.output_width,
            config.output_height,
            channel);
        auto value = round_and_clamp_device(
            static_cast<double>(resized) *
            config.brightness_factor);

        if (inside_watermark)
        {
            const auto watermark_x = x - watermark_origin_x;
            const auto watermark_y = y - watermark_origin_y;
            const auto watermark_pixel =
                static_cast<std::uint64_t>(watermark_y) *
                    config.watermark_width +
                watermark_x;
            const auto watermark_index =
                static_cast<std::uint64_t>(watermark_y) *
                    config.watermark_stride +
                static_cast<std::uint64_t>(watermark_x) * 3U +
                channel;
            const auto effective_alpha =
                static_cast<double>(
                    watermark_alpha[watermark_pixel]) /
                255.0 * config.watermark_opacity;
            value = round_and_clamp_device(
                static_cast<double>(value) *
                    (1.0 - effective_alpha) +
                static_cast<double>(
                    watermark_color[watermark_index]) *
                    effective_alpha);
        }
        output[output_index + channel] = value;
    }
}

}  // namespace

cudaError_t launch_process_batch(
    const std::uint8_t* input,
    const DeviceImageDescriptor* descriptors,
    std::uint8_t* output,
    std::uint32_t image_count,
    const std::uint8_t* watermark_color,
    const std::uint8_t* watermark_alpha,
    const DeviceProcessingConfig& config,
    cudaStream_t stream)
{
    constexpr std::uint32_t block_size = 16;
    const dim3 block(block_size, block_size, 1);
    const dim3 grid(
        (config.output_width + block_size - 1U) / block_size,
        (config.output_height + block_size - 1U) / block_size,
        image_count);
    process_batch_kernel<<<grid, block, 0, stream>>>(
        input,
        descriptors,
        output,
        watermark_color,
        watermark_alpha,
        config);
    return cudaGetLastError();
}

}  // namespace parallelpix::cuda::detail
