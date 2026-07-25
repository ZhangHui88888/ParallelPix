from __future__ import annotations

from pathlib import Path

import pandas as pd
import streamlit as st

from .charts import cuda_timing_chart, overview_charts, scalability_chart
from .components import RunContextMeta, render_run_context
from .i18n import Language, tr
from .models import BenchmarkRequest
from .preview import preview_tr, render_preview_workspace
from .lifecycle import load_cold_start_measurements
from .results import run_options, select_run, valid_rows


def _metric_value(series: pd.Series, strategy: str, suffix: str) -> str:
    values = series.dropna()
    if values.empty:
        return "N/A"
    value = values.min() if strategy == "min" else values.max()
    return f"{value:.2f}{suffix}"


def _render_metric_cards(
    selected: pd.DataFrame, valid: pd.DataFrame, language: Language
) -> None:
    columns = st.columns(4)
    columns[0].metric(
        tr("validated_configurations", language), f"{len(valid)} / {len(selected)}"
    )
    columns[1].metric(
        tr("best_speedup", language), _metric_value(valid["speedup"], "max", "×")
    )
    columns[2].metric(
        tr("peak_throughput", language),
        _metric_value(valid["images_per_second"], "max", " /s"),
    )
    columns[3].metric(
        tr("fastest_compute", language),
        _metric_value(valid["compute_ms"], "min", " ms"),
    )


def _render_empty_results(request: BenchmarkRequest, language: Language) -> None:
    tab_labels = tuple(
        tr(key, language) for key in ("overview", "scalability", "cuda_timing", "raw_data")
    )
    tabs = st.tabs(tab_labels)
    with tabs[0]:
        render_preview_workspace(request, language)
    for index, tab in enumerate(tabs[1:], start=1):
        with tab:
            with st.container(key=f"empty_results_{index}"):
                st.subheader(tr("empty_results_title", language))
                st.caption(preview_tr("tab_hint", language))


def render_results(request: BenchmarkRequest, language: Language) -> None:
    tab_labels = tuple(
        tr(key, language) for key in ("overview", "scalability", "cuda_timing", "raw_data")
    )
    frame = st.session_state.results_frame
    results_column, context_column = st.columns((2.36, 1), vertical_alignment="top")
    if frame is None:
        with results_column:
            _render_empty_results(request, language)
        with context_column:
            render_run_context(request, language)
        return

    options = run_options(frame)
    cold_start_measurements = load_cold_start_measurements(
        Path(st.session_state.results_source)
    )
    option_ids = [run_id for run_id, _ in options]
    timestamps = {run_id: timestamp for run_id, timestamp in options}
    preferred = next(
        (run_id for run_id in option_ids if run_id in st.session_state.new_run_ids),
        option_ids[0],
    )
    if (
        st.session_state.get("select_new_run_after_execute")
        or st.session_state.get("selected_run_id") not in option_ids
    ):
        st.session_state.selected_run_id = preferred
        st.session_state.select_new_run_after_execute = False

    with results_column:
        selected_run_id = st.selectbox(
            tr("benchmark_run", language),
            option_ids,
            format_func=lambda run_id: (
                f"{run_id} · {timestamps[run_id].strftime('%Y-%m-%d %H:%M UTC')}"
            ),
            key="selected_run_id",
        )
        selected = select_run(frame, selected_run_id)
        valid = valid_rows(selected)
        invalid_count = len(selected) - len(valid)

        _render_metric_cards(selected, valid, language)
        if invalid_count:
            st.error(tr("invalid_configurations", language, count=invalid_count))
        st.caption(
            f"{tr('read_only_source', language)}: {st.session_state.results_source}"
        )

        tabs = st.tabs(tab_labels)
        with tabs[0]:
            if valid.empty:
                st.warning(tr("no_validated_overview", language))
            else:
                for index, figure in enumerate(overview_charts(valid, language)):
                    st.plotly_chart(figure, width="stretch", key=f"overview_{index}")
        with tabs[1]:
            figure = scalability_chart(valid, language)
            if figure is None:
                st.info(tr("no_openmp_rows", language))
            else:
                st.plotly_chart(figure, width="stretch", key="scalability")
        with tabs[2]:
            figure = cuda_timing_chart(valid, language)
            if figure is None:
                st.info(tr("no_cuda_rows", language))
            else:
                st.plotly_chart(figure, width="stretch", key="cuda_timing")
        with tabs[3]:
            st.dataframe(selected, width="stretch", hide_index=True)
            st.download_button(
                tr("download_selected_run", language),
                selected.to_csv(index=False).encode("utf-8"),
                file_name=f"{selected_run_id}.csv",
                mime="text/csv",
                key="download_selected_run",
            )

    with context_column:
        render_run_context(
            request,
            language,
            selected_run_id,
            RunContextMeta(
                recorded_at=timestamps[selected_run_id],
                result_count=len(selected),
                source=st.session_state.results_source,
                cold_start_cli_duration=(
                    f"{cold_start_measurements[selected_run_id]:.2f} ms"
                    if selected_run_id in cold_start_measurements
                    else None
                ),
            ),
        )
