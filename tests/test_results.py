from __future__ import annotations

from pathlib import Path

import pandas as pd
import pytest

from tools.parallelpix_dashboard.results import (
    ResultsError,
    load_results,
    run_options,
    select_run,
    valid_rows,
)


DEMO_RESULTS = (
    Path(__file__).resolve().parents[1]
    / "tools"
    / "parallelpix_dashboard"
    / "assets"
    / "demo_results.csv"
)


def test_demo_results_preserve_history_and_exclude_invalid_row() -> None:
    frame = load_results(DEMO_RESULTS)

    options = run_options(frame)
    latest = select_run(frame, options[0][0])
    valid = valid_rows(latest)

    assert [run_id for run_id, _ in options] == [
        "demo-20260725-001",
        "demo-20260724-001",
    ]
    assert len(latest) == 14
    assert len(valid) == 13
    assert not valid["validation_passed"].eq(False).any()


def test_missing_required_column_blocks_results(tmp_path: Path) -> None:
    path = tmp_path / "missing.csv"
    pd.DataFrame([{"run_id": "broken"}]).to_csv(path, index=False)

    with pytest.raises(ResultsError, match="Missing required CSV columns"):
        load_results(path)


def test_empty_csv_blocks_results(tmp_path: Path) -> None:
    path = tmp_path / "empty.csv"
    path.write_text("", encoding="utf-8")

    with pytest.raises(ResultsError, match="empty"):
        load_results(path)
