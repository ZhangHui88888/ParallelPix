#include "parallelpix/io/image_io.hpp"

#include <algorithm>
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

}  // namespace

BatchResult load_batch(
    const ImageCatalog& catalog,
    std::size_t requested_count)
{
    BatchResult result;
    if (requested_count == 0)
    {
        result.issues.push_back(make_error(
            IoIssueCode::InvalidRequestedCount,
            catalog.input_directory,
            "Requested image count must be greater than zero."));
        return result;
    }

    std::vector<Image> images;
    images.reserve(std::min(requested_count, catalog.files.size()));

    for (const auto& path : catalog.files)
    {
        auto decoded = decode_image(path);
        if (decoded.ok())
        {
            images.push_back(std::move(*decoded.image));
            if (images.size() == requested_count)
            {
                result.images = std::move(images);
                return result;
            }
            continue;
        }

        for (auto& issue : decoded.issues)
        {
            issue.severity = IoSeverity::Warning;
            result.issues.push_back(std::move(issue));
        }
    }

    result.issues.push_back(make_error(
        IoIssueCode::InsufficientValidImages,
        catalog.input_directory,
        "The catalog does not contain enough decodable images for the "
        "requested batch size."));
    return result;
}

}  // namespace parallelpix::io
