from __future__ import annotations

from pathlib import Path

from tools.parallelpix_dashboard.cold_start import (
    ColdStartConfiguration,
    ColdStartMeasurement,
    cold_start_configurations,
    cold_start_path,
    load_cold_start_measurements,
    record_cold_start_measurements,
)
from tools.parallelpix_dashboard.models import BenchmarkRequest, RunMode


def make_request(tmp_path: Path) -> BenchmarkRequest:
    return BenchmarkRequest(
        mode=RunMode.LOCAL_CLI,
        cli_path=tmp_path / "parallelpix.exe",
        input_dir=tmp_path / "images",
        output_dir=tmp_path / "output",
        watermark_path=tmp_path / "watermark.png",
        result_csv=tmp_path / "benchmark.csv",
        backends=("sequential",),
        image_counts=(1, 50),
        thread_counts=(),
        cuda_batch_sizes=(),
        measure_cold_start=True,
    )


def test_cold_start_expands_each_selected_configuration(tmp_path: Path) -> None:
    assert cold_start_configurations(make_request(tmp_path)) == (
        ColdStartConfiguration("sequential", 1),
        ColdStartConfiguration("sequential", 50),
    )


def test_cold_start_measurements_are_keyed_by_run_and_configuration(tmp_path: Path) -> None:
    result_csv = tmp_path / "benchmark.csv"
    measurements = (
        ColdStartMeasurement(ColdStartConfiguration("sequential", 1), 125.0),
        ColdStartMeasurement(ColdStartConfiguration("sequential", 50), 500.0),
    )

    record_cold_start_measurements(result_csv, ("run-1",), measurements)
    frame = load_cold_start_measurements(result_csv)

    assert cold_start_path(result_csv).name == "benchmark.csv.cold-start.csv"
    assert frame["run_id"].tolist() == ["run-1", "run-1"]
    assert frame["image_count"].tolist() == [1, 50]
    assert frame["cold_start_cli_ms"].tolist() == [125.0, 500.0]
