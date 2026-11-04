# PRD — Direct-call gate

Status: In progress. The gate is implemented, opt-in and unit-tested; measurement is next.
Source: [#503](https://github.com/chrissotraidis/bluewake/pull/503)'s stack (`b53855071`). Measured on the [#437](https://github.com/chrissotraidis/bluewake/pull/437) recipe (`perf/clang-runtime-matrix@14c558d`: `main` with #503, #504 and #531, Clang 22.1.8, libc++).
Scope: the cost of the check before each direct call between translated chunks. Excluded: what the check decides, which stays the same.
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)

## Problem

Outset on the Steam Deck is still under the game's 30 FPS, with the game thread the limit (#503). Before every direct guest call, translated code asks `bw_direct_call_ready_plain` whether it may skip the dispatch loop, and there are about a billion of these calls in a long run.

A profile of the #437 build shows what that costs:

| | Share of all samples |
|---|---:|
| Translated chunks | 58.6% |
| Native routines | 12.2% |
| **Direct-call checks** (`bw_direct_call_ready_plain`/`_full`, `bw_host_quiet`, `host_can_skip_observation`) | **8.4%** |
| Dispatch loop | 5.0% |

Most of the check's samples land on its entry and its first pointer load. That's the call itself, and fetching it: the helpers are `static`, so the module has 840 copies of each, one per chunk.

## What earlier work rules out

#503 cut the check's work but always kept the call:

| #503 step | What it did | Result |
|---|---|---|
| v1 | The whole check inlined | Chunks about 11% bigger, and slower |
| M1-T04 ceiling | The check cut to 70 bytes, still a call | +3.1%, mostly from taking interrupts late, which isn't exact |
| Stack 1 | Range tests settled at build time | +0.7%, retained |
| M4-T01 | One host word instead of the four interrupt reads, still a call | ≤ 0.3%, inconsistent: no-go |

What was never tried is a few instructions inline, with the exact check kept out of line.

## Goal

Answer the common case of the check inline, in a few instructions, without changing any answer:
- every call the gate lets through, the full check would have let through;
- any other call takes today's check.

## Success and constraints

| Metric | Target | Validation |
|---|---|---|
| Correctness | Every answer equal to the full check's | Unit test over all host and CPU states; `BLUEWAKE_DIRECT_SKIP_VERIFY=1` with zero unsafe gate answers |
| Guest behaviour | Checkpoints identical to the control | render_bench, five scenes |
| Outset game FPS | Ahead of the control in every interleaved Game Mode launch (at least six); no minimum size | Game Mode launches alternating control and candidate |
| Risk to players | None by default | Opt-in: `BLUEWAKE_DIRECT_GATE=1` |

## Decisions

- **Opt-in (2026-11-04):** off unless `BLUEWAKE_DIRECT_GATE=1`, until it's measured. A Config → Performance switch comes once it has a result (the section lives in #531).
- **Own PR, stacked on #503:** it builds on stack 1's plain check. **Merge #503 first.**
