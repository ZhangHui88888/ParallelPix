#include "parallelpix/openmp/scheduling.hpp"
#include "test_support.hpp"

PP_TEST("OpenMP scheduling prefers image-level parallelism")
{
    using parallelpix::openmp::SchedulingStrategy;
    using parallelpix::openmp::choose_scheduling_strategy;

    PP_REQUIRE_EQ(
        choose_scheduling_strategy(8, 4),
        SchedulingStrategy::Images);
    PP_REQUIRE_EQ(
        choose_scheduling_strategy(4, 4),
        SchedulingStrategy::Images);
    PP_REQUIRE_EQ(
        choose_scheduling_strategy(1, 1),
        SchedulingStrategy::Images);
}

PP_TEST("OpenMP scheduling falls back to row parallelism for small batches")
{
    using parallelpix::openmp::SchedulingStrategy;
    using parallelpix::openmp::choose_scheduling_strategy;

    PP_REQUIRE_EQ(
        choose_scheduling_strategy(1, 8),
        SchedulingStrategy::Rows);
    PP_REQUIRE_EQ(
        choose_scheduling_strategy(2, 4),
        SchedulingStrategy::Rows);
}
