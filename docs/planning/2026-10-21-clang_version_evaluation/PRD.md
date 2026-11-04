# PRD — Clang version evaluation

Status: Current 2×2 measured: Clang 22 with libc++ selected; Game Mode grid open
Source: pre-merge [PR #446](https://github.com/chrissotraidis/bluewake/pull/446)
at `48a5872e213db41117e5bb4321a7aefbcf77993e`
Current matrix foundation: upstream `main` at `ee991d312d97` plus successful
performance work through [#531](https://github.com/chrissotraidis/bluewake/pull/531);
packaging candidate [#472](https://github.com/chrissotraidis/bluewake/pull/472)
Frozen combined snapshot: [`14c558d62`](https://github.com/irregularthanis/bluewake/commit/14c558d62)
on `perf/clang-runtime-matrix`; M3 source/default contracts pass.
Parent milestone: [PR #421, M5](https://github.com/chrissotraidis/bluewake/pull/421)
Parent status: [PR #421, M5](https://github.com/chrissotraidis/bluewake/pull/421)

## Motivation

The shared Linux container provides Clang 18 and optional Clang 22, while #472
proposes replacing the AppImage host's libc++ with statically linked
libstdc++. The project has not measured whether that runtime gain persists
under both compilers or whether the two choices interact. This plan determines
the fastest correct reproducible combination on Steam Deck.

## Scope

- Build a 2×2 grid: Clang 18/22 × libc++/static libstdc++, from one revision.
- Create one combined source snapshot: current `main` (including merged
  #480/#495/#501), then #503 and #531; #531 already carries #504.
- Hold source, `-march=x86-64-v3`, options, dependencies, and packaging
  constant. Hold the Dawn source/provider constant so the runtime comparison
  does not also compare two Dawn builds.
- Reuse one exact game module within each compiler pair; each compiler's
  trained PGO profile remains part of that compiler recipe.
- Enable #504's opt-in Efficient Soft Effects explicitly in every measurement
  cell and retain #531's default GX batching. Exclude planning-only #529/#530.
- Compare correctness, artifact properties, build cost, and Steam Deck runtime.
- Change no compiler default until the required qualified evidence is complete.

The frozen source keeps qualified performance defaults aligned across Linux,
Windows, Android, macOS and personal iOS builds. Efficient Soft Effects remains
opt-in because it trades effect resolution for speed; experimental native sets
remain off until separately qualified.

Excluded: changing CI's host-test compiler, changing gameplay code, publishing
private game artifacts, or treating a successful build as performance proof.

## Outcomes

- Each artifact records compiler, C++ runtime, Dawn provider/revision,
  effective flags, PGO hash, module hash and size, source, and ABI result.
- Follow the parent plan's Steam Deck procedure: freeze environment and
  workloads; collect three A/A runs in headless and rendered modes; capture
  headless, rendered, and Game Mode profiles; then run at least five
  interleaved four-cell rounds and repeat the Game Mode scene.
- Guest checkpoints and required Linux/AppImage checks match for all four cells.
- Report the libstdc++ delta under each compiler, the compiler delta under each
  runtime, and their interaction. Retain a recipe only when qualified evidence
  exceeds noise; otherwise preserve current defaults.

## Success and constraints

| Metric | Baseline | Decision rule | Evidence |
|---|---|---|---|
| Game-thread throughput | Clang 18 control | Candidate median improves beyond A/A noise across the target scenes | `bench_tour.py compare` summaries |
| Game Mode pacing | Matched Clang 18 run | Direction agrees with controlled runs; report FPS, busy cause and waits | Session-log summary |
| Correctness | Control checkpoints | All compared checkpoints match | Benchmark output |
| Build identity | Frozen source/options/dependencies | Compiler and its derived PGO/module artifacts are recorded | Provenance and effective-command record |
| Compatibility | Current AppImage ceiling | ABI audit and launch smoke pass | Audit/smoke logs |
| Runtime effect | libc++ within each compiler pair | Static-libstdc++ delta exceeds matched A/A noise | Rendered per-scene and overall deltas |
| Interaction | Difference between the two runtime deltas | Report `(D−C)−(B−A)`; no minimum required | 2×2 comparison summary |

Private discs, modules, AppImages, states, raw logs, and `perf.data` remain off
the repository. A certified current native-entry control is blocked by
[#451](https://github.com/chrissotraidis/bluewake/issues/451); this does not
turn the pre-merge #446 recipe result into a current-control result.

## Findings

The historical fixed-state five-scene headless grid favored the Clang 18
recipe by 3.2% with matching checkpoints. That result used pre-merge #446 and
does not answer the current 2×2 question. #472 reported an approximately 3%
rendered gain from static libstdc++ on Clang 18, but used an older source and
also changed the Dawn provider. Both findings motivated the grid.

**The current grid reverses the compiler result and refutes the runtime one.**
Four cells from the frozen `14c558d62`, render_bench over five scenes, five
interleaved rounds per cell (two sessions) on the Steam Deck and on a desktop
(Ryzen 7 5800X3D, RX 9070 XT):

| Contrast | Steam Deck, session 1 / 2 | Desktop, session 1 / 2 |
|---|---:|---:|
| Clang 22 vs 18, libc++ (C vs A) | **+1.0% / +1.3%** | **+1.1% / +0.9%** |
| Clang 22 vs 18, static libstdc++ (D vs B) | **+1.4% / +1.8%** | **+0.7% / +0.8%** |
| static libstdc++ vs libc++, Clang 18 (B vs A) | −0.5% / −0.6% | +0.5% / +0.2% |
| static libstdc++ vs libc++, Clang 22 (D vs C) | −0.1% / −0.1% | 0.0% / +0.1% |
| Interaction (D−C)−(B−A) | +0.4 / +0.5 points | −0.5 / −0.1 points |

- Guest checkpoints identical in every run of every cell on both machines.
- Clang 22 is faster in all 8 contrasts: about +1.3% on the Deck, +0.9% on the
  desktop. The old 3.2% Clang 18 lead came from pre-merge #446 and does not
  hold on the current tree.
- The runtime has no effect: under ±0.6%, opposite in sign between machines.
  #472's +3% came from its other changes, not static libstdc++.
- A/A noise is 0.6% on the Deck and 0.0% on the desktop, below every compiler
  contrast. #472's packaging (static libstdc++ with packaged Dawn) on Clang 22
  is noise: −0.6% Deck, +0.2% desktop.
- Decision: Clang 22 with libc++ (cell C). Making it the default is a separate
  change, gated on the ABI audit and smoke test.
