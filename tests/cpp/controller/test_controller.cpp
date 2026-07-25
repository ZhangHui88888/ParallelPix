#include "../test_support.hpp"

#include "parallelpix/controller/controller.hpp"

#include <filesystem>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using parallelpix::m2::BenchmarkPlan;
using parallelpix::m2::ExitCode;
using parallelpix::m2::FailureCategory;
using parallelpix::m2::IBenchmarkPipeline;
using parallelpix::m2::LogEvent;
using parallelpix::m2::LogLevel;
using parallelpix::m2::LogSink;
using parallelpix::m2::WorkflowIssue;
using parallelpix::m2::WorkflowSummary;
using parallelpix::m2::run_cli;

std::vector<std::string_view> valid_arguments()
{
    return {
        "benchmark",
        "--input", "data/images",
        "--output", "output",
        "--watermark", "data/watermark.png",
        "--backends", "sequential,openmp,cuda",
        "--image-counts", "10,50,100",
        "--threads", "1,2,4,8",
        "--cuda-batches", "1,4,8",
        "--warmups", "2",
        "--repetitions", "5",
        "--csv", "results/benchmark.csv",
        "--append",
    };
}

WorkflowSummary successful_summary()
{
    return {
        24,
        24,
        0,
        0,
        std::filesystem::path("results/benchmark.csv"),
        {},
        std::nullopt,
    };
}

class FakePipeline final : public IBenchmarkPipeline
{
public:
    WorkflowSummary response = successful_summary();
    int calls = 0;
    std::optional<BenchmarkPlan> received_plan;
    bool should_throw = false;
    bool emit_progress = false;

    WorkflowSummary execute(const BenchmarkPlan& plan, const LogSink& log) override
    {
        ++calls;
        received_plan = plan;
        if (should_throw)
        {
            throw std::runtime_error("pipeline exploded");
        }
        if (emit_progress)
        {
            log({LogLevel::Info, "pipeline", "processing started"});
        }
        return response;
    }
};

PP_TEST("help and invalid requests never invoke the benchmark pipeline")
{
    FakePipeline pipeline;
    std::ostringstream output;
    std::ostringstream errors;

    const auto help_code = run_cli({"--help"}, pipeline, output, errors);
    const auto invalid_code = run_cli({"benchmark"}, pipeline, output, errors);

    PP_REQUIRE_EQ(help_code, static_cast<int>(ExitCode::Success));
    PP_REQUIRE_EQ(invalid_code, static_cast<int>(ExitCode::InvalidArguments));
    PP_REQUIRE_EQ(pipeline.calls, 0);
    PP_REQUIRE(output.str().find("Usage:") != std::string::npos);
    PP_REQUIRE(errors.str().find("[ERROR][cli]") != std::string::npos);
    PP_REQUIRE(errors.str().find("[RESULT] status=failed code=64") != std::string::npos);
}

PP_TEST("a valid request invokes the pipeline once with the normalized plan")
{
    FakePipeline pipeline;
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::Success));
    PP_REQUIRE_EQ(pipeline.calls, 1);
    PP_REQUIRE(pipeline.received_plan.has_value());
    PP_REQUIRE_EQ(pipeline.received_plan->experiments.size(), std::size_t(24));
    PP_REQUIRE(errors.str().empty());
}

PP_TEST("pipeline progress and successful result use the stable log format")
{
    FakePipeline pipeline;
    pipeline.emit_progress = true;
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, 0);
    PP_REQUIRE(
        output.str().find("[INFO][pipeline] processing started") != std::string::npos);
    PP_REQUIRE(
        output.str().find(
            "[RESULT] status=success code=0 planned=24 succeeded=24 failed=0 skipped=0") !=
        std::string::npos);
    PP_REQUIRE(
        output.str().find("csv=\"results/benchmark.csv\"") != std::string::npos);
}

PP_TEST("result paths are emitted as UTF-8 bytes")
{
    FakePipeline pipeline;
    pipeline.response.csv_path =
        std::filesystem::u8path(u8"results/结果.csv");
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, 0);
    PP_REQUIRE(
        output.str().find(std::string(u8"csv=\"results/结果.csv\"")) !=
        std::string::npos);
}

PP_TEST("mixed workflow results return partial success")
{
    FakePipeline pipeline;
    pipeline.response = {
        24,
        20,
        3,
        1,
        std::filesystem::path("results/benchmark.csv"),
        {{FailureCategory::BackendUnavailable, "CUDA unavailable"}},
        FailureCategory::BackendUnavailable,
    };
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::PartialSuccess));
    PP_REQUIRE(
        output.str().find("[WARN][pipeline] CUDA unavailable") != std::string::npos);
    PP_REQUIRE(
        output.str().find("[RESULT] status=partial code=2") != std::string::npos);
}

PP_TEST("any successful workflow without a CSV is an output failure")
{
    FakePipeline pipeline;
    pipeline.response.csv_path = std::nullopt;
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::OutputFailure));
    PP_REQUIRE(
        errors.str().find("did not provide a result CSV") != std::string::npos);
}

PP_TEST("an engaged but empty CSV path is still an output failure")
{
    FakePipeline pipeline;
    pipeline.response.csv_path = std::filesystem::path{};
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::OutputFailure));
    PP_REQUIRE(
        errors.str().find("did not provide a result CSV") != std::string::npos);
}

PP_TEST("contradictory workflow counts are rejected as an internal failure")
{
    FakePipeline pipeline;
    pipeline.response.failed = 1;
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::InternalFailure));
    PP_REQUIRE(errors.str().find("inconsistent counts") != std::string::npos);
}

PP_TEST("complete workflow failure maps the primary failure category")
{
    const std::vector<std::pair<FailureCategory, ExitCode>> mappings = {
        {FailureCategory::Input, ExitCode::InputFailure},
        {FailureCategory::BackendUnavailable, ExitCode::BackendUnavailable},
        {FailureCategory::Processing, ExitCode::InternalFailure},
        {FailureCategory::Output, ExitCode::OutputFailure},
        {FailureCategory::Internal, ExitCode::InternalFailure},
    };

    for (const auto& [category, expected_code] : mappings)
    {
        FakePipeline pipeline;
        pipeline.response = {
            24,
            0,
            24,
            0,
            std::nullopt,
            {{category, "workflow failed"}},
            category,
        };
        std::ostringstream output;
        std::ostringstream errors;

        const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

        PP_REQUIRE_EQ(exit_code, static_cast<int>(expected_code));
        PP_REQUIRE(errors.str().find("[RESULT] status=failed") != std::string::npos);
    }
}

PP_TEST("pipeline exceptions are contained and mapped to internal failure")
{
    FakePipeline pipeline;
    pipeline.should_throw = true;
    std::ostringstream output;
    std::ostringstream errors;

    const auto exit_code = run_cli(valid_arguments(), pipeline, output, errors);

    PP_REQUIRE_EQ(exit_code, static_cast<int>(ExitCode::InternalFailure));
    PP_REQUIRE(errors.str().find("pipeline exploded") != std::string::npos);
    PP_REQUIRE(errors.str().find("[RESULT] status=failed code=70") != std::string::npos);
}

}  // namespace
