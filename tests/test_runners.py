from __future__ import annotations

import sys
from pathlib import Path

from tools.parallelpix_dashboard.models import BenchmarkRequest, RunMode, RunStatus
from tools.parallelpix_dashboard.runners import DemoRunner, SubprocessRunner


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEMO_RESULTS = (
    PROJECT_ROOT / "tools" / "parallelpix_dashboard" / "assets" / "demo_results.csv"
)
FAKE_CLI = PROJECT_ROOT / "tests" / "fixtures" / "fake_cli.py"


def make_request(tmp_path: Path) -> BenchmarkRequest:
    return BenchmarkRequest(
        mode=RunMode.LOCAL_CLI,
        cli_path=Path(sys.executable),
        input_dir=tmp_path,
        output_dir=tmp_path / "output",
        watermark_path=tmp_path / "watermark.png",
        result_csv=tmp_path / "benchmark.csv",
        backends=("sequential",),
        image_counts=(10,),
        thread_counts=(),
        cuda_batch_sizes=(),
    )


def fake_command(exit_code: int, should_write: bool):
    def factory(request: BenchmarkRequest) -> list[str]:
        return [
            sys.executable,
            str(FAKE_CLI),
            str(request.result_csv),
            str(exit_code),
            "write" if should_write else "skip",
        ]

    return factory


def test_demo_runner_returns_bundled_history(tmp_path: Path) -> None:
    request = make_request(tmp_path)
    logs: list[str] = []

    result = DemoRunner(DEMO_RESULTS).run(request, logs.append)

    assert result.status == RunStatus.SUCCESS
    assert result.csv_path == DEMO_RESULTS
    assert len(result.run_ids) == 2
    assert all(line.startswith("[DEMO]") for line in logs)


def test_subprocess_runner_maps_success_and_new_run_id(tmp_path: Path) -> None:
    request = make_request(tmp_path)
    result = SubprocessRunner(fake_command(0, True), PROJECT_ROOT).run(request, lambda _: None)

    assert result.status == RunStatus.SUCCESS
    assert result.run_ids == ("test-run-new",)
    assert result.csv_path == request.result_csv


def test_subprocess_runner_maps_partial_success(tmp_path: Path) -> None:
    request = make_request(tmp_path)
    result = SubprocessRunner(fake_command(2, True), PROJECT_ROOT).run(request, lambda _: None)

    assert result.status == RunStatus.PARTIAL
    assert result.exit_code == 2


def test_subprocess_runner_maps_nonzero_failure(tmp_path: Path) -> None:
    request = make_request(tmp_path)
    result = SubprocessRunner(fake_command(7, False), PROJECT_ROOT).run(request, lambda _: None)

    assert result.status == RunStatus.FAILED
    assert result.exit_code == 7


def test_subprocess_runner_requires_new_run_id(tmp_path: Path) -> None:
    request = make_request(tmp_path)
    result = SubprocessRunner(fake_command(0, False), PROJECT_ROOT).run(request, lambda _: None)

    assert result.status == RunStatus.FAILED
    assert "did not create" in result.message
