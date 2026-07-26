#include "parallelpix/openmp/processor.hpp"

#include "parallelpix/openmp/scheduling.hpp"
#include "process_image.hpp"

#include <omp.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::openmp {
namespace {

ProcessingIssue make_issue(
    ProcessingIssueCode code,
    std::optional<std::size_t> image_index,
    const std::filesystem::path& source_path,
    std::string message)
{
    return {
        code,
        image_index,
        source_path,
        std::move(message),
    };
}

bool prepare_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    std::uint32_t thread_count,
    std::vector<CropRegion>& crops,
    std::vector<ProcessingIssue>& issues)
{
    if (thread_count == 0 ||
        thread_count >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
        issues.push_back(make_issue(
            ProcessingIssueCode::InvalidConfig,
            std::nullopt,
            {},
            "OpenMP thread count must fit a positive signed integer."));
        return false;
    }
    if (!is_valid_processing_config(config))
    {
        issues.push_back(make_issue(
            ProcessingIssueCode::InvalidConfig,
            std::nullopt,
            {},
            "Processing configuration is invalid."));
        return false;
    }
    if (images.empty())
    {
        issues.push_back(make_issue(
            ProcessingIssueCode::EmptyBatch,
            std::nullopt,
            {},
            "Image batch must contain at least one image."));
        return false;
    }
    if (!is_valid_watermark(watermark))
    {
        issues.push_back(make_issue(
            ProcessingIssueCode::InvalidWatermark,
            std::nullopt,
            watermark.color.source_path,
            "Watermark does not satisfy the shared image invariants."));
        return false;
    }
    if (watermark.color.width > config.output_width ||
        config.watermark_margin >
            config.output_width - watermark.color.width ||
        watermark.color.height > config.output_height ||
        config.watermark_margin >
            config.output_height - watermark.color.height)
    {
        issues.push_back(make_issue(
            ProcessingIssueCode::WatermarkDoesNotFit,
            std::nullopt,
            watermark.color.source_path,
            "Watermark and its right/bottom margin do not fit the output."));
        return false;
    }

    crops.reserve(images.size());
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto& image = images[index];
        if (!is_valid_image(image))
        {
            issues.push_back(make_issue(
                ProcessingIssueCode::InvalidImage,
                index,
                image.source_path,
                "Input image does not satisfy the shared image invariants."));
            continue;
        }

        const auto crop = compute_center_crop(
            image.width,
            image.height,
            config.output_width,
            config.output_height);
        if (!crop)
        {
            issues.push_back(make_issue(
                ProcessingIssueCode::InvalidImage,
                index,
                image.source_path,
                "A center crop could not be computed for the input image."));
            continue;
        }
        crops.push_back(*crop);
    }
    return issues.empty();
}

class ProgressTracker
{
public:
    ProgressTracker(
        std::size_t total,
        const BatchProgressSink& sink) noexcept
        : total_(total), sink_(sink)
    {
    }

    void record(double image_ms) noexcept
    {
        if (!sink_)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        ++processed_;
        ++interval_count_;
        interval_ms_ += image_ms;
        if (interval_count_ < progress_interval_ &&
            processed_ != total_)
        {
            return;
        }

        try
        {
            sink_(
                processed_,
                interval_ms_ /
                    static_cast<double>(interval_count_));
        }
        catch (...)
        {
            // Progress reporting is observational and must not invalidate
            // completed image processing inside an OpenMP worker.
        }
        interval_count_ = 0;
        interval_ms_ = 0.0;
    }

private:
    static constexpr std::size_t progress_interval_ = 10;
    std::size_t total_ = 0;
    BatchProgressSink sink_;
    std::mutex mutex_;
    std::size_t processed_ = 0;
    std::size_t interval_count_ = 0;
    double interval_ms_ = 0.0;
};

}  // namespace

BatchProcessingResult process_batch(
    const std::vector<Image>& images,
    const Watermark& watermark,
    const ProcessingConfig& config,
    std::uint32_t thread_count,
    const BatchProgressSink& progress)
{
    BatchProcessingResult result;
    std::vector<CropRegion> crops;
    if (!prepare_batch(
            images,
            watermark,
            config,
            thread_count,
            crops,
            result.issues))
    {
        return result;
    }

    std::vector<Image> outputs;
    outputs.reserve(images.size());
    for (const auto& image : images)
    {
        outputs.push_back(detail::create_output_image(image, config));
    }

    omp_set_dynamic(0);
    const auto omp_threads = static_cast<int>(thread_count);
    ProgressTracker tracker(images.size(), progress);
    const auto strategy =
        choose_scheduling_strategy(images.size(), thread_count);

    if (strategy == SchedulingStrategy::Images)
    {
#pragma omp parallel for schedule(dynamic, 1) num_threads(omp_threads)
        for (std::int64_t index = 0;
             index < static_cast<std::int64_t>(images.size());
             ++index)
        {
            const auto item = static_cast<std::size_t>(index);
            const auto started_at = std::chrono::steady_clock::now();
            detail::process_image_serial(
                images[item],
                watermark,
                config,
                crops[item],
                outputs[item]);
            const auto elapsed =
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - started_at)
                    .count();
            tracker.record(elapsed);
        }
    }
    else
    {
        for (std::size_t index = 0; index < images.size(); ++index)
        {
            const auto started_at = std::chrono::steady_clock::now();
            detail::process_image_parallel_rows(
                images[index],
                watermark,
                config,
                crops[index],
                omp_threads,
                outputs[index]);
            const auto elapsed =
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - started_at)
                    .count();
            tracker.record(elapsed);
        }
    }

    result.images = std::move(outputs);
    return result;
}

}  // namespace parallelpix::openmp
