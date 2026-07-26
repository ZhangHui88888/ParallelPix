from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from pathlib import Path
from typing import Callable, Protocol


SUPPORTED_BACKENDS = ("sequential", "openmp", "cuda")


class RunMode(StrEnum):
    DEMO = "demo"
    LOCAL_CLI = "local_cli"


class RunStatus(StrEnum):
    IDLE = "idle"
    VALIDATING = "validating"
    RUNNING = "running"
    SUCCESS = "success"
    PARTIAL = "partial"
    FAILED = "failed"


def normalize_backends(backends: tuple[str, ...]) -> tuple[str, ...]:
    """Return supported backends in stable order and add the CPU baseline."""
    selected = {backend.lower() for backend in backends}
    if selected.intersection({"openmp", "cuda"}):
        selected.add("sequential")
    return tuple(backend for backend in SUPPORTED_BACKENDS if backend in selected)


@dataclass(frozen=True)
class BenchmarkRequest:
    mode: RunMode
    cli_path: Path
    input_dir: Path
    output_dir: Path
    watermark_path: Path
    result_csv: Path
    backends: tuple[str, ...]
    image_counts: tuple[int, ...]
    thread_counts: tuple[int, ...]
    cuda_batch_sizes: tuple[int, ...]
    warmups: int = 2
    repetitions: int = 5
    input_errors: tuple[str, ...] = ()
    measure_cold_start: bool = False

    @property
    def normalized_backends(self) -> tuple[str, ...]:
        return normalize_backends(self.backends)

    def command_args(self) -> list[str]:
        args = [
            str(self.cli_path),
            "benchmark",
            "--input",
            str(self.input_dir),
            "--output",
            str(self.output_dir),
            "--watermark",
            str(self.watermark_path),
            "--backends",
            ",".join(self.normalized_backends),
            "--image-counts",
            ",".join(map(str, self.image_counts)),
            "--warmups",
            str(self.warmups),
            "--repetitions",
            str(self.repetitions),
            "--csv",
            str(self.result_csv),
        ]
        if self.measure_cold_start:
            args.append("--cold-start")
        if self.thread_counts:
            args.extend(["--threads", ",".join(map(str, self.thread_counts))])
        if self.cuda_batch_sizes:
            args.extend(["--cuda-batches", ",".join(map(str, self.cuda_batch_sizes))])
        args.append("--append")
        return args


@dataclass(frozen=True)
class RunResult:
    status: RunStatus
    exit_code: int
    csv_path: Path | None
    run_ids: tuple[str, ...]
    logs: tuple[str, ...]
    message: str
    cold_start_cli_ms: float | None = None


LogEmitter = Callable[[str], None]


class BenchmarkRunner(Protocol):
    def run(self, request: BenchmarkRequest, emit_log: LogEmitter) -> RunResult:
        """Run a benchmark request and return its UI-facing outcome."""
