from pathlib import Path

from tools.parallelpix_dashboard.trajectory import (
    load_trajectory_samples,
    parse_progress_event,
    save_trajectory_samples,
    trajectory_chart,
)


def test_live_trajectory_uses_processed_images_and_keeps_a_following_window() -> None:
    first = parse_progress_event(
        "[INFO][trajectory] backend=sequential run_id=run-1 thread_count=0 cuda_batch_size=0 image_count=1000 processed=10 ms_per_image=12.5"
    )
    last = parse_progress_event(
        "[INFO][trajectory] backend=sequential run_id=run-1 thread_count=0 cuda_batch_size=0 image_count=1000 processed=250 ms_per_image=11.5"
    )

    assert first is not None
    assert last is not None
    figure = trajectory_chart([first, last], "en", window=200)

    assert figure is not None
    assert list(figure.data[0].x) == [10, 250]
    assert list(figure.layout.xaxis.range) == [50, 250]


def test_trajectory_is_saved_by_run_id(tmp_path: Path) -> None:
    sample = parse_progress_event(
        "[INFO][trajectory] backend=sequential run_id=run-1 thread_count=0 cuda_batch_size=0 image_count=10 processed=10 ms_per_image=12.5"
    )
    assert sample is not None

    result_csv = tmp_path / "benchmark.csv"
    save_trajectory_samples(result_csv, [sample])

    assert load_trajectory_samples(result_csv, "run-1") == [sample]
    assert load_trajectory_samples(result_csv, "other-run") == []
