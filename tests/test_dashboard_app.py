from __future__ import annotations

import shutil
from pathlib import Path

from streamlit.testing.v1 import AppTest


DASHBOARD = Path(__file__).resolve().parents[1] / "tools" / "dashboard.py"
DEMO_RESULTS = (
    Path(__file__).resolve().parents[1]
    / "tools"
    / "parallelpix_dashboard"
    / "assets"
    / "demo_results.csv"
)


def test_default_dashboard_renders_demo_console() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    assert not app.exception
    assert app.title[0].value == "ParallelPix"
    assert [tab.label for tab in app.tabs] == [
        "Overview",
        "Scalability",
        "CUDA Timing",
        "Raw Data",
    ]
    assert not app.warning
    assert any(
        caption.value == "Results are for demonstration only."
        for caption in app.caption
    )


def test_matrix_values_are_free_text_inputs() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    assert app.text_input(key="image_counts_input").value == "10, 50, 100"
    app.text_input(key="image_counts_input").set_value("1, 7").run()

    assert not app.exception
    assert app.text_input(key="image_counts_input").value == "1, 7"


def test_demo_run_loads_metrics_and_history() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    app.button(key="run_benchmark").click().run(timeout=10)

    assert not app.exception
    assert app.session_state["run_status"] == "success"
    assert app.session_state["results_are_demo"] is True
    assert len(app.metric) == 4
    assert app.selectbox(key="selected_run_id").value == "demo-20260725-001"


def test_new_run_reselects_the_latest_run_instead_of_stale_history() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()
    app.button(key="run_benchmark").click().run(timeout=10)
    app.selectbox(key="selected_run_id").set_value("demo-20260724-001").run()

    app.button(key="run_benchmark").click().run(timeout=10)

    assert app.selectbox(key="selected_run_id").value == "demo-20260725-001"


def test_local_cli_mode_reports_missing_paths_without_crashing() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()
    app.selectbox(key="run_mode").set_value("Local CLI").run()
    app.text_input(key="cli_path").set_value("missing/parallelpix.exe").run()

    app.button(key="run_benchmark").click().run(timeout=10)

    assert not app.exception
    assert app.session_state["run_status"] == "failed"
    assert app.session_state["results_are_demo"] is False
    assert any("CLI executable not found" in error.value for error in app.error)
    assert not any("Demo data" in warning.value for warning in app.warning)


def test_local_cli_restores_existing_csv_history_after_refresh(tmp_path: Path) -> None:
    result_csv = tmp_path / "history.csv"
    shutil.copyfile(DEMO_RESULTS, result_csv)
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    app.text_input(key="result_csv").set_value(str(result_csv)).run()
    app.selectbox(key="run_mode").set_value("Local CLI").run()

    assert not app.exception
    assert app.session_state["results_are_demo"] is False
    assert app.session_state["results_frame"] is not None
    assert app.selectbox(key="selected_run_id").value == "demo-20260725-001"


def test_language_switch_updates_streamlit_owned_copy() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    assert app.segmented_control(key="ui_language").value == "en"
    assert app.selectbox(key="run_mode").label == "Run mode"

    app.segmented_control(key="ui_language").set_value("zh").run()

    assert not app.exception
    assert app.session_state["ui_language"] == "zh"
    assert app.selectbox(key="run_mode").label == "运行模式"
    assert [tab.label for tab in app.tabs] == ["概览", "可扩展性", "CUDA 时序", "原始数据"]


def test_default_page_exposes_matrix_summary_without_results() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    assert app.session_state["results_frame"] is None
    assert len(app.metric) == 0
    assert len(app.get("plotly_chart")) == 4
    markdown = "\n".join(element.value for element in app.markdown)
    captions = "\n".join(element.value for element in app.caption)
    assert "Performance overview" in markdown
    assert "Preview data — not measured" in captions
    assert "Backends" in markdown
    assert "Image sets" in markdown
    assert "CPU threads" in markdown
    assert "CUDA batch sizes" in markdown
    assert "Sequential · OpenMP · CUDA" in markdown
    assert "Experiment matrix" in markdown
    assert "Total measurements" in captions
    assert "3 image sets × (1 sequential + 4 threads + 3 batches) × 5 reps" in markdown
    assert "120" in markdown
    assert "Run status" in markdown


def test_backend_checkboxes_keep_sequential_as_parallel_baseline() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    assert app.checkbox(key="backend_sequential").value is True
    assert app.checkbox(key="backend_openmp").value is True
    assert app.checkbox(key="backend_cuda").value is True

    app.checkbox(key="backend_openmp").set_value(False).run()
    app.checkbox(key="backend_cuda").set_value(False).run()
    app.checkbox(key="backend_sequential").set_value(False).run()
    assert app.session_state["backends"] == ()

    app.checkbox(key="backend_openmp").set_value(True).run()
    assert app.session_state["backend_sequential"] is True
    assert app.session_state["backends"] == ("Sequential", "OpenMP")


def test_chinese_idle_preview_is_localized() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    app.segmented_control(key="ui_language").set_value("zh").run()

    markdown = "\n".join(element.value for element in app.markdown)
    captions = "\n".join(element.value for element in app.caption)
    assert "性能概览" in markdown
    assert "预览数据——未经测量" in captions
    assert "Sequential · OpenMP · CUDA" in markdown


def test_demo_results_keep_four_metrics_and_show_run_context() -> None:
    app = AppTest.from_file(str(DASHBOARD), default_timeout=10).run()

    app.button(key="run_benchmark").click().run(timeout=10)

    assert not app.exception
    assert len(app.metric) == 4
    assert app.metric[0].label == "Validated configs"
    markdown = "\n".join(element.value for element in app.markdown)
    assert "Run context" in markdown
    assert "results/benchmark.csv" in markdown
    assert "E:/projects/learning/ParallelPix/results/benchmark.csv" not in markdown
    assert "demo-20260725-001" in markdown
    assert "Performance overview" not in markdown
    assert "Run status" in markdown
    assert "Result rows" in markdown
