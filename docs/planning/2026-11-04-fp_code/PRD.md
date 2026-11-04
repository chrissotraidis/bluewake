# PRD — Floating-point code in the translated game

Status: In progress. M1 (one FP-available test per stretch of a prepaid copy) is implemented, opt-in and unit-tested; the measurement build is next.
Source: BlueWake `main` at `ee991d312`. Measured on the [#437](https://github.com/chrissotraidis/bluewake/pull/437) recipe (`perf/clang-runtime-matrix@14c558d`: `main` with #503, #504 and #531, Clang 22.1.8, libc++).
Scope: the host code the translation emits around guest FP instructions. Excluded: what any instruction computes, which stays bit-exact.
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)

## Problem

Outset on the Steam Deck is still under the game's 30 FPS, with the game thread the limit ([#503](https://github.com/chrissotraidis/bluewake/pull/503)). Its profile is flat, so the gain has to come from something most translated code pays.

FP is that cost. A profile of the #437 build at Outset (desktop, headless, the six hottest chunks resolved to guest instructions with line tables):

| Guest instruction class | Share of the hottest chunks' time |
|---|---:|
| FP arithmetic | 22.4% |
| FP loads and stores | 14.0% |
| Integer loads and stores | 23.1% |
| Integer ALU | 15.5% |
| Cycle bookkeeping | 12.3% |

- **Per operation:** one `ps_madd` (two multiply-adds) compiles to about 117 x86 instructions.
- **Where the cost is:**
  - a `pc` store and the MSR[FP] test on every FP instruction;
  - every operand reloaded after the special-value fallback rejoins;
  - finiteness and NaN tests;
  - the FPRF classification of every result.
- **FP loads and stores skip the lean memory path:** the per-instruction FP test makes every one of them ineligible for `lean_memory.py`, so each still stores `pc` and the cycle suffix and checks the deadline.

## Goal

Cheaper emitted code around guest FP instructions, with every guest-visible result, flag and register unchanged, one measured step at a time.

## Steps

| Step | Change |
|---|---|
| M1 | One FP-available test per stretch of a prepaid copy, so FP loads and stores become lean (this PR) |
| M2 | `lfs`: zeros through the hardware's widening (denormals stay on the generic path: the host's DAZ is on in non-IEEE mode) |
| M3 | ~~`frsp` and `fctiw` inline~~: no-go, Deck −0.36% (desktop +0.6%); reverted |
| M4 | A special-value fallback that doesn't clobber the guest registers |
| M5 | Lazy FPRF: the result class computed only where the FP status register is read |
| — | ~~`psq_l`/`psq_st` specialized on the quantization registers~~: dropped, the generic path is 0.09% of samples after M1 and M2 |

Each later step is planned and measured after the one before.

## Success and constraints

| Metric | Target | Validation |
|---|---|---|
| Correctness | Exact: no guest-visible difference | Unit tests; guest checkpoints identical to the control |
| Outset game FPS | Ahead of the control in every interleaved Game Mode launch (at least six); no minimum size | Game Mode launches alternating control and candidate, render_bench five scenes |
| Risk to players | None by default | Opt-in builder option (`--fp-checks` for M1) until measured |

## Decisions

- **Opt-in:** each step is a builder option, off by default, until it's measured.
- **Exactness first:** the per-stretch test keeps the first test of each stretch at its place, so an FP-unavailable trap (the Dolphin OS switches FPU state lazily) behaves exactly as before.
