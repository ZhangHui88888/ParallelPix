#include "runner_internal.hpp"

#include "parallelpix/benchmark/statistics.hpp"
#include "parallelpix/benchmark/validation.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::benchmark::detail {
namespace {

using Clock = std::chrono::steady_clock;

ExperimentOutcome fail(
    m2::FailureCategory category,
    std::string message)
{
    return {
        ExperimentStatus::Failed,
        category,
        std::move(message),
        std::nullopt,
    };
}

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

std::string first_batch_issue(const io::BatchResult& result)
{
    return result.issues.empty()
        ? std::string("Image batch loading failed.")
        : result.issues.back().message;
}

std::string first_processing_issue(const BackendExecution& execution)
{
    return execution.processing.issues.empty()
        ? std::string("Backend processing failed.")
        : execution.processing.issues.front().message;
}

void log_batch_warnings(
    const io::BatchResult& batch,
    const m2::LogSink& log)
{
    for (const auto& issue : batch.issues)
    {
        if (issue.severity == io::IoSeverity::Warning)
        {
            emit(log, m2::LogLevel::Warning, "input", issue.message);
        }
    }
}

std::uint8_t tolerance_for(m2::Backend backend)
{
    return backend == m2::Backend::Cuda ? std::uint8_t{1} : std::uint8_t{0};
}

std::string output_resolution(const ProcessingConfig& config)
{
    return std::to_string(config.output_width) + "x" +
        std::to_string(config.output_height);
}

std::optional<std::string> capture_effective_cuda_batch_size(
    const BackendExecution& execution,
    std::optional<std::uint32_t>& effective_batch_size)
{
    if (!execution.effective_cuda_batch_size ||
        *execution.effective_cuda_batch_size == 0)
    {
        return std::string(
            "CUDA executor did not provide a valid effective batch size.");
    }
    if (effective_batch_size &&
        *effective_batch_size != *execution.effective_cuda_batch_size)
    {
        return std::string(
            "CUDA executor changed the effective batch size within one "
            "experiment.");
    }
    effective_batch_size = execution.effective_cuda_batch_size;
    return std::nullopt;
}

}  // namespace

ExperimentOutcome measure_experiment(
    const m2::BenchmarkPlan& plan,
    const m2::ExperimentSpec& experiment,
    IBackendExecutor& executor,
    const io::ImageCatalog& preflight_catalog,
    const Watermark& watermark,
    const std::vector<std::filesystem::path>& expected_sources,
    const RunMetadata& metadata,
    std::optional<double> sequential_compute_ms,
    const m2::LogSink& log)
{
    const ProcessingConfig config;
    auto warmup_batch =
        io::load_batch(preflight_catalog, experiment.image_count);
    log_batch_warnings(warmup_batch, log);
    if (!warmup_batch.ok())
    {
        return fail(m2::FailureCategory::Input, first_batch_issue(warmup_batch));
    }
    if (!sources_match(
            *warmup_batch.images, expected_sources, experiment.image_count))
    {
        return fail(
            m2::FailureCategory::Input,
            "Input image selection changed after benchmark preflight.");
    }

    std::optional<std::uint32_t> effective_cuda_batch_size;
    bool cuda_batch_adjustment_logged = false;
    const auto log_cuda_batch_adjustment = [&] {
        if (!cuda_batch_adjustment_logged &&
            effective_cuda_batch_size &&
            experiment.cuda_batch_size &&
            *effective_cuda_batch_size !=
                *experiment.cuda_batch_size)
        {
            emit(
                log,
                m2::LogLevel::Warning,
                "cuda",
                "Requested CUDA batch size " +
                    std::to_string(*experiment.cuda_batch_size) +
                    " was adjusted to " +
                    std::to_string(*effective_cuda_batch_size) + '.');
            cuda_batch_adjustment_logged = true;
        }
    };
    for (std::uint32_t warmup = 0; warmup < plan.warmups; ++warmup)
    {
        const auto execution = executor.execute(
            *warmup_batch.images, watermark, config, experiment);
        if (!execution.processing.ok())
        {
            return fail(
                m2::FailureCategory::Processing,
                "Warm-up failed: " + first_processing_issue(execution));
        }
        if (experiment.backend == m2::Backend::Cuda)
        {
            const auto issue = capture_effective_cuda_batch_size(
                execution, effective_cuda_batch_size);
            if (issue)
            {
                return fail(m2::FailureCategory::Internal, *issue);
            }
            log_cuda_batch_adjustment();
        }
    }
    warmup_batch.images.reset();

    std::vector<double> compute_samples;
    std::vector<double> end_to_end_samples;
    std::vector<double> h2d_samples;
    std::vector<double> kernel_samples;
    std::vector<double> d2h_samples;
    compute_samples.reserve(plan.repetitions);
    end_to_end_samples.reserve(plan.repetitions);

    bool validation_passed = true;
    bool structural_validation_failure = false;
    std::uint32_t maximum_pixel_error = 0;
    std::string validation_message;
    std::string input_resolution;

    for (std::uint32_t repetition = 0;
         repetition < plan.repetitions;
         ++repetition)
    {
        const auto end_to_end_start = Clock::now();
        auto catalog = io::scan_catalog(plan.input_dir);
        if (!catalog.ok())
        {
            return fail(
                m2::FailureCategory::Input,
                catalog.issues.empty()
                    ? std::string("Input catalog scan failed.")
                    : catalog.issues.front().message);
        }
        auto batch = io::load_batch(*catalog.catalog, experiment.image_count);
        log_batch_warnings(batch, log);
        if (!batch.ok())
        {
            return fail(
                m2::FailureCategory::Input, first_batch_issue(batch));
        }
        if (!sources_match(
                *batch.images, expected_sources, experiment.image_count))
        {
            return fail(
                m2::FailureCategory::Input,
                "Input image selection changed during measured repetitions.");
        }
        if (input_resolution.empty())
        {
            input_resolution = resolution_label(*batch.images);
        }

        std::vector<ProgressSample> progress_samples;
        progress_samples.reserve(experiment.image_count);
        const ProgressSink progress = repetition == 0
            ? [&progress_samples](const ProgressSample& sample) {
                  progress_samples.push_back(sample);
              }
            : ProgressSink{};
        const auto compute_start = Clock::now();
        auto execution = executor.execute(
            *batch.images, watermark, config, experiment, progress);
        const auto compute_end = Clock::now();
        if (!execution.processing.ok())
        {
            return fail(
                m2::FailureCategory::Processing,
                first_processing_issue(execution));
        }

        if (experiment.backend == m2::Backend::Cuda)
        {
            if (!execution.cuda_phase)
            {
                return fail(
                    m2::FailureCategory::Internal,
                    "CUDA executor did not provide phase timing.");
            }
            const auto issue = capture_effective_cuda_batch_size(
                execution, effective_cuda_batch_size);
            if (issue)
            {
                return fail(m2::FailureCategory::Internal, *issue);
            }
            log_cuda_batch_adjustment();
            h2d_samples.push_back(execution.cuda_phase->h2d_ms);
            kernel_samples.push_back(execution.cuda_phase->kernel_ms);
            d2h_samples.push_back(execution.cuda_phase->d2h_ms);
        }

        auto output_experiment = experiment;
        if (experiment.backend == m2::Backend::Cuda)
        {
            output_experiment.cuda_batch_size =
                effective_cuda_batch_size;
        }
        const auto directory =
            experiment_output_directory(
                plan.output_dir, metadata, output_experiment);
        const auto written =
            write_outputs(directory, *execution.processing.images);
        if (!written.paths)
        {
            return fail(m2::FailureCategory::Output, written.message);
        }
        const auto end_to_end_end = Clock::now();

        compute_samples.push_back(
            std::chrono::duration<double, std::milli>(
                compute_end - compute_start)
                .count());
        end_to_end_samples.push_back(
            std::chrono::duration<double, std::milli>(
                end_to_end_end - end_to_end_start)
                .count());

        if (log)
        {
            for (const auto& sample : progress_samples)
            {
                log({
                    m2::LogLevel::Info,
                    "trajectory",
                    "backend=" + backend_name(experiment.backend) +
                        " run_id=" + metadata.run_id +
                        " thread_count=" + std::to_string(
                            experiment.thread_count.value_or(0)) +
                        " cuda_batch_size=" + std::to_string(
                            experiment.cuda_batch_size.value_or(0)) +
                        " image_count=" +
                        std::to_string(experiment.image_count) +
                        " processed=" +
                        std::to_string(sample.processed_images) +
                        " ms_per_image=" +
                        std::to_string(sample.batch_ms_per_image),
                });
            }
        }

        auto candidate = decode_paths(*written.paths);
        if (!candidate.images)
        {
            return fail(m2::FailureCategory::Output, candidate.message);
        }

        ValidationResult validation;
        if (experiment.backend == m2::Backend::Sequential)
        {
            validation = validate_batches(
                *execution.processing.images,
                *candidate.images,
                tolerance_for(experiment.backend));
        }
        else
        {
            const auto reference_paths = sequential_reference_paths(
                plan.output_dir,
                metadata,
                experiment.image_count,
                *written.paths);
            auto reference = decode_paths(reference_paths);
            if (!reference.images)
            {
                return fail(
                    m2::FailureCategory::Processing,
                    "Sequential reference output is unavailable: " +
                        reference.message);
            }
            validation = validate_batches(
                *reference.images,
                *candidate.images,
                tolerance_for(experiment.backend));
        }

        if (!validation.passed)
        {
            validation_passed = false;
            if (validation_message.empty())
            {
                validation_message = validation.message;
            }
        }
        if (validation.max_pixel_error)
        {
            maximum_pixel_error = std::max(
                maximum_pixel_error,
                static_cast<std::uint32_t>(*validation.max_pixel_error));
        }
        else
        {
            structural_validation_failure = true;
        }
    }

    const auto compute = summarize_samples(compute_samples);
    const auto end_to_end = summarize_samples(end_to_end_samples);
    if (!compute || !end_to_end || compute->median_ms <= 0.0)
    {
        return fail(
            m2::FailureCategory::Internal,
            "Measured samples could not be summarized.");
    }

    BenchmarkRecord record;
    record.run_id = metadata.run_id;
    record.recorded_at_utc = metadata.recorded_at_utc;
    record.backend = backend_name(experiment.backend);
    record.thread_count = experiment.thread_count;
    record.cuda_batch_size = experiment.backend == m2::Backend::Cuda
        ? effective_cuda_batch_size
        : experiment.cuda_batch_size;
    record.image_count = experiment.image_count;
    record.input_resolution = input_resolution;
    record.output_resolution = output_resolution(config);
    record.warmups = plan.warmups;
    record.repetitions = plan.repetitions;
    record.compute_ms = compute->median_ms;
    record.compute_min_ms = compute->min_ms;
    record.compute_max_ms = compute->max_ms;
    record.compute_stddev_ms = compute->stddev_ms;
    record.end_to_end_ms = end_to_end->median_ms;
    record.end_to_end_min_ms = end_to_end->min_ms;
    record.end_to_end_max_ms = end_to_end->max_ms;
    record.end_to_end_stddev_ms = end_to_end->stddev_ms;

    const auto compute_seconds = compute->median_ms / 1000.0;
    record.images_per_second =
        static_cast<double>(experiment.image_count) / compute_seconds;
    record.megapixels_per_second =
        static_cast<double>(experiment.image_count) *
        static_cast<double>(config.output_width) *
        static_cast<double>(config.output_height) /
        1'000'000.0 / compute_seconds;
    record.validation_passed = validation_passed;
    if (!structural_validation_failure)
    {
        record.max_pixel_error = maximum_pixel_error;
    }

    if (validation_passed)
    {
        if (experiment.backend == m2::Backend::Sequential)
        {
            record.speedup = 1.0;
        }
        else if (sequential_compute_ms && *sequential_compute_ms > 0.0)
        {
            record.speedup = *sequential_compute_ms / compute->median_ms;
            if (experiment.backend == m2::Backend::OpenMP &&
                experiment.thread_count)
            {
                record.parallel_efficiency =
                    *record.speedup /
                    static_cast<double>(*experiment.thread_count);
            }
        }
    }

    if (experiment.backend == m2::Backend::Cuda)
    {
        const auto h2d = summarize_samples(h2d_samples);
        const auto kernel = summarize_samples(kernel_samples);
        const auto d2h = summarize_samples(d2h_samples);
        if (!h2d || !kernel || !d2h)
        {
            return fail(
                m2::FailureCategory::Internal,
                "CUDA phase samples could not be summarized.");
        }
        record.h2d_ms = h2d->median_ms;
        record.kernel_ms = kernel->median_ms;
        record.d2h_ms = d2h->median_ms;
    }

    if (!validation_passed)
    {
        return {
            ExperimentStatus::Failed,
            m2::FailureCategory::Processing,
            validation_message.empty()
                ? std::string("Output validation failed.")
                : validation_message,
            std::move(record),
        };
    }

    return {
        ExperimentStatus::Succeeded,
        m2::FailureCategory::Internal,
        {},
        std::move(record),
    };
}

}  // namespace parallelpix::benchmark::detail
