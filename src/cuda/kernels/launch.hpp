#pragma once

#include "kernels/kernel_contract.hpp"

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace parallelpix::cuda::detail {

cudaError_t launch_process_batch(
    const std::uint8_t* input,
    const DeviceImageDescriptor* descriptors,
    std::uint8_t* output,
    std::uint32_t image_count,
    const std::uint8_t* watermark_color,
    const std::uint8_t* watermark_alpha,
    const DeviceProcessingConfig& config,
    cudaStream_t stream);

}  // namespace parallelpix::cuda::detail
