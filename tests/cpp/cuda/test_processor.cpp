#include "parallelpix/cuda/processor.hpp"
#include "parallelpix/sequential/processor.hpp"
#include "processing_test_support.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using parallelpix::test::sequential::has_issue;
using parallelpix::test::sequential::make_solid_image;
using parallelpix::test::sequential::make_watermark;
using parallelpix::test::sequential::small_config;

parallelpix::Image make_gradient(
    std::uint32_t width,
    std::uint32_t height,
    std::uint8_t seed,
    const std::string& source)
{
    std::vector<std::uint8_t> pixels;
    pixels.reserve(
        static_cast<std::size_t>(width) * height *
        parallelpix::image_channel_count);
    for (std::uint32_t y = 0; y < height; ++y)
    {
        for (std::uint32_t x = 0; x < width; ++x)
        {
            for (std::uint32_t channel = 0;
                 channel < parallelpix::image_channel_count;
                 ++channel)
            {
                pixels.push_back(static_cast<std::uint8_t>(
                    (seed + x * 29U + y * 47U + channel * 71U) %
                    256U));
            }
        }
    }
    return {
        width,
        height,
        parallelpix::image_channel_count,
        static_cast<std::size_t>(width) *
            parallelpix::image_channel_count,
        source,
        std::move(pixels),
    };
}

std::uint32_t maximum_pixel_error(
    const std::vector<parallelpix::Image>& expected,
    const std::vector<parallelpix::Image>& actual)
{
    PP_REQUIRE_EQ(actual.size(), expected.size());
    std::uint32_t maximum = 0;
    for (std::size_t image_index = 0;
         image_index < expected.size();
         ++image_index)
    {
        PP_REQUIRE_EQ(actual[image_index].width, expected[image_index].width);
        PP_REQUIRE_EQ(actual[image_index].height, expected[image_index].height);
        PP_REQUIRE_EQ(
            actual[image_index].source_path,
            expected[image_index].source_path);
        PP_REQUIRE_EQ(
            actual[image_index].pixels.size(),
            expected[image_index].pixels.size());
        for (std::size_t pixel = 0;
             pixel < expected[image_index].pixels.size();
             ++pixel)
        {
            const auto difference = static_cast<std::uint32_t>(std::abs(
                static_cast<int>(actual[image_index].pixels[pixel]) -
                static_cast<int>(expected[image_index].pixels[pixel])));
            maximum = std::max(maximum, difference);
        }
    }
    return maximum;
}

parallelpix::ProcessingConfig varied_config()
{
    parallelpix::ProcessingConfig config;
    config.output_width = 7;
    config.output_height = 5;
    config.brightness_factor = 1.10;
    config.watermark_opacity = 0.35;
    config.watermark_margin = 1;
    return config;
}

}  // namespace

PP_TEST("CUDA processor matches Sequential for variable images and batches")
{
    std::vector<parallelpix::Image> inputs;
    inputs.push_back(make_solid_image(3, 5, 0, "black.png"));
    inputs.push_back(make_solid_image(5, 3, 255, "white.png"));
    for (std::uint32_t index = 2; index < 8; ++index)
    {
        inputs.push_back(make_gradient(
            3U + index,
            4U + index % 3U,
            static_cast<std::uint8_t>(index * 17U),
            "gradient-" + std::to_string(index) + ".png"));
    }

    const auto config = varied_config();
    const auto watermark = make_watermark(2, 2, 173, 191);
    const auto expected = parallelpix::sequential::process_batch(
        inputs, watermark, config);
    PP_REQUIRE(expected.ok());

    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    for (const auto batch_size : {1U, 4U, 8U})
    {
        std::size_t progress_calls = 0;
        std::size_t completed = 0;
        const auto actual = processor.process_batch(
            inputs,
            watermark,
            config,
            batch_size,
            [&](std::size_t value, double milliseconds_per_image) {
                ++progress_calls;
                completed = value;
                PP_REQUIRE(milliseconds_per_image >= 0.0);
            });

        PP_REQUIRE(actual.processing.ok());
        PP_REQUIRE(actual.phase_timing.has_value());
        PP_REQUIRE(actual.phase_timing->h2d_ms >= 0.0);
        PP_REQUIRE(actual.phase_timing->kernel_ms >= 0.0);
        PP_REQUIRE(actual.phase_timing->d2h_ms >= 0.0);
        PP_REQUIRE_EQ(actual.effective_batch_size, batch_size);
        PP_REQUIRE_EQ(completed, inputs.size());
        PP_REQUIRE_EQ(
            progress_calls,
            (inputs.size() + batch_size - 1U) / batch_size);
        PP_REQUIRE(
            maximum_pixel_error(
                *expected.images, *actual.processing.images) <= 1U);
    }
}

PP_TEST("CUDA processor rejects invalid batch image and watermark inputs")
{
    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    const auto watermark = make_watermark(1, 1, 10, 255);
    const auto image = make_solid_image(2, 2, 20);

    const auto invalid_batch = processor.process_batch(
        {image}, watermark, small_config(), 0);
    PP_REQUIRE(has_issue(
        invalid_batch.processing.issues,
        parallelpix::ProcessingIssueCode::InvalidConfig));

    auto invalid_image = image;
    invalid_image.pixels.clear();
    const auto image_result = processor.process_batch(
        {invalid_image}, watermark, small_config(), 1);
    PP_REQUIRE(has_issue(
        image_result.processing.issues,
        parallelpix::ProcessingIssueCode::InvalidImage));

    auto invalid_watermark = watermark;
    invalid_watermark.alpha.clear();
    const auto watermark_result = processor.process_batch(
        {image}, invalid_watermark, small_config(), 1);
    PP_REQUIRE(has_issue(
        watermark_result.processing.issues,
        parallelpix::ProcessingIssueCode::InvalidWatermark));
}

PP_TEST("CUDA processor reports progress after a successful batch")
{
    const std::vector<parallelpix::Image> inputs{
        make_solid_image(3, 3, 10, "one.png"),
        make_solid_image(3, 3, 20, "two.png"),
        make_solid_image(3, 3, 30, "three.png"),
    };
    const auto watermark = make_watermark(1, 1, 0, 0);
    std::vector<std::size_t> completed;

    parallelpix::cuda::Processor processor;
    PP_REQUIRE(processor.availability().available);
    const auto result = processor.process_batch(
        inputs,
        watermark,
        small_config(),
        2,
        [&](std::size_t value, double) {
            completed.push_back(value);
        });

    PP_REQUIRE(result.processing.ok());
    PP_REQUIRE_EQ(
        completed,
        std::vector<std::size_t>({2, 3}));
}
