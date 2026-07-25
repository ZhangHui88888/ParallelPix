#pragma once

#include "parallelpix/common/image.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace parallelpix::benchmark {

struct ValidationResult
{
    bool passed = false;
    std::optional<std::uint8_t> max_pixel_error;
    std::optional<std::size_t> image_index;
    std::string message;
};

[[nodiscard]] ValidationResult validate_batches(
    const std::vector<Image>& reference,
    const std::vector<Image>& candidate,
    std::uint8_t tolerance);

}  // namespace parallelpix::benchmark
