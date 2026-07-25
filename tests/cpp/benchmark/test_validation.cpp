#include "parallelpix/benchmark/validation.hpp"
#include "processing_test_support.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

using parallelpix::test::sequential::make_solid_image;

}  // namespace

PP_TEST("batch validation accepts exact and tolerated pixels")
{
    const std::vector<parallelpix::Image> reference{
        make_solid_image(2, 1, 20),
    };
    auto candidate = reference;

    const auto exact =
        parallelpix::benchmark::validate_batches(reference, candidate, 0);
    PP_REQUIRE(exact.passed);
    PP_REQUIRE_EQ(
        exact.max_pixel_error,
        std::optional<std::uint8_t>{std::uint8_t{0}});

    candidate[0].pixels[2] = 21;
    const auto tolerated =
        parallelpix::benchmark::validate_batches(reference, candidate, 1);
    PP_REQUIRE(tolerated.passed);
    PP_REQUIRE_EQ(
        tolerated.max_pixel_error,
        std::optional<std::uint8_t>{std::uint8_t{1}});

    const auto rejected =
        parallelpix::benchmark::validate_batches(reference, candidate, 0);
    PP_REQUIRE(!rejected.passed);
    PP_REQUIRE_EQ(
        rejected.max_pixel_error,
        std::optional<std::uint8_t>{std::uint8_t{1}});
}

PP_TEST("batch validation reports structural mismatches without fake error")
{
    const std::vector<parallelpix::Image> reference{
        make_solid_image(2, 1, 20),
    };

    const auto empty =
        parallelpix::benchmark::validate_batches({}, {}, 0);
    PP_REQUIRE(!empty.passed);
    PP_REQUIRE(!empty.max_pixel_error.has_value());

    const auto count_mismatch =
        parallelpix::benchmark::validate_batches(reference, {}, 0);
    PP_REQUIRE(!count_mismatch.passed);
    PP_REQUIRE(!count_mismatch.max_pixel_error.has_value());

    const std::vector<parallelpix::Image> shape_mismatch{
        make_solid_image(1, 1, 20),
    };
    const auto shape = parallelpix::benchmark::validate_batches(
        reference, shape_mismatch, 0);
    PP_REQUIRE(!shape.passed);
    PP_REQUIRE_EQ(shape.image_index, std::optional<std::size_t>{0});
    PP_REQUIRE(!shape.max_pixel_error.has_value());

    auto invalid = reference;
    invalid[0].pixels.clear();
    const auto bad =
        parallelpix::benchmark::validate_batches(reference, invalid, 0);
    PP_REQUIRE(!bad.passed);
    PP_REQUIRE_EQ(bad.image_index, std::optional<std::size_t>{0});
}
