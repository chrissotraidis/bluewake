# BlueWake goal loop - v12 (2026-09-14)

Supersedes v11 ([GOAL_PROMPT_V11_2026-09-14.md](GOAL_PROMPT_V11_2026-09-14.md)). The PRD still defines what
finished means. V11 said the gate is the play scene and the first act is to profile it. The profile is now
taken, and it moves the aim in one place and confirms it in another.

## The claim this loop is built on

The objective has two gates, and they live in two different phases.

* **Can a human play it.** The gate is controllable Outset gameplay. The phase is the **play scene**, and it
  is the slowest measured phase, 2.76 fps. Its profile is now in hand.
* **Under five minutes.** The gate is time-to-playable, 15.3 min, and **90.9% of that is the new-game intro**,
  not the play scene. The play scene begins at retrace 13910, *after* the control boundary at 13850.

V11 named only the first gate and called the play scene "the gate." Both are real gates, but only one of them
owns the governing number. The loop's mistake now would be to spend the budget on the phase that is not the
one losing the minutes.

## The play-scene profile, taken

One `sample` of the emulation thread, 6 s on the running bench process at retrace 13,938, inside the play
window. 4,680 main-thread samples, call graph at line 23, binary images at line 931.

| share | samples | where |
| --- | --- | --- |
| **62.0%** | 2,900 | `bluewake_composite_dispatch_until_boundary` -> `selected_dispatch` -> recompiled guest code |
| 14.4% | 676 | `host_sync_cycle_devices_end_turn` -> `DSP::Interpreter::Step` (7.2% DSP total) + `Adapter::run_cycles`, audio DMA, VI, DI |
| 10.5% | 493 | unmapped `main+29616` -> inlined `main+99060`: the PI-interrupt path and draw-tag work |
| 3.6% | 170 | `host_refresh_interrupt_sources` -> `host_cycle_deadline_distance` -> `dol_event_clock_next_deadline` |
| 3.4% | 157 | `main+9480` inlined + `bluewake_card_runtime_dispatch` |
| 2.1% | 96 | `bluewake_cycle_domain_prepare_dispatch` -> `host_cycle_deadline_distance` |
| 1.0% | 45 | `bluewake_cycle_domain_end_turn` -> `host_cycle_advance_clock` |

Guest code tops out at 654 samples, `func_802416E0`, 14.0%, and falls away immediately: `func_8003D6E0`
393, `func_802456E0` 167, `func_803256E0` 139. The GX FIFO writer, `func_803256E0`, is **3.0%** here
against 21% on the title path.

**Two findings that decide the loop.**

First, the play scene is **guest-code-bound, not render-bound**: 62% is genuine execution of recompiled guest
code, spread thin, with nothing above 14%. The render share that led v11 on existing evidence is 3%. So
**H-RENDER-OFFLOAD is deprioritized** - its ceiling on this phase is the 3% the writer costs - and
**H-DSP-THREAD is small here too**, about 7% against 28% in the intro. **H-GUEST-DISPATCH is the long pole**
on this phase, and it is a recompiler problem, not a host-code problem.

Second, **about 20% of the play scene is host-side per-turn interrupt, deadline and device-sync bookkeeping
that this project owns**: `host_sync_cycle_devices_end_turn` 14.4%, the PI-interrupt path 10.5%,
`host_refresh_interrupt_sources` 3.6%, the deadline recomputation reached from
`bluewake_cycle_domain_prepare_dispatch` 2.1%. This is host code, it is reachable from six call sites in
one file, and it is the cheapest lever the loop has ever found. **H-INTERRUPT-CHURN** is new in this loop.

This is not dispatch-loop overhead. `cmake/composite/dispatch_loop.c` is 33 lines, `dispatch()` is called
once, and `edge_service` is NULL so the loop returns immediately. The 62% is guest execution.

## The arithmetic the loop has to face

The phase table, measured on the product path:

    boot            1 -> 773        772 frames     84.0 s    9.19 fps
    new-game intro  773 -> 13850  13077 frames    835.8 s   15.65 fps   <- owns the number
    play scene    13910 -> 14992   1082 frames    392.3 s    2.76 fps   <- owns playability
    whole path      1 -> 14992    14991 frames   1316.1 s   11.39 fps

Time-to-playable is 919.8 s, and 835.8 s of it (90.9%) is the intro. Under five minutes means 13,850
retraces in at most 300 s, which is **46.2 fps average against the 15.06 fps measured: a 3.07x compounded
speedup**. The largest single share on the owning phase is the intro's DSP at 28%, and parallelizing all of
it is 1.39x by itself. The play scene needs its own multiple for the other clauses.

So no single lever reaches the number. This loop's unit of progress is **one measured share, removed or moved
off the emulation thread, with the digest held**, and reaching the objective takes several of them. Saying
that out loud is the point; a loop that hunts for the one change that fixes 3x will spend another ten days
not finding it.

## Retired by measurement, do not reopen

* **H-SAVE-CONTINUE - killed.** The route card is byte-identical to the user card except 14 header bytes, so
  the route already runs the new-game flow on a card that strings reports as holding save data. There is no
  distinct Outset save to Continue into.
* **H-DSP-SHADOW - falsified.** With the LLE DSP adapter off the guest stalls in JAS before the title:
  `title_ready=0`, every milestone zero, `saved_pc=0x80307EAC` waiting on a DSP interrupt that never comes.
  The adapter is load-bearing; its cost can be parallelized, not deleted.
* **H-INTRO-HOLD - falsified.** A real OS-delivered four-second held START did not end the intro.
* **H-RENDER-OFFLOAD as the lead - deprioritized, not killed.** 3.0% of the play scene, against 21% of the
  title path. It may matter later; it does not lead now.

## The census is already built, so the first act is free

The host already carries an opt-in counter for exactly H-INTERRUPT-CHURN. `host_cycle_deadline_distance`
attributes every dispatch budget to the term that bound it - VI, DSP, audio DMA, decrementer, the one-cycle
clamp, or the cycle-domain cap - and counts how many evaluations returned the one-cycle distance, which is
the signature of churn. It is inert unless `BLUEWAKE_DEADLINE_CENSUS` is set, it writes only to stderr at
exit, and it never touches route state. `BLUEWAKE_DEADLINE_CENSUS_WINDOW` counts from a chosen retrace
onward.

One product-path run with the census set therefore sizes H-INTERRUPT-CHURN on a real phase before any code
changes, in the same process, for the cost of a run the loop was going to take anyway. Read it as a decision:
a high one-cycle fraction means the deadline term is being re-derived far more often than it changes, and
that is a coalescing change; a low one means the 20% is spread across the device sync itself and the lever is
thinner than the samples suggest.

## The gap still owed, and it is cheap

*"Double-click BlueWake.app, press J once when PRESS START appears, and the file menu comes up."*

No frame exists yet of Link driven by a real key press, and the file menu was reached by the synthetic pad
pulse. A synthetic pulse is a route driver, not keyboard evidence.

The title screen is at about 84 s, not 15 minutes, so the first clause of the report can be retired in a run
of two or three minutes with no synthetic pulse at all: launch with `BLUEWAKE_INPUT_PROBE=1`, focus the
window, press J, and require `[input-probe]` to show nonzero `kbFocus` **and** `[input-chain]` to show
`cpad_trig=0x0100`, with a captured frame of the menu. Anything less is not evidence.

## Harness facts worth not rediscovering

* **Launch.** `open build/runtime-host-dsp/BlueWake.app` is what a double-click does; instrumented is
  `open -n --stdout F --stderr F --env NAME=VALUE ... build/runtime-host-dsp/BlueWake.app`. `open` reaps
  its child when the launching shell exits, so keep the launching shell alive for the whole run.
* **One BlueWake process at a time.** `scripts/bench.sh` refuses to overlap with `bluewake_host`; honour
  that by hand too.
* **A real key press.** `/tmp/bwfocus PID 400` immediately before every press, then
  `/tmp/bwkey PID <keycode> <hold_ms>`. Key codes are macOS virtual keycodes and the app reads them through
  SDL: **38 is J (A), 36 is RETURN (START), 40 is K (B), 32 is U (X), 34 is I (Y), 16 is Y, 6 is Z**. v9, v10
  and v11 all carried `37` for U; the binding table at
  `ref/recompcore/GXRuntime/backends/aurora/aurora_input.cpp:20` is `SDL_SCANCODE_J -> PAD_BUTTON_A`,
  `U -> PAD_BUTTON_X`, `RETURN -> PAD_BUTTON_START`. A press counts only with nonzero `kbFocus` and the
  expected `cpad_trig`; do not gate on `msFocus`, which reads 0x0 on presses that land.
* **Rate, measured not guessed.** `BLUEWAKE_FRAME_TIMING=1` prints `[frame-timing] retrace=N us=T` on every
  retrace. Do not quote a rate that did not come from that line or from `scripts/bench.sh`.
* **Frame evidence.** `BLUEWAKE_CAPTURE_OPENING_FRAME` with `BLUEWAKE_CAPTURE_RETRACE` reads back the guest
  framebuffer at an exact retrace, and `BLUEWAKE_CAPTURE_INTERVAL` re-arms it into a labelled series. Every
  PPM and PNG stays in /tmp.
* **Do not run disk-heavy work while a measurement run is live.** A repo-wide scan, `lldb`, `atos` and
  `otool` during the last bench's live window dragged its median to 1.94 fps against the 3.10 fps baseline.
  The run's digest and turn count were still exact and are usable; its fps numbers are not, and were not
  recorded as a baseline.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path, or a human can now see, hear
or do something they could not before. A profile is a cost paid to find the work, never progress by itself.

One hypothesis, one small change, one build, one measurement, and at most one short section appended to
`docs/archive/status/CURRENT.md`. No status document may be longer than the change it describes. Never add a new
tier, slice or patch for a unit that already compiles whole.

If an iteration ends with no change to an observable and no change to a number it **failed**: revert it and
change the mechanism, not the constant. Three failures of the same shape means stop, measure the layer
underneath, and say so out loud. Report to the user every three iterations in one paragraph.

## The governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reproduces the 2026-09-01
baseline.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |
| **time-to-playable** | **15.3 min measured 2026-09-14**, 919.8 s; target under 5 min, 46 fps sustained over 13,850 retraces |

The digest, the turn count and the card are host-independent; fps is not. A/B only against numbers measured
on the host you are standing on. A change that buys fps by moving the digest is a behavior change, not a
result. The play scene is retraces 13910-14992 on this route; 13910:14100 is the bench's live window.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git, every
  PPM and PNG stays in /tmp, and `ref/` is gitignored so an edit there ships as a patch under `patches/aurora/`.
* Preserve user data. The save card is at `~/Library/Application Support/BlueWake/GZLE01.card`; drive the
  new-game flow against a copy and keep the /tmp backup current.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. **A synthetic button press is a
  route driver, not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass. Never write that no product gate advanced as a successful
  outcome; that sentence is the failure signature of this project.

## The work, in order

1. **Take the deadline census on the product path** with `BLUEWAKE_DEADLINE_CENSUS=1` and a window in the
   phase under test, and turn the source split and the one-cycle fraction into a yes or no on H-INTERRUPT-CHURN.
   No code change, one run.
2. **Retire the first clause of the report**: a real OS-delivered key press on the title screen brings up the
   file menu, proven by `[input-probe]` focus, `[input-chain]` `cpad_trig=0x0100`, and a captured frame.
   Then drive the rest of the route by keyboard.
3. **H-DSP-THREAD.** The largest single share on the phase that owns the governing number, 28% of the intro.
   Parallelize it off the emulation thread; hold the digest and the paced-PCM fingerprint.
4. **H-INTERRUPT-CHURN**, at whatever size step 1 measured, coalescing the refresh and deadline recomputation
   only where it provably does not move the digest.
5. **H-GUEST-DISPATCH.** 58% of the intro and 62% of the play scene, flat on both. The long pole on both gates.
   Measure per-instruction cost before touching the recompiler.
6. **M1c, then M2, M3 and H1/H2/H3** as recorded in v4: one press by a human hand on a double-clicked window;
   Outset median at least 15 fps then 30 fps with the digest unchanged; Link hair correct at the GX owner with
   a screenshot and no geometry regression; continuous intelligible audio; a physical controller including
   reconnect; a ten-minute unattended soak with bounded RSS; and one-command clean build and launch from a
   fresh clone with the private disc.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS START appears, and the file
menu comes up; the new-game intro costs X minutes and Y reaches Outset; here is the screenshot of Outset with
Link under your control.* Silence is not acceptable; neither is a document about why it cannot be done.

