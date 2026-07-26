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


def _style(figure: go.Figure, title: str, *, compact: bool = False) -> go.Figure:
    figure.update_layout(
        title=title,
        legend_title_text="",
        height=245 if compact else None,
        margin=dict(l=48, r=16, t=42, b=44) if compact else dict(l=16, r=16, t=64, b=16),
        hovermode="x unified",
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#CBD5E1"),
        title_font=dict(color="#CBD5E1", size=14 if compact else 18),
        legend=(
            dict(
                bgcolor="rgba(0,0,0,0)",
                orientation="h",
                yanchor="bottom",
                y=1.02,
                xanchor="right",
                x=1,
            )
            if compact
            else dict(bgcolor="rgba(0,0,0,0)")
        ),
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure


def _narrow_single_bar(figure: go.Figure, data: pd.DataFrame) -> None:
    """Keep one categorical bar from filling most of an otherwise empty chart."""
    if len(data) != 1:
        return
    figure.update_traces(width=0.35)


def final_median_chart(frame: pd.DataFrame, language: Language = "en") -> go.Figure:
    """Compare median processing time per image; lower bars are faster."""
    data = frame.copy()
    data["configuration"] = data.apply(
        lambda row: _config_label(row, language), axis=1
    )
    data["median_ms_per_image"] = data["compute_ms"] / data["image_count"]
    data = data.sort_values("median_ms_per_image", ascending=False)
    figure = px.bar(
        data,
        x="configuration",
        y="median_ms_per_image",
        color="backend",
        color_discrete_map=BACKEND_COLORS,
        text="median_ms_per_image",
        labels={
            "median_ms_per_image": tr("median_ms_per_image", language),
            "configuration": tr("configuration", language),
        },
        hover_data={"image_count": False, "backend": False},
    )
    figure.update_traces(
        width=0.08, texttemplate="%{text:.2f}", textposition="outside"
    )
    figure.update_layout(
        title=tr("chart_final_median", language),
        height=245,
        margin=dict(l=48, r=24, t=42, b=44),
        showlegend=False,
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#CBD5E1"),
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure


def _final_metric_chart(
    frame: pd.DataFrame, metric: str, title_key: str, language: Language
) -> go.Figure:
    data = frame.copy()
    data["configuration"] = data.apply(
        lambda row: _config_label(row, language), axis=1
    )
    figure = px.bar(
        data.sort_values(metric, ascending=True),
        x="configuration",
        y=metric,
        color="backend",
        color_discrete_map=BACKEND_COLORS,
        text=metric,
        labels={metric: tr(metric, language), "configuration": tr("configuration", language)},
    )
    figure.update_traces(
        width=0.08, texttemplate="%{text:.2f}", textposition="outside"
    )
    figure.update_layout(
        title=tr(title_key, language),
        height=245,
        margin=dict(l=48, r=24, t=42, b=44),
        showlegend=False,
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#CBD5E1"),
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure


def final_speedup_chart(frame: pd.DataFrame, language: Language = "en") -> go.Figure:
    return _final_metric_chart(frame, "speedup", "chart_final_speedup", language)


def final_throughput_chart(
    frame: pd.DataFrame, language: Language = "en"
) -> go.Figure:
    return _final_metric_chart(
        frame, "images_per_second", "chart_final_throughput", language
    )


def final_total_duration_chart(
    frame: pd.DataFrame, language: Language = "en"
) -> go.Figure:
    data = frame.copy()
    data["end_to_end_seconds"] = data["end_to_end_ms"] / 1_000.0
    return _final_metric_chart(
        data, "end_to_end_seconds", "chart_final_total_duration", language
    )


def final_cold_start_chart(
    frame: pd.DataFrame, language: Language = "en"
) -> go.Figure | None:
    if "cold_start_cli_ms" not in frame:
        return None
    data = frame.dropna(subset=["cold_start_cli_ms"])
    if data.empty:
        return None
    return _final_metric_chart(
        data, "cold_start_cli_ms", "chart_final_cold_start", language
    )


def _overview_data(
    frame: pd.DataFrame, language: Language, image_count: int | None
) -> pd.DataFrame:
    data = frame.copy()
    if data.empty:
        data["configuration"] = pd.Series(dtype="string")
        return data
    selected_count = image_count if image_count is not None else int(data["image_count"].max())
    data = data.loc[data["image_count"].eq(selected_count)].copy()
    data["configuration"] = data.apply(
        lambda row: _config_label(row, language), axis=1
    )
    return data


def overview_charts(
    frame: pd.DataFrame,
    language: Language = "en",
    *,
    compact: bool = False,
    image_count: int | None = None,
) -> tuple[go.Figure, ...]:
    data = _overview_data(frame, language, image_count)
    configuration_order = data["configuration"].drop_duplicates().tolist()

    compute = px.bar(
        data,
        x="configuration",
        y="compute_ms",
        color="backend",
        barmode="group",
        color_discrete_map=BACKEND_COLORS,
        category_orders={"configuration": configuration_order},
        labels={
            "configuration": tr("configuration", language),
            "compute_ms": tr("median_compute_ms", language),
        },
        hover_data=["image_count"],
    )
    _narrow_single_bar(compute, data)
    speedup = px.bar(
        data,
        x="configuration",
        y="speedup",
        color="backend",
        barmode="group",
        color_discrete_map=BACKEND_COLORS,
        category_orders={"configuration": configuration_order},
        labels={
            "configuration": tr("configuration", language),
            "speedup": tr("speedup", language),
        },
        hover_data=["image_count"],
    )
    _narrow_single_bar(speedup, data)
    throughput = px.bar(
        data,
        x="configuration",
        y="images_per_second",
        color="backend",
        barmode="group",
        color_discrete_map=BACKEND_COLORS,
        category_orders={"configuration": configuration_order},
        labels={
            "configuration": tr("configuration", language),
            "images_per_second": tr("images_per_second", language),
        },
        hover_data=["image_count"],
    )
    _narrow_single_bar(throughput, data)
    figures: list[go.Figure] = [
        _style(compute, tr("chart_compute_title", language), compact=compact),
        _style(speedup, tr("chart_speedup_title", language), compact=compact),
        _style(throughput, tr("chart_throughput_title", language), compact=compact),
    ]
    cold_start = (
        data.dropna(subset=["cold_start_cli_ms"])
        if "cold_start_cli_ms" in data.columns
        else pd.DataFrame()
    )
    if not cold_start.empty:
        figure = px.bar(
            cold_start,
            x="configuration",
            y="cold_start_cli_ms",
            color="backend",
            barmode="group",
            color_discrete_map=BACKEND_COLORS,
            category_orders={"configuration": configuration_order},
            labels={
                "configuration": tr("configuration", language),
                "cold_start_cli_ms": tr("cold_start_baseline_ms", language),
            },
            hover_data=["image_count"],
        )
        _narrow_single_bar(figure, cold_start)
        figures.append(
            _style(figure, tr("chart_cold_start_title", language), compact=compact)
        )
    return tuple(figures)


def overview_focus_chart(
    frame: pd.DataFrame,
    language: Language = "en",
    *,
    compact: bool = False,
    image_count: int | None = None,
) -> go.Figure | None:
    """Return the most useful parallel scaling chart for the Overview grid."""
    if frame.empty:
        return None
    selected_count = image_count if image_count is not None else int(frame["image_count"].max())
    cuda = frame.loc[
        frame["backend"].eq("cuda") & frame["image_count"].eq(selected_count)
    ].copy()
    if not cuda.empty:
        data = cuda.sort_values("cuda_batch_size")
        figure = px.line(
            data,
            x="cuda_batch_size",
            y="images_per_second",
            markers=True,
            color_discrete_sequence=[BACKEND_COLORS["cuda"]],
            labels={
                "cuda_batch_size": tr("batch_size", language),
                "images_per_second": tr("images_per_second", language),
            },
        )
        return _style(figure, tr("chart_cuda_throughput_title", language), compact=compact)

    openmp = frame.loc[
        frame["backend"].eq("openmp")
        & frame["thread_count"].notna()
        & frame["image_count"].eq(selected_count)
    ].copy()
    if openmp.empty:
        return None
    data = openmp.sort_values("thread_count")
    figure = px.line(
        data,
        x="thread_count",
        y="speedup",
        markers=True,
        color_discrete_sequence=[BACKEND_COLORS["openmp"]],
        labels={
            "thread_count": tr("threads", language),
            "speedup": tr("speedup", language),
        },
    )
    return _style(figure, tr("chart_scalability_title", language), compact=compact)


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
