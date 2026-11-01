# TECH_PLAN — Dispatch-loop edges in Outset

Status: Planning  
Source: BlueWake `main` at `6b429fea9e60`  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## What exists

| Path | Where | What it already removes |
|---|---|---|
| Chassis loop | [`dispatch_loop.h`](../../../cmake/composite/dispatch_loop.h) `bluewake_chassis_dispatch_loop` | Continues across quiet block boundaries without returning to the host |
| Direct calls | [`direct_calls.py`](../../../scripts/windows/direct_calls.py), [`direct_calls.h`](../../../cmake/composite/direct_calls.h) | `bl` to another chunk runs the callee directly (#234; #503 made the check cheaper) |
| Indirect calls | `direct_calls.py` `transform_indirect`, `bw_call_translated` | `bctrl` with a translated target runs directly |
| Returns | `return_ranges.py` (#235) | A return into another chunk skips the dispatcher's search |
| Dispatch cache | `generate_composite.py` `dolrecomp_find_original` | PC-keyed cache, exact (#448) |
| Host gate | `main.c` `host_chassis_requires_full`, edge service | Decides when the host must act at a boundary |

What reaches the loop is whatever these leave: watched addresses, transfers whose target or return the transforms skip (`ret != site + 4`, labels missing), indirect targets outside translated code, budget and exception stops.

## Approach

1. **Counting build (diagnostic only).** Add an opt-in counter (compile-time `BLUEWAKE_EDGE_CENSUS`, which already exists for related work) at loop entry, recording the source site, the target and the reason for the exit. Report the top sites at exit.
2. **Outset sample.** Run the counting build from #503's worst-angle state in Game Mode; normalize per game frame.
3. **Map.** Map source sites to game functions (decomp `symbols.txt`, as in #503 M1-T02) and to the transform that skipped them.
4. **Bound.** For the largest group, build a ceiling that takes that path directly, unsafely, and A/B it against the #503 stack.
5. **Candidate.** Only if the bound is consistent: an exact change in the transform or the loop, with focused tests.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `cmake/composite/dispatch_loop.h` | Opt-in census at loop entry (kind, reason, site) | Attribution; off in shipped builds |
| `scripts/` (report) | Census → per-frame table, joined with decomp symbols | Repeatable mapping |
| `tests/composite_dispatch_loop_test.c` | Census counts on synthetic edges; zero overhead when off | Keeps the shipped path unchanged |
| Candidate files | Decided by M2 | — |

## Verification

| Layer | Required evidence |
|---|---|
| Census | Off by default (`nm` shows no census symbols in a normal build); counts reproducible across two runs of the same state |
| Exactness | Census build checkpoints identical to the normal build |
| Candidate | Game Mode ≥ 6 interleaved launches ahead of the #503 stack; render_bench checkpoints identical; unit tests in CI |

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Census build and Outset per-frame table |
| M1 | M0 | Sites mapped to game functions and transforms |
| M2 | M1 | Ceiling for the largest group; go/no-go |
| M3 | M2 | Exact candidate, tests and A/B |

## Gaps

- `BLUEWAKE_EDGE_CENSUS` already disables the inline direct-call path in `main.c`. The census build must count without changing which path runs, or its numbers describe a different program.

## References

- [#503 Outset game-thread follow-up](https://github.com/chrissotraidis/bluewake/pull/503) — the 4.3 M figure and method
- [#448 native dispatch no-change result](https://github.com/chrissotraidis/bluewake/pull/448)
- [Game thread: translated dispatch](../../architecture/GameThread.md#translated-dispatch)
