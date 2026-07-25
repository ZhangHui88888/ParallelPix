from __future__ import annotations

import subprocess
from collections.abc import Callable, Sequence
from pathlib import Path

from .models import BenchmarkRequest, LogEmitter, RunResult, RunStatus
from .results import read_run_ids


CommandFactory = Callable[[BenchmarkRequest], Sequence[str]]


class DemoRunner:
    def __init__(self, fixture_path: Path) -> None:
        self.fixture_path = fixture_path

    def run(self, request: BenchmarkRequest, emit_log: LogEmitter) -> RunResult:
        logs = (
            "[DEMO] Preparing deterministic benchmark matrix.",
            f"[DEMO] Backends: {', '.join(request.normalized_backends)}",
            f"[DEMO] Image counts: {', '.join(map(str, request.image_counts))}",
            "[DEMO] Loading bundled sample results; no computation was measured.",
            "[DEMO] Benchmark completed.",
        )
        for line in logs:
            emit_log(line)

        if not self.fixture_path.is_file():
            return RunResult(
                status=RunStatus.FAILED,
                exit_code=1,
                csv_path=None,
                run_ids=(),
                logs=logs,
                message=f"Demo result fixture not found: {self.fixture_path}",
            )

        return RunResult(
            status=RunStatus.SUCCESS,
            exit_code=0,
            csv_path=self.fixture_path,
            run_ids=tuple(sorted(read_run_ids(self.fixture_path))),
            logs=logs,
            message="Demo benchmark completed with bundled sample data.",
        )


class SubprocessRunner:
    def __init__(
        self,
        command_factory: CommandFactory | None = None,
        working_directory: Path | None = None,
    ) -> None:
        self.command_factory = command_factory or (lambda request: request.command_args())
        self.working_directory = working_directory

    def run(self, request: BenchmarkRequest, emit_log: LogEmitter) -> RunResult:
        before_run_ids = read_run_ids(request.result_csv)
        command = [str(part) for part in self.command_factory(request)]
        logs: list[str] = []

        def capture(line: str) -> None:
            clean = line.rstrip("\r\n")
            if clean:
                logs.append(clean)
                emit_log(clean)

        try:
            process = subprocess.Popen(
                command,
                cwd=self.working_directory,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
                shell=False,
            )
            assert process.stdout is not None
            for line in process.stdout:
                capture(line)
            exit_code = process.wait()
        except OSError as error:
            capture(f"Unable to start benchmark CLI: {error}")
            return RunResult(
                status=RunStatus.FAILED,
                exit_code=1,
                csv_path=None,
                run_ids=(),
                logs=tuple(logs),
                message="Benchmark CLI could not be started.",
            )

        if exit_code not in {0, 2}:
            return RunResult(
                status=RunStatus.FAILED,
                exit_code=exit_code,
                csv_path=None,
                run_ids=(),
                logs=tuple(logs),
                message=f"Benchmark CLI failed with exit code {exit_code}.",
            )

        after_run_ids = read_run_ids(request.result_csv)
        new_run_ids = tuple(sorted(after_run_ids.difference(before_run_ids)))
        if not request.result_csv.is_file():
            return RunResult(
                status=RunStatus.FAILED,
                exit_code=exit_code,
                csv_path=None,
                run_ids=(),
                logs=tuple(logs),
                message="Benchmark finished but did not create the result CSV.",
            )
        if not new_run_ids:
            return RunResult(
                status=RunStatus.FAILED,
                exit_code=exit_code,
                csv_path=request.result_csv,
                run_ids=(),
                logs=tuple(logs),
                message="Benchmark finished but did not append a new run_id.",
            )

        status = RunStatus.SUCCESS if exit_code == 0 else RunStatus.PARTIAL
        message = (
            "Benchmark completed successfully."
            if status == RunStatus.SUCCESS
            else "Benchmark completed with partial results."
        )
        return RunResult(
            status=status,
            exit_code=exit_code,
            csv_path=request.result_csv,
            run_ids=new_run_ids,
            logs=tuple(logs),
            message=message,
        )
