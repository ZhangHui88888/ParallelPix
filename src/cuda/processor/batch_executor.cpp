#include "processor/batch_executor.hpp"

#include "kernels/launch.hpp"
#include "processor/batch_packer.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace parallelpix::cuda::detail {
namespace {

DeviceProcessingConfig device_config(
    const ProcessingConfig& config,
    const Watermark& watermark)
{
    return {
        config.output_width,
        config.output_height,
        config.brightness_factor,
        config.watermark_opacity,
        watermark.color.width,
        watermark.color.height,
        static_cast<std::uint64_t>(watermark.color.stride),
        config.watermark_margin,
    };
}

}  // namespace

BatchExecutor::BatchExecutor(int device_index)
{
    check(cudaSetDevice(device_index), "cudaSetDevice");
    stream_.create();
    phase_start_.create();
    phase_stop_.create();
}

ProcessingResult BatchExecutor::run(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    const std::vector<CropRegion>& crops,
    std::uint32_t effective_batch_size,
    const BatchProgressSink& progress)
{
    PhaseTiming timing;
    std::vector<std::pair<std::size_t, double>> progress_samples;
    if (has_runtime_fault_for_testing(RuntimeFault::ApiFailure))
    {
        throw RuntimeError(
            cudaErrorUnknown,
            "injected CUDA API operation");
    }
    watermark_color_.reserve(watermark.color.pixels.size());
    watermark_alpha_.reserve(watermark.alpha.size());
    timing.h2d_ms += measure([&] {
        check(
            cudaMemcpyAsync(
                watermark_color_.data(),
                watermark.color.pixels.data(),
                watermark.color.pixels.size(),
                cudaMemcpyHostToDevice,
                stream_.get()),
            "CUDA watermark color upload");
        check(
            cudaMemcpyAsync(
                watermark_alpha_.data(),
                watermark.alpha.data(),
                watermark.alpha.size(),
                cudaMemcpyHostToDevice,
                stream_.get()),
            "CUDA watermark alpha upload");
    });

    std::vector<Image> outputs;
    outputs.reserve(images.size());
    const auto bytes_per_output = output_image_bytes(config);
    const auto kernel_config = device_config(config, watermark);

    for (std::size_t begin = 0;
         begin < images.size();
         begin += effective_batch_size)
    {
        const auto started_at = std::chrono::steady_clock::now();
        const auto chunk_size = std::min<std::size_t>(
            effective_batch_size, images.size() - begin);
        pack_chunk(
            images,
            crops,
            begin,
            chunk_size,
            host_input_,
            host_descriptors_);

        const auto output_bytes =
            checked_byte_count(bytes_per_output, chunk_size);
        host_output_.resize(output_bytes);
        input_.reserve(host_input_.size());
        descriptors_.reserve(checked_byte_count(
            sizeof(DeviceImageDescriptor),
            host_descriptors_.size()));
        output_.reserve(output_bytes);

        timing.h2d_ms += measure([&] {
            check(
                cudaMemcpyAsync(
                    input_.data(),
                    host_input_.data(),
                    host_input_.size(),
                    cudaMemcpyHostToDevice,
                    stream_.get()),
                "CUDA input upload");
            check(
                cudaMemcpyAsync(
                    descriptors_.data(),
                    host_descriptors_.data(),
                    host_descriptors_.size() *
                        sizeof(DeviceImageDescriptor),
                    cudaMemcpyHostToDevice,
                    stream_.get()),
                "CUDA descriptor upload");
        });

        if (has_runtime_fault_for_testing(RuntimeFault::KernelFailure))
        {
            throw RuntimeError(
                cudaErrorLaunchFailure,
                "injected CUDA kernel launch");
        }
        timing.kernel_ms += measure([&] {
            check(
                launch_process_batch(
                    static_cast<const std::uint8_t*>(input_.data()),
                    static_cast<const DeviceImageDescriptor*>(
                        descriptors_.data()),
                    static_cast<std::uint8_t*>(output_.data()),
                    static_cast<std::uint32_t>(chunk_size),
                    static_cast<const std::uint8_t*>(
                        watermark_color_.data()),
                    static_cast<const std::uint8_t*>(
                        watermark_alpha_.data()),
                    kernel_config,
                    stream_.get()),
                "CUDA process kernel launch");
        });

        timing.d2h_ms += measure([&] {
            check(
                cudaMemcpyAsync(
                    host_output_.data(),
                    output_.data(),
                    output_bytes,
                    cudaMemcpyDeviceToHost,
                    stream_.get()),
                "CUDA output download");
        });

        for (std::size_t offset = 0;
             offset < chunk_size;
             ++offset)
        {
            outputs.push_back(make_output_image(
                images[begin + offset],
                config,
                host_output_.data() + offset * bytes_per_output,
                bytes_per_output));
        }

        if (progress)
        {
            const auto elapsed_ms =
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - started_at)
                    .count();
            progress_samples.emplace_back(
                begin + chunk_size,
                elapsed_ms / static_cast<double>(chunk_size));
        }
    }

    for (const auto& sample : progress_samples)
    {
        progress(sample.first, sample.second);
    }

    ProcessingResult result;
    result.processing.images = std::move(outputs);
    result.phase_timing = timing;
    result.effective_batch_size = effective_batch_size;
    return result;
}

}  // namespace parallelpix::cuda::detail
