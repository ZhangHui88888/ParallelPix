#include "parallelpix/io/image_io.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::io {
namespace {

IoIssue make_error(
    IoIssueCode code,
    const std::filesystem::path& path,
    std::string message)
{
    return {code, IoSeverity::Error, path, std::move(message)};
}

std::optional<std::vector<std::uint8_t>> read_file_bytes(
    const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        return std::nullopt;
    }

    const auto end = stream.tellg();
    const auto byte_count = static_cast<std::streamoff>(end);
    if (byte_count <= 0 ||
        byte_count > std::numeric_limits<std::streamsize>::max() ||
        static_cast<std::uintmax_t>(byte_count) >
            std::numeric_limits<std::size_t>::max())
    {
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(byte_count));
    stream.seekg(0, std::ios::beg);
    stream.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!stream)
    {
        return std::nullopt;
    }
    return bytes;
}

}  // namespace

WatermarkResult decode_watermark(
    const std::filesystem::path& watermark_path)
{
    WatermarkResult result;
    const auto bytes = read_file_bytes(watermark_path);
    if (!bytes.has_value())
    {
        result.issues.push_back(make_error(
            IoIssueCode::WatermarkMissing,
            watermark_path,
            "Watermark file does not exist, is empty, or could not be read."));
        return result;
    }

    try
    {
        const auto decoded = cv::imdecode(*bytes, cv::IMREAD_UNCHANGED);
        if (decoded.empty() || decoded.depth() != CV_8U ||
            decoded.cols <= 0 || decoded.rows <= 0)
        {
            result.issues.push_back(make_error(
                IoIssueCode::WatermarkDecodeFailed,
                watermark_path,
                "Watermark could not be decoded as an 8-bit image."));
            return result;
        }

        const auto source_channels = decoded.channels();
        if (source_channels != 1 && source_channels != 3 &&
            source_channels != 4)
        {
            result.issues.push_back(make_error(
                IoIssueCode::UnsupportedWatermarkChannels,
                watermark_path,
                "Watermark must contain 1, 3, or 4 channels."));
            return result;
        }

        const auto width = static_cast<std::size_t>(decoded.cols);
        const auto height = static_cast<std::size_t>(decoded.rows);
        constexpr auto max_size = std::numeric_limits<std::size_t>::max();
        if (width > max_size / image_channel_count)
        {
            result.issues.push_back(make_error(
                IoIssueCode::WatermarkDecodeFailed,
                watermark_path,
                "Watermark dimensions exceed the supported memory size."));
            return result;
        }

        const auto stride = width * image_channel_count;
        if (height > max_size / stride || height > max_size / width)
        {
            result.issues.push_back(make_error(
                IoIssueCode::WatermarkDecodeFailed,
                watermark_path,
                "Watermark dimensions exceed the supported memory size."));
            return result;
        }

        Watermark watermark;
        watermark.color.width = static_cast<std::uint32_t>(width);
        watermark.color.height = static_cast<std::uint32_t>(height);
        watermark.color.channels = image_channel_count;
        watermark.color.stride = stride;
        watermark.color.source_path = watermark_path;

        const auto pixel_count = width * height;
        watermark.color.pixels.resize(pixel_count * image_channel_count);
        watermark.alpha.resize(pixel_count, 255);

        for (int row = 0; row < decoded.rows; ++row)
        {
            const auto* source = decoded.ptr<std::uint8_t>(row);
            for (int column = 0; column < decoded.cols; ++column)
            {
                const auto source_index =
                    static_cast<std::size_t>(column) *
                    static_cast<std::size_t>(source_channels);
                const auto pixel_index =
                    static_cast<std::size_t>(row) *
                        static_cast<std::size_t>(decoded.cols) +
                    static_cast<std::size_t>(column);
                const auto color_index = pixel_index * image_channel_count;

                if (source_channels == 1)
                {
                    const auto value = source[source_index];
                    watermark.color.pixels[color_index] = value;
                    watermark.color.pixels[color_index + 1] = value;
                    watermark.color.pixels[color_index + 2] = value;
                }
                else
                {
                    watermark.color.pixels[color_index] = source[source_index];
                    watermark.color.pixels[color_index + 1] =
                        source[source_index + 1];
                    watermark.color.pixels[color_index + 2] =
                        source[source_index + 2];
                    if (source_channels == 4)
                    {
                        watermark.alpha[pixel_index] =
                            source[source_index + 3];
                    }
                }
            }
        }

        if (!is_valid_watermark(watermark))
        {
            result.issues.push_back(make_error(
                IoIssueCode::WatermarkDecodeFailed,
                watermark_path,
                "Decoded watermark does not satisfy the image invariants."));
            return result;
        }

        result.watermark = std::move(watermark);
        return result;
    }
    catch (const cv::Exception& error)
    {
        result.issues.push_back(make_error(
            IoIssueCode::WatermarkDecodeFailed,
            watermark_path,
            "OpenCV failed to decode the watermark: " +
                std::string(error.what())));
        return result;
    }
}

}  // namespace parallelpix::io
