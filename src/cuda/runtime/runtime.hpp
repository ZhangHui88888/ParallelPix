#pragma once

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace parallelpix::cuda::detail {

class RuntimeError final : public std::runtime_error
{
public:
    RuntimeError(cudaError_t code, const std::string& operation);

    [[nodiscard]] cudaError_t code() const noexcept;

private:
    cudaError_t code_;
};

void check(cudaError_t code, const char* operation);

struct DeviceStatus
{
    bool available = false;
    std::string message;
    int device_index = 0;
    std::uint32_t max_grid_z = 0;
};

[[nodiscard]] DeviceStatus query_device_status();

enum class RuntimeFault
{
    None,
    NoDevice,
    ApiFailure,
    KernelFailure,
};

void set_runtime_fault_for_testing(RuntimeFault fault) noexcept;
void reset_runtime_fault_for_testing() noexcept;
[[nodiscard]] bool has_runtime_fault_for_testing(
    RuntimeFault fault) noexcept;

void set_allocation_limit_for_testing(std::size_t bytes) noexcept;
void reset_allocation_limit_for_testing() noexcept;

class Stream
{
public:
    Stream() = default;
    ~Stream();

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    Stream(Stream&& other) noexcept;
    Stream& operator=(Stream&& other) noexcept;

    void create();
    [[nodiscard]] cudaStream_t get() const noexcept;

private:
    cudaStream_t stream_ = nullptr;
};

class Event
{
public:
    Event() = default;
    ~Event();

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&& other) noexcept;
    Event& operator=(Event&& other) noexcept;

    void create();
    void record(cudaStream_t stream);
    void synchronize();
    [[nodiscard]] double elapsed_ms_to(const Event& stop) const;

private:
    cudaEvent_t event_ = nullptr;
};

class DeviceBuffer
{
public:
    DeviceBuffer() = default;
    ~DeviceBuffer();

    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    DeviceBuffer(DeviceBuffer&& other) noexcept;
    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept;

    void reserve(std::size_t bytes);
    [[nodiscard]] void* data() noexcept;
    [[nodiscard]] const void* data() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;

private:
    void* data_ = nullptr;
    std::size_t capacity_ = 0;
};

}  // namespace parallelpix::cuda::detail
