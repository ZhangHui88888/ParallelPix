from __future__ import annotations

import csv
from pathlib import Path


LIFECYCLE_COLUMNS = ("run_id", "cold_start_cli_ms")


def lifecycle_path(result_csv: Path) -> Path:
    return result_csv.with_suffix(result_csv.suffix + ".lifecycle.csv")


def record_cold_start_measurements(
    result_csv: Path, run_ids: tuple[str, ...], elapsed_ms: float
) -> None:
    """Persist one external CLI lifecycle duration for each produced run id."""
    destination = lifecycle_path(result_csv)
    destination.parent.mkdir(parents=True, exist_ok=True)
    write_header = not destination.exists()
    with destination.open("a", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=LIFECYCLE_COLUMNS)
        if write_header:
            writer.writeheader()
        for run_id in run_ids:
            writer.writerow({"run_id": run_id, "cold_start_cli_ms": f"{elapsed_ms:.6f}"})


def load_cold_start_measurements(result_csv: Path) -> dict[str, float]:
    """Return the latest valid measurement for each benchmark run id."""
    source = lifecycle_path(result_csv)
    if not source.is_file():
        return {}

    measurements: dict[str, float] = {}
    try:
        with source.open("r", newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            if reader.fieldnames != list(LIFECYCLE_COLUMNS):
                return {}
            for row in reader:
                run_id = (row.get("run_id") or "").strip()
                try:
                    elapsed_ms = float(row.get("cold_start_cli_ms") or "")
                except ValueError:
                    continue
                if run_id and elapsed_ms >= 0.0:
                    measurements[run_id] = elapsed_ms
    except OSError:
        return {}
    return measurements
