#include "parallelpix/io/image_io.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

namespace {

using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::has_issue;

parallelpix::Image make_image(
    std::vector<std::uint8_t> pixels,
    const std::filesystem::path& source = "generated")
{
    return {
        2,
        1,
        parallelpix::image_channel_count,
        6,
        source,
        std::move(pixels),
    };
}

}  // namespace

PP_TEST("output directory preparation creates and cleans its own probe")
{
    TemporaryDirectory temporary;
    const auto output = temporary.path() / "nested" / "output";

    const auto prepared =
        parallelpix::io::prepare_output_directory(output);
    PP_REQUIRE(prepared.ok());
    PP_REQUIRE(std::filesystem::is_directory(output));
    PP_REQUIRE(std::filesystem::is_empty(output));

    const auto file_path = temporary.path() / "not-a-directory";
    std::ofstream(file_path) << "file";
    const auto invalid =
        parallelpix::io::prepare_output_directory(file_path);
    PP_REQUIRE(!invalid.ok());
    PP_REQUIRE(has_issue(
        invalid.issues,
        parallelpix::io::IoIssueCode::OutputPathNotDirectory));
}

PP_TEST("PNG writing validates the image and destination extension")
{
    TemporaryDirectory temporary;
    auto invalid_image = make_image({1, 2, 3});

    const auto invalid = parallelpix::io::write_png(
        invalid_image,
        temporary.path() / "invalid.png",
        parallelpix::io::WriteMode::FailIfExists);
    PP_REQUIRE(!invalid.ok());
    PP_REQUIRE(has_issue(
        invalid.issues,
        parallelpix::io::IoIssueCode::InvalidImage));

    const auto image = make_image({1, 2, 3, 4, 5, 6});
    const auto wrong_extension = parallelpix::io::write_png(
        image,
        temporary.path() / "image.jpg",
        parallelpix::io::WriteMode::FailIfExists);
    PP_REQUIRE(!wrong_extension.ok());
    PP_REQUIRE(has_issue(
        wrong_extension.issues,
        parallelpix::io::IoIssueCode::OutputExtensionNotPng));
}

PP_TEST("PNG writing round trips exact pixels and honors replacement policy")
{
    TemporaryDirectory temporary;
    const auto output = temporary.path() / L"中文输出" / "result.png";
    const auto first_image = make_image({0, 10, 20, 30, 40, 50});

    const auto first = parallelpix::io::write_png(
        first_image,
        output,
        parallelpix::io::WriteMode::FailIfExists);
    PP_REQUIRE(first.ok());
    PP_REQUIRE(std::filesystem::is_regular_file(output));

    const auto decoded_first = parallelpix::io::decode_image(output);
    PP_REQUIRE(decoded_first.ok());
    PP_REQUIRE_EQ(decoded_first.image->pixels, first_image.pixels);

    const auto conflict = parallelpix::io::write_png(
        make_image({60, 70, 80, 90, 100, 110}),
        output,
        parallelpix::io::WriteMode::FailIfExists);
    PP_REQUIRE(!conflict.ok());
    PP_REQUIRE(has_issue(
        conflict.issues,
        parallelpix::io::IoIssueCode::OutputExists));

    const auto unchanged = parallelpix::io::decode_image(output);
    PP_REQUIRE(unchanged.ok());
    PP_REQUIRE_EQ(unchanged.image->pixels, first_image.pixels);

    const auto replacement_image =
        make_image({60, 70, 80, 90, 100, 110});
    const auto replaced = parallelpix::io::write_png(
        replacement_image,
        output,
        parallelpix::io::WriteMode::ReplaceExisting);
    PP_REQUIRE(replaced.ok());

    const auto decoded_replacement =
        parallelpix::io::decode_image(output);
    PP_REQUIRE(decoded_replacement.ok());
    PP_REQUIRE_EQ(
        decoded_replacement.image->pixels,
        replacement_image.pixels);

    for (const auto& entry :
         std::filesystem::directory_iterator(output.parent_path()))
    {
        PP_REQUIRE(
            entry.path().filename().u8string().find(".parallelpix-") ==
            std::string::npos);
    }
}

PP_TEST("PNG replacement never removes a directory named like an image")
{
    TemporaryDirectory temporary;
    const auto destination = temporary.path() / "directory.png";
    std::filesystem::create_directories(destination);

    const auto result = parallelpix::io::write_png(
        make_image({1, 2, 3, 4, 5, 6}),
        destination,
        parallelpix::io::WriteMode::ReplaceExisting);
    PP_REQUIRE(!result.ok());
    PP_REQUIRE(has_issue(
        result.issues,
        parallelpix::io::IoIssueCode::OutputReplaceFailed));
    PP_REQUIRE(std::filesystem::is_directory(destination));
}
