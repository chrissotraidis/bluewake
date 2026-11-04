# PROGRESS — Clang version evaluation

Status: The historical headless grid favors Clang 18 by 3.2%; no default
changes. A current 2×2 Clang/runtime grid, cross-referenced with
[#472](https://github.com/chrissotraidis/bluewake/pull/472), is planned.

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
- [ ] M2-T02: Confirm rendered scene loading, then establish rendered and Game Mode A/A noise.
- [ ] M2-T03: Run five interleaved rendered and Game Mode pairs, then retain a winner or record an inconclusive result.

See the [ordered remaining measurement sequence](TECH_PLAN.md#remaining-measurement-order).

### M3: Freeze the current 2×2 contract

Frozen foundation: [`14c558d62`](https://github.com/irregularthanis/bluewake/commit/14c558d62)
(`main` + #503 + #531, with #504). Source/default contracts pass; playable compiled gates remain open.

- [x] M3-T01: Create one tested snapshot from current `main` + #503 + #531 (#531 already carries #504); pin its SHA, dependencies, options, Dawn and Deck settings.
- [x] M3-T02: Select libc++ or static libstdc++ independently of compiler and Dawn; record all identities in provenance.
- [x] M3-T03: Run #503/#504/#531 focused tests and add builder tests proving recipe selection and cross-platform qualified defaults.

### M4: Produce and validate the four cells (depends: M3)

- [x] M4-T01: Build A/B with Clang 18 and C/D with Clang 22 using identical frozen inputs (2026-11-03: `build_script` full builds A and C; B and D `--reuse` host-only over them).
- [x] M4-T02: Verify A/B share one module hash, C/D share one module hash, and all cells share the Dawn identity (published provenance: A/B module `9a1839c6…`, C/D `7618ce31…`; all `vendor` Dawn, commit `14c558d`).
- [ ] M4-T03: Run source tests, ABI audit, launch smoke and checkpoint preflight for every AppImage.

### M5: Measure and decide (depends: M4)

- [ ] M5-T01: Run the full interleaved five-scene headless grid with matching checkpoints.
- [ ] M5-T02: Establish rendered A/A noise, then run at least five interleaved four-cell rendered rounds.
- [ ] M5-T03: Run the matched Game Mode grid and #472 packaged-Dawn confirmation; report main effects, interaction and decision.

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
| M5-T01 (Deck, session 1) | `m437-a`…`m437-d` (M4); local `m437-grid/` (private) | `render_bench` (the frozen #437 bench plus a local `--set` for all builds), 5 scenes, `--pairs 2` (A/B/C/D/D/C/B/A twice, 4 runs per cell); render scale 3, Smooth Motion, widescreen, efficient soft effects on for all | **Steam Deck**, Game Mode (`--display desktop` uses gamescope's X server) | **Guest checkpoints identical across every run of all four builds.** Overall FPS: A 38.23, B 38.06, C 38.63, D 38.60. **Compiler: Clang 22 +1.0% (C vs A) and +1.4% (D vs B), ahead in 9 of 10 scene comparisons** (the exception, Butterfly Room with libc++, −0.3%). **Runtime: static libstdc++ −0.5% (Clang 18) and −0.1% (Clang 22), signs flipping by scene: no effect**; #472's +3% does not reproduce. Butterfly and Outset picture flags are capture timing (identical checkpoints, equal flicker counts). 2 of the planned 5 rounds |
| M4-T01/T02 | `perf/clang-runtime-matrix@14c558d`; private AppImages `m437-a` `badc7d5e…`, `m437-b` `93278766…`, `m437-c` `7a1a05ca…`, `m437-d` `422aa922…` | `build_script m437-{a,c} … [--clang22] --appimage-stdlib libc++`; `m437-{b,d} … --appimage-stdlib static-libstdc++ --reuse` the A/C folder | Linux workstation, CI container, same private disc | **Four cells, one source and Dawn (`vendor`).** A: Clang 18.1.3, libc++; B: Clang 18.1.3, static libstdc++; module `9a1839c6…` and profile `2ca56453…` shared. C: Clang 22.1.8, libc++; D: Clang 22.1.8, static libstdc++; module `7618ce31…` and profile `d1f39ead…` shared. B and D are host-only rebuilds; their AppImages differ from their partners'. Full builds ≈ 50 min each, host-only ≈ 11 min |
| M0-T01 | #446 `48a5872`; private Clang 18/22 AppImages | Container builds, ABI audit, extraction/help smoke | Same private disc; `x86-64-v3`; separate clean outputs | Both builds and checks passed; source is not upstream main. |
| M0-T02 | Same private AppImages | Record compiler-specific PGO and artifact hashes | Same frozen #446 inputs | Recipe identity recorded; compiler-specific PGO is part of each recipe. |
| M0-T03 | Same private AppImages | Repeat private container builds in separate outputs | Same disc and frozen inputs | Both recipes reproduced. |
| M1-T01 | Private AppImage SHA-256 `ad467905…c243a76b5e` / `97e9291d…e7c73cf25` | Clean container builds | #446 `48a5872`; separate outputs | Clang 18 and Clang 22 artifacts produced. |
| M1-T02 | Same AppImages | Compare frozen source/options and compiler-derived artifact identity | Same dependency revisions and `x86-64-v3` target | Only compiler-specific recipe artifacts differ. |
| M1-T03 | Same AppImages and fixed state | ABI audit, extraction/help smoke, and headless checkpoint validation | Steam Deck; private fixed setup state | ABI/smoke pass; checkpoints match. |
| M2-T01 | private #446 AppImages; private fixed setup state | #450 `--setup-state`; discard warmup; five interleaved A/B/B/A pairs | Steam Deck on mains; five default scenes; 10 samples/recipe | All 31 checkpoints match. Clang 18 recipe median +3.2% overall. |
| M2-T02 | — | Rendered scene loading was not confirmed; repeat qualified rendered and Game Mode comparison | — | Open; excluded from recipe selection. |
| M3-T01 | [`14c558d62`](https://github.com/irregularthanis/bluewake/commit/14c558d62) | Stack/merge-base, dependency-lock and effective-default audit | Current upstream `main` + #503 + #531 (#504 included) | PASS: upstream base `ee991d312d97`; RecompCore profile/lock both `a4dbaaa811c…`; qualified defaults aligned across supported builders |
| M3-T02 | [`14c558d62`](https://github.com/irregularthanis/bluewake/commit/14c558d62) | Builder selectors/provenance plus `render_bench` synthetic suite | Same combined branch; no game data | PASS: independent compiler/runtime/Dawn selection; arbitrary-build balanced benchmark and pairwise report; defaults unchanged |
| M3-T03 | [`14c558d62`](https://github.com/irregularthanis/bluewake/commit/14c558d62) | Linux 29, Windows 11, Android 6, PadMint 10, render benchmark 24 tests; syntax, audit and diff checks | No private disc or game artifact | PASS: #503/#504/#531 contracts and cross-platform default policy; Efficient Soft Effects remains deliberate opt-in |
