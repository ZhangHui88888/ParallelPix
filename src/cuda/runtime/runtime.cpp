#include "runtime/runtime.hpp"

#include <atomic>
#include <string>

namespace parallelpix::cuda::detail {
namespace {

std::atomic<RuntimeFault> runtime_fault{RuntimeFault::None};

std::string error_message(
    cudaError_t code,
    const std::string& operation)
{
    const auto* description = cudaGetErrorString(code);
    return operation + " failed: " +
        (description == nullptr ? std::string("unknown CUDA error")
                                : std::string(description));
}

}  // namespace

RuntimeError::RuntimeError(
    cudaError_t code,
    const std::string& operation)
    : std::runtime_error(error_message(code, operation)),
      code_(code)
{
}

cudaError_t RuntimeError::code() const noexcept
{
    return code_;
}

void check(cudaError_t code, const char* operation)
{
    if (code != cudaSuccess)
    {
        throw RuntimeError(code, operation);
    }
}

DeviceStatus query_device_status()
{
    if (has_runtime_fault_for_testing(RuntimeFault::NoDevice))
    {
        return {
            false,
            "Injected CUDA device discovery failure.",
            0,
            0,
        };
    }

    int count = 0;
    const auto count_result = cudaGetDeviceCount(&count);
    if (count_result != cudaSuccess)
    {
        return {
            false,
            error_message(count_result, "CUDA device discovery"),
            0,
            0,
        };
    }
    if (count <= 0)
    {
        return {
            false,
            "No CUDA-capable device is available.",
            0,
            0,
        };
    }

    cudaDeviceProp properties{};
    const auto properties_result =
        cudaGetDeviceProperties(&properties, 0);
    if (properties_result != cudaSuccess)
    {
        return {
            false,
            error_message(properties_result, "CUDA device properties"),
            0,
            0,
        };
    }
    if (properties.maxGridSize[2] <= 0)
    {
        return {
            false,
            "CUDA device reports an invalid grid-z limit.",
            0,
            0,
        };
    }

    const auto max_grid_z =
        static_cast<std::uint32_t>(properties.maxGridSize[2]);
    return {
        true,
        "CUDA device 0 is available: " +
            std::string(properties.name),
        0,
        max_grid_z,
    };
}

void set_runtime_fault_for_testing(RuntimeFault fault) noexcept
{
    runtime_fault.store(fault, std::memory_order_relaxed);
}

void reset_runtime_fault_for_testing() noexcept
{
    runtime_fault.store(RuntimeFault::None, std::memory_order_relaxed);
}

bool has_runtime_fault_for_testing(RuntimeFault fault) noexcept
{
    return runtime_fault.load(std::memory_order_relaxed) == fault;
}

}  // namespace parallelpix::cuda::detail
