from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path

import pandas as pd

from .models import BenchmarkRequest


COLD_START_COLUMNS = (
    "run_id",
    "backend",
    "thread_count",
    "cuda_batch_size",
    "image_count",
    "cold_start_cli_ms",
)

# Legacy per-configuration probe files remain readable for existing runs.


@dataclass(frozen=True)
class ColdStartConfiguration:
    backend: str
    image_count: int
    thread_count: int | None = None
    cuda_batch_size: int | None = None


@dataclass(frozen=True)
class ColdStartMeasurement:
    configuration: ColdStartConfiguration
    elapsed_ms: float


def cold_start_path(result_csv: Path) -> Path:
    return result_csv.with_suffix(result_csv.suffix + ".cold-start.csv")


def cold_start_configurations(
    request: BenchmarkRequest,
) -> tuple[ColdStartConfiguration, ...]:
    configurations: list[ColdStartConfiguration] = []
    for image_count in request.image_counts:
        if "sequential" in request.normalized_backends:
            configurations.append(ColdStartConfiguration("sequential", image_count))
        if "openmp" in request.normalized_backends:
            configurations.extend(
                ColdStartConfiguration("openmp", image_count, thread_count)
                for thread_count in request.thread_counts
            )
        if "cuda" in request.normalized_backends:
            configurations.extend(
                ColdStartConfiguration("cuda", image_count, cuda_batch_size=batch_size)
                for batch_size in request.cuda_batch_sizes
            )
    return tuple(configurations)


def _value(value: int | None) -> str:
    return "" if value is None else str(value)


def record_cold_start_measurements(
    result_csv: Path,
    parent_run_ids: tuple[str, ...],
    measurements: tuple[ColdStartMeasurement, ...],
) -> None:
    destination = cold_start_path(result_csv)
    destination.parent.mkdir(parents=True, exist_ok=True)
    write_header = not destination.exists()
    with destination.open("a", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=COLD_START_COLUMNS)
        if write_header:
            writer.writeheader()
        for run_id in parent_run_ids:
            for measurement in measurements:
                configuration = measurement.configuration
                writer.writerow(
                    {
                        "run_id": run_id,
                        "backend": configuration.backend,
                        "thread_count": _value(configuration.thread_count),
                        "cuda_batch_size": _value(configuration.cuda_batch_size),
                        "image_count": configuration.image_count,
                        "cold_start_cli_ms": f"{measurement.elapsed_ms:.6f}",
                    }
                )


def load_cold_start_measurements(result_csv: Path) -> pd.DataFrame:
    source = cold_start_path(result_csv)
    if not source.is_file():
        return pd.DataFrame(columns=COLD_START_COLUMNS)
    try:
        frame = pd.read_csv(source)
    except (OSError, UnicodeError, pd.errors.EmptyDataError, pd.errors.ParserError):
        return pd.DataFrame(columns=COLD_START_COLUMNS)
    if tuple(frame.columns) != COLD_START_COLUMNS:
        return pd.DataFrame(columns=COLD_START_COLUMNS)
    for column in ("thread_count", "cuda_batch_size", "image_count", "cold_start_cli_ms"):
        frame[column] = pd.to_numeric(frame[column], errors="coerce")
    return frame
