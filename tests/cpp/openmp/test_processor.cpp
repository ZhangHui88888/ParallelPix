#include "parallelpix/openmp/processor.hpp"
#include "parallelpix/sequential/processor.hpp"
#include "processing_test_support.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using parallelpix::BatchProcessingResult;
using parallelpix::Image;
using parallelpix::ProcessingConfig;
using parallelpix::ProcessingIssueCode;
using parallelpix::Watermark;
using parallelpix::test::sequential::has_issue;
using parallelpix::test::sequential::make_image;
using parallelpix::test::sequential::make_watermark;

std::vector<Image> patterned_inputs(std::size_t count)
{
    std::vector<Image> images;
    images.reserve(count);
    for (std::size_t image_index = 0; image_index < count; ++image_index)
    {
        std::vector<std::uint8_t> pixels;
        pixels.reserve(4 * 3 * parallelpix::image_channel_count);
        for (std::uint32_t y = 0; y < 3; ++y)
        {
            for (std::uint32_t x = 0; x < 4; ++x)
            {
                for (std::uint32_t channel = 0;
                     channel < parallelpix::image_channel_count;
                     ++channel)
                {
                    pixels.push_back(static_cast<std::uint8_t>(
                        (image_index * 17 + y * 31 + x * 13 +
                         channel * 7) %
                        256));
                }
            }
        }
        images.push_back(make_image(
            4,
            3,
            std::move(pixels),
            "image-" + std::to_string(image_index) + ".png"));
    }
    return images;
}

ProcessingConfig test_config()
{
    ProcessingConfig config;
    config.output_width = 7;
    config.output_height = 5;
    config.brightness_factor = 1.10;
    config.watermark_opacity = 0.35;
    config.watermark_margin = 1;
    return config;
}

Watermark test_watermark()
{
    return make_watermark(2, 2, 211, 128);
}

void require_same_images(
    const BatchProcessingResult& actual,
    const BatchProcessingResult& expected)
{
    PP_REQUIRE(actual.ok());
    PP_REQUIRE(expected.ok());
    PP_REQUIRE_EQ(actual.images->size(), expected.images->size());
    for (std::size_t index = 0; index < actual.images->size(); ++index)
    {
        const auto& left = (*actual.images)[index];
        const auto& right = (*expected.images)[index];
        PP_REQUIRE_EQ(left.width, right.width);
        PP_REQUIRE_EQ(left.height, right.height);
        PP_REQUIRE_EQ(left.channels, right.channels);
        PP_REQUIRE_EQ(left.stride, right.stride);
        PP_REQUIRE_EQ(left.source_path, right.source_path);
        PP_REQUIRE_EQ(left.pixels, right.pixels);
    }
}

}  // namespace

PP_TEST("OpenMP image scheduling matches Sequential for configured threads")
{
    const auto inputs = patterned_inputs(8);
    const auto watermark = test_watermark();
    const auto config = test_config();
    const auto expected = parallelpix::sequential::process_batch(
        inputs, watermark, config);

    for (const std::uint32_t threads : {1U, 2U, 4U, 8U})
    {
        const auto actual = parallelpix::openmp::process_batch(
            inputs, watermark, config, threads);
        require_same_images(actual, expected);
    }
}

PP_TEST("OpenMP row scheduling matches Sequential for a small batch")
{
    const auto inputs = patterned_inputs(1);
    const auto watermark = test_watermark();
    const auto config = test_config();
    const auto expected = parallelpix::sequential::process_batch(
        inputs, watermark, config);
    const auto actual = parallelpix::openmp::process_batch(
        inputs, watermark, config, 4);

    require_same_images(actual, expected);
}

PP_TEST("OpenMP processor preserves inputs and reports monotonic progress")
{
    const auto inputs = patterned_inputs(12);
    const auto original = inputs;
    const auto watermark = test_watermark();
    const auto original_alpha = watermark.alpha;
    std::vector<std::pair<std::size_t, double>> progress;

    const auto result = parallelpix::openmp::process_batch(
        inputs,
        watermark,
        test_config(),
        4,
        [&progress](std::size_t processed, double ms_per_image) {
            progress.emplace_back(processed, ms_per_image);
        });

    PP_REQUIRE(result.ok());
    PP_REQUIRE(!progress.empty());
    PP_REQUIRE_EQ(progress.back().first, std::size_t{12});
    for (std::size_t index = 1; index < progress.size(); ++index)
    {
        PP_REQUIRE(progress[index - 1].first < progress[index].first);
    }
    for (const auto& sample : progress)
    {
        PP_REQUIRE(sample.second >= 0.0);
    }
    for (std::size_t index = 0; index < inputs.size(); ++index)
    {
        PP_REQUIRE_EQ(inputs[index].pixels, original[index].pixels);
    }
    PP_REQUIRE_EQ(watermark.alpha, original_alpha);
}

PP_TEST("OpenMP processor rejects an invalid thread count")
{
    const auto result = parallelpix::openmp::process_batch(
        patterned_inputs(1),
        test_watermark(),
        test_config(),
        0);

    PP_REQUIRE(!result.ok());
    PP_REQUIRE(has_issue(
        result.issues,
        ProcessingIssueCode::InvalidConfig));
}
