#pragma once

#include <optional>
#include <vector>

namespace parallelpix::benchmark {

struct SummaryStatistics
{
    double median_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    double stddev_ms = 0.0;
};

[[nodiscard]] std::optional<SummaryStatistics> summarize_samples(
    const std::vector<double>& samples);

}  // namespace parallelpix::benchmark
