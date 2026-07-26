#include "parallelpix/benchmark/runner.hpp"
#include "io_test_support.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using parallelpix::benchmark::BackendExecution;
using parallelpix::benchmark::BackendAvailability;
using parallelpix::benchmark::CudaPhaseTiming;
using parallelpix::benchmark::IBackendExecutor;
using parallelpix::m2::Backend;
using parallelpix::m2::BenchmarkPlan;
using parallelpix::m2::CsvMode;
using parallelpix::m2::ExperimentSpec;
using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::copy_fixture;

class CopyExecutor final : public IBackendExecutor
{
public:
    CopyExecutor(
        Backend backend,
        bool corrupt,
        std::optional<std::uint32_t> effective_cuda_batch_size = std::nullopt)
        : backend_(backend),
          corrupt_(corrupt),
          effective_cuda_batch_size_(effective_cuda_batch_size)
    {
    }

    Backend backend() const noexcept override
    {
        return backend_;
    }

    BackendExecution execute(
        const std::vector<parallelpix::Image>& images,
        const parallelpix::Watermark&,
        const parallelpix::ProcessingConfig&,
        const ExperimentSpec& experiment,
        const parallelpix::benchmark::ProgressSink&) override
    {
        ++calls;
        auto outputs = images;
        if (corrupt_ && !outputs.empty() && !outputs.front().pixels.empty())
        {
            outputs.front().pixels.front() =
                static_cast<std::uint8_t>(outputs.front().pixels.front() ^ 0x03);
        }
        parallelpix::BatchProcessingResult result;
        result.images = std::move(outputs);
        if (backend_ == Backend::Cuda)
        {
            return {
                std::move(result),
                parallelpix::benchmark::CudaPhaseTiming{1.0, 2.0, 3.0},
                effective_cuda_batch_size_.value_or(
                    experiment.cuda_batch_size.value_or(0)),
            };
        }
        return {
            std::move(result),
            std::nullopt,
            std::nullopt,
        };
    }

    std::size_t calls = 0;

private:
    Backend backend_;
    bool corrupt_;
    std::optional<std::uint32_t> effective_cuda_batch_size_;
};

class FailingExecutor final : public IBackendExecutor
{
public:
    Backend backend() const noexcept override
    {
        return Backend::Sequential;
    }

    BackendExecution execute(
        const std::vector<parallelpix::Image>&,
        const parallelpix::Watermark&,
        const parallelpix::ProcessingConfig&,
        const ExperimentSpec&,
        const parallelpix::benchmark::ProgressSink&) override
    {
        ++calls;
        parallelpix::BatchProcessingResult result;
        result.issues.push_back({
            parallelpix::ProcessingIssueCode::InvalidImage,
            std::nullopt,
            {},
            "Injected Sequential failure.",
        });
        return {
            std::move(result),
            std::nullopt,
            std::nullopt,
        };
    }

    std::size_t calls = 0;
};

class ProgressCopyExecutor final : public IBackendExecutor
{
public:
    Backend backend() const noexcept override
    {
        return Backend::Sequential;
    }

    BackendExecution execute(
        const std::vector<parallelpix::Image>& images,
        const parallelpix::Watermark&,
        const parallelpix::ProcessingConfig&,
        const ExperimentSpec&,
        const parallelpix::benchmark::ProgressSink& progress) override
    {
        executing = true;
        if (progress)
        {
            progress({images.size(), 1.0});
        }
        executing = false;

        parallelpix::BatchProcessingResult result;
        result.images = images;
        return {std::move(result), std::nullopt};
    }

    bool executing = false;
};

class UnavailableCudaExecutor final : public IBackendExecutor
{
public:
    Backend backend() const noexcept override
    {
        return Backend::Cuda;
    }

    BackendAvailability availability() const override
    {
        return {false, "Injected CUDA device discovery failure."};
    }

    BackendExecution execute(
        const std::vector<parallelpix::Image>&,
        const parallelpix::Watermark&,
        const parallelpix::ProcessingConfig&,
        const ExperimentSpec&,
        const parallelpix::benchmark::ProgressSink&) override
    {
        ++calls;
        return {};
    }

    std::size_t calls = 0;
};

class TimedCudaCopyExecutor final : public IBackendExecutor
{
public:
    Backend backend() const noexcept override
    {
        return Backend::Cuda;
    }

    BackendExecution execute(
        const std::vector<parallelpix::Image>& images,
        const parallelpix::Watermark&,
        const parallelpix::ProcessingConfig&,
        const ExperimentSpec&,
        const parallelpix::benchmark::ProgressSink&) override
    {
        ++calls;
        parallelpix::BatchProcessingResult result;
        result.images = images;
        return {
            std::move(result),
            CudaPhaseTiming{1.0, 2.0, 3.0},
            std::uint32_t{2},
        };
    }

    std::size_t calls = 0;
};

struct FixturePaths
{
    TemporaryDirectory temporary;
    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path watermark;
    std::filesystem::path csv;

    FixturePaths()
    {
        input = temporary.path() / L"输入";
        output = temporary.path() / L"输出";
        watermark = temporary.path() / L"水印.png";
        csv = temporary.path() / L"结果" / L"benchmark.csv";
        copy_fixture("images/color_3x2.png", input / L"商品.png");
        copy_fixture("watermarks/rgba_2x2.png", watermark);
    }
};

BenchmarkPlan make_plan(const FixturePaths& paths)
{
    return {
        paths.input,
        paths.output,
        paths.watermark,
        paths.csv,
        {},
        2,
        5,
        CsvMode::Overwrite,
    };
}

std::vector<std::string> split_csv_row(const std::string& row)
{
    std::vector<std::string> fields;
    std::istringstream stream(row);
    std::string field;
    while (std::getline(stream, field, ','))
    {
        fields.push_back(field);
    }
    if (!row.empty() && row.back() == ',')
    {
        fields.emplace_back();
    }
    return fields;
}

}  // namespace

PP_TEST("benchmark runner executes exact warmups and repetitions")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
    };
    CopyExecutor sequential(Backend::Sequential, false);

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential}, [](const auto&) {});

    PP_REQUIRE_EQ(sequential.calls, std::size_t{7});
    PP_REQUIRE_EQ(summary.planned, std::size_t{1});
    PP_REQUIRE_EQ(summary.succeeded, std::size_t{1});
    PP_REQUIRE_EQ(summary.failed, std::size_t{0});
    PP_REQUIRE_EQ(summary.skipped, std::size_t{0});
    PP_REQUIRE_EQ(summary.csv_path, std::optional(paths.csv));
    PP_REQUIRE(std::filesystem::is_regular_file(paths.csv));
}

PP_TEST("benchmark runner emits trajectory logs after measured execution")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
    };
    ProgressCopyExecutor sequential;
    bool trajectory_logged = false;
    bool trajectory_logged_during_execution = false;

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan,
        {&sequential},
        [&](const auto& event) {
            if (event.stage == "trajectory")
            {
                trajectory_logged = true;
                trajectory_logged_during_execution = sequential.executing;
            }
        });

    PP_REQUIRE_EQ(summary.succeeded, std::size_t{1});
    PP_REQUIRE(trajectory_logged);
    PP_REQUIRE(!trajectory_logged_during_execution);
}

PP_TEST("benchmark runner records invalid output and skips unavailable backend")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
        {Backend::OpenMP, 1, 2, std::nullopt},
        {Backend::Cuda, 1, std::nullopt, 1},
    };
    CopyExecutor sequential(Backend::Sequential, false);
    CopyExecutor openmp(Backend::OpenMP, true);

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential, &openmp}, [](const auto&) {});

    PP_REQUIRE_EQ(summary.planned, std::size_t{3});
    PP_REQUIRE_EQ(summary.succeeded, std::size_t{1});
    PP_REQUIRE_EQ(summary.failed, std::size_t{1});
    PP_REQUIRE_EQ(summary.skipped, std::size_t{1});
    PP_REQUIRE(summary.csv_path.has_value());

    std::ifstream stream(paths.csv);
    std::string contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    PP_REQUIRE(contents.find("openmp,2,,1,") != std::string::npos);
    PP_REQUIRE(contents.find(",false,3,,,") != std::string::npos);
}

PP_TEST("benchmark runner skips a compiled CUDA backend without a device")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
        {Backend::Cuda, 1, std::nullopt, 4},
    };
    CopyExecutor sequential(Backend::Sequential, false);
    UnavailableCudaExecutor cuda;

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential, &cuda}, [](const auto&) {});

    PP_REQUIRE_EQ(summary.succeeded, std::size_t{1});
    PP_REQUIRE_EQ(summary.skipped, std::size_t{1});
    PP_REQUIRE_EQ(summary.failed, std::size_t{0});
    PP_REQUIRE_EQ(cuda.calls, std::size_t{0});
    PP_REQUIRE(std::any_of(
        summary.issues.begin(),
        summary.issues.end(),
        [](const auto& issue) {
            return issue.message.find("device discovery failure") !=
                std::string::npos;
        }));
}

PP_TEST("benchmark runner records CUDA phases and effective batch size")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
        {Backend::Cuda, 1, std::nullopt, 4},
    };
    CopyExecutor sequential(Backend::Sequential, false);
    TimedCudaCopyExecutor cuda;

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential, &cuda}, [](const auto&) {});
    PP_REQUIRE_EQ(summary.succeeded, std::size_t{2});
    PP_REQUIRE_EQ(cuda.calls, std::size_t{7});

    std::ifstream stream(paths.csv);
    std::string header;
    std::string sequential_row;
    std::string cuda_row;
    std::getline(stream, header);
    std::getline(stream, sequential_row);
    std::getline(stream, cuda_row);
    const auto fields = split_csv_row(cuda_row);
    PP_REQUIRE_EQ(fields.size(), std::size_t{27});
    PP_REQUIRE_EQ(fields[2], std::string("cuda"));
    PP_REQUIRE_EQ(fields[4], std::string("2"));
    PP_REQUIRE_EQ(fields[24], std::string("1"));
    PP_REQUIRE_EQ(fields[25], std::string("2"));
    PP_REQUIRE_EQ(fields[26], std::string("3"));
}

PP_TEST("benchmark runner derives throughput speedup and OpenMP efficiency")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
        {Backend::OpenMP, 1, 2, std::nullopt},
    };
    CopyExecutor sequential(Backend::Sequential, false);
    CopyExecutor openmp(Backend::OpenMP, false);

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential, &openmp}, [](const auto&) {});
    PP_REQUIRE_EQ(summary.succeeded, std::size_t{2});

    std::ifstream stream(paths.csv);
    std::string header;
    std::string sequential_row;
    std::string openmp_row;
    std::getline(stream, header);
    std::getline(stream, sequential_row);
    std::getline(stream, openmp_row);
    const auto sequential_fields = split_csv_row(sequential_row);
    const auto openmp_fields = split_csv_row(openmp_row);
    PP_REQUIRE_EQ(sequential_fields.size(), std::size_t{27});
    PP_REQUIRE_EQ(openmp_fields.size(), std::size_t{27});
    PP_REQUIRE_EQ(sequential_fields[20], std::string("1"));

    const auto compute_ms = std::stod(openmp_fields[10]);
    const auto images_per_second = std::stod(openmp_fields[18]);
    const auto speedup = std::stod(openmp_fields[20]);
    const auto efficiency = std::stod(openmp_fields[21]);
    PP_REQUIRE(
        std::abs(images_per_second * compute_ms / 1000.0 - 1.0) <
        0.001);
    PP_REQUIRE(speedup > 0.0);
    PP_REQUIRE(std::abs(efficiency * 2.0 - speedup) < 0.00001);
}

PP_TEST("benchmark runner skips dependent backends when sequential fails")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
        {Backend::OpenMP, 1, 2, std::nullopt},
    };
    FailingExecutor sequential;
    CopyExecutor openmp(Backend::OpenMP, false);

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential, &openmp}, [](const auto&) {});

    PP_REQUIRE_EQ(sequential.calls, std::size_t{1});
    PP_REQUIRE_EQ(openmp.calls, std::size_t{0});
    PP_REQUIRE_EQ(summary.succeeded, std::size_t{0});
    PP_REQUIRE_EQ(summary.failed, std::size_t{1});
    PP_REQUIRE_EQ(summary.skipped, std::size_t{1});
    PP_REQUIRE(!summary.csv_path.has_value());
    PP_REQUIRE_EQ(
        summary.primary_failure,
        std::optional<parallelpix::m2::FailureCategory>{
            parallelpix::m2::FailureCategory::Processing});
}

PP_TEST("benchmark runner append mode creates a new run identifier")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.csv_mode = CsvMode::Append;
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
    };
    CopyExecutor sequential(Backend::Sequential, false);

    const auto first = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential}, [](const auto&) {});
    const auto second = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential}, [](const auto&) {});
    PP_REQUIRE_EQ(first.succeeded, std::size_t{1});
    PP_REQUIRE_EQ(second.succeeded, std::size_t{1});

    std::ifstream stream(paths.csv);
    std::string header;
    std::string first_row;
    std::string second_row;
    std::getline(stream, header);
    std::getline(stream, first_row);
    std::getline(stream, second_row);
    const auto first_separator = first_row.find(',');
    const auto second_separator = second_row.find(',');
    PP_REQUIRE(first_separator != std::string::npos);
    PP_REQUIRE(second_separator != std::string::npos);
    PP_REQUIRE(
        first_row.substr(0, first_separator) !=
        second_row.substr(0, second_separator));
}

PP_TEST("benchmark runner maps invalid input to a global failure")
{
    FixturePaths paths;
    auto plan = make_plan(paths);
    plan.input_dir = paths.temporary.path() / "missing";
    plan.experiments = {
        {Backend::Sequential, 1, std::nullopt, std::nullopt},
    };
    CopyExecutor sequential(Backend::Sequential, false);

    const auto summary = parallelpix::benchmark::run_benchmark_plan(
        plan, {&sequential}, [](const auto&) {});

    PP_REQUIRE_EQ(summary.failed, std::size_t{1});
    PP_REQUIRE_EQ(sequential.calls, std::size_t{0});
    PP_REQUIRE(!summary.csv_path.has_value());
    PP_REQUIRE_EQ(
        summary.primary_failure,
        std::optional<parallelpix::m2::FailureCategory>{
            parallelpix::m2::FailureCategory::Input});
}
