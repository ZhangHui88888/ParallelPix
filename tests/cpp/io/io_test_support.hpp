#pragma once

#include "parallelpix/io/image_io.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace parallelpix::test::io {

inline const std::filesystem::path fixture_root =
    PARALLELPIX_TEST_FIXTURES_DIR;

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        static std::atomic<std::uint64_t> counter{0};
        const auto tick =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("parallelpix-m3-" + std::to_string(tick) + "-" +
             std::to_string(counter.fetch_add(1)));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    const std::filesystem::path& path() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

inline bool has_issue(
    const std::vector<parallelpix::io::IoIssue>& issues,
    parallelpix::io::IoIssueCode code)
{
    return std::any_of(
        issues.begin(),
        issues.end(),
        [code](const auto& issue) { return issue.code == code; });
}

inline bool has_issue(
    const std::vector<parallelpix::io::IoIssue>& issues,
    parallelpix::io::IoIssueCode code,
    parallelpix::io::IoSeverity severity)
{
    return std::any_of(
        issues.begin(),
        issues.end(),
        [code, severity](const auto& issue) {
            return issue.code == code && issue.severity == severity;
        });
}

inline void copy_fixture(
    const std::filesystem::path& relative,
    const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        fixture_root / relative,
        destination,
        std::filesystem::copy_options::overwrite_existing);
}

}  // namespace parallelpix::test::io
