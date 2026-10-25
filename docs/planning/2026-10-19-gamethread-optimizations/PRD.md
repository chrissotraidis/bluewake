# PRD — Steam Deck game-thread optimization

Status: Planning only

Source: upstream `main` at `2ab8118fc09fa08569100201a871616d18a1b771`

Scope: profile, select, implement, and qualify one measured game-thread
optimization plus bounded Deck build-time experiments; excluded: renderer-only
tuning, 60 Hz simulation, and speculative actor parallelism

[Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

Architecture: [game thread](../../architecture/GameThread.md) ·
[runtime](../../architecture/Runtime.md) ·
[performance workflow](../../performance/Overview.md)

## Motivation

Steam Deck players can remain below the game's 30 FPS target while the GPU has
headroom and the game thread is saturated ([#215](https://github.com/chrissotraidis/bluewake/issues/215)).
The confirmed goal is both to reduce measured game-thread cost and to reach a
stable 30 FPS in repeatable demanding Deck scenes without changing gameplay.

Completed experiments retain their own recorded source revision and conclusion.
A new current-control comparison must record its own revision rather than
generalizing an earlier experiment's evidence.

## Requirements

### FR-01 — Reproducible Steam Deck baseline

Architecture: [performance workflow](../../performance/Overview.md#performance-workflow).

- Measure the same private state, scene, settings, build inputs, pacing, power
  condition, and warmup across repeated runs.
- Produce raw session logs, benchmark summaries, `perf` data/reports, hashes,
  and environment metadata. Missing or mismatched inputs make a run incomplete.

### FR-02 — Profile before selecting code

Architecture: [performance domains](../../performance/Overview.md#performance-domains).

- Separate translated game work, host/dispatch bookkeeping, guest-state access,
  services, GX work, blocking, and presentation.
- Rank exclusive self-time and cycles/instructions per retrace. A hot symbol is
  a candidate, not proof that changing it improves the product.

### FR-03 — Implement one bounded candidate

Architecture: [game-module dispatch](../../architecture/GameModule.md#translated-code-dispatch).

- Select only a path activated in the measured scene with a defensible upper
  bound. Preserve guest state, deadlines, arithmetic, ordering, and fallback.
- The first experiment is a serial native replacement for
  `cBgW::GroundCrossGrpRp`; it must preserve recursive traversal and callback
  order. Parallel child traversal is explicitly out of scope.
- Keep gameplay/timing changes off by default until affected-platform evidence
  supports enablement. Runtime-owned work follows the RecompCore PR/pin process.

### FR-04 — Demonstrate both performance outcomes

- Reduce game-thread cost beyond same-build noise in the selected workload.
- Reach stable 30 FPS in the target rendered Deck scene, or report the remaining
  gap and reject any claim that the goal is complete.

### FR-05 — Preserve correctness and portability

Architecture: [test evidence levels](../../architecture/Testing.md#evidence-levels).

- Candidate-specific source tests and differential/oracle checks must pass with
  positive path activation; serial fallback is not candidate evidence.
- Run applicable Linux/Windows CI and Deck gameplay checks. Private discs,
  modules, states, cards, and raw personal logs remain local.

### FR-06 — Evaluate Deck build-time optimization

Architecture: [build process](../../architecture/Build.md#6-train).

- Compare compiler/tuning and profile variants independently against the same
  control, source, training input and Deck workload.
- Keep portable Linux defaults unchanged unless cross-device evidence supports
  them; a Deck-specific result is not a general x86-64 result.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Rendered game rate | Unknown; remeasure | Median ≥29.5 FPS with no sustained game-thread dip after warmup | Five interleaved A/B runs in the same Deck Game Mode scene |
| Game-thread cost | Unknown; remeasure | Improvement exceeds the larger of 3% or measured A/A noise | Unpaced throughput, busy share, cycles and instructions per retrace |
| Correctness | Untested for candidate | Required oracle/checkpoint and gameplay checks pass | Source tests, private oracle, benchmark checkpoints, rendered play |
| Regression | Unknown | No material low-load, GX-wait, audio, input, save, or shutdown regression | Control scene, session diagnostics, manual route, lifecycle tests |
| Build-time variant | Unknown | Improvement beyond A/A noise without correctness regression | Interleaved Deck A/B runs from matched artifacts |

## Clarifications

- Confirmed by the user on 2026-10-20: both stable 30 FPS and measurable
  game-thread reduction are required.
- Confirmed by the user on 2026-10-20: proceed with the bounded native
  `GroundCrossGrpRp` reimplementation plan.
- The reported [#433](https://github.com/chrissotraidis/bluewake/issues/433)
  regression was retracted. A certified current control requiring native-entry
  generation is blocked by [#451](https://github.com/chrissotraidis/bluewake/issues/451).
