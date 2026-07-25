#include "parallelpix/io/image_io.hpp"
#include "parallelpix/sequential/processor.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <filesystem>

namespace {

using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::copy_fixture;
using parallelpix::test::io::fixture_root;

}  // namespace

PP_TEST("M3 to M4 to PNG integration preserves exact processed pixels")
{
    TemporaryDirectory temporary;
    const auto input_directory = temporary.path() / L"输入图片";
    const auto input_path = input_directory / L"中文图片.png";
    copy_fixture(L"images/中文图片.png", input_path);

    const auto catalog =
        parallelpix::io::scan_catalog(input_directory);
    PP_REQUIRE(catalog.ok());
    const auto batch =
        parallelpix::io::load_batch(*catalog.catalog, 1);
    PP_REQUIRE(batch.ok());

    const auto watermark = parallelpix::io::decode_watermark(
        fixture_root / "watermarks/rgba_2x2.png");
    PP_REQUIRE(watermark.ok());

    parallelpix::ProcessingConfig config;
    config.output_width = 4;
    config.output_height = 4;
    config.watermark_margin = 0;
    const auto processed =
        parallelpix::sequential::process_batch(
            *batch.images,
            *watermark.watermark,
            config);
    PP_REQUIRE(processed.ok());
    PP_REQUIRE_EQ(processed.images->size(), std::size_t{1});
    PP_REQUIRE_EQ((*processed.images)[0].source_path, input_path);

    const auto output_directory = temporary.path() / L"处理结果";
    const auto prepared =
        parallelpix::io::prepare_output_directory(output_directory);
    PP_REQUIRE(prepared.ok());
    const auto output_path = output_directory / L"顺序基线.png";
    const auto written = parallelpix::io::write_png(
        (*processed.images)[0],
        output_path,
        parallelpix::io::WriteMode::FailIfExists);
    PP_REQUIRE(written.ok());

    const auto decoded =
        parallelpix::io::decode_image(output_path);
    PP_REQUIRE(decoded.ok());
    PP_REQUIRE_EQ(
        decoded.image->width,
        config.output_width);
    PP_REQUIRE_EQ(
        decoded.image->height,
        config.output_height);
    PP_REQUIRE_EQ(
        decoded.image->pixels,
        (*processed.images)[0].pixels);
}
