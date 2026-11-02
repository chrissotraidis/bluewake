# TECH_PLAN — Dispatch-loop edges in Outset

Status: Closed: no change qualifies; planning record only (census and candidate code removed)  
Source: BlueWake `main` at `6b429fea9e60`  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Outcome (2026-11-02)

**No change to the dispatch loop qualifies.** Every route was bounded or measured on the Steam Deck in Game Mode, from the worst-angle Outset state, at 16:9 with the widescreen mod:

| Route | Result |
|---|---|
| Fewer turn ends (DSP step 4× rarer, not exact; a bound) | +0.4%, ahead 3/3 pairs but by 0.03–0.23 FPS: not worth an exact redesign |
| Direct-call depth cap 32 → 128 | No change in the census |
| `bctr` and `blrl` run directly, all `bctr` sites (trained on its own code) | +0.4%, behind in 1 of 3 pairs |
| The same, only `__ptmf_scall`'s `bctr` and the GX `blrl` | 0.0%, behind in 5 of 6 pairs |

The loop's ≈ 35,800 boundaries per frame are cheap individually; no group large enough to remove is also expensive enough to show.

**The rewrite's code was removed from this PR.** It stays in the branch history: [`911e0e8`](https://github.com/irregularthanis/bluewake/commit/911e0e8f6) (all `bctr`/`blrl`) and [`3335807`](https://github.com/irregularthanis/bluewake/commit/33358074b) (narrowed).

**This PR merges only the planning record.** The opt-in loop census (`BLUEWAKE_LOOP_CENSUS`) that produced these findings was removed too. It stays in the branch history for any future counting build: [`5864a65`](https://github.com/irregularthanis/bluewake/commit/5864a655f) (census), [`3e03a5d`](https://github.com/irregularthanis/bluewake/commit/3e03a5d54) (host report), [`5a6fb7e`](https://github.com/irregularthanis/bluewake/commit/5a6fb7e69) (call sites by link register).

**Follow-up:** the census pointed at the skinning native, which declines exactly the big models that cost the most. Planned in the native-extensions follow-up.

Also learned: a candidate that changes chunk code must be trained on its own code. A reused profile made the same rewrite measure −4.9%.


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

## Candidate (M3, removed): `bctr` tail jumps and `blrl` calls run directly

**Problem.** M0–M2 attributed the loop's traffic. Apart from watched calls (kept on purpose) and returns after turn ends (≤ 0.4%, closed), the largest remaining group comes from two instructions [`direct_calls.py`](../../../scripts/windows/direct_calls.py) never rewrites:

| Instruction | Where | Per Outset frame | What happens today |
|---|---|---:|---|
| `bctr` (unconditional) | `__ptmf_scall` (0x80328DE8), which runs C++ pointer-to-member calls | ≈ 2,000 calls | The chunk returns with `pc = ctr`; the loop dispatches the member; its `blr` back to the caller also goes round the loop |
| `blrl` | GX (0x80324F10), the texture-region callback | ≈ 490 calls | The chunk jumps to its return dispatcher; the callee runs from the loop; so does its return |

**Change.** In `transform_indirect`, next to the existing `bctrl` rewrite:

```c
/* bctr: run the target where the loop would have dispatched it, then leave
   with the guest where the target left it. */
ctx->pc = target;
if (bw_direct_call_ready(ctx, target))
    (void)bw_call_translated(ctx, target);
return;

/* blrl: as bctrl, with the target read from lr before lr is set. */
ctx->lr = RET;
ctx->pc = target;
if (bw_direct_call_ready(ctx, target) && bw_call_translated(ctx, target) &&
    ctx->pc == RET && bw_direct_call_ready(ctx, RET))
    goto label_RET;
goto return_dispatch_CHUNK;   /* the original path */
```

Through `__ptmf_scall`, the caller reached the helper with a direct `bl`. After the member returns, `pc` equals the caller's return address, so the caller's existing direct-call check carries on in place: neither the call nor the return goes round the loop.

**Exactness.** Only the host path changes, not the guest instructions or their order:
- `bw_direct_call_ready` keeps the exception, budget, interrupt and watched-address checks;
- `bw_call_translated` refuses watched targets and targets with no translation;
- any refusal takes the original path.

The loop would have done the same: dispatch the target, then continue from wherever it left the guest.

**Measuring it.** The rewrite changes the control flow of 220 chunks (29.8% of Outset's chunk time), so the candidate must be trained on its own code. A reused profile no longer matches those chunks: the first A/B with a reused profile came out 4.9% slower, which measured that mismatch, not the change.

**Risk.** `bctr` also implements switch tables. Those now run their case block nested instead of through the loop, which is still exact but adds call depth; a deep switch loop could reach the 32-call cap and unwind as before. The A/B shows whether that costs anything.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `cmake/composite/loop_census.h`, `dispatch_loop.h` | Opt-in census: entries, boundaries by target and skipped/asked, exit reasons | Attribution; compiles to nothing in shipped builds |
| `scripts/` (report) | Census → per-frame table, joined with decomp symbols | Repeatable mapping |
| `tests/loop_census_test.c`, CI (Windows, Linux) | Census counts on synthetic runs: every exit reason, targets, entries | Proves the counts; the existing loop test proves the path is unchanged |
| ~~`scripts/windows/direct_calls.py`, `tests/test_direct_calls.py`~~ | ~~`bctr` and `blrl` run their target directly~~ | Removed after M3: no consistent gain (history: `911e0e8`, `3335807`) |

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
