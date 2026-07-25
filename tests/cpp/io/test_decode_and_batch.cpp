#include "parallelpix/io/image_io.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

namespace {

using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::copy_fixture;
using parallelpix::test::io::fixture_root;
using parallelpix::test::io::has_issue;

}  // namespace

PP_TEST("image decoding normalizes PNG JPEG grayscale and Unicode paths")
{
    const auto color = parallelpix::io::decode_image(
        fixture_root / "images/color_3x2.png");
    PP_REQUIRE(color.ok());
    PP_REQUIRE(parallelpix::is_valid_image(*color.image));
    PP_REQUIRE_EQ(color.image->width, std::uint32_t{3});
    PP_REQUIRE_EQ(color.image->height, std::uint32_t{2});
    PP_REQUIRE_EQ(color.image->stride, std::size_t{9});
    PP_REQUIRE_EQ(color.image->pixels[0], std::uint8_t{0});
    PP_REQUIRE_EQ(color.image->pixels[1], std::uint8_t{0});
    PP_REQUIRE_EQ(color.image->pixels[2], std::uint8_t{255});

    const auto photo = parallelpix::io::decode_image(
        fixture_root / "images/photo_4x3.jpg");
    PP_REQUIRE(photo.ok());
    PP_REQUIRE_EQ(photo.image->width, std::uint32_t{4});
    PP_REQUIRE_EQ(photo.image->height, std::uint32_t{3});

    const auto grayscale = parallelpix::io::decode_image(
        fixture_root / "images/grayscale_2x2.png");
    PP_REQUIRE(grayscale.ok());
    PP_REQUIRE_EQ(grayscale.image->pixels[3], std::uint8_t{64});
    PP_REQUIRE_EQ(grayscale.image->pixels[4], std::uint8_t{64});
    PP_REQUIRE_EQ(grayscale.image->pixels[5], std::uint8_t{64});

    TemporaryDirectory temporary;
    const auto unicode_path = temporary.path() / L"中文路径.png";
    copy_fixture(L"images/中文图片.png", unicode_path);
    const auto unicode = parallelpix::io::decode_image(unicode_path);
    PP_REQUIRE(unicode.ok());
    PP_REQUIRE_EQ(unicode.image->source_path, unicode_path);
}

PP_TEST("watermark decoding preserves alpha and normalizes opaque inputs")
{
    const auto rgba = parallelpix::io::decode_watermark(
        fixture_root / "watermarks/rgba_2x2.png");
    PP_REQUIRE(rgba.ok());
    PP_REQUIRE(parallelpix::is_valid_watermark(*rgba.watermark));
    PP_REQUIRE_EQ(rgba.watermark->alpha[0], std::uint8_t{0});
    PP_REQUIRE_EQ(rgba.watermark->alpha[1], std::uint8_t{64});
    PP_REQUIRE_EQ(rgba.watermark->alpha[2], std::uint8_t{128});
    PP_REQUIRE_EQ(rgba.watermark->alpha[3], std::uint8_t{255});
    PP_REQUIRE_EQ(rgba.watermark->color.pixels[2], std::uint8_t{255});

    const auto rgb = parallelpix::io::decode_watermark(
        fixture_root / "watermarks/rgb_2x1.png");
    PP_REQUIRE(rgb.ok());
    PP_REQUIRE(std::all_of(
        rgb.watermark->alpha.begin(),
        rgb.watermark->alpha.end(),
        [](std::uint8_t alpha) { return alpha == 255; }));

    const auto grayscale = parallelpix::io::decode_watermark(
        fixture_root / "watermarks/grayscale_1x2.png");
    PP_REQUIRE(grayscale.ok());
    PP_REQUIRE_EQ(grayscale.watermark->color.pixels[0], std::uint8_t{25});
    PP_REQUIRE_EQ(grayscale.watermark->color.pixels[1], std::uint8_t{25});
    PP_REQUIRE_EQ(grayscale.watermark->color.pixels[2], std::uint8_t{25});

    const auto corrupt = parallelpix::io::decode_watermark(
        fixture_root / "watermarks/corrupt.png");
    PP_REQUIRE(!corrupt.ok());
    PP_REQUIRE(has_issue(
        corrupt.issues,
        parallelpix::io::IoIssueCode::WatermarkDecodeFailed,
        parallelpix::io::IoSeverity::Error));
}

PP_TEST("batch loading skips corrupt images but never shrinks the request")
{
    const parallelpix::io::ImageCatalog catalog{
        fixture_root / "images",
        {
            fixture_root / "images/corrupt.png",
            fixture_root / "images/color_3x2.png",
            fixture_root / "images/grayscale_2x2.png",
            fixture_root / L"images/中文图片.png",
        },
    };

    const auto first = parallelpix::io::load_batch(catalog, 2);
    PP_REQUIRE(first.ok());
    PP_REQUIRE_EQ(first.images->size(), std::size_t{2});
    PP_REQUIRE_EQ(
        (*first.images)[0].source_path.filename().u8string(),
        std::string("color_3x2.png"));
    PP_REQUIRE_EQ(
        (*first.images)[1].source_path.filename().u8string(),
        std::string("grayscale_2x2.png"));
    PP_REQUIRE(has_issue(
        first.issues,
        parallelpix::io::IoIssueCode::ImageDecodeFailed,
        parallelpix::io::IoSeverity::Warning));

    const auto repeated = parallelpix::io::load_batch(catalog, 2);
    PP_REQUIRE(repeated.ok());
    PP_REQUIRE_EQ(
        (*repeated.images)[0].source_path,
        (*first.images)[0].source_path);
    PP_REQUIRE_EQ(
        (*repeated.images)[1].source_path,
        (*first.images)[1].source_path);

    const auto insufficient = parallelpix::io::load_batch(catalog, 4);
    PP_REQUIRE(!insufficient.ok());
    PP_REQUIRE(has_issue(
        insufficient.issues,
        parallelpix::io::IoIssueCode::InsufficientValidImages,
        parallelpix::io::IoSeverity::Error));

    const auto zero = parallelpix::io::load_batch(catalog, 0);
    PP_REQUIRE(!zero.ok());
    PP_REQUIRE(has_issue(
        zero.issues,
        parallelpix::io::IoIssueCode::InvalidRequestedCount,
        parallelpix::io::IoSeverity::Error));
}
