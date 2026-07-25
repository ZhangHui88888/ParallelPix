#pragma once

#include "parallelpix/common/processing.hpp"

namespace parallelpix::sequential {

[[nodiscard]] Image process_image(
    const Image& input,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const CropRegion& crop);

}  // namespace parallelpix::sequential
