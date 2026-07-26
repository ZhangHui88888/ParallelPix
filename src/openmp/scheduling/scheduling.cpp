#include "parallelpix/openmp/scheduling.hpp"

namespace parallelpix::openmp {

SchedulingStrategy choose_scheduling_strategy(
    std::size_t image_count,
    std::uint32_t thread_count) noexcept
{
    if (thread_count > 1 && image_count < thread_count)
    {
        return SchedulingStrategy::Rows;
    }
    return SchedulingStrategy::Images;
}

}  // namespace parallelpix::openmp
