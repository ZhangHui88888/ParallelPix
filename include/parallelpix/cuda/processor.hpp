#pragma once

#include "parallelpix/common/processing.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace parallelpix::cuda {

struct Availability
{
    bool available = false;
    std::string message;
};

struct PhaseTiming
{
    double h2d_ms = 0.0;
    double kernel_ms = 0.0;
    double d2h_ms = 0.0;
};

struct ProcessingResult
{
    BatchProcessingResult processing;
    std::optional<PhaseTiming> phase_timing;
    std::uint32_t effective_batch_size = 0;
};

using BatchProgressSink = std::function<void(std::size_t, double)>;

class Processor
{
public:
    Processor();
    ~Processor();

    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;
    Processor(Processor&&) noexcept;
    Processor& operator=(Processor&&) noexcept;

    [[nodiscard]] Availability availability() const;

    ProcessingResult process_batch(
        const std::vector<Image>& images,
        const Watermark& watermark,
        const ProcessingConfig& config,
        std::uint32_t requested_batch_size,
        const BatchProgressSink& progress = {});

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace parallelpix::cuda
