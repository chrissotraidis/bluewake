# PRD — The direct-call gate and FP checks on Windows

[Progress](PROGRESS.md) · [Technical plan](TECH_PLAN.md) · Builds on [#552](https://github.com/chrissotraidis/bluewake/pull/552) and [#553](https://github.com/chrissotraidis/bluewake/pull/553)

## Why

Two speed changes reach only the Linux build today:

- #552's direct-call gate: Steam Deck +5.5%.
- #553's fp_checks pass: Steam Deck +3.05%, desktop +3.2%.

Their code is shared: the composite headers, `main.c` and `scripts/windows/fp_checks.py`. Only `scripts/linux/build.py` knows the options, so a Windows build never prepares fp_checks, and it can't train with the gate open.

## What changes for players

- **Windows builds prepare fp_checks by default**, as Linux builds do.
- **The gate is on by default**, as on Linux: the module asks the app to open it. `BLUEWAKE_DIRECT_GATE=0` turns it off for an A/B.

## Not in scope

- macOS: its builder is a separate script with its own option set. It comes next, after a Windows measurement.
- Measuring on Windows before the defaults ship. That is M2 in the progress file.
- #554's huge pages, which are Linux-only by nature.

## Success

- Windows CI builds and its focused tests pass.
- A Windows A/B, at least six interleaved launches with checkpoints identical, shows the gain is real there. **Not yet measured.**
