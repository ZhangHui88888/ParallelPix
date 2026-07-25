#pragma once

#include "parallelpix/common/image.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace parallelpix::io {

enum class IoSeverity
{
    Warning,
    Error,
};

enum class IoIssueCode
{
    InputDirectoryMissing,
    InputPathNotDirectory,
    CatalogReadFailed,
    NoSupportedImages,
    InvalidRequestedCount,
    ImageDecodeFailed,
    InsufficientValidImages,
    WatermarkMissing,
    WatermarkDecodeFailed,
    UnsupportedWatermarkChannels,
    InvalidImage,
    OutputPathNotDirectory,
    OutputDirectoryCreateFailed,
    OutputDirectoryNotWritable,
    OutputProbeCleanupFailed,
    OutputExtensionNotPng,
    OutputExists,
    OutputEncodeFailed,
    OutputReplaceFailed,
};

struct IoIssue
{
    IoIssueCode code = IoIssueCode::CatalogReadFailed;
    IoSeverity severity = IoSeverity::Error;
    std::filesystem::path path;
    std::string message;
};

struct ImageCatalog
{
    std::filesystem::path input_directory;
    std::vector<std::filesystem::path> files;
};

struct CatalogResult
{
    std::optional<ImageCatalog> catalog;
    std::vector<IoIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return catalog.has_value();
    }
};

struct ImageResult
{
    std::optional<Image> image;
    std::vector<IoIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return image.has_value();
    }
};

struct WatermarkResult
{
    std::optional<Watermark> watermark;
    std::vector<IoIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return watermark.has_value();
    }
};

struct BatchResult
{
    std::optional<std::vector<Image>> images;
    std::vector<IoIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return images.has_value();
    }
};

struct IoStatus
{
    bool success = false;
    std::vector<IoIssue> issues;

    [[nodiscard]] bool ok() const noexcept
    {
        return success;
    }
};

enum class WriteMode
{
    FailIfExists,
    ReplaceExisting,
};

CatalogResult scan_catalog(const std::filesystem::path& input_directory);

ImageResult decode_image(const std::filesystem::path& image_path);

WatermarkResult decode_watermark(const std::filesystem::path& watermark_path);

BatchResult load_batch(
    const ImageCatalog& catalog,
    std::size_t requested_count);

IoStatus prepare_output_directory(
    const std::filesystem::path& output_directory);

IoStatus write_png(
    const Image& image,
    const std::filesystem::path& destination,
    WriteMode mode);

}  // namespace parallelpix::io
