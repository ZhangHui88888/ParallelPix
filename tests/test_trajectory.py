from pathlib import Path

from tools.parallelpix_dashboard.trajectory import (
    load_trajectory_samples,
    parse_progress_event,
    save_trajectory_samples,
    trajectory_chart,
)


def test_processing_trajectory_uses_processed_images_and_keeps_a_following_window() -> None:
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


def test_processing_trajectory_resets_window_for_the_next_configuration() -> None:
    completed = parse_progress_event(
        "[INFO][trajectory] backend=cuda run_id=run-1 thread_count=0 cuda_batch_size=8 image_count=1000 processed=1000 ms_per_image=4.0"
    )
    next_configuration = parse_progress_event(
        "[INFO][trajectory] backend=cuda run_id=run-1 thread_count=0 cuda_batch_size=64 image_count=1000 processed=64 ms_per_image=3.0"
    )

    assert completed is not None
    assert next_configuration is not None
    figure = trajectory_chart([completed, next_configuration], "en", window=200)

    assert figure is not None
    assert list(figure.layout.xaxis.range) == [0, 64]
    assert figure.layout.uirevision == "trajectory-run-1-CUDA · batch 64"


def test_completed_trajectory_is_not_labelled_as_live() -> None:
    sample = parse_progress_event(
        "[INFO][trajectory] backend=cuda run_id=run-1 thread_count=0 cuda_batch_size=8 image_count=100 processed=8 ms_per_image=3.0"
    )
    assert sample is not None

    figure = trajectory_chart([sample], "en")

    assert figure is not None
    assert figure.layout.title.text == "Processing trajectory"


def test_trajectory_is_saved_by_run_id(tmp_path: Path) -> None:
    sample = parse_progress_event(
        "[INFO][trajectory] backend=sequential run_id=run-1 thread_count=0 cuda_batch_size=0 image_count=10 processed=10 ms_per_image=12.5"
    )
    assert sample is not None

    result_csv = tmp_path / "benchmark.csv"
    save_trajectory_samples(result_csv, [sample])

    assert load_trajectory_samples(result_csv, "run-1") == [sample]
    assert load_trajectory_samples(result_csv, "other-run") == []
