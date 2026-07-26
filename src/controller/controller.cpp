#include "parallelpix/controller/controller.hpp"

#include "parallelpix/cli/cli.hpp"

#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

namespace parallelpix::m2 {
namespace {

const char* level_name(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "ERROR";
}

void write_log(
    const LogEvent& event,
    std::ostream& output,
    std::ostream& errors)
{
    auto& stream = event.level == LogLevel::Error ? errors : output;
    stream << '[' << level_name(event.level) << "][" << event.stage << "] "
           << event.message << '\n' << std::flush;
}

void write_result(
    std::string_view status,
    ExitCode code,
    const WorkflowSummary* summary,
    std::ostream& output,
    std::ostream& errors)
{
    auto& stream =
        code == ExitCode::Success || code == ExitCode::PartialSuccess ? output : errors;
    stream << "[RESULT] status=" << status << " code=" << static_cast<int>(code);
    if (summary != nullptr)
    {
        stream << " planned=" << summary->planned
               << " succeeded=" << summary->succeeded
               << " failed=" << summary->failed
               << " skipped=" << summary->skipped;
        if (summary->csv_path)
        {
            stream << " csv=\"" << summary->csv_path->u8string() << '"';
        }
    }
    stream << '\n';
}

ExitCode map_failure(FailureCategory category)
{
    switch (category)
    {
    case FailureCategory::Input:
        return ExitCode::InputFailure;
    case FailureCategory::BackendUnavailable:
        return ExitCode::BackendUnavailable;
    case FailureCategory::Output:
        return ExitCode::OutputFailure;
    case FailureCategory::Processing:
    case FailureCategory::Internal:
        return ExitCode::InternalFailure;
    }
    return ExitCode::InternalFailure;
}

bool has_consistent_counts(
    const BenchmarkPlan& plan,
    const WorkflowSummary& summary)
{
    return summary.planned == plan.experiments.size() &&
        summary.succeeded + summary.failed + summary.skipped == summary.planned;
}

void write_issues(
    const WorkflowSummary& summary,
    bool has_successes,
    std::ostream& output,
    std::ostream& errors)
{
    const auto level = has_successes ? LogLevel::Warning : LogLevel::Error;
    for (const auto& issue : summary.issues)
    {
        write_log({level, "pipeline", issue.message}, output, errors);
    }
}

}  // namespace

std::string usage_text()
{
    return
        "Usage:\n"
        "  parallelpix benchmark\n"
        "    --input <dir> --output <dir> --watermark <file>\n"
        "    --backends sequential,openmp,cuda\n"
        "    --image-counts 10,50,100\n"
        "    --threads 1,2,4,8 --cuda-batches 1,4,8\n"
        "    --warmups 2 --repetitions 5\n"
        "    --csv <file> [--append]\n";
}

int run_cli(
    const std::vector<std::string_view>& arguments,
    IBenchmarkPipeline& pipeline,
    std::ostream& output,
    std::ostream& errors)
{
    const auto parsed = parse_cli(arguments);
    if (std::holds_alternative<HelpRequest>(parsed))
    {
        output << usage_text();
        return static_cast<int>(ExitCode::Success);
    }
    if (std::holds_alternative<CliError>(parsed))
    {
        for (const auto& message : std::get<CliError>(parsed).messages)
        {
            write_log({LogLevel::Error, "cli", message}, output, errors);
        }
        errors << usage_text();
        write_result(
            "failed", ExitCode::InvalidArguments, nullptr, output, errors);
        return static_cast<int>(ExitCode::InvalidArguments);
    }

    const auto plan = build_benchmark_plan(std::get<BenchmarkRequest>(parsed));
    write_log(
        {
            LogLevel::Info,
            "controller",
            "Accepted benchmark plan with " +
                std::to_string(plan.experiments.size()) + " configurations.",
        },
        output,
        errors);

    WorkflowSummary summary;
    try
    {
        const LogSink log = [&](const LogEvent& event) {
            write_log(event, output, errors);
        };
        summary = pipeline.execute(plan, log);
    }
    catch (const std::exception& exception)
    {
        write_log(
            {
                LogLevel::Error,
                "pipeline",
                "Unhandled pipeline error: " + std::string(exception.what()),
            },
            output,
            errors);
        write_result(
            "failed", ExitCode::InternalFailure, nullptr, output, errors);
        return static_cast<int>(ExitCode::InternalFailure);
    }
    catch (...)
    {
        write_log(
            {LogLevel::Error, "pipeline", "Unhandled non-standard pipeline error."},
            output,
            errors);
        write_result(
            "failed", ExitCode::InternalFailure, nullptr, output, errors);
        return static_cast<int>(ExitCode::InternalFailure);
    }

    if (!has_consistent_counts(plan, summary))
    {
        write_log(
            {
                LogLevel::Error,
                "controller",
                "Pipeline returned inconsistent counts.",
            },
            output,
            errors);
        write_result(
            "failed", ExitCode::InternalFailure, &summary, output, errors);
        return static_cast<int>(ExitCode::InternalFailure);
    }

    write_issues(summary, summary.succeeded > 0, output, errors);

    if (summary.succeeded > 0 &&
        (!summary.csv_path || summary.csv_path->empty()))
    {
        write_log(
            {
                LogLevel::Error,
                "controller",
                "Pipeline succeeded but did not provide a result CSV.",
            },
            output,
            errors);
        write_result(
            "failed", ExitCode::OutputFailure, &summary, output, errors);
        return static_cast<int>(ExitCode::OutputFailure);
    }

    if (summary.succeeded == summary.planned)
    {
        write_result("success", ExitCode::Success, &summary, output, errors);
        return static_cast<int>(ExitCode::Success);
    }

    if (summary.succeeded > 0)
    {
        write_result(
            "partial", ExitCode::PartialSuccess, &summary, output, errors);
        return static_cast<int>(ExitCode::PartialSuccess);
    }

    const auto category =
        summary.primary_failure.value_or(FailureCategory::Internal);
    const auto exit_code = map_failure(category);
    write_result("failed", exit_code, &summary, output, errors);
    return static_cast<int>(exit_code);
}

}  // namespace parallelpix::m2
