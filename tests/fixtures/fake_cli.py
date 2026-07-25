from __future__ import annotations

import csv
import sys
from pathlib import Path


CSV_COLUMNS = (
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


def append_result(path: Path) -> None:
    row = {column: "" for column in CSV_COLUMNS}
    row.update(
        {
            "run_id": "test-run-new",
            "recorded_at_utc": "2026-07-25T10:00:00Z",
            "backend": "sequential",
            "image_count": 10,
            "input_resolution": "1920x1080",
            "output_resolution": "1024x1024",
            "warmups": 2,
            "repetitions": 5,
            "compute_ms": 100,
            "compute_min_ms": 98,
            "compute_max_ms": 104,
            "compute_stddev_ms": 2,
            "end_to_end_ms": 125,
            "end_to_end_min_ms": 122,
            "end_to_end_max_ms": 130,
            "end_to_end_stddev_ms": 3,
            "images_per_second": 100,
            "megapixels_per_second": 104.86,
            "speedup": 1,
            "validation_passed": "true",
            "max_pixel_error": 0,
        }
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    write_header = not path.exists()
    with path.open("a", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=CSV_COLUMNS)
        if write_header:
            writer.writeheader()
        writer.writerow(row)


def main() -> int:
    result_path = Path(sys.argv[1])
    exit_code = int(sys.argv[2])
    should_write = sys.argv[3] == "write"
    print("fake-cli: benchmark started", flush=True)
    if should_write:
        append_result(result_path)
        print("fake-cli: result appended", flush=True)
    return exit_code


raise SystemExit(main())
