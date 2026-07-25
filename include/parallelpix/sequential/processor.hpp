#pragma once

#include "parallelpix/common/processing.hpp"

#include <vector>

namespace parallelpix::sequential {

[[nodiscard]] BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config = {});

}  // namespace parallelpix::sequential
