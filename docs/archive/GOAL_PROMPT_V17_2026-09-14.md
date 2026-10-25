# BlueWake goal loop — v17 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of BlueWake.app with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the
keyboard without a crash, and time-to-playable is under five minutes, with the
five governing numbers holding on this host.

## What changed since v16: three of its four asks are done, and its conclusion
## about the cap is wrong

**D1 landed.** `scripts/route_digest.py` canonicalises the `hash=` field of
`[cycle-delivery] summary`, and both completed caps produce
`0d4cee87923dea4bc09ef89af34ed438c1d9542368058423c722c3ad50bf7b7a`. The
canonicalisation was verified in both directions with mutation tests - a moved
boot milestone retrace, a changed `external=` count, a changed `dsp=` cadence, a
changed `first_dsp_cycle` and a moved stop-line PC all still move the digest.
Only the timing aggregate is canonicalised away.

**The cap is the shipping default**, including the built-in default in `main()`,
so a double-click with no exported variables gets the measured policy. The fresh
default run at `/tmp/bw-v16-default` holds the new digest UNCHANGED at
521,121,740 turns, stops normally at `0x80307EF4`, and takes 594.4 s wall with a
7.74 fps play-window median against the control's 3.03.

**The product path answered the report.** `BlueWake.app` opened from an empty
environment, Aurora window up, no synthetic pad pulse anywhere: a real J took the
title to `file-select` at retrace 1,060, real J/RETURN/J walked Name Entry, and
the route reached `new-game-intro` 18,763, `opening-complete` 31,838 and
`play-scene` 31,898. On the play scene a real held W reached the guest as
`jut_hold=0x8000000` and a real RETURN as `cpad_trig=0x0010`. The process then ran
on past retrace 33,486 without a crash. Frames are in `/tmp/bw-v16-outset`.

**And v16's closing sentence is wrong.** v16 wrote that the remaining 4.9 min
"cannot come from the cap, which is now exhausted as a lever." It is not
exhausted, and the profile taken this loop says so.

## The correction: the cap was adopted as a pair, and only half of it was swept

The adopted policy is not a cap. It is a pair: 256 inside a 1,024-cycle deadline
horizon, 1,024 outside it (`main.c:3647`, `cycle_domain.c:bounded_budget`). The
route's mean turn length is therefore **219 guest cycles** - and 219 is *below*
256, which says the near band owns almost every turn. The 1,024 outside it was
worth 36.5% of all host turns on its own, and **nothing above 1,024 has ever been
measured.** The promoted constant came from an existing 2026-08-31 mode that
already chose 256/1024; it was never swept, and the census is explicit that the
cap, not a device, owns 97.9% of budget decisions, so the turn length is a lever
and not a device-fixed quantity.

The gate that made cap work circular is gone. v14 already decided the
interrupt-acceptance schedule must not gate the digest; v16 found that the
schedule was still reachable inside the record set and removed it; D1 is
verified. So a cap change can now be measured against a digest that gates only
guest state, which is exactly the instrument this lever always needed.

## The profile this loop took, and what it prices

`sample` of the live product run at retrace ~33,400, 6 s at 1 ms, 4,870
main-thread samples (`/tmp/bw-v17-playscene-sample.txt`):

| share | symbol | reading |
| --- | --- | --- |
| 13.6% | `main` self | the per-turn host wrapper's own body |
| 5.4% | `host_cycle_deadline_distance` | the budget evaluation, ~2.8x per turn |
| ~4% | `host_refresh_interrupt_sources`, `dol_interrupts_set_source`, `dol_di_interrupt_pending` | interrupt-source refresh, once per turn |
| 0.8% | `bluewake_card_runtime_dispatch` | once per turn |
| 0.7% + 0.5% + 0.5% | `bluewake_cycle_domain_end_turn`, `host_sync_cycle_devices_end_turn`, `host_alias_rel_pc` | once per turn |
| **~26-30%** | **all of the above** | **paid once per turn, 521,121,740 times** |
| ~40% | the `func_*` corpus, `func_802416E0` 10.5% at the top, then `func_8003D6E0` 6.8% | paid per executed guest instruction |
| ~8% | `DSP::Interpreter::Step` and its operands | 28% of the intro, ~7% of the play scene |
| ~5% | `gxruntime::aurora_recomp` + GX shadow frontend | the render path on this windowed run |

The arithmetic this loop has to respect follows directly. A mean turn of 219
cycles is 219 cycles of guest work bought with 26-30% of wall time; the wrapper
share divides by the turn length and the corpus share does not. **An infinite cap
still leaves ~70% of the current 594 s, so the cap cannot reach the target alone
- and the corpus cannot be skipped either.** 594 s -> under 300 s is 1.98x, of
which the cap can plausibly return 1.25-1.35x and the corpus and the DSP have to
find the rest.

## Hypotheses this loop owns

* **H-TURN-COST** - new, and it is the profile read as a quantity: the same
  wrapper runs 521,121,740 times and costs 26-30% of wall time, so a turn that
  covers twice the cycles should cost a nearly constant amount more. Falsified
  if doubling the mean turn does not halve the wrapper's share.
* **H-CAP-SWEEP** - new: the 1,024 far bound is an unswept constant. Flat 1,024,
  then 2,048, then 4,096, each measured on the identical route with the D1
  digest as the falsifier. Registered prediction, before the runs: flat 1,024
  lands between 130M and 240M turns with the digest unchanged at
  `0d4cee87...` and time-to-playable at or under 500 s. If the digest moves, the
  turn length is visible in guest state after all and the sweep stops there.
* **H-DSP-THREAD** - unchanged and now the largest single share on the phase
  that owns the metric: 28% of the intro, and 94% of time-to-playable is the
  intro. Worth up to ~1.2x on the governing number, at the cost of a real
  determinism argument, so it is ranked below the sweep and above the corpus.
* **H-CORPUS** - replaces v16's H-GUEST-DISPATCH as stated. v16 named two
  targets to measure first: the generated loop accounting and the per-instruction
  memory helpers (`ref/recompcore/GXRuntime/include/core/cpu.h:291`). The
  profile adds the one the previous loops never named: **every guest store goes
  through a pointer the compiler cannot prove does not alias `CPUState`**, so it
  reloads guest registers after each one. The 206 DOL chunk translation units
  are built `-O2 -ffp-contract=off` with hidden visibility and no LTO
  (`cmake/composite/CMakeLists.txt`), so nothing is inlined across them either.
  Measure the aliasing effect on one hot function before touching the build.

## The work, in order

1. **The cap sweep, before anything else**, because it needs no rebuild, it is
   one environment variable, and it is the only lever whose price is already
   known. One run per step, digest-gated, `BLUEWAKE_BENCH_CYCLE_CAP` only. Update
   `scripts/bench.sh` and `scripts/play.sh` and `main()` only for a step that
   passes, and only with the run that passed.
2. **The per-turn cost as a number.** H-TURN-COST is the reason the sweep is paid:
   turns and wall seconds together give the cost of a turn at two turn lengths.
   Report it as such, not as a headline fps.
3. **H-DSP-THREAD.** The intro is the phase that owns time-to-playable and the
   DSP is its largest removable share. The paced-PCM fingerprint and the digest
   both have to hold.
4. **H-CORPUS**, measured first: take one hot generated function and one memory
   operation and price them on this M2 before changing a build flag or a helper.
   The aliasing question is the first measurement, not a rewrite.
5. **The owed v13 residue:** the controlled "one press moves Link N units"
   displacement test, which needs a player-position trace; then M1c, M2, M3 and
   H1/H2/H3 as recorded in v4.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an iteration.**
A profile is a cost paid to find the work, never progress by itself. Report to
the user every three iterations in one paragraph.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86...`, is
  verified byte-identical to its backup before every run.
* One BlueWake process at a time. No disk-heavy work - repo-wide scans, `lldb`,
  `atos`, `otool` - while a measurement run is live.
* No silent stubs, no no-op substitutes, no weakening or deleting a test because
  it exposes a failure. **A synthetic button press is a route driver, not
  evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass.
* Never write that no product gate advanced as a successful outcome; that
  sentence is the failure signature of this project.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*
Silence is not acceptable; neither is a document about why it cannot be done.
