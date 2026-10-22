# PRD — Native dispatch and numeric-path evaluation

Status: Draft
Source: BlueWake `main` at `56a9537`; parent [runtime plan PR #421](https://github.com/chrissotraidis/bluewake/pull/421)
Scope: Steam Deck evidence and one selected M3 runtime experiment; excludes changing worker defaults, broad native rewrites, and graphics work.

## Motivation

Steam Deck Game Mode remains game-thread bound below the intended 30 FPS. M1
(GroundCross) and M2 (guest aliases) were rejected after measurement. M3 now
examines the remaining native dispatch and numeric paths without assuming that
more threads or more native code helps.

## Outcomes

- Identify whether dispatch, native math/vector routing, or neither is a
  repeatable cost on the fixed Deck route.
- Select at most one serial reduction with a concrete equivalence contract, or
  reject M3 with recorded evidence.
- Consider worker execution only when production telemetry proves a batch reaches
  the existing 256-item grain and the parallel backend actually activates.

## Success and constraints

| Measure | Baseline | Target | Validation |
| --- | --- | --- | --- |
| Candidate attribution | No current M3 selection | Named path with activation evidence, or explicit rejection | Fixed-route Deck profile plus module reports |
| Correctness | Existing translated/native contracts | No checkpoint or focused-test mismatch | Existing guard/component tests and checkpoint route |
| Player impact | Current matched control | Better than its matched control beyond noise, if implemented | Five interleaved Deck pairs and island check |

No compile result, worker count, or CPU utilization alone qualifies a change.

## Clarifications

- The user-confirmed goal is better Steam Deck game-thread performance; 30 FPS
  is desirable but not the acceptance threshold for an individual candidate.
- M3 may end with no implementation if the measured paths are already small,
  inactive, or lack a safe distinct reduction.
