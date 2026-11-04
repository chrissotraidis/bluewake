# TECH_PLAN — Direct-call gate

Status: In progress. Implemented, opt-in (`BLUEWAKE_DIRECT_GATE=1`) and unit-tested; measurement next.
[Product brief](PRD.md) · [Tracker](PROGRESS.md) · Stacked on [#503](https://github.com/chrissotraidis/bluewake/pull/503)

## Why

Every experiment in [#503](https://github.com/chrissotraidis/bluewake/pull/503) kept the check as a call: the M1-T04 ceiling (70 bytes, +3.1%, mostly from taking interrupts late), stack 1 (+0.7%) and the M4-T01 word (≤ 0.3%). Inlining the *whole* check made the chunks about 11% bigger and slower (v1). What was never tried is a few instructions inline, with the exact check kept out of line.

A profile of the #437 build (Clang 22, this stack with #504 and #531) shows the call is still a large cost:
- **The setup:** desktop PC, Outset headless and unpaced, `perf record`.
- **The share:** the direct-call checks are **8.4% of all samples** (`bw_direct_call_ready_plain`/`_full`, `bw_host_quiet`, `host_can_skip_observation`).
- **Where the samples land:** mostly on the function's entry and its first pointer load. That's the call and fetching it, not the tests.
- **The copies:** the helpers are `static`, so the module holds 840 copies of each, one per chunk.

## Design

One host word, the gate, read inline before a direct call to a constant target in the main executable's code, outside both ranges.

| Byte | Meaning | Set by |
|---|---|---|
| 0 | The interrupt sources are dirty | The host's existing flag: **its storage is this byte**, so every write updates the word |
| 1 | A decrementer exception is pending | The same: the existing flag lives here |
| 2 | Closed: the address-only answer is off or not simple, or a single address lies in the main code | `host_skip_fast_refresh` |
| 3 | PI interrupt pending (`pi_cause & pi_mask`) | `host_direct_gate_pi`, where the dirty flag clears and before each dispatch |

- **Word zero:** every host-side test of the plain check holds, so only the depth, an exception and the budget remain inline. With nothing pending, the guest's MSR[EE] can't matter.
- **Word non-zero:** the call takes today's out-of-line check. That's exact; at worst it's a missed shortcut.
- **The singles:** both are REL raw-image addresses, so a constant target in the main code can't equal them. The host closes the gate if one ever lies there.
- **The PI registers:** they change only while the sources are dirty (#503 M4-T02) or in the main loop between turns (the PE commits). That's why the PI byte is taken before each dispatch.

**Opt-in:** `BLUEWAKE_DIRECT_GATE=1` hands the word over; otherwise the gate stays closed. The builder's `--direct-gate` trains with it open, so a measurement build profiles the path it measures. The default training fingerprint is unchanged.

## Changes

| File | Change |
|---|---|
| [`direct_skip.h`](../../../cmake/composite/direct_skip.h) | `BwHostGate`, the main-code bounds |
| [`direct_calls.h`](../../../cmake/composite/direct_calls.h) | `bw_direct_call_ready_main`: the gate inline, else the plain check; the full check cross-checks the gate under verify |
| [`direct_calls.c`](../../../cmake/composite/direct_calls.c) | The gate pointer (closed unless direct calls and the answer are on and verify is off), the `bluewake_composite_direct_gate` export, the verify counts |
| [`main.c`](../../../runtime/host/src/main.c) | The two flags stored in the word; the closed and PI bytes; the handover behind `BLUEWAKE_DIRECT_GATE=1`; `[direct-gate] verify:` at exit |
| [`build.py`](../../../scripts/linux/build.py) | `--direct-gate` for training; fingerprint and provenance |
| [`direct_calls_test.c`](../../../tests/direct_calls_test.c) | The gate against the full check in 1,024 states; verify catches a stale byte; the conservative side; every path that closes it |

## Verification

| Level | Check |
|---|---|
| Unit | The above, at -O0 and -O2, in Windows and Linux CI |
| Runtime | `BLUEWAKE_DIRECT_SKIP_VERIFY=1` with the gate handed over: zero unsafe gate answers in Outset and a scene change |
| Exactness | render_bench: guest checkpoints identical to the control in all five scenes |
| Performance | Ahead of the control in every interleaved Game Mode launch (at least six); no minimum size |

## Risks

- **Code growth.** About ten instructions at each of about 32,000 call sites. [#503](https://github.com/chrissotraidis/bluewake/pull/503)'s v1 shows growth can cost more than it saves, and only the A/B answers that.
- **A PI writer this design doesn't know about.** The verify counter would show it as unsafe answers, and the checkpoints would drift.

## Implementation order

| Step | Depends | Deliverable |
|---|---|---|
| M0 | #503 | The remaining cost attributed (done) |
| M1 | M0 | The gate, opt-in, with its unit test (done) |
| M2 | M1 | The candidate built on the #437 recipe and trained with the gate open; verify run |
| M3 | M2 | Desktop first read, then the Deck Game Mode A/B and render_bench checkpoints |
| M4 | M3 | Keep opt-in, propose as the default, or no-go |

## References

- [#503 TECH_PLAN](../2026-10-29-outset_cpu_followup/TECH_PLAN.md): stack 1, the M1-T04 ceiling, the M4 interrupt-word specification and its writer table
- [`docs/architecture/GameThread.md`](../../architecture/GameThread.md): the dispatch loop and direct calls
