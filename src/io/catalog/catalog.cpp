#include "parallelpix/io/image_io.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace parallelpix::io {
namespace {

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

bool is_supported_image(const std::filesystem::path& path)
{
    const auto extension = lowercase_ascii(path.extension().u8string());
    return extension == ".jpg" || extension == ".jpeg" || extension == ".png";
}

IoIssue make_error(
    IoIssueCode code,
    const std::filesystem::path& path,
    std::string message)
{
    return {code, IoSeverity::Error, path, std::move(message)};
}

}  // namespace

CatalogResult scan_catalog(const std::filesystem::path& input_directory)
{
    CatalogResult result;

    try
    {
        std::error_code error;
        const auto exists = std::filesystem::exists(input_directory, error);
        if (error)
        {
            result.issues.push_back(make_error(
                IoIssueCode::CatalogReadFailed,
                input_directory,
                "Failed to inspect the input directory: " + error.message()));
            return result;
        }
        if (!exists)
        {
            result.issues.push_back(make_error(
                IoIssueCode::InputDirectoryMissing,
                input_directory,
                "Input directory does not exist."));
            return result;
        }

        const auto is_directory =
            std::filesystem::is_directory(input_directory, error);
        if (error)
        {
            result.issues.push_back(make_error(
                IoIssueCode::CatalogReadFailed,
                input_directory,
                "Failed to inspect the input directory type: " + error.message()));
            return result;
        }
        if (!is_directory)
        {
            result.issues.push_back(make_error(
                IoIssueCode::InputPathNotDirectory,
                input_directory,
                "Input path is not a directory."));
            return result;
        }

        ImageCatalog catalog;
        catalog.input_directory = input_directory;

        for (const auto& entry : std::filesystem::directory_iterator(input_directory))
        {
            if (entry.is_regular_file() && is_supported_image(entry.path()))
            {
                catalog.files.push_back(entry.path());
            }
        }

        std::sort(
            catalog.files.begin(),
            catalog.files.end(),
            [](const auto& left, const auto& right) {
                const auto left_name = left.filename().u8string();
                const auto right_name = right.filename().u8string();
                if (left_name != right_name)
                {
                    return left_name < right_name;
                }
                return left.u8string() < right.u8string();
            });

        if (catalog.files.empty())
        {
            result.issues.push_back(make_error(
                IoIssueCode::NoSupportedImages,
                input_directory,
                "Input directory contains no supported JPG, JPEG, or PNG files."));
            return result;
        }

        result.catalog = std::move(catalog);
        return result;
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        result.issues.push_back(make_error(
            IoIssueCode::CatalogReadFailed,
            input_directory,
            "Failed to scan the input directory: " + std::string(error.what())));
        return result;
    }
}

}  // namespace parallelpix::io
