#include "process_image.hpp"

#include <cstddef>
#include <cstdint>

namespace parallelpix::openmp::detail {
namespace {

void process_output_row(
    const Image& input,
    const ProcessingConfig& config,
    const CropRegion& crop,
    std::uint32_t y,
    Image& output) noexcept
{
    for (std::uint32_t x = 0; x < output.width; ++x)
    {
        const auto output_index =
            static_cast<std::size_t>(y) * output.stride +
            static_cast<std::size_t>(x) * output.channels;
        for (std::uint32_t channel = 0;
             channel < output.channels;
             ++channel)
        {
            const auto resized = sample_bilinear(
                input,
                crop,
                x,
                y,
                output.width,
                output.height,
                channel);
            output.pixels[output_index + channel] =
                apply_brightness(resized, config.brightness_factor);
        }
    }
}

void process_watermark_row(
    const Watermark& watermark,
    const ProcessingConfig& config,
    std::uint32_t watermark_y,
    Image& output) noexcept
{
    const auto origin_x =
        output.width - config.watermark_margin - watermark.color.width;
    const auto origin_y =
        output.height - config.watermark_margin - watermark.color.height;

    for (std::uint32_t watermark_x = 0;
         watermark_x < watermark.color.width;
         ++watermark_x)
    {
        const auto watermark_pixel =
            static_cast<std::size_t>(watermark_y) *
                watermark.color.width +
            watermark_x;
        const auto watermark_index =
            static_cast<std::size_t>(watermark_y) *
                watermark.color.stride +
            static_cast<std::size_t>(watermark_x) *
                watermark.color.channels;
        const auto output_index =
            static_cast<std::size_t>(origin_y + watermark_y) *
                output.stride +
            static_cast<std::size_t>(origin_x + watermark_x) *
                output.channels;

        for (std::uint32_t channel = 0;
             channel < output.channels;
             ++channel)
        {
            output.pixels[output_index + channel] =
                blend_watermark_channel(
                    output.pixels[output_index + channel],
                    watermark.color.pixels[watermark_index + channel],
                    watermark.alpha[watermark_pixel],
                    config.watermark_opacity);
        }
    }
}

}  // namespace

Image create_output_image(
    const Image& input,
    const ProcessingConfig& config)
{
    Image output;
    output.width = config.output_width;
    output.height = config.output_height;
    output.channels = image_channel_count;
    output.stride =
        static_cast<std::size_t>(output.width) * output.channels;
    output.source_path = input.source_path;
    output.pixels.resize(
        output.stride * static_cast<std::size_t>(output.height));
    return output;
}

void process_image_serial(
    const Image& input,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const CropRegion& crop,
    Image& output) noexcept
{
    for (std::uint32_t y = 0; y < output.height; ++y)
    {
        process_output_row(input, config, crop, y, output);
    }
    for (std::uint32_t y = 0; y < watermark.color.height; ++y)
    {
        process_watermark_row(watermark, config, y, output);
    }
}

void process_image_parallel_rows(
    const Image& input,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const CropRegion& crop,
    int thread_count,
    Image& output) noexcept
{
#pragma omp parallel for schedule(static) num_threads(thread_count)
    for (std::int64_t y = 0;
         y < static_cast<std::int64_t>(output.height);
         ++y)
    {
        process_output_row(
            input,
            config,
            crop,
            static_cast<std::uint32_t>(y),
            output);
    }

#pragma omp parallel for schedule(static) num_threads(thread_count)
    for (std::int64_t y = 0;
         y < static_cast<std::int64_t>(watermark.color.height);
         ++y)
    {
        process_watermark_row(
            watermark,
            config,
            static_cast<std::uint32_t>(y),
            output);
    }
}

}  // namespace parallelpix::openmp::detail
