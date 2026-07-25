#include "parallelpix/io/image_io.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <string>
#include <utility>

namespace parallelpix::io {
namespace {

IoIssue make_error(
    IoIssueCode code,
    const std::filesystem::path& path,
    std::string message)
{
    return {code, IoSeverity::Error, path, std::move(message)};
}

std::string unique_token()
{
    static std::atomic<std::uint64_t> counter{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::to_string(
               std::chrono::duration_cast<std::chrono::nanoseconds>(now)
                   .count()) +
        "-" + std::to_string(counter.fetch_add(1));
}

void remove_private_file(const std::filesystem::path& path) noexcept
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

}  // namespace

IoStatus prepare_output_directory(
    const std::filesystem::path& output_directory)
{
    IoStatus result;
    if (output_directory.empty())
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputDirectoryCreateFailed,
            output_directory,
            "Output directory path must not be empty."));
        return result;
    }

    try
    {
        std::error_code error;
        const auto exists = std::filesystem::exists(output_directory, error);
        if (error)
        {
            result.issues.push_back(make_error(
                IoIssueCode::OutputDirectoryCreateFailed,
                output_directory,
                "Failed to inspect the output path: " + error.message()));
            return result;
        }

        if (exists)
        {
            const auto is_directory =
                std::filesystem::is_directory(output_directory, error);
            if (error)
            {
                result.issues.push_back(make_error(
                    IoIssueCode::OutputDirectoryCreateFailed,
                    output_directory,
                    "Failed to inspect the output path type: " +
                        error.message()));
                return result;
            }
            if (!is_directory)
            {
                result.issues.push_back(make_error(
                    IoIssueCode::OutputPathNotDirectory,
                    output_directory,
                    "Output path exists but is not a directory."));
                return result;
            }
        }
        else
        {
            std::filesystem::create_directories(output_directory, error);
            if (error)
            {
                result.issues.push_back(make_error(
                    IoIssueCode::OutputDirectoryCreateFailed,
                    output_directory,
                    "Failed to create the output directory: " +
                        error.message()));
                return result;
            }
        }

        const auto probe = output_directory /
            (".parallelpix-write-probe-" + unique_token() + ".tmp");
        {
            std::ofstream stream(probe, std::ios::binary | std::ios::trunc);
            if (!stream)
            {
                result.issues.push_back(make_error(
                    IoIssueCode::OutputDirectoryNotWritable,
                    output_directory,
                    "Output directory is not writable."));
                return result;
            }
            stream.write("ParallelPix", 11);
            stream.flush();
            if (!stream)
            {
                stream.close();
                remove_private_file(probe);
                result.issues.push_back(make_error(
                    IoIssueCode::OutputDirectoryNotWritable,
                    output_directory,
                    "Output directory write probe failed."));
                return result;
            }
        }

        const auto removed = std::filesystem::remove(probe, error);
        if (error || !removed)
        {
            result.issues.push_back(make_error(
                IoIssueCode::OutputProbeCleanupFailed,
                probe,
                "Output write probe could not be removed."));
            return result;
        }

        result.success = true;
        return result;
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputDirectoryCreateFailed,
            output_directory,
            "Output directory preparation failed: " +
                std::string(error.what())));
        return result;
    }
}

}  // namespace parallelpix::io
