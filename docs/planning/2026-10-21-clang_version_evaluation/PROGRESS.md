# PROGRESS — Clang version evaluation

Status: Preliminary build-path evidence recorded; qualification remains blocked
by unmerged [#446](https://github.com/chrissotraidis/bluewake/pull/446),
[#433](https://github.com/chrissotraidis/bluewake/issues/433), and the rendered
and Game Mode Deck stages.

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) ·
[Parent M5](https://github.com/chrissotraidis/bluewake/pull/421) ·
[M6 follow-up](https://github.com/chrissotraidis/bluewake/pull/438)

## Milestones

### M0: Freeze the comparison contract

- [ ] M0-T01: Resolve #433 and pin the source, dependencies, options, `-march`, training recipe and Deck workload.
- [ ] M0-T02: Complete provenance fields for compiler, generated PGO profile and effective flags.
- [ ] M0-T03: Verify that each recorded recipe can be reproduced privately.

### M1: Produce recipe artifacts (depends: M0)

- [ ] M1-T01: Build clean Clang 18 and Clang 22 AppImages in separate output directories.
- [ ] M1-T02: Confirm provenance matches on shared inputs and records compiler-derived hashes.
- [ ] M1-T03: Run Linux tests, AppImage ABI/smoke checks and checkpoint validation for both artifacts.

### M2: Measure and decide (depends: M1)

- [ ] M2-T01: Establish control A/A noise, then run five interleaved Deck A/B pairs.
- [ ] M2-T02: Capture matched Game Mode summaries and investigate conflicting results when needed.
- [ ] M2-T03: Retain the measured winner beyond noise or record an inconclusive/rejected result without changing the default.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 (preliminary) | #446 `48a5872`; private Clang 18/22 AppImages | Container builds, ABI audit, extraction/help smoke | Same private disc; `x86-64-v3`; separate clean outputs | Both builds and checks passed; source is not yet upstream main |
| M1-T01 (supplemental) | private AppImage SHA-256 `ad467905…c243a76b5e` / `97e9291d…e7c73cf25` | `bench_tour.py … --state quick-23874.bwstate --mode headless`, 3 runs each | Steam Deck; #446; compiler-specific trained PGO profiles | Clang 18 mean 33.84 FPS; Clang 22 mean 32.42 FPS; checkpoints match; one-state evidence superseded by the full tour |
| M2-T01 (headless only) | private #446 AppImages; private fixed setup state | #450 `--setup-state`; discard warmup; five interleaved A/B/B/A pairs | Steam Deck on mains; five default scenes; 10 samples/recipe | All 31 checkpoints match. Clang 18 recipe median +3.2% overall; rendered/Game Mode remains required |
