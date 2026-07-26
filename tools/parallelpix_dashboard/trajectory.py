from __future__ import annotations

import re
import json
from pathlib import Path

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go

from .i18n import Language, tr


_EVENT = re.compile(
    r"\[INFO\]\[trajectory\] backend=(?P<backend>\w+) run_id=(?P<run_id>\S+) "
    r"thread_count=(?P<thread_count>\d+) cuda_batch_size=(?P<cuda_batch_size>\d+) "
    r"image_count=(?P<image_count>\d+) processed=(?P<processed>\d+) "
    r"ms_per_image=(?P<ms_per_image>[0-9.]+)"
)


def parse_progress_event(line: str) -> dict[str, object] | None:
    match = _EVENT.search(line)
    if not match:
        return None
    values = match.groupdict()
    backend = values["backend"]
    configuration = backend.capitalize()
    if backend == "openmp":
        configuration = f"OpenMP · {values['thread_count']} threads"
    elif backend == "cuda":
        configuration = f"CUDA · batch {values['cuda_batch_size']}"
    return {
        "configuration": configuration,
        "run_id": values["run_id"],
        "image_count": int(values["image_count"]),
        "processed": int(values["processed"]),
        "ms_per_image": float(values["ms_per_image"]),
    }


def trajectory_path(result_csv: Path) -> Path:
    return result_csv.with_name(f"{result_csv.name}.trajectory.jsonl")


def save_trajectory_samples(result_csv: Path, samples: list[dict[str, object]]) -> None:
    if not samples:
        return
    destination = trajectory_path(result_csv)
    with destination.open("a", encoding="utf-8") as stream:
        for sample in samples:
            stream.write(json.dumps(sample, ensure_ascii=False) + "\n")


def load_trajectory_samples(result_csv: Path, run_id: str) -> list[dict[str, object]]:
    source = trajectory_path(result_csv)
    if not source.is_file():
        return []
    samples: list[dict[str, object]] = []
    try:
        with source.open(encoding="utf-8") as stream:
            for line in stream:
                sample = json.loads(line)
                if sample.get("run_id") == run_id:
                    samples.append(sample)
    except (OSError, json.JSONDecodeError):
        return []
    return samples


def trajectory_chart(
    samples: list[dict[str, object]], language: Language, *, window: int = 200
) -> go.Figure | None:
    if not samples:
        return None
    data = pd.DataFrame(samples)
    figure = px.line(
        data,
        x="processed",
        y="ms_per_image",
        color="configuration",
        markers=True,
        labels={
            "processed": tr("processed_images", language),
            "ms_per_image": tr("batch_ms_per_image", language),
            "configuration": tr("configuration", language),
        },
    )
    latest = int(data["processed"].max())
    figure.update_xaxes(range=[max(0, latest - window), latest])
    figure.update_layout(
        title=tr("chart_live_trajectory", language),
        height=300,
        margin=dict(l=48, r=16, t=48, b=44),
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#CBD5E1"),
        uirevision="trajectory",
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure
