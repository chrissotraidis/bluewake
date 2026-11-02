# PRD — Dispatch-loop edges in Outset

Status: Closed: no change qualifies; planning record only (census and candidate code removed)  
Source: BlueWake `main` at `6b429fea9e60`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503)  
Scope: the game-thread cost of control transfers that still go round the module's dispatch loop. Excluded: the direct-call check (settled in #503), game-function hooks.

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


## Problem

A transfer from one translated chunk to another either runs directly or returns to the chassis loop ([`dispatch_loop.h`](../../../cmake/composite/dispatch_loop.h)). Going round the loop means the loop checks the turn, asks the host's edge service, looks up the target and dispatches it. Direct calls ([#234](https://github.com/chrissotraidis/bluewake/pull/234)), returns into another chunk ([#235](https://github.com/chrissotraidis/bluewake/pull/235)) and the exact PC-keyed dispatch cache ([#448](https://github.com/chrissotraidis/bluewake/pull/448)) already remove most of it. In #503's Outset profile (Steam Deck, Game Mode, stage select off) the loop still costs about **4.3 M cycles per game frame**, about 3% of the frame:

| Symbol | M cycles/frame |
|---|---:|
| `chassis_dispatch` (module) | 2.3 |
| `host_chassis_requires_full` (host) | 1.0 |
| `bw_call_translated` (module, indirect calls run directly) | 0.6 |
| `bw_find_chunk` (module) | 0.3 |

Nobody knows yet which transfers these are. Without that, any change is a guess, and #503 showed guesses here are easy to get wrong.

## What we found (2026-11-01)

| Finding | Evidence | Consequence |
|---|---|---|
| ≈ 35,800 boundaries per game frame go round the loop | Loop census, Game Mode, worst-angle Outset state, 16:9 + widescreen | ≈ 120 cycles each on average |
| ≈ 40% are returns after a turn ended mid-call | 76–89% of returns follow calls that ran directly | Their frames were unwound by the turn end |
| Turns end at the DSP step (92%) | Host deadline census | Removing turn ends is worth ≤ 0.4% (DSP-rate bound): **closed** |
| The direct-call depth cap is not a cause | Cap 32 → 128 changed nothing | **Closed** |
| ≈ 2,800 calls per frame are watched on purpose | Actor-search hooks, `OSDisable/RestoreInterrupts` | Must stay |
| **≈ 2,500 calls per frame use instructions the builder never runs directly** | Call sites by link register: `bctr` in `__ptmf_scall` (C++ pointer-to-member calls, ≈ 2,000), `blrl` (≈ 490) | **The candidate** |

## The candidate: what it solves

The builder already runs `bl` and `bctrl` calls between chunks directly (#234, and the indirect form), which keeps them out of the loop. Two other ways of transferring control were never covered:

- **`bctr`, a jump through the count register.** The compiler's `__ptmf_scall` helper ends in one. It runs every C++ pointer-to-member call, for example `J3DShapeMtxConcatView::loadMtxConcatView`, about 1,750 times per Outset frame. The jump ends the chunk, the member function is dispatched by the loop, and its return to the original caller goes round the loop too.
- **`blrl`, a call through the link register.** GX uses one to call its texture-region callback, about 490 times per frame.

**The intent:** run those targets directly, the way a `bctrl` already runs, so the calls and their returns stop going round the loop. That is about 5,000 of the 35,800 boundaries per frame, an estimated +0.3–0.6%.

**Why it's safe:** the same guest code runs in the same order. Only the host-side path changes, and every runtime check stays: watched addresses, the edge service's readiness, the turn's budget and exceptions. Whenever a check refuses, the original path runs. Guest checkpoints must be identical to the control's.

## Outcomes

- Counts per game frame of transfers that go round the loop, grouped by kind and reason:
  - **kind:** indirect call, return to another chunk, branch, fallback;
  - **reason:** watched address, not quiet, budget, unknown target.
- The few call sites or reasons that make up most of it, mapped to game functions.
- A bounded candidate (ceiling build first), or a recorded no-go.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Attribution | 4.3 M cycles/frame, unattributed | ≥ 80% of loop entries explained by kind and reason | Counting build, Outset state |
| Candidate gain | #503 stack on `main` | Ahead in every interleaved Game Mode launch (≥ 6); no minimum size | Game Mode A/B |
| Exactness | Control checkpoints | Identical in all five render_bench scenes; verify mode clean where it applies | render_bench |

Counting must be opt-in (a diagnostic build or switch) and must not change the shipped module.

## Decisions

- **Count before changing:** the first deliverable is data.
- **Ceiling before code:** any candidate is bounded by a diagnostic build first, as in #503's M4.
- **Acceptance** follows #503's rule: consistent, exact, tested; size is not a bar.
