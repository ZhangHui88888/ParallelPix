#include "parallelpix/sequential/processor.hpp"
#include "processing_test_support.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <optional>

namespace {

using parallelpix::test::sequential::has_issue;
using parallelpix::test::sequential::make_solid_image;
using parallelpix::test::sequential::make_watermark;
using parallelpix::test::sequential::small_config;

}  // namespace

PP_TEST("sequential validation rejects expected errors without partial output")
{
    const auto watermark = make_watermark(1, 1, 0, 0);
    const auto empty = parallelpix::sequential::process_batch(
        {},
        watermark,
        small_config());
    PP_REQUIRE(!empty.ok());
    PP_REQUIRE(has_issue(
        empty.issues,
        parallelpix::ProcessingIssueCode::EmptyBatch));

    auto invalid_config = small_config();
    invalid_config.output_width = 0;
    const auto bad_config = parallelpix::sequential::process_batch(
        {make_solid_image(1, 1, 0)},
        watermark,
        invalid_config);
    PP_REQUIRE(!bad_config.ok());
    PP_REQUIRE(has_issue(
        bad_config.issues,
        parallelpix::ProcessingIssueCode::InvalidConfig));

    auto invalid_image = make_solid_image(1, 1, 0, "invalid.png");
    invalid_image.pixels.clear();
    const auto mixed = parallelpix::sequential::process_batch(
        {
            make_solid_image(1, 1, 0, "valid.png"),
            invalid_image,
        },
        watermark,
        small_config());
    PP_REQUIRE(!mixed.ok());
    PP_REQUIRE(!mixed.images.has_value());
    PP_REQUIRE(has_issue(
        mixed.issues,
        parallelpix::ProcessingIssueCode::InvalidImage));
    PP_REQUIRE_EQ(
        mixed.issues.front().image_index,
        std::optional<std::size_t>{1});

    auto invalid_watermark = watermark;
    invalid_watermark.alpha.clear();
    const auto bad_watermark =
        parallelpix::sequential::process_batch(
            {make_solid_image(1, 1, 0)},
            invalid_watermark,
            small_config());
    PP_REQUIRE(!bad_watermark.ok());
    PP_REQUIRE(has_issue(
        bad_watermark.issues,
        parallelpix::ProcessingIssueCode::InvalidWatermark));

    const auto oversized = parallelpix::sequential::process_batch(
        {make_solid_image(1, 1, 0)},
        make_watermark(3, 1, 0, 0),
        small_config());
    PP_REQUIRE(!oversized.ok());
    PP_REQUIRE(has_issue(
        oversized.issues,
        parallelpix::ProcessingIssueCode::WatermarkDoesNotFit));
}
