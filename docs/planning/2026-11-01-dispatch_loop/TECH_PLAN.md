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

1. **Counting build (diagnostic only).** `cmake/composite/loop_census.h`, compile-time `BLUEWAKE_LOOP_CENSUS`, separate from the host's `BLUEWAKE_EDGE_CENSUS` (which also switches off the inline direct-call path). It counts host entries, each boundary by target address (with whether the edge service was skipped) and why the loop returned. Calls and returns are told apart offline: a target at a decomp function start is a call.
2. **Outset sample.** Run the counting build from #503's worst-angle state in Game Mode; normalize per game frame.
3. **Map.** Map source sites to game functions (decomp `symbols.txt`, as in #503 M1-T02) and to the transform that skipped them.
4. **Bound.** For the largest group, build a ceiling that takes that path directly, unsafely, and A/B it against the #503 stack.
5. **Candidate.** Only if the bound is consistent: an exact change in the transform or the loop, with focused tests.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `cmake/composite/loop_census.h`, `dispatch_loop.h` | Opt-in census: entries, boundaries by target and skipped/asked, exit reasons | Attribution; compiles to nothing in shipped builds |
| `scripts/` (report) | Census → per-frame table, joined with decomp symbols | Repeatable mapping |
| `tests/loop_census_test.c`, CI (Windows, Linux) | Census counts on synthetic runs: every exit reason, targets, entries | Proves the counts; the existing loop test proves the path is unchanged |
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

- `BLUEWAKE_EDGE_CENSUS` disables the inline direct-call path in `main.c`, so it is not used; the loop census is separate and counts without changing the path.

## References

- [#503 Outset game-thread follow-up](https://github.com/chrissotraidis/bluewake/pull/503) — the 4.3 M figure and method
- [#448 native dispatch no-change result](https://github.com/chrissotraidis/bluewake/pull/448)
- [Game thread: translated dispatch](../../architecture/GameThread.md#translated-dispatch)
