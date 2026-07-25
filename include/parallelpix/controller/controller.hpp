#pragma once

#include "parallelpix/planning/benchmark_plan.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace parallelpix::m2 {

enum class ExitCode : int
{
    Success = 0,
    PartialSuccess = 2,
    InvalidArguments = 64,
    InputFailure = 65,
    BackendUnavailable = 69,
    InternalFailure = 70,
    OutputFailure = 73,
};

enum class LogLevel
{
    Info,
    Warning,
    Error,
};

struct LogEvent
{
    LogLevel level = LogLevel::Info;
    std::string stage;
    std::string message;
};

using LogSink = std::function<void(const LogEvent&)>;

enum class FailureCategory
{
    Input,
    BackendUnavailable,
    Processing,
    Output,
    Internal,
};

struct WorkflowIssue
{
    FailureCategory category = FailureCategory::Internal;
    std::string message;
};

struct WorkflowSummary
{
    std::size_t planned = 0;
    std::size_t succeeded = 0;
    std::size_t failed = 0;
    std::size_t skipped = 0;
    std::optional<std::filesystem::path> csv_path;
    std::vector<WorkflowIssue> issues;
    std::optional<FailureCategory> primary_failure;
};

class IBenchmarkPipeline
{
public:
    virtual ~IBenchmarkPipeline() = default;

    virtual WorkflowSummary execute(
        const BenchmarkPlan& plan,
        const LogSink& log) = 0;
};

int run_cli(
    const std::vector<std::string_view>& arguments,
    IBenchmarkPipeline& pipeline,
    std::ostream& output,
    std::ostream& errors);

std::string usage_text();

}  // namespace parallelpix::m2
