#include "runtime/runtime.hpp"

#include <atomic>
#include <limits>
#include <utility>

namespace parallelpix::cuda::detail {
namespace {

std::atomic<std::size_t> allocation_limit{
    std::numeric_limits<std::size_t>::max()};

}  // namespace

void set_allocation_limit_for_testing(std::size_t bytes) noexcept
{
    allocation_limit.store(bytes, std::memory_order_relaxed);
}

void reset_allocation_limit_for_testing() noexcept
{
    allocation_limit.store(
        std::numeric_limits<std::size_t>::max(),
        std::memory_order_relaxed);
}

Stream::~Stream()
{
    if (stream_ != nullptr)
    {
        cudaStreamDestroy(stream_);
    }
}

Stream::Stream(Stream&& other) noexcept
    : stream_(std::exchange(other.stream_, nullptr))
{
}

Stream& Stream::operator=(Stream&& other) noexcept
{
    if (this != &other)
    {
        if (stream_ != nullptr)
        {
            cudaStreamDestroy(stream_);
        }
        stream_ = std::exchange(other.stream_, nullptr);
    }
    return *this;
}

void Stream::create()
{
    if (stream_ == nullptr)
    {
        check(
            cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
            "cudaStreamCreateWithFlags");
    }
}

cudaStream_t Stream::get() const noexcept
{
    return stream_;
}

Event::~Event()
{
    if (event_ != nullptr)
    {
        cudaEventDestroy(event_);
    }
}

Event::Event(Event&& other) noexcept
    : event_(std::exchange(other.event_, nullptr))
{
}

Event& Event::operator=(Event&& other) noexcept
{
    if (this != &other)
    {
        if (event_ != nullptr)
        {
            cudaEventDestroy(event_);
        }
        event_ = std::exchange(other.event_, nullptr);
    }
    return *this;
}

void Event::create()
{
    if (event_ == nullptr)
    {
        check(cudaEventCreate(&event_), "cudaEventCreate");
    }
}

void Event::record(cudaStream_t stream)
{
    check(cudaEventRecord(event_, stream), "cudaEventRecord");
}

void Event::synchronize()
{
    check(cudaEventSynchronize(event_), "cudaEventSynchronize");
}

double Event::elapsed_ms_to(const Event& stop) const
{
    float elapsed = 0.0F;
    check(
        cudaEventElapsedTime(&elapsed, event_, stop.event_),
        "cudaEventElapsedTime");
    return static_cast<double>(elapsed);
}

DeviceBuffer::~DeviceBuffer()
{
    if (data_ != nullptr)
    {
        cudaFree(data_);
    }
}

DeviceBuffer::DeviceBuffer(DeviceBuffer&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)),
      capacity_(std::exchange(other.capacity_, 0))
{
}

DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (data_ != nullptr)
        {
            cudaFree(data_);
        }
        data_ = std::exchange(other.data_, nullptr);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    return *this;
}

void DeviceBuffer::reserve(std::size_t bytes)
{
    if (bytes > allocation_limit.load(std::memory_order_relaxed))
    {
        throw RuntimeError(
            cudaErrorMemoryAllocation,
            "injected CUDA allocation limit");
    }
    if (bytes <= capacity_)
    {
        return;
    }

    void* replacement = nullptr;
    check(cudaMalloc(&replacement, bytes), "cudaMalloc");
    if (data_ != nullptr)
    {
        const auto free_result = cudaFree(data_);
        if (free_result != cudaSuccess)
        {
            cudaFree(replacement);
            throw RuntimeError(free_result, "cudaFree");
        }
    }
    data_ = replacement;
    capacity_ = bytes;
}

void* DeviceBuffer::data() noexcept
{
    return data_;
}

const void* DeviceBuffer::data() const noexcept
{
    return data_;
}

std::size_t DeviceBuffer::capacity() const noexcept
{
    return capacity_;
}

}  // namespace parallelpix::cuda::detail
