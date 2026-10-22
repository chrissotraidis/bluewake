# PRD — Clang version evaluation

Status: Draft  
Source: pre-merge [PR #446](https://github.com/chrissotraidis/bluewake/pull/446)
at `48a5872e213db41117e5bb4321a7aefbcf77993e`
Parent milestone: [PR #421, M5](https://github.com/chrissotraidis/bluewake/pull/421)
Follow-up plan: [PR #438](https://github.com/chrissotraidis/bluewake/pull/438)

## Motivation

The shared Linux container provides Clang 18 and optional Clang 22, but the
project has not made a matched Steam Deck comparison. This plan determines
which compiler produces the better current BlueWake build without mixing in a
different profile, source tree, CPU target, or gameplay change.

## Scope

- Build comparable Clang 18 and Clang 22 AppImages from one revision.
- Hold `-march=x86-64-v3`, optimization options, PGO input, dependencies, and
  packaging constant.
- Compare correctness, artifact properties, build cost, and Steam Deck runtime.
- Change the container default only if Clang 22 wins beyond measured noise.

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
- The decision retains the faster compiler only when the gain exceeds noise;
  otherwise Clang 18 remains the default and the result is inconclusive.

## Success and constraints

| Metric | Baseline | Decision rule | Evidence |
|---|---|---|---|
| Game-thread throughput | Clang 18 control | Candidate median improves beyond A/A noise across the target scenes | `bench_tour.py compare` summaries |
| Game Mode pacing | Matched Clang 18 run | Direction agrees with controlled runs; report FPS, busy cause and waits | Session-log summary |
| Correctness | Control checkpoints | All compared checkpoints match | Benchmark output |
| Build identity | One frozen recipe | Only compiler version may differ | Provenance and effective-command record |
| Compatibility | Current AppImage ceiling | ABI audit and launch smoke pass | Audit/smoke logs |

Private discs, modules, AppImages, states, raw logs, and `perf.data` remain off
the repository. Qualification is blocked until
[#433](https://github.com/chrissotraidis/bluewake/issues/433) is resolved.

## Preliminary baseline

The #446 revision built and passed private AppImage smoke/ABI checks on the
Deck. Three headless fixed-state runs were deterministic for each compiler:
Clang 18 averaged 33.84 game FPS and Clang 22 averaged 32.42 FPS. This is
useful supplemental build-path evidence, not an M5 decision: it covers only
one headless saved state, and each build generated its own compiler-specific
PGO profile. M0 must provide one verified profile before an isolated compiler
comparison can begin.
