#include "parallelpix/benchmark/reporting.hpp"

#include "parallelpix/io/image_io.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace parallelpix::benchmark {
namespace {

constexpr std::string_view csv_header =
    "run_id,recorded_at_utc,backend,thread_count,cuda_batch_size,"
    "image_count,input_resolution,output_resolution,warmups,repetitions,"
    "compute_ms,compute_min_ms,compute_max_ms,compute_stddev_ms,"
    "end_to_end_ms,end_to_end_min_ms,end_to_end_max_ms,end_to_end_stddev_ms,"
    "images_per_second,megapixels_per_second,speedup,parallel_efficiency,"
    "validation_passed,max_pixel_error,h2d_ms,kernel_ms,d2h_ms";

std::filesystem::path normalized_parent(
    const std::filesystem::path& destination)
{
    const auto parent = destination.parent_path();
    return parent.empty() ? std::filesystem::path(".") : parent;
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

ReportingResult failure(std::string message)
{
    return {false, std::move(message)};
}

ReportingResult check_existing_file(
    const std::filesystem::path& destination,
    m2::CsvMode mode)
{
    std::error_code error;
    const auto exists = std::filesystem::exists(destination, error);
    if (error)
    {
        return failure(
            "Failed to inspect the result CSV: " + error.message());
    }
    if (!exists)
    {
        return {true, {}};
    }
    if (!std::filesystem::is_regular_file(destination, error) || error)
    {
        return failure("Result CSV path is not a regular file.");
    }
    if (mode == m2::CsvMode::Overwrite)
    {
        return {true, {}};
    }

    std::ifstream stream(destination, std::ios::binary);
    std::string header;
    if (!stream || !std::getline(stream, header))
    {
        return failure("Existing result CSV is empty or unreadable.");
    }
    if (!header.empty() && header.back() == '\r')
    {
        header.pop_back();
    }
    if (header != csv_header)
    {
        return failure(
            "Existing result CSV header does not match the M7 schema.");
    }
    return {true, {}};
}

std::optional<std::string> read_existing(
    const std::filesystem::path& destination,
    std::string& error_message)
{
    std::ifstream stream(destination, std::ios::binary);
    if (!stream)
    {
        error_message = "Existing result CSV could not be opened.";
        return std::nullopt;
    }

    std::ostringstream contents;
    contents << stream.rdbuf();
    if (!stream.good() && !stream.eof())
    {
        error_message = "Existing result CSV could not be read completely.";
        return std::nullopt;
    }
    return contents.str();
}

std::string csv_escape(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos)
    {
        return value;
    }

    std::string escaped = "\"";
    for (const auto character : value)
    {
        if (character == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += character;
        }
    }
    escaped += '"';
    return escaped;
}

std::string format_double(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(6) << value;
    auto text = stream.str();
    while (!text.empty() && text.back() == '0')
    {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.')
    {
        text.pop_back();
    }
    return text == "-0" ? "0" : text;
}

template <typename Value>
std::string format_optional_integer(const std::optional<Value>& value)
{
    return value ? std::to_string(*value) : std::string{};
}

std::string format_optional_double(const std::optional<double>& value)
{
    return value ? format_double(*value) : std::string{};
}

std::string record_line(const BenchmarkRecord& record)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream
        << csv_escape(record.run_id) << ','
        << csv_escape(record.recorded_at_utc) << ','
        << csv_escape(record.backend) << ','
        << format_optional_integer(record.thread_count) << ','
        << format_optional_integer(record.cuda_batch_size) << ','
        << record.image_count << ','
        << csv_escape(record.input_resolution) << ','
        << csv_escape(record.output_resolution) << ','
        << record.warmups << ','
        << record.repetitions << ','
        << format_double(record.compute_ms) << ','
        << format_double(record.compute_min_ms) << ','
        << format_double(record.compute_max_ms) << ','
        << format_double(record.compute_stddev_ms) << ','
        << format_double(record.end_to_end_ms) << ','
        << format_double(record.end_to_end_min_ms) << ','
        << format_double(record.end_to_end_max_ms) << ','
        << format_double(record.end_to_end_stddev_ms) << ','
        << format_double(record.images_per_second) << ','
        << format_double(record.megapixels_per_second) << ','
        << format_optional_double(record.speedup) << ','
        << format_optional_double(record.parallel_efficiency) << ','
        << (record.validation_passed ? "true" : "false") << ','
        << format_optional_integer(record.max_pixel_error) << ','
        << format_optional_double(record.h2d_ms) << ','
        << format_optional_double(record.kernel_ms) << ','
        << format_optional_double(record.d2h_ms);
    return stream.str();
}

bool move_into_place(
    const std::filesystem::path& temporary,
    const std::filesystem::path& destination,
    std::string& error_message)
{
#ifdef _WIN32
    const auto flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
    if (MoveFileExW(temporary.c_str(), destination.c_str(), flags) != 0)
    {
        return true;
    }
    error_message =
        std::system_category().message(static_cast<int>(GetLastError()));
    return false;
#else
    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
    if (!error)
    {
        return true;
    }
    error_message = error.message();
    return false;
#endif
}

}  // namespace

std::string_view benchmark_csv_header() noexcept
{
    return csv_header;
}

ReportingResult validate_csv_destination(
    const std::filesystem::path& destination,
    m2::CsvMode mode)
{
    if (destination.empty())
    {
        return failure("Result CSV path is empty.");
    }

    const auto prepared =
        io::prepare_output_directory(normalized_parent(destination));
    if (!prepared.ok())
    {
        const auto message = prepared.issues.empty()
            ? std::string("Result CSV directory is not writable.")
            : prepared.issues.front().message;
        return failure(message);
    }
    return check_existing_file(destination, mode);
}

ReportingResult write_benchmark_records(
    const std::filesystem::path& destination,
    m2::CsvMode mode,
    const std::vector<BenchmarkRecord>& records)
{
    if (records.empty())
    {
        return failure("No benchmark records were provided.");
    }

    const auto validated = validate_csv_destination(destination, mode);
    if (!validated.success)
    {
        return validated;
    }

    std::string contents;
    std::error_code error;
    const auto exists = std::filesystem::exists(destination, error);
    if (error)
    {
        return failure(
            "Failed to inspect the result CSV: " + error.message());
    }
    if (mode == m2::CsvMode::Append && exists)
    {
        std::string read_error;
        const auto existing = read_existing(destination, read_error);
        if (!existing)
        {
            return failure(std::move(read_error));
        }
        contents = *existing;
        if (!contents.empty() && contents.back() != '\n')
        {
            contents += '\n';
        }
    }
    else
    {
        contents = std::string(csv_header) + '\n';
    }

    for (const auto& record : records)
    {
        contents += record_line(record);
        contents += '\n';
    }

    const auto temporary = normalized_parent(destination) /
        std::filesystem::u8path(
            destination.filename().u8string() + ".parallelpix-" +
            unique_token() + ".tmp");
    {
        std::ofstream stream(
            temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            return failure("Temporary result CSV could not be created.");
        }
        stream.write(
            contents.data(), static_cast<std::streamsize>(contents.size()));
        stream.flush();
        if (!stream)
        {
            stream.close();
            remove_private_file(temporary);
            return failure("Temporary result CSV could not be written completely.");
        }
    }

    std::string move_error;
    if (!move_into_place(temporary, destination, move_error))
    {
        remove_private_file(temporary);
        return failure(
            "Temporary result CSV could not be moved into place: " +
            move_error);
    }
    return {true, {}};
}

}  // namespace parallelpix::benchmark
