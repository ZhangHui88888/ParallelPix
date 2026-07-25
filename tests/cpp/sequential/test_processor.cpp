#include "parallelpix/sequential/processor.hpp"
#include "processing_test_support.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <vector>

namespace {

using parallelpix::test::sequential::make_image;
using parallelpix::test::sequential::make_solid_image;
using parallelpix::test::sequential::make_watermark;
using parallelpix::test::sequential::small_config;

}  // namespace

PP_TEST("sequential processor preserves identity pixels and source order")
{
    const auto first = make_image(
        2,
        2,
        {
            0, 10, 20,
            30, 40, 50,
            60, 70, 80,
            90, 100, 110,
        },
        "first.png");
    const auto second = make_solid_image(
        2, 2, 17, "second.png");
    const std::vector<parallelpix::Image> inputs{first, second};
    const auto watermark = make_watermark(1, 1, 255, 255);

    const auto result = parallelpix::sequential::process_batch(
        inputs,
        watermark,
        small_config());

    PP_REQUIRE(result.ok());
    PP_REQUIRE_EQ(result.images->size(), std::size_t{2});
    PP_REQUIRE_EQ((*result.images)[0].source_path, first.source_path);
    PP_REQUIRE_EQ((*result.images)[1].source_path, second.source_path);
    PP_REQUIRE_EQ((*result.images)[0].pixels, first.pixels);
    PP_REQUIRE_EQ((*result.images)[1].pixels, second.pixels);
    PP_REQUIRE_EQ(inputs[0].pixels, first.pixels);
    PP_REQUIRE_EQ(watermark.alpha, std::vector<std::uint8_t>({255}));
}

PP_TEST("sequential processor applies centered crop before scaling")
{
    std::vector<std::uint8_t> pixels;
    for (int row = 0; row < 2; ++row)
    {
        for (const auto value :
             std::vector<std::uint8_t>{10, 20, 30, 40})
        {
            pixels.insert(pixels.end(), 3, value);
        }
    }
    const auto input =
        make_image(4, 2, std::move(pixels));

    const auto result = parallelpix::sequential::process_batch(
        {input},
        make_watermark(1, 1, 0, 0),
        small_config());

    PP_REQUIRE(result.ok());
    PP_REQUIRE_EQ(
        (*result.images)[0].channels,
        parallelpix::image_channel_count);
    PP_REQUIRE_EQ(
        (*result.images)[0].pixels,
        std::vector<std::uint8_t>({
            20, 20, 20,
            30, 30, 30,
            20, 20, 20,
            30, 30, 30,
        }));
}

PP_TEST("sequential processor applies brightness then bottom right watermark")
{
    auto config = small_config();
    config.brightness_factor = 1.10;
    config.watermark_opacity = 0.35;
    const auto input = make_solid_image(2, 2, 100);

    const auto result = parallelpix::sequential::process_batch(
        {input},
        make_watermark(1, 1, 200, 255),
        config);

    PP_REQUIRE(result.ok());
    PP_REQUIRE_EQ(
        (*result.images)[0].pixels,
        std::vector<std::uint8_t>({
            110, 110, 110,
            110, 110, 110,
            110, 110, 110,
            142, 142, 142,
        }));
}

PP_TEST("sequential processor honors the configured right and bottom margin")
{
    auto config = small_config();
    config.output_width = 4;
    config.output_height = 4;
    config.watermark_opacity = 1.0;
    config.watermark_margin = 1;

    const auto result = parallelpix::sequential::process_batch(
        {make_solid_image(4, 4, 10)},
        make_watermark(1, 1, 200, 255),
        config);

    PP_REQUIRE(result.ok());
    const auto& output = (*result.images)[0];
    const auto watermarked_index =
        static_cast<std::size_t>(2) * output.stride +
        static_cast<std::size_t>(2) * output.channels;
    const auto bottom_right_index =
        static_cast<std::size_t>(3) * output.stride +
        static_cast<std::size_t>(3) * output.channels;
    PP_REQUIRE_EQ(
        output.pixels[watermarked_index],
        std::uint8_t{200});
    PP_REQUIRE_EQ(
        output.pixels[bottom_right_index],
        std::uint8_t{10});
}

PP_TEST("sequential processor is deterministic and uses default output size")
{
    const auto input = make_solid_image(1, 1, 50);
    const auto watermark = make_watermark(1, 1, 50, 0);

    const auto first = parallelpix::sequential::process_batch(
        {input},
        watermark);
    const auto second = parallelpix::sequential::process_batch(
        {input},
        watermark);

    PP_REQUIRE(first.ok());
    PP_REQUIRE(second.ok());
    PP_REQUIRE_EQ((*first.images)[0].width, std::uint32_t{1024});
    PP_REQUIRE_EQ((*first.images)[0].height, std::uint32_t{1024});
    PP_REQUIRE_EQ(
        (*first.images)[0].pixels,
        (*second.images)[0].pixels);
}
