# Local stability follow-up, October 1, 2026

Work on PR #12, stacked on PR #10. The iPad and Parallels are untouched.
Tests use synthetic data on the M3 Max Mac; no personal module is loaded unless
explicitly identified below. Compilation and mocks do not prove gameplay.
Public releases remain paused. Smooth Motion remains experimental and off by default.

## Save data

- 1.1: reproduced the remove-and-retry path with injected rename failure (regression
  assertion failed before fix). Deleted the fallback. The same ASan/UBSan regression
  passes and reopening the card returns its previous bytes. Registered on Mac and
  native Windows CI. Runtime patch 0115. Real in-game save-failure UI is unverified.
