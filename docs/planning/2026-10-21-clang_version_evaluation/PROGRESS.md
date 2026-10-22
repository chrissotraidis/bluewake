# PROGRESS — Clang version evaluation

Status: Headless selection evidence favors Clang 18; rendered and Game Mode
observations are recorded but do not yet satisfy their gates. Qualification
remains blocked by unmerged [#446](https://github.com/chrissotraidis/bluewake/pull/446),
[#433](https://github.com/chrissotraidis/bluewake/issues/433), repeated rendered
comparison after the tour stage-completion parser is repaired, and repeated
Game Mode comparison.

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
| M2-T01 (rendered, preliminary) | private #446 AppImages; same fixed setup state | One warmed Desktop Mode rendered pair; five default scenes | Steam Deck Desktop Mode; Aurora; unpaced; 31 checkpoints/recipe | Clang 18 mean 272.06 versus Clang 22 265.67 game FPS (+2.4%). Raw logs show every requested warp, but the tour parser does not recognize this build's completion marker and reports `not loaded`; repeat after repair. |
| M2-T02 (Game Mode, directional) | private #446 AppImages; verified running AppImage/process | One 30-second 99 Hz `perf` capture per recipe, attached over SSH | Steam Deck Game Mode; same Fortress sea location; zero lost samples | Both runs reported a game-thread limit at roughly 27–29 game FPS. This is a paired observation, not the repeated controlled Game Mode comparison required for selection. |
