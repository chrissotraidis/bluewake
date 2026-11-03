# BlueWake goal loop — v15 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of BlueWake.app with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the
keyboard without a crash, and time-to-playable is under five minutes, with the
five governing numbers holding on this host.

## What changed since v14

V14 predicted that the cap/digest coupling was delivery *identity* and not a
cycle stamp, and that any divergence would sit in the cycle field. Half of that
is now measured, and it is sharper than predicted: **over the entire range the
digest can see, there is no divergence at all.**

1. **The recorded digest covers the early route and only the early route.**
   `history=1024`, and delivery ordinal 1024 lands at cycle **44,638,873** of a
   route total of **114,210,000,002** cycles — four hundredths of one percent.
   The 1,050-record digest is a boot/title-phase invariant. It is structurally
   blind to the play scene, which is where v13 watched it move.
2. **The dynamic cap is byte-identical to cap-256 across ordinals 1024–1600.**
   All **577/577** traced records agree on `cause`, `pc` and `context` **and on
   the cycle itself**; the per-record cycle delta is 0 everywhere, over cycles
   44,638,873–79,429,169. The cap does not change the guest in this region, not
   even its interrupt latency.
3. **And it is faster where it counts.** Wall clock to the same guest frame,
   both runs, same host binary, same card, same 14,100 retraces:

   | retrace | cap-256 | dynamic | saved |
   | --- | --- | --- | --- |
   | 1,000 | 105.02 s | 86.85 s | 17.3% |
   | 2,000 | 170.79 s | 137.17 s | 19.7% |
   | 3,000 | 236.53 s | 188.22 s | 20.4% |
   | 3,393 | 262.70 s | 208.45 s | **20.6%** |

The reading that fits both facts is that the dynamic policy is **not** a large
fixed cap. It grows the cap only where turns are expensive, so the cheap early
route still runs at control granularity — which is exactly why the trace is
identical there — and the win appears on the phase that costs wall clock.
H-DELIVERY-IDENTITY therefore holds, exactly and not merely bounded, over the
region the digest observes. The region it cannot observe is the one still owed.

## Hypotheses this loop owns

* **H-DELIVERY-IDENTITY** — status: holds over the recorded range, 577/577 with
  zero cycle shift. Owed: the same test across the play window, on the ordinals
  that fall there, because that is where v13 recorded movement.
* **H-CAP-TRADE** — the cap's turn reduction is concentrated where turns are
  expensive, so the win is available on the phase that owns playability without
  a behaviour change. Falsified if the play-window fps of the dynamic run is
  below its own live control.
* **H-DELIVERY-PHASE** — the follow-on, and now the natural one: if the win is
  real, decouple the delivery-question granularity from the turn length so a
  long turn still asks the interrupt question at the PC a short turn would have
  asked it at. That makes the win free rather than traded against interrupt
  latency.
* **H-GUEST-DISPATCH** — still the long pole: 58–62% of both phases, flat, top
  symbol 14.0%, spread thin. The only lever big enough to move time-to-playable
  by a factor.

## Pre-registered predictions for the run now in flight

`/tmp/bw-dyncap-b`, `BLUEWAKE_BENCH_CYCLE_CAP=dynamic`, 14,100 retraces,
`BLUEWAKE_DELIVERY_TRACE=1024:1600`, against the completed cap-256 control at
`/tmp/bw-cap256-b`. Registered before the run finishes:

1. `[delivery-hash] external=` identical to the control's **146,755**.
2. `no_cycle=` identical to the control's **D30E92D828D75F6B**. This is the
   hypothesis.
3. `cycle_sum=` no smaller than the control's **8,146,892,685,758,348**, and any
   excess bounded by `external × 1024`.
4. Host turns below **820,034,526** by at least 20%.
5. Play-window median fps (13910:14100) no worse than the control's **3.03**.

## The fork, restated against today's evidence

* **1–4 hold and 5 holds** — the cap is adoptable as a measured
  time-to-playable win with no behaviour change, and H-DELIVERY-PHASE becomes
  the next build.
* **1–3 hold and 5 fails** — the cap buys boot time and costs play latency. Not
  adoptable by default, and H-DELIVERY-PHASE stops being optional.
* **2 fails** — dynamic is rejected on the evidence, in one short section, and
  the work returns to H-GUEST-DISPATCH without further cap work.

Re-recording a digest so a run passes stays forbidden on every branch.

## The numbers on this host

| number | cap-256 control (measured today) | dynamic |
| --- | --- | --- |
| route digest | `2b7a1fa5…` | pending |
| host turns | 820,034,526 | pending |
| delivery identity (`no_cycle`) | `D30E92D828D75F6B` | pending |
| traced 1024–1600 | 577 records, cycles 44,638,873–79,429,169 | **identical, delta 0** |
| seconds to retrace 3,393 | 262.70 | **208.45 (−20.6%)** |
| median fps, window 13910:14100 | 3.03 | pending |
| time-to-playable | **695.8 s ≈ 11.6 min** | pending |

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. A document is not an iteration.
A profile is a cost paid to find the work, never progress by itself. Report to
the user every three iterations in one paragraph.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86…`,
  is verified byte-identical to its backup before every run.
* One BlueWake process at a time. No disk-heavy work — repo-wide scans,
  `lldb`, `atos`, `otool` — while a measurement run is live.
* No silent stubs, no no-op substitutes, no weakening or deleting a test
  because it exposes a failure. **A synthetic button press is a route driver,
  not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass.
* Never write that no product gate advanced as a successful outcome; that
  sentence is the failure signature of this project.

## The work, in order

1. **Finish the pair and read the five numbers.** Commit the result and the
   `cycle_invariance.py` line with it. No other work while it runs.
2. **Move the trace window into the play scene and repeat the identity diff.**
   The recorded range proves nothing about the play window, and the play window
   is where v13 saw the digest move. Map ordinal to retrace (the delivery
   ordinal is monotone in cycle, so a one-off trace of the play-scene ordinal
   range prices this), then diff identity at the ordinals that fall in
   13910:14100. This is the measurement that actually settles the hypothesis.
3. **Then, and only then, H-DELIVERY-PHASE** if the win is real: make the
   interrupt question independent of turn length, so long turns stop paying for
   their length in delivery latency.
4. **H-GUEST-DISPATCH.** First measurement before any recompiler edit: the cost
   of a generated block and of one memory operation on this M2. The generated
   loop accounting (`ctx->downcount -= N` with `DOLRECOMP_C_LOOP_CYCLE_BUDGET`)
   and the per-instruction memory helpers (`mem_write32` → `get_ram_ptr` +
   `clear_matching_reservation` + a `g_mem_write_journal` global check +
   `write_be32`, `ref/recompcore/GXRuntime/include/core/cpu.h:291`) are the two
   named targets. Measure, then choose.
5. **H-DSP-THREAD** — 28% of the intro, about 7% of the play scene, holding the
   digest and the paced-PCM fingerprint.
6. **The owed v13 residue:** the controlled "one press moves Link N units"
   displacement test, which needs a player-position trace; then M1c, M2, M3 and
   H1/H2/H3 as recorded in v4.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes
and Y reaches Outset; here is the screenshot of Outset with Link under your
control.* Silence is not acceptable; neither is a document about why it cannot
be done.
