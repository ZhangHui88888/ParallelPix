#pragma once

#include "parallelpix/cli/cli.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace parallelpix::benchmark {

struct BenchmarkRecord
{
    std::string run_id;
    std::string recorded_at_utc;
    std::string backend;
    std::optional<std::uint32_t> thread_count;
    std::optional<std::uint32_t> cuda_batch_size;
    std::uint32_t image_count = 0;
    std::string input_resolution;
    std::string output_resolution;
    std::uint32_t warmups = 0;
    std::uint32_t repetitions = 0;
    double compute_ms = 0.0;
    double compute_min_ms = 0.0;
    double compute_max_ms = 0.0;
    double compute_stddev_ms = 0.0;
    double end_to_end_ms = 0.0;
    double end_to_end_min_ms = 0.0;
    double end_to_end_max_ms = 0.0;
    double end_to_end_stddev_ms = 0.0;
    double images_per_second = 0.0;
    double megapixels_per_second = 0.0;
    std::optional<double> speedup;
    std::optional<double> parallel_efficiency;
    bool validation_passed = false;
    std::optional<std::uint32_t> max_pixel_error;
    std::optional<double> h2d_ms;
    std::optional<double> kernel_ms;
    std::optional<double> d2h_ms;
};

struct ReportingResult
{
    bool success = false;
    std::string message;
};

[[nodiscard]] std::string_view benchmark_csv_header() noexcept;

[[nodiscard]] ReportingResult validate_csv_destination(
    const std::filesystem::path& destination,
    m2::CsvMode mode);

[[nodiscard]] ReportingResult write_benchmark_records(
    const std::filesystem::path& destination,
    m2::CsvMode mode,
    const std::vector<BenchmarkRecord>& records);

}  // namespace parallelpix::benchmark
