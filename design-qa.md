# M1 Dashboard Design QA

## Source and matched state

- Visual source of truth: `docs/design/assets/m1-data-first-lab-bench.png`
- Reference viewport: 1487×1058, normalized by one horizontal pixel to 1488×1058.
- Implementation: `docs/test/screenshots/m1-implementation-idle-en-1488x1058.png`
- Same-state comparison: `docs/test/screenshots/m1-design-qa-idle-comparison-1488x1058.png`
- Responsive captures:
  - `docs/test/screenshots/m1-implementation-idle-en-2048x1024.png`
  - `docs/test/screenshots/m1-implementation-idle-en-1280x720.png`
- Chinese capture: `docs/test/screenshots/m1-implementation-idle-zh-1488x1058.png`
- Compared state: default Demo / Idle before any benchmark result is loaded.

The earlier populated-result comparison was not a valid fidelity gate for the
reference Idle screen. This report supersedes that conclusion and uses the same
viewport and interaction state on both sides.

## Fidelity assessment

- Layout: the 272px sidebar, 24px main inset, compact header, status row, four
  matrix cards, tab line, 2×2 preview workspace, and right context panel align
  with the reference geometry at 1488×1058.
- Preview semantics: the sample charts are visibly labeled `preview` and
  `not measured`; Idle keeps `results_frame` empty and exposes no real metrics,
  run history, run ID, or download.
- Density: the preview and context panels share a 712px height; the context
  includes paths, experiment matrix, a mathematically correct measurement
  formula, and an Idle run-status section.
- Wide and narrow behavior: the 2048px view uses the full available width.
  At 1280px the context stacks below the charts. Neither view has horizontal
  overflow.
- Interaction: EN/中 is visible and clickable despite Streamlit's toolbar
  overlay. The backend checkboxes enforce the Sequential baseline, the Demo
  action replaces Preview with measured-result UI, all four tabs work, and Raw
  Data exposes the table and CSV download.
- Localization: Preview titles, chart labels, matrix cards, context, and status
  copy switch to Chinese while `Sequential`, `OpenMP`, `CUDA`, paths, run IDs,
  and CSV field names remain unchanged.

## Findings and resolution

1. P1 · State mismatch: the previous QA compared a populated Demo result with
   an Idle reference.
   - Fix: implement a dedicated, isolated Idle Preview and compare the same
     state at 1488×1058.
   - Result: resolved.
2. P2 · Width and density: the main area was capped at 1600px and the context
   column was too narrow at the user's 2048×1024 viewport.
   - Fix: remove the max-width cap, fix the sidebar at 272px, and rebalance the
     result/context columns.
   - Result: resolved.
3. P2 · Header interaction: the right-side language control sat underneath the
   Streamlit toolbar hit area.
   - Fix: reserve toolbar space and allow pointer events to reach EN/中 while
     preserving Deploy and menu button interaction.
   - Result: resolved.
4. P2 · Context height: stacked caption/value widgets pushed Run status below
   the target viewport.
   - Fix: use compact two-column context rows and reduce heading/divider
     spacing.
   - Result: resolved.

P0 findings: none.
P1 findings: none.
Open P2 findings: none.

## Verification

- AppTest and unit suite: 26 passed.
- Browser viewport checks: 1488×1058, 2048×1024, and 1280×720 passed.
- Browser interaction checks: EN/中, Demo run, Raw Data, and download passed.
- Horizontal overflow: 0px at all three checked viewports.
- Browser console: no application errors; WebSocket close warnings occurred
  only during intentional local server restarts.
- Python compile check: passed.
- `git diff --check`: passed; line-ending notices only.

final result: passed
