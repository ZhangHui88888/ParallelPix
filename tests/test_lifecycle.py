from __future__ import annotations

from tools.parallelpix_dashboard.lifecycle import (
    lifecycle_path,
    load_cold_start_measurements,
    record_cold_start_measurements,
)


def test_cold_start_measurements_persist_by_run_id(tmp_path) -> None:
    result_csv = tmp_path / "benchmark.csv"

    record_cold_start_measurements(result_csv, ("run-a",), 15.25)
    record_cold_start_measurements(result_csv, ("run-a", "run-b"), 20.5)

    assert lifecycle_path(result_csv).name == "benchmark.csv.lifecycle.csv"
    assert load_cold_start_measurements(result_csv) == {
        "run-a": 20.5,
        "run-b": 20.5,
    }
