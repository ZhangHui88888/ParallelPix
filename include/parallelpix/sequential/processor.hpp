#pragma once

#include "parallelpix/common/processing.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace parallelpix::sequential {

using BatchProgressSink = std::function<void(std::size_t, double)>;

[[nodiscard]] BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config = {},
    const BatchProgressSink& progress = {});

}  // namespace parallelpix::sequential
