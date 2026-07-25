from __future__ import annotations

from pathlib import Path

import pandas as pd


REQUIRED_COLUMNS = (
    "run_id",
    "recorded_at_utc",
    "backend",
    "thread_count",
    "cuda_batch_size",
    "image_count",
    "input_resolution",
    "output_resolution",
    "warmups",
    "repetitions",
    "compute_ms",
    "compute_min_ms",
    "compute_max_ms",
    "compute_stddev_ms",
    "end_to_end_ms",
    "end_to_end_min_ms",
    "end_to_end_max_ms",
    "end_to_end_stddev_ms",
    "images_per_second",
    "megapixels_per_second",
    "speedup",
    "parallel_efficiency",
    "validation_passed",
    "max_pixel_error",
    "h2d_ms",
    "kernel_ms",
    "d2h_ms",
)

NUMERIC_COLUMNS = tuple(
    column
    for column in REQUIRED_COLUMNS
    if column
    not in {
        "run_id",
        "recorded_at_utc",
        "backend",
        "input_resolution",
        "output_resolution",
        "validation_passed",
    }
)


class ResultsError(ValueError):
    """Raised when a benchmark CSV cannot be safely displayed."""


def _parse_validation(series: pd.Series) -> pd.Series:
    if series.dtype == bool:
        return series
    normalized = series.astype(str).str.strip().str.lower()
    mapping = {"true": True, "1": True, "yes": True, "false": False, "0": False, "no": False}
    invalid = normalized[~normalized.isin(mapping)].unique()
    if invalid.size:
        raise ResultsError(f"Invalid validation_passed values: {', '.join(invalid)}")
    return normalized.map(mapping).astype(bool)


def load_results(path: Path) -> pd.DataFrame:
    if not path.is_file():
        raise ResultsError(f"Result CSV not found: {path}")
    try:
        frame = pd.read_csv(path)
    except pd.errors.EmptyDataError as error:
        raise ResultsError("Result CSV is empty.") from error
    except (OSError, UnicodeError, pd.errors.ParserError) as error:
        raise ResultsError(f"Unable to read result CSV: {error}") from error

    missing = [column for column in REQUIRED_COLUMNS if column not in frame.columns]
    if missing:
        raise ResultsError(f"Missing required CSV columns: {', '.join(missing)}")
    if frame.empty:
        raise ResultsError("Result CSV contains no benchmark rows.")

    frame = frame.loc[:, REQUIRED_COLUMNS].copy()
    frame["run_id"] = frame["run_id"].astype(str).str.strip()
    frame["backend"] = frame["backend"].astype(str).str.strip().str.lower()
    frame["recorded_at_utc"] = pd.to_datetime(
        frame["recorded_at_utc"], utc=True, errors="coerce"
    )
    if frame["run_id"].eq("").any():
        raise ResultsError("CSV contains an empty run_id.")
    if frame["recorded_at_utc"].isna().any():
        raise ResultsError("CSV contains an invalid recorded_at_utc value.")

    frame["validation_passed"] = _parse_validation(frame["validation_passed"])
    for column in NUMERIC_COLUMNS:
        frame[column] = pd.to_numeric(frame[column], errors="coerce")
    return frame


def read_run_ids(path: Path) -> set[str]:
    if not path.is_file():
        return set()
    try:
        frame = pd.read_csv(path, usecols=["run_id"], dtype={"run_id": str})
    except (OSError, ValueError, pd.errors.ParserError, pd.errors.EmptyDataError):
        return set()
    return {value.strip() for value in frame["run_id"].dropna() if value.strip()}


def run_options(frame: pd.DataFrame) -> list[tuple[str, pd.Timestamp]]:
    timestamps = frame.groupby("run_id", sort=False)["recorded_at_utc"].max()
    ordered = timestamps.sort_values(ascending=False)
    return list(ordered.items())


def select_run(frame: pd.DataFrame, run_id: str) -> pd.DataFrame:
    return frame.loc[frame["run_id"].eq(run_id)].copy()


def valid_rows(frame: pd.DataFrame) -> pd.DataFrame:
    return frame.loc[frame["validation_passed"]].copy()
