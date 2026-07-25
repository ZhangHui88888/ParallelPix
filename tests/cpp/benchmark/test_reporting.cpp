#include "parallelpix/benchmark/reporting.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

using parallelpix::benchmark::BenchmarkRecord;
using parallelpix::test::io::TemporaryDirectory;

BenchmarkRecord make_record(std::string run_id)
{
    BenchmarkRecord record;
    record.run_id = std::move(run_id);
    record.recorded_at_utc = "2026-07-25T10:00:00.000Z";
    record.backend = "sequential";
    record.image_count = 1;
    record.input_resolution = "4x3";
    record.output_resolution = "1024x1024";
    record.warmups = 2;
    record.repetitions = 5;
    record.compute_ms = 10.0;
    record.compute_min_ms = 9.0;
    record.compute_max_ms = 11.0;
    record.compute_stddev_ms = 0.5;
    record.end_to_end_ms = 20.0;
    record.end_to_end_min_ms = 19.0;
    record.end_to_end_max_ms = 21.0;
    record.end_to_end_stddev_ms = 0.75;
    record.images_per_second = 100.0;
    record.megapixels_per_second = 104.8576;
    record.speedup = 1.0;
    record.validation_passed = true;
    record.max_pixel_error = 0;
    return record;
}

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    std::ostringstream output;
    output << stream.rdbuf();
    return output.str();
}

std::size_t line_count(const std::string& text)
{
    std::size_t count = 0;
    for (const auto character : text)
    {
        if (character == '\n')
        {
            ++count;
        }
    }
    return count;
}

}  // namespace

PP_TEST("benchmark reporting writes the exact M1 schema and empty optionals")
{
    TemporaryDirectory temporary;
    const auto path = temporary.path() / L"结果" / L"基准.csv";

    const auto result = parallelpix::benchmark::write_benchmark_records(
        path,
        parallelpix::m2::CsvMode::Overwrite,
        {make_record("run-one")});
    PP_REQUIRE(result.success);

    const auto text = read_text(path);
    PP_REQUIRE(
        text.rfind(
            std::string(parallelpix::benchmark::benchmark_csv_header()) + '\n',
            0) == 0);
    PP_REQUIRE(text.find("run-one") != std::string::npos);
    PP_REQUIRE(text.find("sequential,,,1,") != std::string::npos);
    PP_REQUIRE_EQ(line_count(text), std::size_t{2});
}

PP_TEST("benchmark reporting appends atomically and rejects a foreign header")
{
    TemporaryDirectory temporary;
    const auto path = temporary.path() / "benchmark.csv";
    auto first = make_record("run-one");
    auto second = make_record("run-two");

    PP_REQUIRE(parallelpix::benchmark::write_benchmark_records(
                   path,
                   parallelpix::m2::CsvMode::Overwrite,
                   {first})
                   .success);
    PP_REQUIRE(parallelpix::benchmark::write_benchmark_records(
                   path,
                   parallelpix::m2::CsvMode::Append,
                   {second})
                   .success);
    const auto appended = read_text(path);
    PP_REQUIRE_EQ(line_count(appended), std::size_t{3});
    PP_REQUIRE(appended.find("run-one") != std::string::npos);
    PP_REQUIRE(appended.find("run-two") != std::string::npos);

    PP_REQUIRE(parallelpix::benchmark::write_benchmark_records(
                   path,
                   parallelpix::m2::CsvMode::Overwrite,
                   {second})
                   .success);
    const auto overwritten = read_text(path);
    PP_REQUIRE(overwritten.find("run-one") == std::string::npos);
    PP_REQUIRE(overwritten.find("run-two") != std::string::npos);
    PP_REQUIRE_EQ(line_count(overwritten), std::size_t{2});

    const auto foreign = temporary.path() / "foreign.csv";
    {
        std::ofstream stream(foreign, std::ios::binary);
        stream << "other,header\nuntouched,row\n";
    }
    const auto before = read_text(foreign);
    const auto rejected =
        parallelpix::benchmark::write_benchmark_records(
            foreign,
            parallelpix::m2::CsvMode::Append,
            {first});
    PP_REQUIRE(!rejected.success);
    PP_REQUIRE_EQ(read_text(foreign), before);
}

PP_TEST("benchmark reporting rejects empty record batches")
{
    TemporaryDirectory temporary;
    const auto result = parallelpix::benchmark::write_benchmark_records(
        temporary.path() / "empty.csv",
        parallelpix::m2::CsvMode::Overwrite,
        {});
    PP_REQUIRE(!result.success);
}
