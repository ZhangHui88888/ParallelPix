from __future__ import annotations

from tools.parallelpix_dashboard import styles


def test_results_context_stacks_below_charts_at_narrow_desktop_widths(
    monkeypatch,
) -> None:
    captured: dict[str, object] = {}

    def capture_markdown(body: str, *, unsafe_allow_html: bool) -> None:
        captured["body"] = body
        captured["unsafe_allow_html"] = unsafe_allow_html

    monkeypatch.setattr(styles.st, "markdown", capture_markdown)

    styles.apply_styles()

    body = str(captured["body"])
    assert ".block-container" in body
    assert "max-width: none" in body
    assert '[data-testid="stSidebar"]' in body
    assert "width: 17rem" in body
    assert '[data-testid="stDeployButton"]' in body
    assert "display: none" in body
    assert ".st-key-matrix_card_0" in body
    assert "@media (max-width: 1280px)" in body
    assert ':has(.st-key-run_context) > [data-testid="stColumn"]' in body
    assert captured["unsafe_allow_html"] is True
