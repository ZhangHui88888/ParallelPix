#include "parallelpix/benchmark/validation.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

namespace parallelpix::benchmark {
namespace {

ValidationResult structural_failure(
    std::string message,
    std::optional<std::size_t> image_index = std::nullopt)
{
    return {false, std::nullopt, image_index, std::move(message)};
}

}  // namespace

ValidationResult validate_batches(
    const std::vector<Image>& reference,
    const std::vector<Image>& candidate,
    std::uint8_t tolerance)
{
    if (reference.empty())
    {
        return structural_failure("Reference batch is empty.");
    }
    if (reference.size() != candidate.size())
    {
        return structural_failure(
            "Reference and candidate batches contain different image counts.");
    }

    std::uint8_t maximum_error = 0;
    for (std::size_t index = 0; index < reference.size(); ++index)
    {
        const auto& expected = reference[index];
        const auto& actual = candidate[index];
        if (!is_valid_image(expected))
        {
            return structural_failure(
                "Reference image violates the shared image contract.", index);
        }
        if (!is_valid_image(actual))
        {
            return structural_failure(
                "Candidate image violates the shared image contract.", index);
        }
        if (expected.width != actual.width ||
            expected.height != actual.height ||
            expected.channels != actual.channels ||
            expected.stride != actual.stride ||
            expected.pixels.size() != actual.pixels.size())
        {
            return structural_failure(
                "Reference and candidate image shapes differ.", index);
        }

        for (std::size_t pixel = 0; pixel < expected.pixels.size(); ++pixel)
        {
            const auto difference = static_cast<unsigned int>(std::abs(
                static_cast<int>(expected.pixels[pixel]) -
                static_cast<int>(actual.pixels[pixel])));
            maximum_error = std::max(
                maximum_error, static_cast<std::uint8_t>(difference));
        }
    }

    if (maximum_error > tolerance)
    {
        return {
            false,
            maximum_error,
            std::nullopt,
            "Maximum per-channel pixel error exceeds the allowed tolerance.",
        };
    }

    return {true, maximum_error, std::nullopt, {}};
}

}  // namespace parallelpix::benchmark
