# PROGRESS — Clang version evaluation

Status: The qualified headless grid favors Clang 18 by 3.2%; no default changes.
Rendered scene loading and repeated Game Mode comparison remain open. A
certified current native-entry control is blocked by
[#451](https://github.com/chrissotraidis/bluewake/issues/451).

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) ·
[Parent M5](https://github.com/chrissotraidis/bluewake/pull/421) ·
[Current parent status](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Freeze the #446 comparison contract

- [x] M0-T01: Pin source `48a5872`, dependencies, options, `-march`, training recipe and Deck workload.
- [x] M0-T02: Record compiler, generated PGO profile, and artifact identity for both recipes.
- [x] M0-T03: Reproduce both recorded recipes privately.

### M1: Produce recipe artifacts (depends: M0)

- [x] M1-T01: Build clean Clang 18 and Clang 22 AppImages in separate output directories.
- [x] M1-T02: Confirm shared inputs and compiler-derived artifact identity.
- [x] M1-T03: Validate ABI/extraction smoke and headless checkpoints for both artifacts.

### M2: Measure and decide (depends: M1)

- [x] M2-T01: Run five interleaved headless Deck A/B/B/A pairs with matching checkpoints.
- [ ] M2-T02: Confirm loaded rendered scenes and capture repeated controlled rendered/Game Mode comparisons.
- [ ] M2-T03: Retain the measured winner beyond noise or record an inconclusive/rejected result without changing the default.

## Evidence ledger

## Run findings

| Run | What ran | Outcome | Decision use |
|---|---|---|---|
| Build and smoke | Clean Clang 18 and Clang 22 container builds from `48a5872`, followed by ABI audit and extraction/help smoke | Both AppImages built and passed the private smoke checks | Validates the compared artifacts; not a performance result |
| Headless benchmark | Five fixed scenes, 10 interleaved A/B/B/A samples per recipe, with 31 checkpoints | Clang 18 recipe median was 3.2% faster overall; checkpoints matched | **Accepted headless finding** |
| Rendered Desktop attempt | One warmed rendered tour across the five scenes | The completion parser reported scenes not loaded, so throughput cannot be accepted | Excluded; repeat after confirming scene loading |
| Game Mode attempt | One 30-second profile per recipe at the same sea location | Directional profile only; not a repeated controlled comparison | Excluded; repeat controlled runs |

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | #446 `48a5872`; private Clang 18/22 AppImages | Container builds, ABI audit, extraction/help smoke | Same private disc; `x86-64-v3`; separate clean outputs | Both builds and checks passed; source is not upstream main. |
| M0-T02 | Same private AppImages | Record compiler-specific PGO and artifact hashes | Same frozen #446 inputs | Recipe identity recorded; compiler-specific PGO is part of each recipe. |
| M0-T03 | Same private AppImages | Repeat private container builds in separate outputs | Same disc and frozen inputs | Both recipes reproduced. |
| M1-T01 | Private AppImage SHA-256 `ad467905…c243a76b5e` / `97e9291d…e7c73cf25` | Clean container builds | #446 `48a5872`; separate outputs | Clang 18 and Clang 22 artifacts produced. |
| M1-T02 | Same AppImages | Compare frozen source/options and compiler-derived artifact identity | Same dependency revisions and `x86-64-v3` target | Only compiler-specific recipe artifacts differ. |
| M1-T03 | Same AppImages and fixed state | ABI audit, extraction/help smoke, and headless checkpoint validation | Steam Deck; private fixed setup state | ABI/smoke pass; checkpoints match. |
| M2-T01 | private #446 AppImages; private fixed setup state | #450 `--setup-state`; discard warmup; five interleaved A/B/B/A pairs | Steam Deck on mains; five default scenes; 10 samples/recipe | All 31 checkpoints match. Clang 18 recipe median +3.2% overall. |
| M2-T02 | — | Rendered scene loading was not confirmed; repeat qualified rendered and Game Mode comparison | — | Open; excluded from recipe selection. |
