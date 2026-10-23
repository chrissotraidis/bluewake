# PRD — Clang version evaluation

Status: Partial result; no recipe-default decision
Source: pre-merge [PR #446](https://github.com/chrissotraidis/bluewake/pull/446)
at `48a5872e213db41117e5bb4321a7aefbcf77993e`
Parent milestone: [PR #421, M5](https://github.com/chrissotraidis/bluewake/pull/421)
Parent status: [PR #421, M5](https://github.com/chrissotraidis/bluewake/pull/421)

## Motivation

The shared Linux container provides Clang 18 and optional Clang 22, but the
project has not compared complete compiler build recipes on Steam Deck. This
plan determines which correct reproducible recipe runs BlueWake fastest.

## Scope

- Build comparable Clang 18 and Clang 22 AppImages from one revision.
- Hold source, `-march=x86-64-v3`, options, dependencies, and packaging
  constant; treat each compiler's trained PGO profile as part of its recipe.
- Compare correctness, artifact properties, build cost, and Steam Deck runtime.
- Change no compiler default until the required qualified evidence is complete.

Excluded: changing CI's host-test compiler, changing gameplay code, publishing
private game artifacts, or treating a successful build as performance proof.

## Outcomes

- Each artifact records compiler version, effective flags, PGO hash, module
  hash and size, source revision, and AppImage ABI result.
- Follow the parent plan's Steam Deck procedure: freeze environment and
  workloads; collect three A/A runs in headless and rendered modes; capture
  headless, rendered, and Game Mode profiles; then run five interleaved A/B
  pairs in both modes and repeat the Game Mode scene.
- Guest checkpoints and required Linux/AppImage checks match for both variants.
- The decision retains a recipe only when qualified evidence exceeds noise;
  otherwise Clang 18 remains the default and the result is inconclusive.

## Success and constraints

| Metric | Baseline | Decision rule | Evidence |
|---|---|---|---|
| Game-thread throughput | Clang 18 control | Candidate median improves beyond A/A noise across the target scenes | `bench_tour.py compare` summaries |
| Game Mode pacing | Matched Clang 18 run | Direction agrees with controlled runs; report FPS, busy cause and waits | Session-log summary |
| Correctness | Control checkpoints | All compared checkpoints match | Benchmark output |
| Build identity | Frozen source/options/dependencies | Compiler and its derived PGO/module artifacts are recorded | Provenance and effective-command record |
| Compatibility | Current AppImage ceiling | ABI audit and launch smoke pass | Audit/smoke logs |

Private discs, modules, AppImages, states, raw logs, and `perf.data` remain off
the repository. A certified current native-entry control is blocked by
[#451](https://github.com/chrissotraidis/bluewake/issues/451); this does not
turn the pre-merge #446 recipe result into a current-control result.

## Findings

The fixed-state five-scene headless grid favored the Clang 18 recipe by 3.2%
with matching checkpoints. That is the only qualified performance result.
Rendered scene loading was not confirmed, and Game Mode was not repeated as a
controlled comparison; neither result participates in recipe selection.
