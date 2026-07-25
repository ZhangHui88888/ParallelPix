from __future__ import annotations

from pathlib import Path

from tools.parallelpix_dashboard.models import BenchmarkRequest, RunMode, normalize_backends
from tools.parallelpix_dashboard.sidebar import parse_positive_integer_list
from tools.parallelpix_dashboard.validation import validate_request


def make_request(tmp_path: Path, **changes: object) -> BenchmarkRequest:
    values: dict[str, object] = {
        "mode": RunMode.DEMO,
        "cli_path": tmp_path / "parallelpix.exe",
        "input_dir": tmp_path / "images",
        "output_dir": tmp_path / "output",
        "watermark_path": tmp_path / "watermark.png",
        "result_csv": tmp_path / "benchmark.csv",
        "backends": ("openmp", "cuda"),
        "image_counts": (10, 50, 100),
        "thread_counts": (1, 2, 4, 8),
        "cuda_batch_sizes": (1, 4, 8),
    }
    values.update(changes)
    return BenchmarkRequest(**values)  # type: ignore[arg-type]


def test_parallel_backend_adds_sequential_in_stable_order() -> None:
    assert normalize_backends(("cuda", "openmp")) == ("sequential", "openmp", "cuda")


def test_command_args_match_m2_contract(tmp_path: Path) -> None:
    request = make_request(tmp_path)

    args = request.command_args()

    assert args[1] == "benchmark"
    assert args[args.index("--backends") + 1] == "sequential,openmp,cuda"
    assert args[args.index("--image-counts") + 1] == "10,50,100"
    assert args[args.index("--threads") + 1] == "1,2,4,8"
    assert args[args.index("--cuda-batches") + 1] == "1,4,8"
    assert args[-1] == "--append"


def test_sequential_command_omits_unused_parallel_arguments(tmp_path: Path) -> None:
    request = make_request(
        tmp_path,
        backends=("sequential",),
        image_counts=(1,),
        thread_counts=(),
        cuda_batch_sizes=(),
    )

    args = request.command_args()

    assert args[args.index("--backends") + 1] == "sequential"
    assert args[args.index("--image-counts") + 1] == "1"
    assert "--threads" not in args
    assert "--cuda-batches" not in args


def test_free_numeric_matrix_input_parses_and_deduplicates() -> None:
    values, error = parse_positive_integer_list("1, 10, 1", "invalid")

    assert values == (1, 10)
    assert error is None

    values, error = parse_positive_integer_list("1, zero", "invalid")

    assert values == ()
    assert error == "invalid"


def test_demo_request_does_not_require_local_paths(tmp_path: Path) -> None:
    assert validate_request(make_request(tmp_path)) == []


def test_local_request_reports_missing_files(tmp_path: Path) -> None:
    errors = validate_request(make_request(tmp_path, mode=RunMode.LOCAL_CLI))

    assert any("CLI executable not found" in error for error in errors)
    assert any("Input directory not found" in error for error in errors)
    assert any("Watermark file not found" in error for error in errors)


def test_request_rejects_empty_matrix_and_short_repetition_count(tmp_path: Path) -> None:
    errors = validate_request(
        make_request(
            tmp_path,
            backends=(),
            image_counts=(),
            repetitions=4,
            result_csv=tmp_path / "benchmark.json",
        )
    )

    assert "Select at least one processing backend." in errors
    assert "Select at least one image count." in errors
    assert "Benchmark repetitions must be at least 5." in errors
    assert "The result path must use the .csv extension." in errors
