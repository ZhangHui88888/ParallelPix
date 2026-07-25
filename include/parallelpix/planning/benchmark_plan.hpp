#pragma once

#include "parallelpix/cli/cli.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace parallelpix::m2 {

struct ExperimentSpec
{
    Backend backend = Backend::Sequential;
    std::uint32_t image_count = 0;
    std::optional<std::uint32_t> thread_count;
    std::optional<std::uint32_t> cuda_batch_size;
};

struct BenchmarkPlan
{
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
    std::filesystem::path watermark_path;
    std::filesystem::path result_csv;
    std::vector<ExperimentSpec> experiments;
    std::uint32_t warmups = 2;
    std::uint32_t repetitions = 5;
    CsvMode csv_mode = CsvMode::Overwrite;
};

BenchmarkPlan build_benchmark_plan(const BenchmarkRequest& request);

}  // namespace parallelpix::m2
