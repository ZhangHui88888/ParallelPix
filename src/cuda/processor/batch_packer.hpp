#pragma once

#include "kernels/kernel_contract.hpp"

#include "parallelpix/common/processing.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace parallelpix::cuda::detail {

[[nodiscard]] std::size_t checked_byte_count(
    std::size_t item_bytes,
    std::size_t item_count);

void pack_chunk(
    const std::vector<Image>& images,
    const std::vector<CropRegion>& crops,
    std::size_t begin,
    std::size_t count,
    std::vector<std::uint8_t>& packed_input,
    std::vector<DeviceImageDescriptor>& descriptors);

[[nodiscard]] Image make_output_image(
    const Image& input,
    const ProcessingConfig& config,
    const std::uint8_t* pixels,
    std::size_t pixel_count);

[[nodiscard]] std::size_t output_image_bytes(
    const ProcessingConfig& config);

}  // namespace parallelpix::cuda::detail
