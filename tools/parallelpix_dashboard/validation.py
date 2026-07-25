from __future__ import annotations

from pathlib import Path

from .models import BenchmarkRequest, RunMode, SUPPORTED_BACKENDS


def _positive_list(values: tuple[int, ...], label: str) -> list[str]:
    if not values:
        return [f"Select at least one {label}."]
    if any(value <= 0 for value in values):
        return [f"{label.title()} values must be positive integers."]
    return []


def _validate_existing_parent(path: Path, label: str) -> list[str]:
    parent = path.parent
    if parent.exists() and not parent.is_dir():
        return [f"The parent path for {label} is not a directory: {parent}"]
    return []


def validate_request(request: BenchmarkRequest) -> list[str]:
    errors: list[str] = list(request.input_errors)
    unknown = set(request.backends).difference(SUPPORTED_BACKENDS)

    if not request.normalized_backends:
        errors.append("Select at least one processing backend.")
    if unknown:
        errors.append(f"Unsupported backends: {', '.join(sorted(unknown))}.")

    errors.extend(_positive_list(request.image_counts, "image count"))
    if "openmp" in request.normalized_backends:
        errors.extend(_positive_list(request.thread_counts, "OpenMP thread count"))
    if "cuda" in request.normalized_backends:
        errors.extend(_positive_list(request.cuda_batch_sizes, "CUDA batch size"))

    if request.warmups != 2:
        errors.append("M1 requires exactly 2 warm-up runs.")
    if request.repetitions < 5:
        errors.append("Benchmark repetitions must be at least 5.")
    if request.result_csv.suffix.lower() != ".csv":
        errors.append("The result path must use the .csv extension.")

    if request.mode == RunMode.LOCAL_CLI:
        if not request.cli_path.is_file():
            errors.append(f"CLI executable not found: {request.cli_path}")
        if not request.input_dir.is_dir():
            errors.append(f"Input directory not found: {request.input_dir}")
        if not request.watermark_path.is_file():
            errors.append(f"Watermark file not found: {request.watermark_path}")
        if request.output_dir.exists() and not request.output_dir.is_dir():
            errors.append(f"Output path is not a directory: {request.output_dir}")
        errors.extend(_validate_existing_parent(request.output_dir, "output directory"))
        errors.extend(_validate_existing_parent(request.result_csv, "result CSV"))

    return errors
