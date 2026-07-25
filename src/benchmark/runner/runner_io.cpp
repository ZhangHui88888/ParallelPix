#include "runner_internal.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace parallelpix::benchmark::detail {
namespace {

std::tm utc_time(std::time_t value)
{
    std::tm result{};
#ifdef _WIN32
    gmtime_s(&result, &value);
#else
    gmtime_r(&value, &result);
#endif
    return result;
}

std::string random_suffix()
{
    static std::atomic<std::uint32_t> counter{0};
    std::random_device source;
    const auto value = source() ^ counter.fetch_add(1);
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(8) << value;
    return stream.str();
}

std::string configuration_name(const m2::ExperimentSpec& experiment)
{
    auto name = "images-" + std::to_string(experiment.image_count);
    if (experiment.thread_count)
    {
        name += "-threads-" + std::to_string(*experiment.thread_count);
    }
    if (experiment.cuda_batch_size)
    {
        name += "-batch-" + std::to_string(*experiment.cuda_batch_size);
    }
    return name;
}

std::filesystem::path output_filename(
    const Image& image,
    std::size_t index)
{
    auto stem = image.source_path.stem().u8string();
    if (stem.empty())
    {
        stem = "image";
    }
    std::ostringstream prefix;
    prefix << std::setfill('0') << std::setw(6) << index + 1 << '-';
    return std::filesystem::u8path(prefix.str() + stem + ".png");
}

}  // namespace

RunMetadata create_run_metadata(const std::filesystem::path& output_root)
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto utc = utc_time(time);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) %
        1000;

    std::ostringstream recorded;
    recorded << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.'
             << std::setfill('0') << std::setw(3) << milliseconds.count()
             << 'Z';

    for (int attempt = 0; attempt < 16; ++attempt)
    {
        std::ostringstream identifier;
        identifier << "pp-" << std::put_time(&utc, "%Y%m%dT%H%M%S")
                   << std::setfill('0') << std::setw(3)
                   << milliseconds.count() << "Z-" << random_suffix();
        const auto run_id = identifier.str();
        std::error_code error;
        if (!std::filesystem::exists(
                output_root / std::filesystem::u8path(run_id), error) &&
            !error)
        {
            return {run_id, recorded.str()};
        }
    }

    return {
        "pp-fallback-" + random_suffix() + "-" + random_suffix(),
        recorded.str(),
    };
}

std::string backend_name(m2::Backend backend)
{
    switch (backend)
    {
    case m2::Backend::Sequential:
        return "sequential";
    case m2::Backend::OpenMP:
        return "openmp";
    case m2::Backend::Cuda:
        return "cuda";
    }
    return "unknown";
}

std::filesystem::path experiment_output_directory(
    const std::filesystem::path& output_root,
    const RunMetadata& metadata,
    const m2::ExperimentSpec& experiment)
{
    return output_root / std::filesystem::u8path(metadata.run_id) /
        backend_name(experiment.backend) /
        configuration_name(experiment);
}

bool sources_match(
    const std::vector<Image>& images,
    const std::vector<std::filesystem::path>& expected_sources,
    std::size_t count)
{
    if (images.size() != count || expected_sources.size() < count)
    {
        return false;
    }
    for (std::size_t index = 0; index < count; ++index)
    {
        if (images[index].source_path != expected_sources[index])
        {
            return false;
        }
    }
    return true;
}

std::string resolution_label(const std::vector<Image>& images)
{
    if (images.empty())
    {
        return {};
    }
    const auto width = images.front().width;
    const auto height = images.front().height;
    for (const auto& image : images)
    {
        if (image.width != width || image.height != height)
        {
            return "mixed";
        }
    }
    return std::to_string(width) + "x" + std::to_string(height);
}

OutputWriteResult write_outputs(
    const std::filesystem::path& directory,
    const std::vector<Image>& images)
{
    const auto prepared = io::prepare_output_directory(directory);
    if (!prepared.ok())
    {
        return {
            std::nullopt,
            prepared.issues.empty()
                ? std::string("Output directory preparation failed.")
                : prepared.issues.front().message,
        };
    }

    std::vector<std::filesystem::path> paths;
    paths.reserve(images.size());
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto path = directory / output_filename(images[index], index);
        const auto written =
            io::write_png(images[index], path, io::WriteMode::ReplaceExisting);
        if (!written.ok())
        {
            return {
                std::nullopt,
                written.issues.empty()
                    ? std::string("PNG output failed.")
                    : written.issues.front().message,
            };
        }
        paths.push_back(path);
    }
    return {std::move(paths), {}};
}

PersistedBatchResult decode_paths(
    const std::vector<std::filesystem::path>& paths)
{
    std::vector<Image> images;
    images.reserve(paths.size());
    for (const auto& path : paths)
    {
        auto decoded = io::decode_image(path);
        if (!decoded.ok())
        {
            return {
                std::nullopt,
                decoded.issues.empty()
                    ? std::string("Persisted PNG could not be decoded.")
                    : decoded.issues.front().message,
            };
        }
        images.push_back(std::move(*decoded.image));
    }
    return {std::move(images), {}};
}

std::vector<std::filesystem::path> sequential_reference_paths(
    const std::filesystem::path& output_root,
    const RunMetadata& metadata,
    std::uint32_t image_count,
    const std::vector<std::filesystem::path>& candidate_paths)
{
    const m2::ExperimentSpec sequential{
        m2::Backend::Sequential,
        image_count,
        std::nullopt,
        std::nullopt,
    };
    const auto directory =
        experiment_output_directory(output_root, metadata, sequential);

    std::vector<std::filesystem::path> result;
    result.reserve(candidate_paths.size());
    for (const auto& candidate : candidate_paths)
    {
        result.push_back(directory / candidate.filename());
    }
    return result;
}

}  // namespace parallelpix::benchmark::detail
