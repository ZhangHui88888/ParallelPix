#include "parallelpix/io/image_io.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace parallelpix::io {
namespace {

IoIssue make_issue(
    IoIssueCode code,
    IoSeverity severity,
    const std::filesystem::path& path,
    std::string message)
{
    return {code, severity, path, std::move(message)};
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

std::optional<Image> make_bgr_image(
    const cv::Mat& decoded,
    const std::filesystem::path& source_path)
{
    if (decoded.empty() || decoded.depth() != CV_8U ||
        decoded.channels() != static_cast<int>(image_channel_count) ||
        decoded.cols <= 0 || decoded.rows <= 0)
    {
        return std::nullopt;
    }

    const auto width = static_cast<std::size_t>(decoded.cols);
    const auto height = static_cast<std::size_t>(decoded.rows);
    constexpr auto max_size = std::numeric_limits<std::size_t>::max();
    if (width > max_size / image_channel_count)
    {
        return std::nullopt;
    }

    const auto stride = width * image_channel_count;
    if (height > max_size / stride)
    {
        return std::nullopt;
    }

    Image image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.channels = image_channel_count;
    image.stride = stride;
    image.source_path = source_path;
    image.pixels.resize(stride * height);

    if (decoded.isContinuous() &&
        decoded.step == static_cast<std::size_t>(image.stride))
    {
        std::memcpy(image.pixels.data(), decoded.data, image.pixels.size());
    }
    else
    {
        for (int row = 0; row < decoded.rows; ++row)
        {
            std::memcpy(
                image.pixels.data() +
                    static_cast<std::size_t>(row) * image.stride,
                decoded.ptr(row),
                image.stride);
        }
    }

    if (!is_valid_image(image))
    {
        return std::nullopt;
    }
    return image;
}

}  // namespace

ImageResult decode_image(const std::filesystem::path& image_path)
{
    ImageResult result;
    const auto bytes = read_file_bytes(image_path);
    if (!bytes.has_value())
    {
        result.issues.push_back(make_issue(
            IoIssueCode::ImageDecodeFailed,
            IoSeverity::Error,
            image_path,
            "Image file could not be read."));
        return result;
    }

    try
    {
        const auto decoded = cv::imdecode(*bytes, cv::IMREAD_COLOR);
        auto image = make_bgr_image(decoded, image_path);
        if (!image.has_value())
        {
            result.issues.push_back(make_issue(
                IoIssueCode::ImageDecodeFailed,
                IoSeverity::Error,
                image_path,
                "Image could not be decoded as an 8-bit BGR image."));
            return result;
        }

        result.image = std::move(*image);
        return result;
    }
    catch (const cv::Exception& error)
    {
        result.issues.push_back(make_issue(
            IoIssueCode::ImageDecodeFailed,
            IoSeverity::Error,
            image_path,
            "OpenCV failed to decode the image: " + std::string(error.what())));
        return result;
    }
}

}  // namespace parallelpix::io
