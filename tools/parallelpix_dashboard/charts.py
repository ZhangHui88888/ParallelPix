from __future__ import annotations

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go

from .i18n import Language, tr


BACKEND_COLORS = {
    "sequential": "#94A3B8",
    "openmp": "#60A5FA",
    "cuda": "#4ADE80",
}


def _config_label(row: pd.Series, language: Language) -> str:
    backend = str(row["backend"]).capitalize()
    if row["backend"] == "openmp" and pd.notna(row["thread_count"]):
        return f"OpenMP · {int(row['thread_count'])} {tr('threads', language)}"
    if row["backend"] == "cuda" and pd.notna(row["cuda_batch_size"]):
        return f"CUDA · {tr('batch_size', language)} {int(row['cuda_batch_size'])}"
    return backend


def _style(figure: go.Figure, title: str) -> go.Figure:
    figure.update_layout(
        title=title,
        legend_title_text="",
        margin=dict(l=16, r=16, t=64, b=16),
        hovermode="x unified",
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#CBD5E1"),
        title_font=dict(color="#F1F5F9", size=18),
        legend=dict(bgcolor="rgba(0,0,0,0)"),
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure


def overview_charts(
    frame: pd.DataFrame, language: Language = "en"
) -> tuple[go.Figure, go.Figure, go.Figure]:
    data = frame.copy()
    data["configuration"] = data.apply(
        lambda row: _config_label(row, language), axis=1
    )

    compute = px.bar(
        data,
        x="image_count",
        y="compute_ms",
        color="backend",
        barmode="group",
        color_discrete_map=BACKEND_COLORS,
        labels={
            "image_count": tr("images", language),
            "compute_ms": tr("median_compute_ms", language),
        },
        hover_data=["configuration"],
    )
    speedup = px.line(
        data,
        x="image_count",
        y="speedup",
        color="configuration",
        markers=True,
        labels={
            "image_count": tr("images", language),
            "speedup": tr("speedup", language),
        },
    )
    throughput = px.line(
        data,
        x="image_count",
        y="images_per_second",
        color="configuration",
        markers=True,
        labels={
            "image_count": tr("images", language),
            "images_per_second": tr("images_per_second", language),
        },
    )
    return (
        _style(compute, tr("chart_compute_title", language)),
        _style(speedup, tr("chart_speedup_title", language)),
        _style(throughput, tr("chart_throughput_title", language)),
    )


def scalability_chart(
    frame: pd.DataFrame, language: Language = "en"
) -> go.Figure | None:
    data = frame.loc[frame["backend"].eq("openmp") & frame["thread_count"].notna()].copy()
    if data.empty:
        return None
    data["image_count_label"] = data["image_count"].map(
        lambda value: f"{int(value)} {tr('images', language)}"
    )
    figure = px.line(
        data.sort_values("thread_count"),
        x="thread_count",
        y="speedup",
        color="image_count_label",
        markers=True,
        labels={
            "thread_count": tr("threads", language),
            "speedup": tr("speedup", language),
        },
    )
    return _style(figure, tr("chart_scalability_title", language))


def cuda_timing_chart(
    frame: pd.DataFrame, language: Language = "en"
) -> go.Figure | None:
    data = frame.loc[frame["backend"].eq("cuda")].copy()
    if data.empty:
        return None
    data["configuration"] = data.apply(
        lambda row: _config_label(row, language), axis=1
    )
    melted = data.melt(
        id_vars=["image_count", "configuration"],
        value_vars=["h2d_ms", "kernel_ms", "d2h_ms"],
        var_name="stage",
        value_name="milliseconds",
    ).dropna(subset=["milliseconds"])
    if melted.empty:
        return None
    labels = {
        "h2d_ms": tr("host_to_device", language),
        "kernel_ms": tr("kernel", language),
        "d2h_ms": tr("device_to_host", language),
    }
    melted["stage"] = melted["stage"].map(labels)
    figure = px.bar(
        melted,
        x="image_count",
        y="milliseconds",
        color="stage",
        facet_col="configuration",
        barmode="stack",
        labels={
            "image_count": tr("images", language),
            "milliseconds": tr("median_compute_ms", language),
        },
    )
    return _style(figure, tr("chart_cuda_title", language))
