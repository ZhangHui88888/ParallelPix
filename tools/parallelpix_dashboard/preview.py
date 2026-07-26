from __future__ import annotations

from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
import streamlit as st

from .charts import overview_charts, overview_focus_chart
from .i18n import Language
from .models import BenchmarkRequest
from .results import load_results, run_options, select_run


PREVIEW_RESULTS = Path(__file__).resolve().parent / "assets" / "demo_results.csv"
PREVIEW_COPY = {
    "title": ("Performance overview", "性能概览"),
    "preview": ("preview", "预览"),
    "notice": ("Preview data — not measured", "预览数据——未经测量"),
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


def build_preview_figures(
    request: BenchmarkRequest, language: Language
) -> tuple[go.Figure, go.Figure, go.Figure, go.Figure | None]:
    data = _latest_preview()
    data = data.loc[
        data["backend"].isin(request.normalized_backends)
        & data["image_count"].isin(request.image_counts)
    ].copy()

    overview = overview_charts(data, language, compact=True)
    return (*overview[:3], overview_focus_chart(data, language, compact=True))


def render_preview_workspace(request: BenchmarkRequest, language: Language) -> None:
    with st.container(key="preview_workspace"):
        st.markdown(
            f"### {preview_tr('title', language)} "
            f"({preview_tr('preview', language)})"
        )
        st.caption(preview_tr("notice", language))

        figures = build_preview_figures(request, language)
        for row_start in (0, 2):
            for column, figure, index in zip(
                st.columns(2),
                figures[row_start : row_start + 2],
                range(row_start, row_start + 2),
                strict=True,
            ):
                with column:
                    if figure is not None:
                        st.plotly_chart(
                            figure,
                            width="stretch",
                            key=f"preview_chart_{index}",
                            config={"displayModeBar": False},
                        )

        with st.container(key="preview_hint"):
            st.markdown(f"**{preview_tr('hint_title', language)}**")
            st.caption(preview_tr("hint_body", language))
