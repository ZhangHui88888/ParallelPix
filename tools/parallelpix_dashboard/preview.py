from __future__ import annotations

from pathlib import Path

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import streamlit as st

from .charts import BACKEND_COLORS
from .i18n import Language
from .models import BenchmarkRequest
from .results import load_results, run_options, select_run


PREVIEW_RESULTS = Path(__file__).resolve().parent / "assets" / "demo_results.csv"
PREVIEW_COPY = {
    "title": ("Performance overview", "性能概览"),
    "preview": ("preview", "预览"),
    "notice": ("Preview data — not measured", "预览数据——未经测量"),
    "focus_speedup": ("Speedup (higher is better)", "加速比（越高越好）"),
    "focus_throughput": ("Throughput (higher is better)", "吞吐量（越高越好）"),
    "focus_compute": ("Compute time (lower is better)", "计算时间（越低越好）"),
    "speedup_images": ("Speedup vs Image Count", "加速比与图片数量"),
    "throughput_images": ("Throughput vs Image Count", "吞吐量与图片数量"),
    "speedup_threads": ("Speedup vs Threads (OpenMP)", "加速比与线程数（OpenMP）"),
    "throughput_batch": (
        "CUDA Throughput vs Batch Size",
        "CUDA 吞吐量与批大小",
    ),
    "images": ("Images", "图片"),
    "threads": ("Threads", "线程"),
    "batch_size": ("Batch size", "批大小"),
    "speedup": ("Speedup", "加速比"),
    "throughput": ("Images per second", "每秒图片数"),
    "hint_title": ("Run a benchmark to populate the results.", "运行基准测试以生成结果。"),
    "hint_body": (
        "Preview data stays separate from measured runs, history, and downloads.",
        "预览数据与真实测量、运行历史和下载内容完全分离。",
    ),
    "tab_hint": (
        "Run the benchmark to open measured results in this view.",
        "运行基准测试后将在此显示真实测量结果。",
    ),
}


def preview_tr(key: str, language: Language) -> str:
    return PREVIEW_COPY[key][0 if language == "en" else 1]


def _latest_preview() -> pd.DataFrame:
    frame = load_results(PREVIEW_RESULTS)
    latest_run_id = run_options(frame)[0][0]
    return select_run(frame, latest_run_id)


def _style(figure: go.Figure, title: str, x_title: str, y_title: str) -> go.Figure:
    figure.update_layout(
        title=title,
        height=245,
        margin=dict(l=48, r=16, t=42, b=44),
        hovermode="x unified",
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor="rgba(0,0,0,0)",
        font=dict(color="#94A3B8", size=11),
        title_font=dict(color="#CBD5E1", size=14),
        legend=dict(
            bgcolor="rgba(0,0,0,0)",
            orientation="h",
            yanchor="bottom",
            y=1.02,
            xanchor="right",
            x=1,
        ),
        xaxis_title=x_title,
        yaxis_title=y_title,
    )
    figure.update_xaxes(gridcolor="#243247", zerolinecolor="#334155")
    figure.update_yaxes(gridcolor="#243247", zerolinecolor="#334155")
    return figure


def build_preview_figures(
    request: BenchmarkRequest, language: Language
) -> tuple[go.Figure, go.Figure, go.Figure, go.Figure]:
    data = _latest_preview()
    data = data.loc[
        data["backend"].isin(request.normalized_backends)
        & data["image_count"].isin(request.image_counts)
    ].copy()

    parallel = data.loc[
        data["backend"].isin(("openmp", "cuda")) & data["speedup"].notna()
    ]
    best_parallel = (
        parallel.sort_values("speedup")
        .groupby(["backend", "image_count"], as_index=False)
        .tail(1)
    )
    speedup = px.line(
        best_parallel,
        x="image_count",
        y="speedup",
        color="backend",
        markers=True,
        color_discrete_map=BACKEND_COLORS,
    )
    speedup.add_hline(
        y=1,
        line_dash="dot",
        line_color="#64748B",
        opacity=0.75,
    )

    best_throughput = (
        parallel.sort_values("images_per_second")
        .groupby(["backend", "image_count"], as_index=False)
        .tail(1)
    )
    throughput = px.bar(
        best_throughput,
        x="image_count",
        y="images_per_second",
        color="backend",
        barmode="group",
        color_discrete_map=BACKEND_COLORS,
    )

    largest_image_set = max(request.image_counts, default=100)
    openmp = data.loc[
        data["backend"].eq("openmp")
        & data["thread_count"].isin(request.thread_counts)
        & data["image_count"].eq(largest_image_set)
    ].sort_values("thread_count")
    thread_scaling = px.line(
        openmp,
        x="thread_count",
        y="speedup",
        markers=True,
        color_discrete_sequence=[BACKEND_COLORS["openmp"]],
    )

    cuda = data.loc[
        data["backend"].eq("cuda")
        & data["cuda_batch_size"].isin(request.cuda_batch_sizes)
        & data["image_count"].eq(largest_image_set)
    ].sort_values("cuda_batch_size")
    batch_scaling = px.line(
        cuda,
        x="cuda_batch_size",
        y="images_per_second",
        markers=True,
        color_discrete_sequence=[BACKEND_COLORS["cuda"]],
    )

    figures = (
        _style(
            speedup,
            preview_tr("speedup_images", language),
            preview_tr("images", language),
            preview_tr("speedup", language),
        ),
        _style(
            throughput,
            preview_tr("throughput_images", language),
            preview_tr("images", language),
            preview_tr("throughput", language),
        ),
        _style(
            thread_scaling,
            preview_tr("speedup_threads", language),
            preview_tr("threads", language),
            preview_tr("speedup", language),
        ),
        _style(
            batch_scaling,
            preview_tr("throughput_batch", language),
            preview_tr("batch_size", language),
            preview_tr("throughput", language),
        ),
    )
    figures[0].update_xaxes(tickmode="array", tickvals=list(request.image_counts))
    figures[1].update_xaxes(tickmode="array", tickvals=list(request.image_counts))
    figures[2].update_xaxes(tickmode="array", tickvals=list(request.thread_counts))
    figures[3].update_xaxes(tickmode="array", tickvals=list(request.cuda_batch_sizes))
    return figures


def render_preview_workspace(request: BenchmarkRequest, language: Language) -> None:
    with st.container(key="preview_workspace"):
        title_column, focus_column = st.columns((3, 1), vertical_alignment="center")
        with title_column:
            st.markdown(
                f"### {preview_tr('title', language)} "
                f"({preview_tr('preview', language)})"
            )
            st.caption(preview_tr("notice", language))
        with focus_column:
            st.selectbox(
                preview_tr("title", language),
                (
                    preview_tr("focus_speedup", language),
                    preview_tr("focus_throughput", language),
                    preview_tr("focus_compute", language),
                ),
                key="preview_focus",
                label_visibility="collapsed",
            )

        figures = build_preview_figures(request, language)
        for row_start in (0, 2):
            for column, figure, index in zip(
                st.columns(2),
                figures[row_start : row_start + 2],
                range(row_start, row_start + 2),
                strict=True,
            ):
                with column:
                    st.plotly_chart(
                        figure,
                        width="stretch",
                        key=f"preview_chart_{index}",
                        config={"displayModeBar": False},
                    )

        with st.container(key="preview_hint"):
            st.markdown(f"**{preview_tr('hint_title', language)}**")
            st.caption(preview_tr("hint_body", language))
