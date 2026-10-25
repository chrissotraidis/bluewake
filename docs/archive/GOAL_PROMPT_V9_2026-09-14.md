# BlueWake goal loop - v9 (2026-09-14)

Supersedes v8 ([GOAL_PROMPT_V8_2026-09-14.md](GOAL_PROMPT_V8_2026-09-14.md)). The PRD still defines what
finished means. This document defines what counts as a day of work, and it adds the one number v8 was
missing: how long a player waits between pressing Start and playing.

## Where the product stands tonight

A human double-clicks **BlueWake.app** and the game boots with video and audio. Real keyboard presses,
delivered by the OS, drive it forward. Every screen renders correctly:

| screen | reached at retrace | press that caused it |
| --- | --- | --- |
| opening cutscene | 333 onwards | none, it is a movie |
| File Selection | 1324 | J at 1175 |
| quest-log detail panel | ~1900 | J at 1697 |
| Name Entry | ~3000 | J at 2945 |
| new-game legend intro | 773, ends 13850 | none, it is a movie; START and A do not end it |
| open sea room in 3D | 13910 (play scene) | none, follows the intro |
| Outset Island banner | 14402 | none, it is the fly-in |

File Selection takes **two** presses: the first selects the quest log and reveals Start/Copy/Erase/Return,
the second takes Start. That is the game's design, not a defect.

The scripted route, run headless to its bound with no block cap, confirms the game logic goes further than
the keyboard has been driven: every new-game milestone fires and the run closes at the play scene.

    title_ready         retrace=333
    file_select         retrace=535
    name_input_complete retrace=712
    new_game_intro      retrace=773    resource="/res/Object/Opening.arc"
    opening_complete    retrace=13850
    play_scene          retrace=13910

## The finding that sets this loop: the intro is the wall

`new_game_intro` starts at retrace 773. `opening_complete` does not fire until **retrace 13850**.
That is **13,077 guest frames** between pressing Start and reaching the play scene. GameCube VI is 60 Hz, so
13,077 frames is about 3.6 minutes of authored cutscene. The host produces retraces at roughly 6 fps on the
title path, so the same stretch costs a player on the order of **36 minutes of wall clock**.

Nothing about that is an input defect and nothing about it is a crash. It is the performance milestone
(M2) expressed as a number a player feels, and it is now the largest single obstacle between this project
and a person actually playing - ahead of Link's hair and ahead of audio quality.

An earlier session showed the title's opening movie correctly ignores input. That does **not** establish
that the new-game intro ignores input. Whether Opening.arc accepts START to skip is an open, cheap,
decisive question, and this loop answers it before it optimizes anything.

## The metric this loop makes first-class: time-to-playable

**Time-to-playable is double-click to controllable Outset, with no exported variables.** It is the only
number in this project that is simultaneously a performance result and a playability result, and it is
measured on the product path or it is not measured.

Two levers move it, and they are independent:

1. **Skip the intro**, if the game supports it. Cheap, large, player-visible. Test it first.
2. **Raise the frame rate on the intro path**, if it does not. The 13,077 frames are authored content and
   must execute; the only variable is how fast the host executes them.

**Measured value, first time: about 20 minutes** from double-click to Outset Island visible, of which about
11.5 minutes is the new-game intro. Measured on the product path with a capture series: intro at retrace
773, opening-complete 13850, play-scene 13910, Outset banner 14402. Intro pace measured at 10-13 fps, not
the 6 fps an earlier figure implied, so the earlier 36-minute estimate was too pessimistic. Target: under 5
minutes.

## New falsifiable hypotheses, in the order they are cheap

* **H-INTRO-SKIP.** ANSWERED, not supported. START landed in the guest at retrace 2206 and A at 2506, and
  `opening_complete` still fired at 13850. The intro is authored content that runs to its end. Two doors
  stay open, both narrow and both untested: whether any press earlier than 2206 behaves differently, and
  whether the retail intro ignores input for the same reason the title movie does. Do not reopen without a
  measurement.
* **H-INTRO-HOLD.** A single 2-frame pulse is too short for the intro to observe; a held button is
  required. Falsified if a press of at least 300 ms during the intro also fails to shorten it.
* **H-INTRO-FPS.** Partly answered, and it points the other way: the intro window ran at 10-13 fps on this
  host, so the intro's cost is the 13,077 authored frames themselves rather than a path-specific slowdown.
  The open question is whether those frames can execute faster without moving the digest.
* **H-NAME-GRID.** The Name Entry grid answers d-pad plus A the way the other menus do, so a keyboard route
  reaches Outset with no synthetic input. Falsified if a synthetic route reaches Outset while the keyboard
  route stops on the same screen.

Every one of these is answered by a run that already exists and by a log line that is already printed. None
of them require a new tier, a new slice or a patch.

## Harness facts worth not rediscovering

* **The product path is the app bundle.** `open build/runtime-host-dsp/BlueWake.app` is what a double-click
  does. The bare `bluewake_host` Mach-O cannot take focus on macOS 14+ and is a debugging convenience only.
  The instrumented equivalent is
  `open --stdout F --stderr F --env NAME=VALUE ... build/runtime-host-dsp/BlueWake.app`.
* **`open` reaps its child when the launching shell exits.** Keep the launching shell alive for the whole
  run, or the run dies mid-flight and looks like a crash.
* **One BlueWake process at a time.**
* **`BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1` arms a latched synthetic new-game sequence** - file-slot select,
  Start, name characters, name END, confirm - which is how an unattended route crosses the file menu and
  the name grid. This is a route driver, not evidence that the keyboard works.
* **Generic scheduled pulses share one button value.** `BLUEWAKE_PAD_BUTTONS` sets the button that
  `PULSE_RETRACE`, `PULSE2_RETRACE` and `PULSE3_RETRACE` all deliver. Two scheduled pulses cannot
  differ in button. The name END pulse is the one exception, hard-coded to START (0x1000).
* **Delivering a real key.** `/tmp/bwfocus PID 400` immediately before every press (this Mac is in active
  use and loses focus), then `/tmp/bwkey 0 38 600`; pid 0 is a global HID post, 38 is J, RETURN (36) is
  START. A press counts only when the probe shows pressed=1 with non-zero kbFocus **and** the guest line
  shows cpad_trig=0x0100. Do not gate on msFocus; it reads 0x0 on presses that land.
* **Frame evidence.** `BLUEWAKE_CAPTURE_OPENING_FRAME` plus `BLUEWAKE_CAPTURE_RETRACE` reads back the
  guest framebuffer at an exact retrace; adding `BLUEWAKE_CAPTURE_INTERVAL` yields a labelled series. This
  is stronger than a screen grab because it is exact-retrace and it names what it captured. Every capture
  stays in /tmp.

## The iteration contract

An iteration counts only if **time-to-playable fell**, or a human can now see, hear or do something they
could not before, **or** one of the governing numbers moved. A diagnostic is a cost paid to make a
measurement trustworthy, never progress by itself.

One hypothesis, one small change, one build, one measurement, and at most one line appended to
`docs/archive/status/CURRENT.md`. No status document may be longer than the change it describes. Host-only rebuilds
are the cheap ones: `cmake --build build/runtime-host-dsp --target bluewake_host -j 8`, about two minutes,
which also re-assembles and re-signs the bundle. Extend the existing `host_input_chain_probe` rather than
adding a parallel mechanism.

If an iteration ends with no change to an observable and no change to a number, it **failed**: revert it and
change the mechanism, not the constant. One failure is a signal; three failures of the same shape means
stop, measure the layer underneath, and say so out loud. Report to the user every three iterations in one
paragraph: what a human can now see, hear or do that they could not before, plus the numbers.

## The work, in order

1. **Answer H-INTRO-SKIP.** Press START during the new-game intro on the product path and read the retrace
   at which `opening_complete` fires. If it fires early, the 36-minute wall largely collapses and that is
   the largest player-visible win available today. If it fires at 13850, the same measurement is the
   evidence for the next item.
2. **Reach controllable Outset by keyboard and photograph every screen on the way**, with the press that
   caused each one. This settles H-NAME-GRID and settles whether game logic advances past the file menu
   under real input. If a synthetic route reaches Outset while the keyboard route stops on the same screen,
   the defect is in input, not in game logic.
3. **If the intro is not skippable, cut its cost.** Measure retrace rate inside the intro window separately
   from the rate after `play_scene` (H-INTRO-FPS), then attack whichever is slower.
4. **M1c - the human press.** One press by a human hand, physical keyboard, on a window opened by
   double-clicking BlueWake.app. No synthesized event substitutes. Ask the user and report the result.
5. **M2, M3 and H1/H2/H3** as recorded in v4: Outset median at least 15 fps then 30 fps with the digest
   unchanged; Link hair correct at the GX owner with a screenshot and no geometry regression; continuous
   intelligible audio under human review; physical controller including reconnect; a ten-minute unattended
   soak with bounded RSS and no crash; one-command clean build and launch from a fresh clone plus the
   user's own disc, signed, with no Nintendo data in the artifact.

## The governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reproduces the 2026-09-01
baseline exactly.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |
| **time-to-playable** | **unmeasured; ~36 min of intro at 6 fps on the scripted route** |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers measured
on the host you are standing on. A change that moves fps but moves the digest is not a result, it is a
behavior change.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git; every
  PPM and PNG from this work stays in /tmp. `ref/` is gitignored, so an edit made there ships as a patch
  under `patches/aurora/`, never as committed vendored source.
* Preserve user data and unrelated work. The user's save card is
  `~/Library/Application Support/BlueWake/GZLE01.card`; drive the new-game flow against a copy, never
  against the save slot, and keep the backup in /tmp current.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. A synthetic button press is a
  route driver, **not** evidence that the keyboard works.
* Never add a new tier, slice or patch for a unit that already compiles whole. New code must remove cost,
  not add diagnostics.
* Never weaken, skip or re-record a digest to pass. Do not write that no product gate advanced as a
  successful outcome; that sentence is this project's failure signature.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS START appears, and the
file menu comes up; the new-game intro costs X minutes and Y closes it; here is the screenshot of Outset
with Link under your control.* Silence is not acceptable; neither is a document about why it cannot be done.
