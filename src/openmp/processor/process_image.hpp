#pragma once

#include "parallelpix/common/processing.hpp"

namespace parallelpix::openmp::detail {

Image create_output_image(
    const Image& input,
    const ProcessingConfig& config);

void process_image_serial(
    const Image& input,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const CropRegion& crop,
    Image& output) noexcept;

void process_image_parallel_rows(
    const Image& input,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const CropRegion& crop,
    int thread_count,
    Image& output) noexcept;

}  // namespace parallelpix::openmp::detail
