#pragma once

#include "parallelpix/cuda/processor.hpp"

#include "kernels/kernel_contract.hpp"
#include "runtime/runtime.hpp"

#include <cstdint>
#include <vector>

namespace parallelpix::cuda::detail {

class BatchExecutor
{
public:
    explicit BatchExecutor(int device_index);

    ProcessingResult run(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        const std::vector<CropRegion>& crops,
        std::uint32_t effective_batch_size,
        const BatchProgressSink& progress);

private:
    template <typename Operation>
    double measure(Operation&& operation)
    {
        phase_start_.record(stream_.get());
        operation();
        phase_stop_.record(stream_.get());
        phase_stop_.synchronize();
        return phase_start_.elapsed_ms_to(phase_stop_);
    }

    Stream stream_;
    Event phase_start_;
    Event phase_stop_;
    DeviceBuffer input_;
    DeviceBuffer descriptors_;
    DeviceBuffer output_;
    DeviceBuffer watermark_color_;
    DeviceBuffer watermark_alpha_;
    std::vector<std::uint8_t> host_input_;
    std::vector<DeviceImageDescriptor> host_descriptors_;
    std::vector<std::uint8_t> host_output_;
};

}  // namespace parallelpix::cuda::detail
