#include "parallelpix/io/image_io.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::copy_fixture;
using parallelpix::test::io::has_issue;

}  // namespace

PP_TEST("image and watermark contracts reject malformed storage")
{
    parallelpix::Image image;
    image.width = 2;
    image.height = 1;
    image.channels = 3;
    image.stride = 6;
    image.pixels = {1, 2, 3, 4, 5};
    PP_REQUIRE(!parallelpix::is_valid_image(image));

    image.pixels.push_back(6);
    PP_REQUIRE(parallelpix::is_valid_image(image));

    parallelpix::Watermark watermark;
    watermark.color = image;
    watermark.alpha = {255};
    PP_REQUIRE(!parallelpix::is_valid_watermark(watermark));
    watermark.alpha.push_back(0);
    PP_REQUIRE(parallelpix::is_valid_watermark(watermark));
}

PP_TEST("catalog validates paths and reports an empty supported set")
{
    TemporaryDirectory temporary;

    const auto missing =
        parallelpix::io::scan_catalog(temporary.path() / "missing");
    PP_REQUIRE(!missing.ok());
    PP_REQUIRE(has_issue(
        missing.issues,
        parallelpix::io::IoIssueCode::InputDirectoryMissing,
        parallelpix::io::IoSeverity::Error));

    const auto file_path = temporary.path() / "input.txt";
    std::ofstream(file_path) << "file";
    const auto file_result = parallelpix::io::scan_catalog(file_path);
    PP_REQUIRE(!file_result.ok());
    PP_REQUIRE(has_issue(
        file_result.issues,
        parallelpix::io::IoIssueCode::InputPathNotDirectory,
        parallelpix::io::IoSeverity::Error));

    const auto empty_directory = temporary.path() / "empty";
    std::filesystem::create_directories(empty_directory);
    std::ofstream(empty_directory / "notes.txt") << "ignored";
    const auto empty_result =
        parallelpix::io::scan_catalog(empty_directory);
    PP_REQUIRE(!empty_result.ok());
    PP_REQUIRE(has_issue(
        empty_result.issues,
        parallelpix::io::IoIssueCode::NoSupportedImages,
        parallelpix::io::IoSeverity::Error));
}

PP_TEST("catalog scans non-recursively and sorts supported extensions")
{
    TemporaryDirectory temporary;
    copy_fixture(
        "images/photo_4x3.jpg",
        temporary.path() / "02-photo.JPG");
    copy_fixture(
        "images/color_3x2.png",
        temporary.path() / "01-color.png");
    copy_fixture(
        "images/grayscale_2x2.png",
        temporary.path() / "03-gray.JpEg");
    copy_fixture(
        "images/color_3x2.png",
        temporary.path() / "nested" / "00-nested.png");
    std::ofstream(temporary.path() / "notes.txt") << "ignored";

    const auto result = parallelpix::io::scan_catalog(temporary.path());
    PP_REQUIRE(result.ok());
    PP_REQUIRE_EQ(result.catalog->files.size(), std::size_t{3});
    PP_REQUIRE_EQ(
        result.catalog->files[0].filename().u8string(),
        std::string("01-color.png"));
    PP_REQUIRE_EQ(
        result.catalog->files[1].filename().u8string(),
        std::string("02-photo.JPG"));
    PP_REQUIRE_EQ(
        result.catalog->files[2].filename().u8string(),
        std::string("03-gray.JpEg"));
}
