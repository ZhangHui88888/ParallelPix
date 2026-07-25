from __future__ import annotations

from pathlib import Path

import streamlit as st

from .i18n import Language, tr
from .models import BenchmarkRequest, RunMode


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def _project_path(value: str) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else PROJECT_ROOT / path


def _section_label(key: str, language: Language) -> None:
    st.sidebar.markdown(
        f'<div class="sidebar-section-label">{tr(key, language)}</div>',
        unsafe_allow_html=True,
    )


def render_sidebar(language: Language) -> tuple[BenchmarkRequest, bool]:
    _section_label("configuration", language)
    mode_label = st.sidebar.selectbox(
        tr("run_mode", language),
        ("Demo", "Local CLI"),
        key="run_mode",
        format_func=lambda value: tr(
            "mode_demo" if value == "Demo" else "mode_local_cli", language
        ),
        help=tr("run_mode_help", language),
    )
    mode = RunMode.DEMO if mode_label == "Demo" else RunMode.LOCAL_CLI

    cli_path = st.sidebar.text_input(
        tr("cli_executable", language),
        value="build/Release/parallelpix.exe",
        disabled=mode == RunMode.DEMO,
        key="cli_path",
    )
    input_dir = st.sidebar.text_input(
        tr("input_directory", language), value="data/images", key="input_dir"
    )
    output_dir = st.sidebar.text_input(
        tr("output_directory", language), value="output", key="output_dir"
    )
    watermark = st.sidebar.text_input(
        tr("watermark_image", language),
        value="data/watermark.png",
        key="watermark_path",
    )
    result_csv = st.sidebar.text_input(
        tr("result_csv", language), value="results/benchmark.csv", key="result_csv"
    )

    _section_label("experiment_matrix", language)
    legacy_backends = tuple(
        st.session_state.get("backends", ("Sequential", "OpenMP", "CUDA"))
    )
    backend_defaults = {
        "backend_sequential": "Sequential" in legacy_backends,
        "backend_openmp": "OpenMP" in legacy_backends,
        "backend_cuda": "CUDA" in legacy_backends,
    }
    for key, default in backend_defaults.items():
        if key not in st.session_state:
            st.session_state[key] = default

    parallel_selected = bool(
        st.session_state.backend_openmp or st.session_state.backend_cuda
    )
    if parallel_selected:
        st.session_state.backend_sequential = True

    st.sidebar.caption(tr("backends", language))
    sequential = st.sidebar.checkbox(
        "Sequential",
        key="backend_sequential",
        disabled=parallel_selected,
    )
    openmp = st.sidebar.checkbox("OpenMP", key="backend_openmp")
    cuda = st.sidebar.checkbox("CUDA", key="backend_cuda")
    backend_labels = tuple(
        label
        for label, selected in (
            ("Sequential", sequential),
            ("OpenMP", openmp),
            ("CUDA", cuda),
        )
        if selected
    )
    st.session_state.backends = backend_labels
    backends = tuple(label.lower() for label in backend_labels)
    if "sequential" not in backends and set(backends).intersection({"openmp", "cuda"}):
        st.sidebar.info(tr("baseline_info", language))

    image_counts = tuple(
        st.sidebar.multiselect(
            tr("image_counts", language),
            (10, 50, 100, 250, 500),
            default=(10, 50, 100),
            key="image_counts",
        )
    )
    thread_counts: tuple[int, ...] = ()
    if "openmp" in backends:
        thread_counts = tuple(
            st.sidebar.multiselect(
                tr("openmp_threads", language),
                (1, 2, 4, 8, 16, 32),
                default=(1, 2, 4, 8),
                key="thread_counts",
            )
        )
    cuda_batch_sizes: tuple[int, ...] = ()
    if "cuda" in backends:
        cuda_batch_sizes = tuple(
            st.sidebar.multiselect(
                tr("cuda_batch_sizes", language),
                (1, 2, 4, 8, 16),
                default=(1, 4, 8),
                key="cuda_batch_sizes",
            )
        )

    repetitions = int(
        st.sidebar.number_input(
            tr("measured_repetitions", language),
            min_value=5,
            max_value=30,
            value=5,
            step=1,
            key="repetitions",
        )
    )
    st.sidebar.caption(tr("warmup_caption", language))
    run_clicked = st.sidebar.button(
        tr("run_benchmark", language),
        type="primary",
        icon=":material/play_arrow:",
        width="stretch",
        key="run_benchmark",
    )

    request = BenchmarkRequest(
        mode=mode,
        cli_path=_project_path(cli_path),
        input_dir=_project_path(input_dir),
        output_dir=_project_path(output_dir),
        watermark_path=_project_path(watermark),
        result_csv=_project_path(result_csv),
        backends=backends,
        image_counts=image_counts,
        thread_counts=thread_counts,
        cuda_batch_sizes=cuda_batch_sizes,
        repetitions=repetitions,
    )
    return request, run_clicked
