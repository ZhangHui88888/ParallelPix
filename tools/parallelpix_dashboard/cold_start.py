from __future__ import annotations

import csv
import tempfile
from dataclasses import dataclass, replace
from pathlib import Path

import pandas as pd

from .models import BenchmarkRequest, LogEmitter, RunStatus
from .results import load_results
from .runners import SubprocessRunner


COLD_START_COLUMNS = (
    "run_id",
    "backend",
    "thread_count",
    "cuda_batch_size",
    "image_count",
    "cold_start_cli_ms",
)


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


def _matches(row: pd.Series, configuration: ColdStartConfiguration) -> bool:
    thread_count = None if pd.isna(row["thread_count"]) else int(row["thread_count"])
    cuda_batch_size = (
        None if pd.isna(row["cuda_batch_size"]) else int(row["cuda_batch_size"])
    )
    return (
        row["backend"] == configuration.backend
        and int(row["image_count"]) == configuration.image_count
        and thread_count == configuration.thread_count
        and cuda_batch_size == configuration.cuda_batch_size
    )


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


class ColdStartProbeRunner:
    """Measure each configuration in a fresh child process without polluting CSV history."""

    def __init__(self, working_directory: Path) -> None:
        self.working_directory = working_directory

    def run(
        self,
        request: BenchmarkRequest,
        parent_run_ids: tuple[str, ...],
        emit_log: LogEmitter,
    ) -> tuple[ColdStartMeasurement, ...]:
        measurements: list[ColdStartMeasurement] = []
        configurations = cold_start_configurations(request)
        with tempfile.TemporaryDirectory(prefix="parallelpix-cold-start-") as temporary:
            root = Path(temporary)
            for index, configuration in enumerate(configurations):
                emit_log(
                    "[COLD START] "
                    f"{configuration.backend}, images={configuration.image_count} "
                    f"({index + 1}/{len(configurations)})"
                )
                probe_request = replace(
                    request,
                    backends=(configuration.backend,),
                    image_counts=(configuration.image_count,),
                    thread_counts=(configuration.thread_count,)
                    if configuration.thread_count is not None
                    else (),
                    cuda_batch_sizes=(configuration.cuda_batch_size,)
                    if configuration.cuda_batch_size is not None
                    else (),
                    output_dir=root / f"output-{index}",
                    result_csv=root / f"result-{index}.csv",
                    measure_cold_start=False,
                )
                result = SubprocessRunner(working_directory=self.working_directory).run(
                    probe_request,
                    emit_log,
                )
                if result.status not in {RunStatus.SUCCESS, RunStatus.PARTIAL}:
                    emit_log("[COLD START] Probe failed; configuration was not recorded.")
                    continue
                if result.cold_start_cli_ms is None or result.csv_path is None:
                    continue
                try:
                    frame = load_results(result.csv_path)
                except Exception:
                    emit_log("[COLD START] Probe CSV was invalid; configuration was not recorded.")
                    continue
                matched = frame.loc[frame.apply(_matches, axis=1, configuration=configuration)]
                if matched.empty or not matched["validation_passed"].all():
                    emit_log("[COLD START] Probe did not produce a validated configuration.")
                    continue
                measurements.append(
                    ColdStartMeasurement(configuration, result.cold_start_cli_ms)
                )

        if measurements:
            record_cold_start_measurements(
                request.result_csv, parent_run_ids, tuple(measurements)
            )
        return tuple(measurements)
