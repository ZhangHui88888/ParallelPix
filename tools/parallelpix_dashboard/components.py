from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from html import escape
from pathlib import Path

import streamlit as st

from .i18n import Language, localize_message, tr
from .models import BenchmarkRequest, RunStatus


PROJECT_ROOT = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class RunContextMeta:
    recorded_at: datetime | None = None
    result_count: int | None = None
    source: str | None = None
    duration: str | None = None


def _display_path(path: Path) -> str:
    try:
        return path.relative_to(PROJECT_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def _joined(values: tuple[object, ...]) -> str:
    return " · ".join(map(str, values))


def render_matrix_summary(request: BenchmarkRequest, language: Language) -> None:
    backend_keys = {
        "sequential": "backend_sequential",
        "openmp": "backend_openmp",
        "cuda": "backend_cuda",
    }
    backends = tuple(
        tr(backend_keys[backend], language) for backend in request.normalized_backends
    )
    summaries = (
        (
            "matrix_backends",
            _joined(backends),
            tr("selected_count", language, count=len(backends)),
        ),
        (
            "matrix_image_sets",
            _joined(request.image_counts),
            tr("matrix_counts", language, count=len(request.image_counts)),
        ),
        (
            "matrix_cpu_threads",
            _joined(request.thread_counts) or tr("not_selected", language),
            tr("config_count", language, count=len(request.thread_counts)),
        ),
        (
            "matrix_cuda_batches",
            _joined(request.cuda_batch_sizes) or tr("not_selected", language),
            tr("config_count", language, count=len(request.cuda_batch_sizes)),
        ),
    )
    with st.container(key="matrix_summary"):
        for index, (column, summary) in enumerate(
            zip(st.columns(4), summaries, strict=True)
        ):
            label_key, value, count = summary
            with column:
                with st.container(key=f"matrix_card_{index}"):
                    label_column, count_column = st.columns(
                        (3, 2), vertical_alignment="center"
                    )
                    with label_column:
                        st.markdown(f"**{tr(label_key, language)}**")
                    with count_column:
                        st.badge(count, color="gray")
                    st.markdown(value or tr("not_selected", language))


def render_saved_status(language: Language) -> None:
    status = RunStatus(st.session_state.run_status)
    if status == RunStatus.IDLE:
        with st.container(key="run_status"):
            label, message = st.columns((1, 8), vertical_alignment="center")
            with label:
                st.badge(tr("status_idle", language), color="gray")
            with message:
                st.markdown(tr("idle_message", language))
        return

    state = "error" if status == RunStatus.FAILED else "complete"
    expanded = status in {RunStatus.FAILED, RunStatus.PARTIAL}
    with st.status(
        localize_message(st.session_state.run_message, language),
        state=state,
        expanded=expanded,
    ):
        if st.session_state.run_logs:
            st.code("\n".join(st.session_state.run_logs), language="text")
    if status == RunStatus.PARTIAL:
        st.warning(tr("partial_warning", language))


def _context_row(label: str, value: str, *, code: bool = False) -> None:
    value_class = "context-value context-code" if code else "context-value"
    st.markdown(
        '<div class="context-row">'
        f'<span class="context-label">{escape(label)}</span>'
        f'<span class="{value_class}">{escape(value)}</span>'
        "</div>",
        unsafe_allow_html=True,
    )


def _measurement_formula(
    request: BenchmarkRequest, language: Language
) -> str:
    components: list[str] = []
    configuration_count = 0
    if "sequential" in request.normalized_backends:
        components.append(f"1 {tr('sequential_short', language)}")
        configuration_count += 1
    if "openmp" in request.normalized_backends:
        count = len(request.thread_counts)
        components.append(f"{count} {tr('threads_short', language)}")
        configuration_count += count
    if "cuda" in request.normalized_backends:
        count = len(request.cuda_batch_sizes)
        components.append(f"{count} {tr('batches_short', language)}")
        configuration_count += count
    total = len(request.image_counts) * configuration_count * request.repetitions
    formula = (
        f"{len(request.image_counts)} {tr('image_sets_formula', language)} "
        f"× ({' + '.join(components)}) × "
        f"{request.repetitions} {tr('reps_short', language)} = {total}"
    )
    return formula


def render_run_context(
    request: BenchmarkRequest,
    language: Language,
    run_id: str | None = None,
    metadata: RunContextMeta | None = None,
) -> None:
    metadata = metadata or RunContextMeta()
    mode_key = "mode_demo" if request.mode.value == "demo" else "mode_local_cli"
    items = (
        (tr("mode", language), tr(mode_key, language)),
        (tr("cli_executable", language), _display_path(request.cli_path)),
        (tr("input_directory", language), _display_path(request.input_dir)),
        (tr("output_directory", language), _display_path(request.output_dir)),
        (tr("watermark_image", language), _display_path(request.watermark_path)),
        (tr("result_csv", language), _display_path(request.result_csv)),
    )
    formula = _measurement_formula(request, language)
    matrix_items = (
        (
            tr("backends", language),
            _joined(
                tuple(
                    tr(f"backend_{name}", language)
                    for name in request.normalized_backends
                )
            ),
        ),
        (tr("image_counts", language), ", ".join(map(str, request.image_counts))),
        (tr("openmp_threads", language), ", ".join(map(str, request.thread_counts)) or tr("no_run", language)),
        (
            tr("cuda_batch_sizes", language),
            ", ".join(map(str, request.cuda_batch_sizes)) or tr("no_run", language),
        ),
        (tr("measured_repetitions", language), str(request.repetitions)),
    )
    status = RunStatus(st.session_state.run_status)
    status_items = (
        (
            tr("last_run", language),
            metadata.recorded_at.strftime("%Y-%m-%d %H:%M UTC")
            if metadata.recorded_at
            else tr("no_run", language),
        ),
        (
            tr("status", language),
            tr(f"status_{status.value}", language) if run_id else tr("no_run", language),
        ),
        (tr("duration", language), metadata.duration or tr("no_run", language)),
        (
            tr("result_rows", language),
            str(metadata.result_count)
            if metadata.result_count is not None
            else tr("no_run", language),
        ),
        (
            tr("result_source", language),
            _display_path(Path(metadata.source))
            if metadata.source
            else tr("no_run", language),
        ),
        (tr("run_id", language), run_id or tr("no_run", language)),
    )
    with st.container(key="run_context"):
        st.markdown(f"### {tr('run_context', language)}")
        for label, value in items:
            _context_row(label, value, code=label != tr("mode", language))
        st.divider()
        st.markdown(f"#### {tr('context_experiment_matrix', language)}")
        for label, value in matrix_items:
            _context_row(label, value)
        st.caption(tr("total_measurements", language))
        st.markdown(f"**{formula}**")
        st.divider()
        st.markdown(f"#### {tr('context_run_status', language)}")
        for label, value in status_items:
            _context_row(label, value, code=label in {tr("result_source", language), tr("run_id", language)})
