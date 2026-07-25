from __future__ import annotations

from pathlib import Path

import streamlit as st

from .components import render_matrix_summary, render_saved_status
from .i18n import Language, localize_message, tr
from .models import BenchmarkRequest, RunMode, RunStatus
from .results import ResultsError, load_results
from .runners import DemoRunner, SubprocessRunner
from .sidebar import render_sidebar
from .styles import apply_styles
from .validation import validate_request
from .views import render_results


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEMO_RESULTS = Path(__file__).resolve().parent / "assets" / "demo_results.csv"
MAX_LOG_LINES = 500


def _initialize_state() -> None:
    defaults = {
        "run_status": RunStatus.IDLE.value,
        "run_message": "Configure a benchmark matrix and start a run.",
        "run_logs": [],
        "results_frame": None,
        "results_source": "",
        "results_are_demo": False,
        "new_run_ids": (),
    }
    for key, value in defaults.items():
        if key not in st.session_state:
            st.session_state[key] = value


def _store_failure(message: str, logs: list[str]) -> None:
    st.session_state.run_status = RunStatus.FAILED.value
    st.session_state.run_message = message
    st.session_state.run_logs = logs[-MAX_LOG_LINES:]
    st.session_state.results_frame = None
    st.session_state.results_source = ""
    st.session_state.results_are_demo = False
    st.session_state.new_run_ids = ()


def _execute(request: BenchmarkRequest, language: Language) -> None:
    st.session_state.run_status = RunStatus.VALIDATING.value
    errors = validate_request(request)
    if errors:
        logs = [f"Validation error: {error}" for error in errors]
        _store_failure("Benchmark request validation failed.", logs)
        with st.status(
            tr("request_validation_failed", language), state="error", expanded=True
        ):
            for error in errors:
                st.error(localize_message(error, language))
        return

    logs: list[str] = []
    st.session_state.run_status = RunStatus.RUNNING.value
    status_box = st.status(tr("benchmark_running", language), expanded=True)
    with status_box:
        log_placeholder = st.empty()

    def emit_log(line: str) -> None:
        logs.append(line)
        del logs[:-MAX_LOG_LINES]
        log_placeholder.code("\n".join(logs), language="text")

    runner = (
        DemoRunner(DEMO_RESULTS)
        if request.mode == RunMode.DEMO
        else SubprocessRunner(working_directory=PROJECT_ROOT)
    )
    result = runner.run(request, emit_log)
    st.session_state.run_status = result.status.value
    st.session_state.run_message = result.message
    st.session_state.run_logs = logs[-MAX_LOG_LINES:]
    st.session_state.new_run_ids = result.run_ids
    st.session_state.results_are_demo = request.mode == RunMode.DEMO

    if result.status in {RunStatus.SUCCESS, RunStatus.PARTIAL} and result.csv_path:
        try:
            st.session_state.results_frame = load_results(result.csv_path)
            st.session_state.results_source = str(result.csv_path)
        except ResultsError as error:
            _store_failure(str(error), logs + [str(error)])
            status_box.update(
                label=tr("results_invalid", language), state="error", expanded=True
            )
            return
    else:
        st.session_state.results_frame = None
        st.session_state.results_source = ""

    if result.status == RunStatus.SUCCESS:
        status_box.update(
            label=tr("benchmark_completed", language), state="complete", expanded=False
        )
    elif result.status == RunStatus.PARTIAL:
        status_box.update(
            label=tr("benchmark_partial", language), state="complete", expanded=True
        )
        st.warning(localize_message(result.message, language))
    else:
        status_box.update(
            label=tr("benchmark_failed", language), state="error", expanded=True
        )


def render_language_selector() -> Language:
    selected = st.segmented_control(
        "Language / 语言",
        ("en", "zh"),
        default="en",
        format_func=lambda code: {"en": "EN", "zh": "中"}[code],
        key="ui_language",
        label_visibility="collapsed",
        width="content",
    )
    return "zh" if selected == "zh" else "en"


def render_app() -> None:
    st.set_page_config(
        page_title="ParallelPix Benchmark",
        page_icon=":material/speed:",
        layout="wide",
        initial_sidebar_state="expanded",
    )
    apply_styles()
    _initialize_state()

    with st.container(key="app_header"):
        title_column, mode_column, language_column = st.columns(
            (6, 2.1, 1), vertical_alignment="top"
        )
        with language_column:
            with st.container(key="language_controls"):
                language = render_language_selector()
        with title_column:
            st.title(tr("app_title", language))
            st.caption(tr("app_subtitle", language))

    request, run_clicked = render_sidebar(language)
    with mode_column:
        with st.container(key="mode_controls"):
            if request.mode == RunMode.DEMO:
                st.badge(tr("demo_badge", language), color="orange")
                st.caption(tr("demo_notice", language))
            else:
                st.badge(tr("local_badge", language), color="gray")
                st.caption(tr("local_notice", language))

    if run_clicked:
        _execute(request, language)
    else:
        render_saved_status(language)
    if request.mode == RunMode.LOCAL_CLI and st.session_state.results_are_demo:
        st.warning(tr("displayed_demo_banner", language))
    render_matrix_summary(request, language)
    render_results(request, language)
