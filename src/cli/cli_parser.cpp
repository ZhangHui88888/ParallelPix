#include "parallelpix/cli/cli.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace parallelpix::m2 {
namespace {

using OptionMap = std::unordered_map<std::string, std::string>;

std::string trim(std::string_view value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char item) {
        return std::isspace(item) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char item) {
        return std::isspace(item) != 0;
    }).base();

    if (first >= last)
    {
        return {};
    }
    return std::string(first, last);
}

CliError error(std::string message)
{
    return CliError{{std::move(message)}};
}

std::optional<std::uint32_t> parse_positive_integer(std::string_view value)
{
    const auto cleaned = trim(value);
    if (cleaned.empty())
    {
        return std::nullopt;
    }

    std::uint32_t parsed = 0;
    const auto result = std::from_chars(
        cleaned.data(), cleaned.data() + cleaned.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != cleaned.data() + cleaned.size() ||
        parsed == 0)
    {
        return std::nullopt;
    }
    return parsed;
}

std::optional<std::uint32_t> parse_nonnegative_integer(std::string_view value)
{
    const auto cleaned = trim(value);
    std::uint32_t parsed = 0;
    const auto result = std::from_chars(
        cleaned.data(), cleaned.data() + cleaned.size(), parsed);
    if (cleaned.empty() || result.ec != std::errc{} ||
        result.ptr != cleaned.data() + cleaned.size())
    {
        return std::nullopt;
    }
    return parsed;
}

std::variant<std::vector<std::uint32_t>, CliError> parse_positive_list(
    std::string_view value,
    std::string_view option)
{
    std::vector<std::uint32_t> parsed;
    std::size_t start = 0;

    while (start <= value.size())
    {
        const auto separator = value.find(',', start);
        const auto length = separator == std::string_view::npos
            ? value.size() - start
            : separator - start;
        const auto item = value.substr(start, length);
        const auto number = parse_positive_integer(item);
        if (!number)
        {
            return error(
                std::string(option) +
                " must be a comma-separated list of positive 32-bit integers.");
        }
        parsed.push_back(*number);

        if (separator == std::string_view::npos)
        {
            break;
        }
        start = separator + 1;
    }

    if (parsed.empty())
    {
        return error(std::string(option) + " must contain at least one value.");
    }
    return parsed;
}

std::variant<std::vector<Backend>, CliError> parse_backends(std::string_view value)
{
    std::vector<Backend> parsed;
    std::size_t start = 0;

    while (start <= value.size())
    {
        const auto separator = value.find(',', start);
        const auto length = separator == std::string_view::npos
            ? value.size() - start
            : separator - start;
        const auto item = trim(value.substr(start, length));

        if (item == "sequential")
        {
            parsed.push_back(Backend::Sequential);
        }
        else if (item == "openmp")
        {
            parsed.push_back(Backend::OpenMP);
        }
        else if (item == "cuda")
        {
            parsed.push_back(Backend::Cuda);
        }
        else
        {
            return error(
                "--backends supports only sequential, openmp, and cuda.");
        }

        if (separator == std::string_view::npos)
        {
            break;
        }
        start = separator + 1;
    }

    if (parsed.empty())
    {
        return error("--backends must contain at least one backend.");
    }
    return parsed;
}

bool has_backend(const std::vector<Backend>& backends, Backend backend)
{
    return std::find(backends.begin(), backends.end(), backend) != backends.end();
}

std::string lower_extension(const std::filesystem::path& path)
{
    auto extension = path.extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char item) {
        return static_cast<char>(std::tolower(item));
    });
    return extension;
}

}  // namespace

CliParseResult parse_cli(const std::vector<std::string_view>& arguments)
{
    if (arguments.empty())
    {
        return error("A command is required. Use 'parallelpix benchmark --help'.");
    }
    if (arguments.front() == "--help" && arguments.size() == 1)
    {
        return HelpRequest{};
    }
    if (arguments.front() == "--help")
    {
        return error(
            "Unexpected argument after --help: " +
            std::string(arguments[1]) + ".");
    }
    if (arguments.front() != "benchmark")
    {
        return error(
            "Unsupported command '" + std::string(arguments.front()) +
            "'. Expected 'benchmark'.");
    }
    if (arguments.size() == 2 && arguments[1] == "--help")
    {
        return HelpRequest{};
    }

    const std::unordered_set<std::string> value_options = {
        "--input",
        "--output",
        "--watermark",
        "--backends",
        "--image-counts",
        "--threads",
        "--cuda-batches",
        "--warmups",
        "--repetitions",
        "--csv",
    };
    OptionMap values;
    bool append = false;
    bool cold_start = false;

    for (std::size_t index = 1; index < arguments.size(); ++index)
    {
        const auto option = std::string(arguments[index]);
        if (option == "--append")
        {
            if (append)
            {
                return error("Duplicate option: --append.");
            }
            append = true;
            continue;
        }
        if (option == "--cold-start")
        {
            if (cold_start)
            {
                return error("Duplicate option: --cold-start.");
            }
            cold_start = true;
            continue;
        }
        if (value_options.find(option) == value_options.end())
        {
            return error("Unknown option: " + option + ".");
        }
        if (values.find(option) != values.end())
        {
            return error("Duplicate option: " + option + ".");
        }
        if (index + 1 >= arguments.size() ||
            arguments[index + 1].substr(0, 2) == "--")
        {
            return error("Missing value for option: " + option + ".");
        }
        values.emplace(option, std::string(arguments[++index]));
    }

    const std::vector<std::string> required_options = {
        "--input",
        "--output",
        "--watermark",
        "--backends",
        "--image-counts",
        "--warmups",
        "--repetitions",
        "--csv",
    };
    for (const auto& option : required_options)
    {
        if (values.find(option) == values.end() || trim(values.at(option)).empty())
        {
            return error("Missing required option: " + option + ".");
        }
    }

    const auto parsed_backends = parse_backends(values.at("--backends"));
    if (std::holds_alternative<CliError>(parsed_backends))
    {
        return std::get<CliError>(parsed_backends);
    }
    const auto backends = std::get<std::vector<Backend>>(parsed_backends);

    if (has_backend(backends, Backend::OpenMP) &&
        values.find("--threads") == values.end())
    {
        return error("--threads is required when the openmp backend is selected.");
    }
    if (has_backend(backends, Backend::Cuda) &&
        values.find("--cuda-batches") == values.end())
    {
        return error("--cuda-batches is required when the cuda backend is selected.");
    }

    const auto image_counts = parse_positive_list(
        values.at("--image-counts"), "--image-counts");
    if (std::holds_alternative<CliError>(image_counts))
    {
        return std::get<CliError>(image_counts);
    }

    std::vector<std::uint32_t> thread_counts;
    if (const auto threads = values.find("--threads"); threads != values.end())
    {
        const auto parsed = parse_positive_list(threads->second, "--threads");
        if (std::holds_alternative<CliError>(parsed))
        {
            return std::get<CliError>(parsed);
        }
        thread_counts = std::get<std::vector<std::uint32_t>>(parsed);
    }

    std::vector<std::uint32_t> cuda_batch_sizes;
    if (const auto batches = values.find("--cuda-batches"); batches != values.end())
    {
        const auto parsed = parse_positive_list(batches->second, "--cuda-batches");
        if (std::holds_alternative<CliError>(parsed))
        {
            return std::get<CliError>(parsed);
        }
        cuda_batch_sizes = std::get<std::vector<std::uint32_t>>(parsed);
    }

    const auto warmups = parse_nonnegative_integer(values.at("--warmups"));
    if (!warmups)
    {
        return error("--warmups must be a non-negative 32-bit integer.");
    }
    if ((!cold_start && *warmups != 2) || (cold_start && *warmups != 0))
    {
        return error(cold_start
            ? "--cold-start requires --warmups 0."
            : "--warmups must be exactly 2.");
    }

    const auto repetitions = parse_positive_integer(values.at("--repetitions"));
    if (!repetitions)
    {
        return error("--repetitions must be a positive 32-bit integer.");
    }
    if ((!cold_start && *repetitions < 5) || (cold_start && *repetitions != 1))
    {
        return error(cold_start
            ? "--cold-start requires --repetitions 1."
            : "--repetitions must be at least 5.");
    }

    const auto result_csv = std::filesystem::u8path(values.at("--csv"));
    if (lower_extension(result_csv) != ".csv")
    {
        return error("--csv must use the .csv file extension.");
    }

    return BenchmarkRequest{
        std::filesystem::u8path(values.at("--input")),
        std::filesystem::u8path(values.at("--output")),
        std::filesystem::u8path(values.at("--watermark")),
        result_csv,
        backends,
        std::get<std::vector<std::uint32_t>>(image_counts),
        std::move(thread_counts),
        std::move(cuda_batch_sizes),
        *warmups,
        *repetitions,
        cold_start,
        append ? CsvMode::Append : CsvMode::Overwrite,
    };
}

}  // namespace parallelpix::m2
