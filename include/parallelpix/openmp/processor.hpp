#pragma once

#include "parallelpix/common/processing.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace parallelpix::openmp {

using BatchProgressSink = std::function<void(std::size_t, double)>;

BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    std::uint32_t thread_count,
    const BatchProgressSink& progress = {});

}  // namespace parallelpix::openmp
