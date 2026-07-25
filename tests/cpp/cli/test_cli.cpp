#include "../test_support.hpp"

#include "parallelpix/cli/cli.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using parallelpix::m2::Backend;
using parallelpix::m2::BenchmarkRequest;
using parallelpix::m2::CliError;
using parallelpix::m2::CsvMode;
using parallelpix::m2::HelpRequest;
using parallelpix::m2::parse_cli;

std::vector<std::string_view> valid_arguments()
{
    return {
        "benchmark",
        "--input",
        "data/images",
        "--output",
        "output",
        "--watermark",
        "data/watermark.png",
        "--backends",
        "sequential,openmp,cuda",
        "--image-counts",
        "10,50,100",
        "--threads",
        "1,2,4,8",
        "--cuda-batches",
        "1,4,8",
        "--warmups",
        "2",
        "--repetitions",
        "5",
        "--csv",
        "results/benchmark.csv",
        "--append",
    };
}

const BenchmarkRequest& require_request(const parallelpix::m2::CliParseResult& result)
{
    PP_REQUIRE(std::holds_alternative<BenchmarkRequest>(result));
    return std::get<BenchmarkRequest>(result);
}

const CliError& require_error(const parallelpix::m2::CliParseResult& result)
{
    PP_REQUIRE(std::holds_alternative<CliError>(result));
    return std::get<CliError>(result);
}

bool contains_error(const CliError& error, std::string_view fragment)
{
    for (const auto& message : error.messages)
    {
        if (message.find(fragment) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

PP_TEST("help requests do not require benchmark options")
{
    const auto top_level = parse_cli({"--help"});
    const auto benchmark = parse_cli({"benchmark", "--help"});

    PP_REQUIRE(std::holds_alternative<HelpRequest>(top_level));
    PP_REQUIRE(std::holds_alternative<HelpRequest>(benchmark));
}

PP_TEST("top-level help rejects unexpected trailing arguments")
{
    const auto result = parse_cli({"--help", "unexpected"});

    PP_REQUIRE(contains_error(require_error(result), "Unexpected argument"));
}

PP_TEST("the full M1 command contract parses into a benchmark request")
{
    const auto result = parse_cli(valid_arguments());
    const auto& request = require_request(result);

    PP_REQUIRE_EQ(request.input_dir, std::filesystem::path("data/images"));
    PP_REQUIRE_EQ(request.output_dir, std::filesystem::path("output"));
    PP_REQUIRE_EQ(request.watermark_path, std::filesystem::path("data/watermark.png"));
    PP_REQUIRE_EQ(request.result_csv, std::filesystem::path("results/benchmark.csv"));
    PP_REQUIRE_EQ(
        request.backends,
        std::vector<Backend>({Backend::Sequential, Backend::OpenMP, Backend::Cuda}));
    PP_REQUIRE_EQ(request.image_counts, std::vector<std::uint32_t>({10, 50, 100}));
    PP_REQUIRE_EQ(request.thread_counts, std::vector<std::uint32_t>({1, 2, 4, 8}));
    PP_REQUIRE_EQ(request.cuda_batch_sizes, std::vector<std::uint32_t>({1, 4, 8}));
    PP_REQUIRE_EQ(request.warmups, std::uint32_t(2));
    PP_REQUIRE_EQ(request.repetitions, std::uint32_t(5));
    PP_REQUIRE_EQ(request.csv_mode, CsvMode::Append);
}

PP_TEST("missing command and unsupported command are rejected")
{
    const auto missing = parse_cli({});
    const auto unsupported = parse_cli({"run"});

    PP_REQUIRE(contains_error(require_error(missing), "command"));
    PP_REQUIRE(contains_error(require_error(unsupported), "Unsupported command"));
}

PP_TEST("unknown duplicate and missing-value options are rejected")
{
    auto unknown_arguments = valid_arguments();
    unknown_arguments.insert(unknown_arguments.end(), {"--mystery", "value"});

    auto duplicate_arguments = valid_arguments();
    duplicate_arguments.insert(
        duplicate_arguments.end(), {"--input", "other/images"});

    auto missing_value_arguments = valid_arguments();
    const auto csv_option = std::find(
        missing_value_arguments.begin(),
        missing_value_arguments.end(),
        std::string_view("--csv"));
    missing_value_arguments.erase(csv_option + 1);

    PP_REQUIRE(contains_error(require_error(parse_cli(unknown_arguments)), "Unknown option"));
    PP_REQUIRE(contains_error(require_error(parse_cli(duplicate_arguments)), "Duplicate option"));
    PP_REQUIRE(contains_error(require_error(parse_cli(missing_value_arguments)), "Missing value"));
}

PP_TEST("backend-specific lists are required only for selected backends")
{
    const auto sequential = parse_cli({
        "benchmark",
        "--input", "in",
        "--output", "out",
        "--watermark", "watermark.png",
        "--backends", "sequential",
        "--image-counts", "10",
        "--warmups", "2",
        "--repetitions", "5",
        "--csv", "results.csv",
    });
    const auto openmp_without_threads = parse_cli({
        "benchmark",
        "--input", "in",
        "--output", "out",
        "--watermark", "watermark.png",
        "--backends", "openmp",
        "--image-counts", "10",
        "--warmups", "2",
        "--repetitions", "5",
        "--csv", "results.csv",
    });
    const auto cuda_without_batches = parse_cli({
        "benchmark",
        "--input", "in",
        "--output", "out",
        "--watermark", "watermark.png",
        "--backends", "cuda",
        "--image-counts", "10",
        "--warmups", "2",
        "--repetitions", "5",
        "--csv", "results.csv",
    });

    require_request(sequential);
    PP_REQUIRE(contains_error(require_error(openmp_without_threads), "--threads"));
    PP_REQUIRE(contains_error(require_error(cuda_without_batches), "--cuda-batches"));
}

PP_TEST("unused backend lists remain valid for M1 compatibility")
{
    const auto result = parse_cli({
        "benchmark",
        "--input", "in",
        "--output", "out",
        "--watermark", "watermark.png",
        "--backends", "sequential",
        "--image-counts", "10",
        "--threads", "1,2,4,8",
        "--cuda-batches", "1,4,8",
        "--warmups", "2",
        "--repetitions", "5",
        "--csv", "results.csv",
    });

    const auto& request = require_request(result);
    PP_REQUIRE_EQ(request.thread_counts.size(), std::size_t(4));
    PP_REQUIRE_EQ(request.cuda_batch_sizes.size(), std::size_t(3));
}

PP_TEST("numeric lists trim item whitespace")
{
    auto arguments = valid_arguments();
    const auto image_counts = std::find(
        arguments.begin(), arguments.end(), std::string_view("--image-counts"));
    PP_REQUIRE(image_counts != arguments.end());
    *(image_counts + 1) = "10, 50 ,100";

    const auto result = parse_cli(arguments);
    const auto& request = require_request(result);
    PP_REQUIRE_EQ(request.image_counts, std::vector<std::uint32_t>({10, 50, 100}));
}

PP_TEST("UTF-8 path values round-trip through filesystem paths")
{
    auto arguments = valid_arguments();
    const auto csv_option = std::find(
        arguments.begin(), arguments.end(), std::string_view("--csv"));
    *(csv_option + 1) = u8"results/结果.csv";

    const auto result = parse_cli(arguments);
    const auto& request = require_request(result);

    PP_REQUIRE_EQ(request.result_csv.u8string(), std::string(u8"results/结果.csv"));
}

PP_TEST("invalid numeric list values are rejected")
{
    const std::vector<std::string_view> invalid_values = {
        "",
        "1,,2",
        "-1",
        "0",
        "abc",
        "4294967296",
    };

    for (const auto value : invalid_values)
    {
        auto arguments = valid_arguments();
        const auto image_counts = std::find(
            arguments.begin(), arguments.end(), std::string_view("--image-counts"));
        *(image_counts + 1) = value;

        PP_REQUIRE(contains_error(require_error(parse_cli(arguments)), "--image-counts"));
    }
}

PP_TEST("benchmark policy values and CSV extension are validated")
{
    auto wrong_warmups = valid_arguments();
    *(std::find(wrong_warmups.begin(), wrong_warmups.end(), "--warmups") + 1) = "1";

    auto short_repetitions = valid_arguments();
    *(std::find(short_repetitions.begin(), short_repetitions.end(), "--repetitions") + 1) = "4";

    auto wrong_extension = valid_arguments();
    *(std::find(wrong_extension.begin(), wrong_extension.end(), "--csv") + 1) =
        "results.json";

    PP_REQUIRE(contains_error(require_error(parse_cli(wrong_warmups)), "exactly 2"));
    PP_REQUIRE(contains_error(require_error(parse_cli(short_repetitions)), "at least 5"));
    PP_REQUIRE(contains_error(require_error(parse_cli(wrong_extension)), ".csv"));
}

}  // namespace
