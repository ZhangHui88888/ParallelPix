#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace parallelpix::m2 {

enum class Backend
{
    Sequential,
    OpenMP,
    Cuda,
};

enum class CsvMode
{
    Overwrite,
    Append,
};

struct BenchmarkRequest
{
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
    std::filesystem::path watermark_path;
    std::filesystem::path result_csv;
    std::vector<Backend> backends;
    std::vector<std::uint32_t> image_counts;
    std::vector<std::uint32_t> thread_counts;
    std::vector<std::uint32_t> cuda_batch_sizes;
    std::uint32_t warmups = 2;
    std::uint32_t repetitions = 5;
    bool cold_start = false;
    CsvMode csv_mode = CsvMode::Overwrite;
};

struct HelpRequest
{
};

struct CliError
{
    std::vector<std::string> messages;
};

using CliParseResult = std::variant<HelpRequest, BenchmarkRequest, CliError>;

CliParseResult parse_cli(const std::vector<std::string_view>& arguments);

}  // namespace parallelpix::m2
