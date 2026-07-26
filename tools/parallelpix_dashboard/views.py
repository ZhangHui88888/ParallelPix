from __future__ import annotations

from pathlib import Path

import pandas as pd
import streamlit as st

from .charts import (
    cuda_timing_chart,
    final_median_chart,
    final_speedup_chart,
    final_total_duration_chart,
    final_throughput_chart,
    overview_charts,
    overview_focus_chart,
    scalability_chart,
)
from .cold_start import load_cold_start_measurements as load_cold_start_baselines
from .components import RunContextMeta, render_run_context
from .i18n import Language, tr
from .models import BenchmarkRequest
from .preview import preview_tr, render_preview_workspace
from .lifecycle import load_cold_start_measurements
from .results import run_options, select_run, valid_rows
from .trajectory import load_trajectory_samples, trajectory_chart


def _metric_value(series: pd.Series, strategy: str, suffix: str) -> str:
    values = series.dropna()
    if values.empty:
        return "N/A"
    value = values.min() if strategy == "min" else values.max()
    return f"{value:.2f}{suffix}"


def _render_metric_cards(
    selected: pd.DataFrame, valid: pd.DataFrame, language: Language
) -> None:
    has_cold_start = "cold_start_cli_ms" in valid and valid["cold_start_cli_ms"].notna().any()
    columns = st.columns(5 if has_cold_start else 4)
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
    if has_cold_start:
        columns[4].metric(
            tr("cold_start_baseline_ms", language),
            _metric_value(valid["cold_start_cli_ms"], "min", " ms"),
        )


def _attach_cold_start_measurements(selected: pd.DataFrame, result_csv: Path) -> pd.DataFrame:
    probes = load_cold_start_baselines(result_csv)
    if probes.empty:
        selected["cold_start_cli_ms"] = pd.NA
        return selected
    probes = probes.loc[probes["run_id"].eq(str(selected["run_id"].iloc[0]))].copy()
    if probes.empty:
        selected["cold_start_cli_ms"] = pd.NA
        return selected

    key_columns = ["backend", "thread_count", "cuda_batch_size", "image_count"]
    left = selected.copy()
    right = probes[key_columns + ["cold_start_cli_ms"]].copy()
    for column in ("thread_count", "cuda_batch_size"):
        left[column] = left[column].fillna(-1)
        right[column] = right[column].fillna(-1)
    merged = left.merge(right, on=key_columns, how="left")
    for column in ("thread_count", "cuda_batch_size"):
        merged[column] = merged[column].replace(-1, pd.NA)
    return merged


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


def _render_overview_grid(
    valid: pd.DataFrame, language: Language, run_id: str
) -> None:
    image_counts = valid["image_count"].drop_duplicates().tolist()
    with st.container(key="measured_overview_workspace"):
        image_count = st.selectbox(
            tr("overview_image_count", language),
            image_counts,
            format_func=lambda count: f"{count} {tr('images', language)}",
            key=f"overview_image_count_{run_id}",
        )
        figures = (
            *overview_charts(
                valid, language, compact=True, image_count=image_count
            )[:3],
            overview_focus_chart(
                valid, language, compact=True, image_count=image_count
            ),
        )
        for row_start in (0, 2):
            for column, figure, index in zip(
                st.columns(2), figures[row_start : row_start + 2], range(row_start, row_start + 2), strict=True
            ):
                with column:
                    if figure is not None:
                        st.plotly_chart(
                            figure,
                            width="stretch",
                            key=f"overview_{index}",
                            config={"displayModeBar": False},
                        )
                    else:
                        with st.container(key="overview_empty_focus"):
                            st.info(tr("no_parallel_overview", language))


def _render_final_comparison(valid: pd.DataFrame, language: Language) -> None:
    data = valid.copy()
    data["configuration"] = data.apply(
        lambda row: (
            "Sequential"
            if row["backend"] == "sequential"
            else f"OpenMP · {int(row['thread_count'])}"
            if row["backend"] == "openmp"
            else f"CUDA · {int(row['cuda_batch_size'])}"
        ),
        axis=1,
    )
    data[tr("median_ms_per_image", language)] = data["compute_ms"] / data["image_count"]
    data[tr("end_to_end_seconds", language)] = data["end_to_end_ms"] / 1_000.0
    st.subheader(tr("final_comparison", language))
    image_count = int(data["image_count"].iloc[0])
    st.caption(tr("final_average_context", language, count=image_count))
    total_column, median_column = st.columns(2)
    with total_column:
        st.plotly_chart(final_total_duration_chart(valid, language), width="stretch", key="final_total_duration_comparison", config={"displayModeBar": False})
    with median_column:
        st.plotly_chart(final_median_chart(valid, language), width="stretch", key="final_median_comparison", config={"displayModeBar": False})
    speedup_column, throughput_column = st.columns(2)
    with speedup_column:
        st.plotly_chart(final_speedup_chart(valid, language), width="stretch", key="final_speedup_comparison", config={"displayModeBar": False})
    with throughput_column:
        st.plotly_chart(final_throughput_chart(valid, language), width="stretch", key="final_throughput_comparison", config={"displayModeBar": False})
    columns = [
        "configuration",
        tr("median_ms_per_image", language),
        tr("end_to_end_seconds", language),
        "images_per_second",
        "speedup",
    ]
    if "cold_start_cli_ms" in data and data["cold_start_cli_ms"].notna().any():
        columns.append("cold_start_cli_ms")
    st.dataframe(
        data[columns].rename(
            columns={
                "configuration": tr("configuration", language),
                "images_per_second": tr("images_per_second", language),
                "speedup": tr("speedup", language),
                "cold_start_cli_ms": tr("cold_start_cli_ms", language),
            }
        ),
        width="stretch",
        hide_index=True,
    )


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
        selected = _attach_cold_start_measurements(
            select_run(frame, selected_run_id),
            Path(st.session_state.results_source),
        )
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
                trajectory_samples = load_trajectory_samples(
                    Path(st.session_state.results_source), selected_run_id
                )
                trajectory = trajectory_chart(trajectory_samples, language)
                if trajectory is not None:
                    st.plotly_chart(
                        trajectory,
                        width="stretch",
                        key="completed_trajectory",
                        config={"displayModeBar": False},
                    )
                else:
                    st.info(tr("trajectory_not_available", language))
                _render_final_comparison(valid, language)
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
