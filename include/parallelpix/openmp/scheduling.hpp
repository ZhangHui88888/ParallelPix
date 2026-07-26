#pragma once

#include <cstddef>
#include <cstdint>

namespace parallelpix::openmp {

enum class SchedulingStrategy
{
    Images,
    Rows,
};

[[nodiscard]] SchedulingStrategy choose_scheduling_strategy(
    std::size_t image_count,
    std::uint32_t thread_count) noexcept;

}  // namespace parallelpix::openmp
