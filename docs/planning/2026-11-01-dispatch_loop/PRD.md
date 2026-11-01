# PRD — Dispatch-loop edges in Outset

Status: Draft (planning)  
Source: BlueWake `main` at `6b429fea9e60`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503)  
Scope: the game-thread cost of control transfers that still go round the module's dispatch loop. Excluded: the direct-call check (settled in #503), game-function hooks.

## Problem

A transfer from one translated chunk to another either runs directly or returns to the chassis loop ([`dispatch_loop.h`](../../../cmake/composite/dispatch_loop.h)). Going round the loop means the loop checks the turn, asks the host's edge service, looks up the target and dispatches it. Direct calls ([#234](https://github.com/chrissotraidis/bluewake/pull/234)), returns into another chunk ([#235](https://github.com/chrissotraidis/bluewake/pull/235)) and the exact PC-keyed dispatch cache ([#448](https://github.com/chrissotraidis/bluewake/pull/448)) already remove most of it. In #503's Outset profile (Steam Deck, Game Mode, stage select off) the loop still costs about **4.3 M cycles per game frame**, about 3% of the frame:

| Symbol | M cycles/frame |
|---|---:|
| `chassis_dispatch` (module) | 2.3 |
| `host_chassis_requires_full` (host) | 1.0 |
| `bw_call_translated` (module, indirect calls run directly) | 0.6 |
| `bw_find_chunk` (module) | 0.3 |

Nobody knows yet which transfers these are. Without that, any change is a guess, and #503 showed guesses here are easy to get wrong.

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
