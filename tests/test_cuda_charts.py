from __future__ import annotations

from pathlib import Path

from tools.parallelpix_dashboard.charts import cuda_timing_chart
from tools.parallelpix_dashboard.results import load_results, run_options, select_run


DEMO_RESULTS = (
    Path(__file__).resolve().parents[1]
    / "tools"
    / "parallelpix_dashboard"
    / "assets"
    / "demo_results.csv"
)


def test_cuda_timing_chart_uses_all_three_measured_phases() -> None:
    frame = load_results(DEMO_RESULTS)
    latest = select_run(frame, run_options(frame)[0][0])

    figure = cuda_timing_chart(latest, "en")

    assert figure is not None
    assert {trace.name for trace in figure.data} == {
        "Host to device",
        "Kernel",
        "Device to host",
    }
    assert len(figure.layout.annotations) == 3
