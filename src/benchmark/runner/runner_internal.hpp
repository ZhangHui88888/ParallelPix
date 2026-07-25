#pragma once

#include "parallelpix/benchmark/backend.hpp"
#include "parallelpix/benchmark/reporting.hpp"
#include "parallelpix/controller/controller.hpp"
#include "parallelpix/io/image_io.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace parallelpix::benchmark::detail {

struct RunMetadata
{
    std::string run_id;
    std::string recorded_at_utc;
};

enum class ExperimentStatus
{
    Succeeded,
    Failed,
    Skipped,
};

struct ExperimentOutcome
{
    ExperimentStatus status = ExperimentStatus::Failed;
    m2::FailureCategory category = m2::FailureCategory::Internal;
    std::string message;
    std::optional<BenchmarkRecord> record;
};

struct PersistedBatchResult
{
    std::optional<std::vector<Image>> images;
    std::string message;
};

struct OutputWriteResult
{
    std::optional<std::vector<std::filesystem::path>> paths;
    std::string message;
};

[[nodiscard]] RunMetadata create_run_metadata(
    const std::filesystem::path& output_root);

[[nodiscard]] std::string backend_name(m2::Backend backend);

[[nodiscard]] std::filesystem::path experiment_output_directory(
    const std::filesystem::path& output_root,
    const RunMetadata& metadata,
    const m2::ExperimentSpec& experiment);

[[nodiscard]] bool sources_match(
    const std::vector<Image>& images,
    const std::vector<std::filesystem::path>& expected_sources,
    std::size_t count);

[[nodiscard]] std::string resolution_label(
    const std::vector<Image>& images);

[[nodiscard]] OutputWriteResult write_outputs(
    const std::filesystem::path& directory,
    const std::vector<Image>& images);

[[nodiscard]] PersistedBatchResult decode_paths(
    const std::vector<std::filesystem::path>& paths);

[[nodiscard]] std::vector<std::filesystem::path> sequential_reference_paths(
    const std::filesystem::path& output_root,
    const RunMetadata& metadata,
    std::uint32_t image_count,
    const std::vector<std::filesystem::path>& candidate_paths);

ExperimentOutcome measure_experiment(
    const m2::BenchmarkPlan& plan,
    const m2::ExperimentSpec& experiment,
    IBackendExecutor& executor,
    const io::ImageCatalog& preflight_catalog,
    const Watermark& watermark,
    const std::vector<std::filesystem::path>& expected_sources,
    const RunMetadata& metadata,
    std::optional<double> sequential_compute_ms,
    const m2::LogSink& log);

}  // namespace parallelpix::benchmark::detail
