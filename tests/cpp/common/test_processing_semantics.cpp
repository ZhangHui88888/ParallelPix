#include "parallelpix/common/processing.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace {

parallelpix::Image make_two_by_two_image()
{
    return {
        2,
        2,
        parallelpix::image_channel_count,
        6,
        "two-by-two",
        {
            0, 10, 20,
            100, 110, 120,
            200, 210, 220,
            255, 250, 240,
        },
    };
}

}  // namespace

PP_TEST("processing configuration validates numeric and memory bounds")
{
    PP_REQUIRE(parallelpix::is_valid_processing_config({}));

    parallelpix::ProcessingConfig config;
    config.output_width = 0;
    PP_REQUIRE(!parallelpix::is_valid_processing_config(config));

    config = {};
    config.brightness_factor = 0.0;
    PP_REQUIRE(!parallelpix::is_valid_processing_config(config));

    config = {};
    config.watermark_opacity = 1.01;
    PP_REQUIRE(!parallelpix::is_valid_processing_config(config));

    config = {};
    config.output_width = std::numeric_limits<std::uint32_t>::max();
    config.output_height = std::numeric_limits<std::uint32_t>::max();
    PP_REQUIRE(!parallelpix::is_valid_processing_config(config));
}

PP_TEST("center crop covers square wide tall odd and unit geometries")
{
    const auto square =
        parallelpix::compute_center_crop(5, 5, 4, 4);
    PP_REQUIRE(square.has_value());
    PP_REQUIRE_EQ(square->x, std::uint32_t{0});
    PP_REQUIRE_EQ(square->y, std::uint32_t{0});
    PP_REQUIRE_EQ(square->width, std::uint32_t{5});
    PP_REQUIRE_EQ(square->height, std::uint32_t{5});

    const auto wide =
        parallelpix::compute_center_crop(8, 5, 4, 4);
    PP_REQUIRE(wide.has_value());
    PP_REQUIRE_EQ(wide->x, std::uint32_t{1});
    PP_REQUIRE_EQ(wide->width, std::uint32_t{5});
    PP_REQUIRE_EQ(wide->height, std::uint32_t{5});

    const auto tall =
        parallelpix::compute_center_crop(5, 8, 4, 4);
    PP_REQUIRE(tall.has_value());
    PP_REQUIRE_EQ(tall->y, std::uint32_t{1});
    PP_REQUIRE_EQ(tall->width, std::uint32_t{5});
    PP_REQUIRE_EQ(tall->height, std::uint32_t{5});

    const auto unit =
        parallelpix::compute_center_crop(1, 1, 1, 1);
    PP_REQUIRE(unit.has_value());
    PP_REQUIRE_EQ(unit->width, std::uint32_t{1});
    PP_REQUIRE_EQ(unit->height, std::uint32_t{1});

    const auto invalid =
        parallelpix::compute_center_crop(0, 1, 1, 1);
    PP_REQUIRE(!invalid.has_value());
}

PP_TEST("center crop arithmetic remains bounded at uint32 limits")
{
    constexpr auto maximum =
        std::numeric_limits<std::uint32_t>::max();
    const auto crop = parallelpix::compute_center_crop(
        maximum,
        maximum - 1,
        maximum - 1,
        maximum);

    PP_REQUIRE(crop.has_value());
    PP_REQUIRE(crop->width > 0);
    PP_REQUIRE(crop->height > 0);
    PP_REQUIRE(
        static_cast<std::uint64_t>(crop->x) + crop->width <=
        maximum);
    PP_REQUIRE(
        static_cast<std::uint64_t>(crop->y) + crop->height <=
        maximum - 1);
}

PP_TEST("bilinear sampling uses half pixel coordinates and BGR channels")
{
    const auto input = make_two_by_two_image();
    const parallelpix::CropRegion crop{0, 0, 2, 2};

    PP_REQUIRE_EQ(
        parallelpix::sample_bilinear(
            input, crop, 0, 0, 2, 2, 0),
        std::uint8_t{0});
    PP_REQUIRE_EQ(
        parallelpix::sample_bilinear(
            input, crop, 1, 1, 2, 2, 2),
        std::uint8_t{240});

    PP_REQUIRE_EQ(
        parallelpix::sample_bilinear(
            input, crop, 0, 0, 1, 1, 0),
        std::uint8_t{139});
    PP_REQUIRE_EQ(
        parallelpix::sample_bilinear(
            input, crop, 0, 0, 1, 1, 1),
        std::uint8_t{145});
    PP_REQUIRE_EQ(
        parallelpix::sample_bilinear(
            input, crop, 0, 0, 1, 1, 2),
        std::uint8_t{150});
}

PP_TEST("brightness and watermark effects share nearest byte rounding")
{
    PP_REQUIRE_EQ(
        parallelpix::round_and_clamp(10.49),
        std::uint8_t{10});
    PP_REQUIRE_EQ(
        parallelpix::round_and_clamp(10.50),
        std::uint8_t{11});
    PP_REQUIRE_EQ(
        parallelpix::apply_brightness(100, 1.10),
        std::uint8_t{110});
    PP_REQUIRE_EQ(
        parallelpix::apply_brightness(250, 1.10),
        std::uint8_t{255});

    PP_REQUIRE_EQ(
        parallelpix::blend_watermark_channel(
            100, 200, 0, 0.35),
        std::uint8_t{100});
    PP_REQUIRE_EQ(
        parallelpix::blend_watermark_channel(
            100, 200, 64, 0.35),
        std::uint8_t{109});
    PP_REQUIRE_EQ(
        parallelpix::blend_watermark_channel(
            100, 200, 128, 0.35),
        std::uint8_t{118});
    PP_REQUIRE_EQ(
        parallelpix::blend_watermark_channel(
            100, 200, 255, 1.0),
        std::uint8_t{200});
}
