#include "parallelpix/benchmark/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace parallelpix::benchmark {

std::optional<SummaryStatistics> summarize_samples(
    const std::vector<double>& samples)
{
    if (samples.empty() ||
        std::any_of(samples.begin(), samples.end(), [](double value) {
            return !std::isfinite(value) || value < 0.0;
        }))
    {
        return std::nullopt;
    }

    auto ordered = samples;
    std::sort(ordered.begin(), ordered.end());

    const auto middle = ordered.size() / 2;
    const auto median = ordered.size() % 2 == 0
        ? (ordered[middle - 1] + ordered[middle]) / 2.0
        : ordered[middle];
    const auto mean =
        std::accumulate(ordered.begin(), ordered.end(), 0.0) /
        static_cast<double>(ordered.size());

    double squared_difference_sum = 0.0;
    for (const auto value : ordered)
    {
        const auto difference = value - mean;
        squared_difference_sum += difference * difference;
    }

    return SummaryStatistics{
        median,
        ordered.front(),
        ordered.back(),
        std::sqrt(
            squared_difference_sum / static_cast<double>(ordered.size())),
    };
}

}  // namespace parallelpix::benchmark
