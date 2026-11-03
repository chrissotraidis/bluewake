# PRD — Clang version evaluation

Status: Partial compiler result; current 2×2 recipe evaluation planned
Source: pre-merge [PR #446](https://github.com/chrissotraidis/bluewake/pull/446)
at `48a5872e213db41117e5bb4321a7aefbcf77993e`
Current matrix foundation: upstream `main` at `ee991d312d97` plus successful
performance work through [#531](https://github.com/chrissotraidis/bluewake/pull/531);
packaging candidate [#472](https://github.com/chrissotraidis/bluewake/pull/472)
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
- Enable #504's Efficient Soft Effects and retain #531's default GX batching
  identically in every cell. Exclude the no-go, planning-only #529/#530 work.
- Compare correctness, artifact properties, build cost, and Steam Deck runtime.
- Change no compiler default until the required qualified evidence is complete.

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
also changed the Dawn provider. Both findings motivate the grid; neither is a
current recipe decision.
