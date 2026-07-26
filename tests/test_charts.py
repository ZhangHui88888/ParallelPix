from __future__ import annotations

from pathlib import Path

from tools.parallelpix_dashboard.charts import (
    final_median_chart,
    final_speedup_chart,
    final_total_duration_chart,
    final_throughput_chart,
    overview_charts,
    overview_focus_chart,
)
from tools.parallelpix_dashboard.models import BenchmarkRequest, RunMode
from tools.parallelpix_dashboard.preview import build_preview_figures
from tools.parallelpix_dashboard.results import load_results, run_options, select_run


DEMO_RESULTS = (
    Path(__file__).resolve().parents[1]
    / "tools"
    / "parallelpix_dashboard"
    / "assets"
    / "demo_results.csv"
)


def test_preview_and_measured_overview_share_chart_meanings() -> None:
    frame = load_results(DEMO_RESULTS)
    latest = select_run(frame, run_options(frame)[0][0])
    request = BenchmarkRequest(
        mode=RunMode.DEMO,
        cli_path=Path("parallelpix.exe"),
        input_dir=Path("data/benchmark"),
        output_dir=Path("output"),
        watermark_path=Path("data/watermark.png"),
        result_csv=Path("results/benchmark.csv"),
        backends=("sequential", "openmp", "cuda"),
        image_counts=(10, 50, 100),
        thread_counts=(1, 2, 4, 8),
        cuda_batch_sizes=(1, 4, 8),
    )

    preview = build_preview_figures(request, "en")
    measured = (*overview_charts(latest, "en", compact=True)[:3], overview_focus_chart(latest, "en", compact=True))

    assert [figure.layout.title.text for figure in preview if figure is not None] == [
        figure.layout.title.text for figure in measured if figure is not None
    ]
    assert [figure.layout.title.text for figure in preview[:3]] == [
        "Median compute time",
        "Speedup by configuration",
        "Throughput by configuration",
    ]


def test_single_result_compute_bar_has_a_fixed_narrow_width() -> None:
    frame = load_results(DEMO_RESULTS)
    single_result = select_run(frame, run_options(frame)[0][0]).iloc[[0]]

    compute, *_ = overview_charts(single_result, "en", compact=True)

    assert compute.data[0].width == 0.35
    assert list(compute.data[0].x) == ["Sequential"]


def test_final_comparison_charts_use_configuration_on_the_x_axis() -> None:
    frame = load_results(DEMO_RESULTS)
    latest = select_run(frame, run_options(frame)[0][0])

    figure = final_median_chart(latest, "en")

    assert figure.data[0].orientation == "v"
    assert figure.data[0].width == 0.08
    assert figure.layout.title.text == "Median processing time per image — lower is faster"

    speedup = final_speedup_chart(latest, "en")
    assert speedup.data[0].orientation == "v"
    assert speedup.data[0].width == 0.08
    assert speedup.layout.title.text == "Final speedup"

    throughput = final_throughput_chart(latest, "en")
    assert throughput.data[0].orientation == "v"
    assert throughput.data[0].width == 0.08
    assert throughput.layout.title.text == "Final throughput — higher is faster"

    total_duration = final_total_duration_chart(latest, "en")
    assert total_duration.data[0].orientation == "v"
    assert total_duration.data[0].width == 0.08
    assert total_duration.layout.title.text == "End-to-end total duration — lower is faster"


def test_overview_charts_compare_configurations_at_selected_image_count() -> None:
    frame = load_results(DEMO_RESULTS)
    latest = select_run(frame, run_options(frame)[0][0])

    compute, speedup, throughput = overview_charts(
        latest, "en", compact=True, image_count=100
    )[:3]

    expected = ["Sequential", "OpenMP · 1 OpenMP threads", "OpenMP · 2 OpenMP threads", "OpenMP · 4 OpenMP threads", "OpenMP · 8 OpenMP threads"]
    assert list(compute.data[0].x) == ["Sequential"]
    assert list(speedup.data[1].x) == expected[1:]
    assert list(throughput.data[2].x) == ["CUDA · Batch size 1", "CUDA · Batch size 4", "CUDA · Batch size 8"]
