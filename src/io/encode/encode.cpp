#include "parallelpix/io/image_io.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace parallelpix::io {
namespace {

IoIssue make_error(
    IoIssueCode code,
    const std::filesystem::path& path,
    std::string message)
{
    return {code, IoSeverity::Error, path, std::move(message)};
}

std::string lowercase_ascii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
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

std::filesystem::path normalized_parent(
    const std::filesystem::path& destination)
{
    const auto parent = destination.parent_path();
    return parent.empty() ? std::filesystem::path(".") : parent;
}

void remove_private_file(const std::filesystem::path& path) noexcept
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

bool move_into_place(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    WriteMode mode,
    std::string& error_message)
{
#ifdef _WIN32
    DWORD flags = MOVEFILE_WRITE_THROUGH;
    if (mode == WriteMode::ReplaceExisting)
    {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }
    if (MoveFileExW(source.c_str(), destination.c_str(), flags) != 0)
    {
        return true;
    }

    error_message =
        std::system_category().message(static_cast<int>(GetLastError()));
    return false;
#else
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    if (!error)
    {
        return true;
    }

    error_message = error.message();
    return false;
#endif
}

}  // namespace

IoStatus write_png(
    const Image& image,
    const std::filesystem::path& destination,
    WriteMode mode)
{
    IoStatus result;
    if (!is_valid_image(image) ||
        image.width >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        image.height >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
        result.issues.push_back(make_error(
            IoIssueCode::InvalidImage,
            image.source_path,
            "Image does not satisfy the tightly packed 8-bit BGR contract."));
        return result;
    }

    if (lowercase_ascii(destination.extension().u8string()) != ".png")
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputExtensionNotPng,
            destination,
            "Output file must use the .png extension."));
        return result;
    }

    auto directory_status =
        prepare_output_directory(normalized_parent(destination));
    if (!directory_status.ok())
    {
        return directory_status;
    }

    std::error_code error;
    auto destination_exists = std::filesystem::exists(destination, error);
    if (error)
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputEncodeFailed,
            destination,
            "Failed to inspect the output file: " + error.message()));
        return result;
    }
    if (destination_exists && mode == WriteMode::FailIfExists)
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputExists,
            destination,
            "Output file already exists."));
        return result;
    }
    if (destination_exists &&
        !std::filesystem::is_regular_file(destination, error))
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputReplaceFailed,
            destination,
            "Existing output path is not a regular file."));
        return result;
    }
    if (error)
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputReplaceFailed,
            destination,
            "Failed to inspect the existing output file: " + error.message()));
        return result;
    }

    std::vector<std::uint8_t> encoded;
    try
    {
        const cv::Mat image_view(
            static_cast<int>(image.height),
            static_cast<int>(image.width),
            CV_8UC3,
            const_cast<std::uint8_t*>(image.pixels.data()),
            image.stride);
        const std::vector<int> parameters = {
            cv::IMWRITE_PNG_COMPRESSION,
            3,
        };
        if (!cv::imencode(".png", image_view, encoded, parameters) ||
            encoded.empty())
        {
            result.issues.push_back(make_error(
                IoIssueCode::OutputEncodeFailed,
                destination,
                "OpenCV failed to encode the image as PNG."));
            return result;
        }
    }
    catch (const cv::Exception& exception)
    {
        result.issues.push_back(make_error(
            IoIssueCode::OutputEncodeFailed,
            destination,
            "OpenCV failed to encode the image: " +
                std::string(exception.what())));
        return result;
    }

    const auto temporary = normalized_parent(destination) /
        std::filesystem::u8path(
            destination.filename().u8string() + ".parallelpix-" +
            unique_token() + ".tmp");
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            result.issues.push_back(make_error(
                IoIssueCode::OutputEncodeFailed,
                temporary,
                "Temporary output file could not be created."));
            return result;
        }
        stream.write(
            reinterpret_cast<const char*>(encoded.data()),
            static_cast<std::streamsize>(encoded.size()));
        stream.flush();
        if (!stream)
        {
            stream.close();
            remove_private_file(temporary);
            result.issues.push_back(make_error(
                IoIssueCode::OutputEncodeFailed,
                temporary,
                "Encoded PNG could not be written completely."));
            return result;
        }
    }

    destination_exists = std::filesystem::exists(destination, error);
    if (error)
    {
        remove_private_file(temporary);
        result.issues.push_back(make_error(
            IoIssueCode::OutputReplaceFailed,
            destination,
            "Failed to recheck the output file: " + error.message()));
        return result;
    }
    if (destination_exists)
    {
        if (mode == WriteMode::FailIfExists)
        {
            remove_private_file(temporary);
            result.issues.push_back(make_error(
                IoIssueCode::OutputExists,
                destination,
                "Output file appeared while the PNG was being encoded."));
            return result;
        }

        if (!std::filesystem::is_regular_file(destination, error) || error)
        {
            remove_private_file(temporary);
            result.issues.push_back(make_error(
                IoIssueCode::OutputReplaceFailed,
                destination,
                "Existing output path cannot be safely replaced."));
            return result;
        }

    }

    std::string move_error;
    if (!move_into_place(temporary, destination, mode, move_error))
    {
        remove_private_file(temporary);
        result.issues.push_back(make_error(
            mode == WriteMode::ReplaceExisting
                ? IoIssueCode::OutputReplaceFailed
                : IoIssueCode::OutputEncodeFailed,
            destination,
            "Temporary PNG could not be moved into place: " +
                move_error));
        return result;
    }

    result.success = true;
    return result;
}

}  // namespace parallelpix::io
