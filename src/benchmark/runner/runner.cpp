#include "parallelpix/benchmark/runner.hpp"

#include "parallelpix/benchmark/reporting.hpp"
#include "runner_internal.hpp"

#include <algorithm>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::benchmark {
namespace {

void emit(
    const m2::LogSink& log,
    m2::LogLevel level,
    std::string stage,
    std::string message)
{
    if (log)
    {
        log({level, std::move(stage), std::move(message)});
    }
}

void add_issue(
    m2::WorkflowSummary& summary,
    m2::FailureCategory category,
    std::string message)
{
    if (!summary.primary_failure)
    {
        summary.primary_failure = category;
    }
    summary.issues.push_back({category, std::move(message)});
}

m2::WorkflowSummary global_failure(
    const m2::BenchmarkPlan& plan,
    m2::FailureCategory category,
    std::string message)
{
    m2::WorkflowSummary summary;
    summary.planned = plan.experiments.size();
    summary.failed = summary.planned;
    summary.primary_failure = category;
    summary.issues.push_back({category, message});
    return summary;
}

std::string first_catalog_issue(const io::CatalogResult& result)
{
    return result.issues.empty()
        ? std::string("Input catalog scan failed.")
        : result.issues.front().message;
}

std::string first_watermark_issue(const io::WatermarkResult& result)
{
    return result.issues.empty()
        ? std::string("Watermark decoding failed.")
        : result.issues.front().message;
}

std::string first_batch_issue(const io::BatchResult& result)
{
    return result.issues.empty()
        ? std::string("Image batch preflight failed.")
        : result.issues.back().message;
}

}  // namespace

m2::WorkflowSummary run_benchmark_plan(
    const m2::BenchmarkPlan& plan,
    const std::vector<IBackendExecutor*>& executors,
    const m2::LogSink& log)
{
    if (plan.experiments.empty())
    {
        return global_failure(
            plan,
            m2::FailureCategory::Internal,
            "Benchmark plan contains no experiments.");
    }

    const auto csv_destination =
        validate_csv_destination(plan.result_csv, plan.csv_mode);
    if (!csv_destination.success)
    {
        return global_failure(
            plan,
            m2::FailureCategory::Output,
            csv_destination.message);
    }

    const auto prepared_output =
        io::prepare_output_directory(plan.output_dir);
    if (!prepared_output.ok())
    {
        return global_failure(
            plan,
            m2::FailureCategory::Output,
            prepared_output.issues.empty()
                ? std::string("Image output directory is not writable.")
                : prepared_output.issues.front().message);
    }

    const auto catalog = io::scan_catalog(plan.input_dir);
    if (!catalog.ok())
    {
        return global_failure(
            plan,
            m2::FailureCategory::Input,
            first_catalog_issue(catalog));
    }

    const auto watermark = io::decode_watermark(plan.watermark_path);
    if (!watermark.ok())
    {
        return global_failure(
            plan,
            m2::FailureCategory::Input,
            first_watermark_issue(watermark));
    }

    const auto largest = std::max_element(
        plan.experiments.begin(),
        plan.experiments.end(),
        [](const auto& left, const auto& right) {
            return left.image_count < right.image_count;
        });
    auto preflight_batch =
        io::load_batch(*catalog.catalog, largest->image_count);
    for (const auto& issue : preflight_batch.issues)
    {
        if (issue.severity == io::IoSeverity::Warning)
        {
            emit(log, m2::LogLevel::Warning, "preflight", issue.message);
        }
    }
    if (!preflight_batch.ok())
    {
        return global_failure(
            plan,
            m2::FailureCategory::Input,
            first_batch_issue(preflight_batch));
    }

    std::vector<std::filesystem::path> expected_sources;
    expected_sources.reserve(preflight_batch.images->size());
    for (const auto& image : *preflight_batch.images)
    {
        expected_sources.push_back(image.source_path);
    }
    preflight_batch.images.reset();

    std::map<m2::Backend, IBackendExecutor*> executor_by_backend;
    for (auto* executor : executors)
    {
        if (executor != nullptr)
        {
            executor_by_backend.emplace(executor->backend(), executor);
        }
    }

    const auto metadata = detail::create_run_metadata(plan.output_dir);
    emit(
        log,
        m2::LogLevel::Info,
        "benchmark",
        "Starting run " + metadata.run_id + " with " +
            std::to_string(plan.experiments.size()) + " configurations.");

    m2::WorkflowSummary summary;
    summary.planned = plan.experiments.size();
    std::map<std::uint32_t, double> sequential_compute_by_count;
    std::vector<BenchmarkRecord> records;

    for (const auto& experiment : plan.experiments)
    {
        const auto executor = executor_by_backend.find(experiment.backend);
        if (executor == executor_by_backend.end())
        {
            ++summary.skipped;
            add_issue(
                summary,
                m2::FailureCategory::BackendUnavailable,
                detail::backend_name(experiment.backend) +
                    " backend is not available in this build.");
            continue;
        }

        std::optional<double> sequential_compute_ms;
        if (experiment.backend != m2::Backend::Sequential)
        {
            const auto baseline =
                sequential_compute_by_count.find(experiment.image_count);
            if (baseline == sequential_compute_by_count.end())
            {
                ++summary.skipped;
                add_issue(
                    summary,
                    m2::FailureCategory::Processing,
                    "Sequential baseline is unavailable for image count " +
                        std::to_string(experiment.image_count) + '.');
                continue;
            }
            sequential_compute_ms = baseline->second;
        }

        emit(
            log,
            m2::LogLevel::Info,
            "benchmark",
            "Running " + detail::backend_name(experiment.backend) +
                " with image_count=" +
                std::to_string(experiment.image_count) + '.');

        detail::ExperimentOutcome outcome;
        try
        {
            outcome = detail::measure_experiment(
                plan,
                experiment,
                *executor->second,
                *catalog.catalog,
                *watermark.watermark,
                expected_sources,
                metadata,
                sequential_compute_ms,
                log);
        }
        catch (const std::exception& exception)
        {
            outcome = {
                detail::ExperimentStatus::Failed,
                m2::FailureCategory::Internal,
                "Unhandled experiment error: " +
                    std::string(exception.what()),
                std::nullopt,
            };
        }
        catch (...)
        {
            outcome = {
                detail::ExperimentStatus::Failed,
                m2::FailureCategory::Internal,
                "Unhandled non-standard experiment error.",
                std::nullopt,
            };
        }

        if (outcome.status == detail::ExperimentStatus::Succeeded)
        {
            ++summary.succeeded;
            if (experiment.backend == m2::Backend::Sequential &&
                outcome.record)
            {
                sequential_compute_by_count[experiment.image_count] =
                    outcome.record->compute_ms;
            }
        }
        else if (outcome.status == detail::ExperimentStatus::Skipped)
        {
            ++summary.skipped;
        }
        else
        {
            ++summary.failed;
            add_issue(
                summary, outcome.category, outcome.message);
        }

        if (outcome.record)
        {
            records.push_back(std::move(*outcome.record));
        }
    }

    if (!records.empty())
    {
        const auto reported =
            write_benchmark_records(plan.result_csv, plan.csv_mode, records);
        if (!reported.success)
        {
            summary.primary_failure = m2::FailureCategory::Output;
            summary.issues.push_back(
                {m2::FailureCategory::Output, reported.message});
            return summary;
        }
        summary.csv_path = plan.result_csv;
        emit(
            log,
            m2::LogLevel::Info,
            "reporting",
            "Wrote " + std::to_string(records.size()) +
                " benchmark rows to " + plan.result_csv.u8string() + '.');
    }

    if (summary.succeeded > 0)
    {
        summary.primary_failure.reset();
    }
    return summary;
}

}  // namespace parallelpix::benchmark
