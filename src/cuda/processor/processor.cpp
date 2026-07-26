#include "parallelpix/cuda/processor.hpp"

#include "processor/batch_executor.hpp"
#include "runtime/runtime.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::cuda {
namespace {

ProcessingResult failure(
    ProcessingIssueCode code,
    std::string message,
    std::uint32_t effective_batch_size = 0)
{
    ProcessingResult result;
    result.effective_batch_size = effective_batch_size;
    result.processing.issues.push_back({
        code,
        std::nullopt,
        {},
        std::move(message),
    });
    return result;
}

}  // namespace

class Processor::Impl
{
public:
    Impl()
        : device_status_(detail::query_device_status())
    {
        if (!device_status_.available)
        {
            return;
        }

        try
        {
            executor_ = std::make_unique<detail::BatchExecutor>(
                device_status_.device_index);
        }
        catch (const std::exception& error)
        {
            device_status_.available = false;
            device_status_.message =
                "CUDA runtime initialization failed: " +
                std::string(error.what());
        }
    }

    [[nodiscard]] Availability availability() const
    {
        return {
            device_status_.available,
            device_status_.message,
        };
    }

    ProcessingResult process_batch(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        std::uint32_t requested_batch_size,
        const BatchProgressSink& progress)
    {
        if (!device_status_.available)
        {
            return failure(
                ProcessingIssueCode::BackendFailure,
                device_status_.message);
        }
        if (requested_batch_size == 0)
        {
            return failure(
                ProcessingIssueCode::InvalidConfig,
                "CUDA batch size must be greater than zero.");
        }

        auto preparation =
            prepare_processing_batch(images, watermark, config);
        if (!preparation.ok())
        {
            ProcessingResult result;
            result.processing.issues =
                std::move(preparation.issues);
            return result;
        }

        const auto first_batch_size = std::min(
            requested_batch_size, device_status_.max_grid_z);
        try
        {
            return executor_->run(
                images,
                watermark,
                config,
                *preparation.crops,
                first_batch_size,
                progress);
        }
        catch (const detail::RuntimeError& error)
        {
            if (error.code() != cudaErrorMemoryAllocation ||
                first_batch_size <= 1)
            {
                return failure(
                    ProcessingIssueCode::BackendFailure,
                    error.what(),
                    first_batch_size);
            }

            const auto fallback_batch_size =
                std::max<std::uint32_t>(1, first_batch_size / 2);
            try
            {
                return executor_->run(
                    images,
                    watermark,
                    config,
                    *preparation.crops,
                    fallback_batch_size,
                    progress);
            }
            catch (const std::exception& retry_error)
            {
                return failure(
                    ProcessingIssueCode::BackendFailure,
                    "CUDA allocation retry with batch size " +
                        std::to_string(fallback_batch_size) +
                        " failed: " + retry_error.what(),
                    fallback_batch_size);
            }
        }
        catch (const std::exception& error)
        {
            return failure(
                ProcessingIssueCode::BackendFailure,
                error.what(),
                first_batch_size);
        }
    }

private:
    detail::DeviceStatus device_status_;
    std::unique_ptr<detail::BatchExecutor> executor_;
};

Processor::Processor()
    : impl_(std::make_unique<Impl>())
{
}

Processor::~Processor() = default;
Processor::Processor(Processor&&) noexcept = default;
Processor& Processor::operator=(Processor&&) noexcept = default;

Availability Processor::availability() const
{
    return impl_ == nullptr
        ? Availability{false, "CUDA processor has been moved from."}
        : impl_->availability();
}

ProcessingResult Processor::process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    std::uint32_t requested_batch_size,
    const BatchProgressSink& progress)
{
    if (impl_ == nullptr)
    {
        return failure(
            ProcessingIssueCode::BackendFailure,
            "CUDA processor has been moved from.");
    }
    return impl_->process_batch(
        images,
        watermark,
        config,
        requested_batch_size,
        progress);
}

}  // namespace parallelpix::cuda
