#include "../test_support.hpp"
#include "io_test_support.hpp"

#include "parallelpix/benchmark/backend.hpp"
#include "parallelpix/pipeline/pipeline_factory.hpp"

#include "parallelpix/planning/benchmark_plan.hpp"
#include "parallelpix/cli/cli.hpp"
#include "parallelpix/controller/controller.hpp"

#include <filesystem>
#include <optional>

namespace {

using parallelpix::m2::Backend;
using parallelpix::m2::BenchmarkRequest;
using parallelpix::m2::CsvMode;
using parallelpix::m2::build_benchmark_plan;
using parallelpix::m2::make_benchmark_pipeline;
using parallelpix::test::io::TemporaryDirectory;
using parallelpix::test::io::copy_fixture;

PP_TEST("the M7 pipeline runs every backend available on this machine")
{
    TemporaryDirectory temporary;
    const auto input = temporary.path() / L"输入";
    const auto output = temporary.path() / L"输出";
    const auto watermark = temporary.path() / L"水印.png";
    const auto csv = temporary.path() / L"结果.csv";
    copy_fixture("images/color_3x2.png", input / L"商品.png");
    copy_fixture("watermarks/rgba_2x2.png", watermark);

    const BenchmarkRequest request{
        input,
        output,
        watermark,
        csv,
        {Backend::Sequential, Backend::OpenMP, Backend::Cuda},
        {1},
        {2},
        {1},
        2,
        5,
        false,
        CsvMode::Overwrite,
    };
    const auto plan = build_benchmark_plan(request);
    const bool cuda_available = static_cast<bool>(
        parallelpix::benchmark::make_cuda_executor());
    const auto pipeline = make_benchmark_pipeline();

    const auto summary = pipeline->execute(plan, [](const auto&) {});

    PP_REQUIRE_EQ(summary.planned, std::size_t{3});
    PP_REQUIRE_EQ(
        summary.succeeded,
        cuda_available ? std::size_t{3} : std::size_t{2});
    PP_REQUIRE_EQ(summary.failed, std::size_t{0});
    PP_REQUIRE_EQ(
        summary.skipped,
        cuda_available ? std::size_t{0} : std::size_t{1});
    PP_REQUIRE_EQ(summary.csv_path, std::optional(csv));
    PP_REQUIRE(std::filesystem::is_regular_file(csv));
    PP_REQUIRE_EQ(
        summary.issues.size(),
        cuda_available ? std::size_t{0} : std::size_t{1});
    PP_REQUIRE(!summary.primary_failure.has_value());
}

}  // namespace
