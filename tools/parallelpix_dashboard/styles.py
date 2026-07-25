from __future__ import annotations

import streamlit as st


def apply_styles() -> None:
    """Apply the data-first dark theme without changing Streamlit behavior."""
    st.markdown(
        """
        <style>
        :root {
            --pp-bg: #08111f;
            --pp-sidebar: #0d1726;
            --pp-surface: #101c2c;
            --pp-surface-soft: #132238;
            --pp-border: #243247;
            --pp-text: #f1f5f9;
            --pp-muted: #94a3b8;
            --pp-accent: #2dd4bf;
        }

        [data-testid="stAppViewContainer"] {
            background: var(--pp-bg);
        }

        [data-testid="stHeader"] {
            background: transparent;
            pointer-events: none;
        }

        [data-testid="stHeader"] button {
            pointer-events: auto;
        }

        [data-testid="stToolbar"] {
            pointer-events: none;
        }

        [data-testid="stToolbar"] button {
            pointer-events: auto;
        }

        [data-testid="stSidebar"] {
            background: var(--pp-sidebar);
            border-right: 1px solid var(--pp-border);
            flex-basis: 17rem;
            max-width: 17rem;
            min-width: 17rem;
            width: 17rem;
        }

        [data-testid="stSidebar"] > div:first-child {
            width: 17rem;
        }

        [data-testid="stSidebarUserContent"] {
            margin-top: -4.7rem;
            padding-top: 1rem;
        }

        [data-testid="stSidebar"] [data-testid="stVerticalBlock"] {
            gap: 0.5rem;
        }

        .block-container {
            max-width: none;
            padding: 1.2rem 1.25rem 2.5rem 1.5rem;
        }

        [data-testid="stMainBlockContainer"] >
        [data-testid="stVerticalBlock"] {
            gap: 0.625rem;
        }

        h1 {
            color: var(--pp-text);
            font-size: 1.75rem !important;
            letter-spacing: -0.035em;
            line-height: 1.05 !important;
            margin-bottom: 0.25rem !important;
        }

        h2, h3 {
            color: var(--pp-text);
            letter-spacing: -0.02em;
        }

        p, label, [data-testid="stCaptionContainer"] {
            color: var(--pp-muted);
        }

        .sidebar-section-label {
            color: var(--pp-accent);
            font-size: 0.72rem;
            font-weight: 750;
            letter-spacing: 0.09em;
            margin: 0.45rem 0 0.1rem;
            text-transform: uppercase;
        }

        .st-key-app_header [data-testid="stHorizontalBlock"] {
            align-items: flex-start;
        }

        .st-key-app_header [data-testid="stVerticalBlock"] {
            gap: 0.3rem;
        }

        .st-key-app_header {
            margin-bottom: -0.25rem;
        }

        .st-key-app_header h1 {
            margin: 0 !important;
            padding: 0 !important;
        }

        .st-key-app_header [data-testid="stElementContainer"]:has(h1) {
            min-height: 1.85rem;
        }

        .st-key-app_header [data-testid="stCaptionContainer"],
        .st-key-app_header [data-testid="stCaptionContainer"] p {
            height: auto !important;
            margin: 0 !important;
        }

        .st-key-language_controls [data-testid="stVerticalBlock"],
        .st-key-mode_controls [data-testid="stVerticalBlock"] {
            align-items: flex-end;
            gap: 0.28rem;
        }

        .st-key-language_controls {
            transform: translateX(-5rem);
        }

        .st-key-run_status,
        .st-key-run_context,
        .st-key-preview_workspace,
        [class*="st-key-empty_results_"] {
            border: 1px solid var(--pp-border);
            border-radius: 8px;
            background: color-mix(in srgb, var(--pp-surface) 88%, transparent);
        }

        .st-key-run_status {
            padding: 0.7rem 0.9rem;
            margin: 0;
        }

        .st-key-matrix_summary {
            margin: -0.2rem 0 0.25rem;
        }

        .st-key-matrix_card_0,
        .st-key-matrix_card_1,
        .st-key-matrix_card_2,
        .st-key-matrix_card_3 {
            min-height: 4.4rem;
            padding: 0.65rem 1rem 0.35rem;
            border: 1px solid var(--pp-border);
            border-radius: 8px;
            background: color-mix(in srgb, var(--pp-surface) 88%, transparent);
        }

        [class*="st-key-matrix_card_"] [data-testid="stVerticalBlock"] {
            gap: 0.35rem;
        }

        [class*="st-key-matrix_card_"] p {
            margin-bottom: 0.25rem;
        }

        .st-key-preview_workspace {
            min-height: 44.5rem;
            padding: 0.9rem 1rem 0.85rem;
        }

        .st-key-preview_workspace h3 {
            font-size: 1rem !important;
            line-height: 1.3 !important;
            margin-top: 0 !important;
            padding: 0 !important;
        }

        .st-key-preview_workspace [data-testid="stPlotlyChart"] {
            border-radius: 6px;
            overflow: hidden;
        }

        .st-key-preview_workspace >
        [data-testid="stLayoutWrapper"]:nth-child(2) {
            transform: translateY(-1.6rem);
        }

        .st-key-preview_workspace >
        [data-testid="stLayoutWrapper"]:nth-child(3) {
            transform: translateY(-0.9rem);
        }

        .st-key-preview_hint {
            min-height: 5rem;
            padding: 0.55rem 1rem;
            border: 1px dashed #3a4b62;
            border-radius: 6px;
            background: color-mix(in srgb, var(--pp-bg) 70%, transparent);
            transform: translateY(1.1rem);
        }

        [class*="st-key-empty_results_"] {
            min-height: 15rem;
            padding: 4.5rem 1.5rem;
            text-align: center;
        }

        .st-key-run_context {
            margin-top: 3.5rem;
            min-height: 44.5rem;
            padding: 1rem 1rem 0.8rem;
        }

        .st-key-run_context [data-testid="stVerticalBlock"] {
            gap: 0.35rem;
        }

        .st-key-run_context h3 {
            font-size: 1.25rem !important;
            margin: 0 0 0.5rem !important;
            padding: 0 !important;
        }

        .st-key-run_context h4 {
            font-size: 0.92rem !important;
            margin: 0.25rem 0 0.35rem !important;
            padding: 0 !important;
        }

        .st-key-run_context hr {
            margin: 0.75rem 0;
        }

        .context-row {
            display: grid;
            grid-template-columns: minmax(6.5rem, 0.9fr) minmax(0, 1.25fr);
            gap: 0.65rem;
            align-items: start;
            min-height: 1.55rem;
            color: var(--pp-muted);
            font-size: 0.77rem;
            line-height: 1.35;
        }

        .context-value {
            color: #aebbd0;
            text-align: right;
            overflow-wrap: anywhere;
        }

        .context-code {
            color: #4ade80;
            font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
            font-size: 0.72rem;
        }

        [data-testid="stHorizontalBlock"]:has(.st-key-run_context) {
            gap: 0.625rem;
        }

        [data-testid="stMetric"] {
            background: var(--pp-surface);
            border: 1px solid var(--pp-border);
            border-radius: 8px;
            padding: 0.85rem 1rem;
        }

        [data-testid="stMetricLabel"] {
            color: var(--pp-muted);
        }

        [data-testid="stMetricValue"] {
            color: var(--pp-text);
            font-variant-numeric: tabular-nums;
        }

        .stTabs [data-baseweb="tab-list"] {
            border-bottom: 1px solid var(--pp-border);
            gap: 1.4rem;
        }

        .stTabs [data-baseweb="tab"] {
            padding-left: 0;
            padding-right: 0;
        }

        .stTabs [aria-selected="true"] {
            color: var(--pp-accent);
        }

        div[data-baseweb="input"],
        div[data-baseweb="select"] > div,
        [data-testid="stNumberInputContainer"] {
            background: color-mix(in srgb, var(--pp-bg) 72%, var(--pp-surface));
            border-color: var(--pp-border);
            border-radius: 7px;
        }

        button:focus-visible,
        input:focus-visible,
        [role="tab"]:focus-visible {
            outline: 2px solid var(--pp-accent) !important;
            outline-offset: 2px;
        }

        [data-testid="stAlert"] {
            border-radius: 8px;
        }

        @media (max-width: 1280px) {
            [data-testid="stHorizontalBlock"]:has(.st-key-run_context) {
                flex-wrap: wrap;
            }

            [data-testid="stHorizontalBlock"]:has(.st-key-run_context) > [data-testid="stColumn"] {
                flex: 1 1 100% !important;
                width: 100% !important;
            }

            .st-key-run_context {
                margin-top: 0.35rem;
            }
        }

        @media (max-width: 900px) {
            .block-container {
                padding-left: 1rem;
                padding-right: 1rem;
            }

            .st-key-matrix_summary > [data-testid="stVerticalBlock"] >
            [data-testid="stHorizontalBlock"] {
                flex-wrap: wrap;
            }
        }
        </style>
        """,
        unsafe_allow_html=True,
    )
