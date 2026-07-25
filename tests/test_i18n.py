from __future__ import annotations

from tools.parallelpix_dashboard.i18n import localize_message, tr


def test_translation_formats_values_and_keeps_technical_names() -> None:
    assert tr("matrix_counts", "en", count=3) == "3 sets"
    assert tr("matrix_counts", "zh", count=3) == "3 组"
    assert tr("backend_openmp", "zh") == "OpenMP"


def test_dynamic_application_message_is_localized() -> None:
    assert (
        localize_message("Benchmark completed successfully.", "zh")
        == "基准测试已成功完成。"
    )
    assert (
        localize_message("CLI executable not found: C:/parallelpix.exe", "zh")
        == "未找到 CLI 可执行文件：C:/parallelpix.exe"
    )


def test_unknown_runtime_message_is_preserved() -> None:
    assert localize_message("raw CLI output", "zh") == "raw CLI output"
