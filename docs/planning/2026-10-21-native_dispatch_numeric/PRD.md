# PRD — Native dispatch and numeric-path evaluation

Status: Complete; M3 rejected without a runtime change
Source: BlueWake `main` at `56a9537`; parent [runtime plan PR #421](https://github.com/chrissotraidis/bluewake/pull/421)
Scope: Steam Deck evidence and one selected M3 runtime experiment; excludes changing worker defaults, broad native rewrites, and graphics work.

## Motivation

Steam Deck Game Mode remains game-thread bound below the intended 30 FPS. M1
(GroundCross) and M2 (guest aliases) were rejected after measurement. M3 now
examines the remaining native dispatch and numeric paths without assuming that
more threads or more native code helps.

## Recorded outcome

- Dispatch samples resolve to the existing exact PC-keyed cache, not a distinct
  reduction.
- Numeric telemetry identifies no redundant readiness or routing operation.
- The only pool client remains below its 256-item grain, so no worker-default
  change is selected.

## Success and constraints

| Measure | Evidence | Result | Validation |
| --- | --- | --- | --- |
| Candidate attribution | Fixed-route Deck profile and module reports | No distinct equivalence-safe reduction | Source and annotation review |
| Worker eligibility | `array_max=8`, `parallel=0` | Existing 256-item threshold is not reached | Normal-exit module report |
| Player impact | No code change | Not applicable | A/B and island qualification are not run without a candidate |

No compile result, worker count, or CPU utilization alone qualifies a change.

## Clarifications

- The user-confirmed goal is better Steam Deck game-thread performance; 30 FPS
  is desirable but not the acceptance threshold for an individual candidate.
- M3 ends with no implementation: the measured paths do not expose a safe,
  distinct reduction.
