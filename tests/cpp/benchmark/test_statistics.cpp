#include "parallelpix/benchmark/statistics.hpp"
#include "test_support.hpp"

#include <cmath>
#include <limits>
#include <vector>

PP_TEST("benchmark statistics reject empty invalid and negative samples")
{
    PP_REQUIRE(
        !parallelpix::benchmark::summarize_samples({}).has_value());
    PP_REQUIRE(
        !parallelpix::benchmark::summarize_samples({-1.0}).has_value());
    PP_REQUIRE(!parallelpix::benchmark::summarize_samples(
                    {std::numeric_limits<double>::infinity()})
                    .has_value());
}

PP_TEST("benchmark statistics summarize single odd and even populations")
{
    const auto single =
        parallelpix::benchmark::summarize_samples({4.0});
    PP_REQUIRE(single.has_value());
    PP_REQUIRE_EQ(single->median_ms, 4.0);
    PP_REQUIRE_EQ(single->min_ms, 4.0);
    PP_REQUIRE_EQ(single->max_ms, 4.0);
    PP_REQUIRE_EQ(single->stddev_ms, 0.0);

    const auto odd =
        parallelpix::benchmark::summarize_samples({9.0, 1.0, 5.0});
    PP_REQUIRE(odd.has_value());
    PP_REQUIRE_EQ(odd->median_ms, 5.0);
    PP_REQUIRE_EQ(odd->min_ms, 1.0);
    PP_REQUIRE_EQ(odd->max_ms, 9.0);

    const auto even =
        parallelpix::benchmark::summarize_samples({8.0, 2.0, 6.0, 4.0});
    PP_REQUIRE(even.has_value());
    PP_REQUIRE_EQ(even->median_ms, 5.0);
    PP_REQUIRE_EQ(even->min_ms, 2.0);
    PP_REQUIRE_EQ(even->max_ms, 8.0);
    PP_REQUIRE(std::abs(even->stddev_ms - std::sqrt(5.0)) < 0.000001);
}
