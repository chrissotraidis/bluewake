# BlueWake goal prompt — v4 (2026-09-14)

This supersedes v3 ([GOAL_PROMPT_2026-09-14.md](GOAL_PROMPT_2026-09-14.md)) as the durable goal
given to the implementation agent. The PRD still defines what "finished" means. This document
defines what counts as a day's work.

## Why v3 also failed

v3 fixed the right two things. It moved delivery from last to first, and it made "a human can see
it" the literal definition of M1. It still failed, and the whole day went to a document.

v3's fault was narrower and worth naming precisely: **its first milestone asked for a picture, and
the picture it asked for was only reachable through an interaction nothing had ever verified.**
M1's evidence was "a screenshot of Outset with Link in it". Outset is upstream of the title
screen's first accepted button press. So the day was spent establishing whether a key reaches the
game, the answer was not reached, and the iteration ended with an honest page of analysis rather
than a key press. The analysis was real work and it produced facts nobody had before. It was still
not the deliverable, and v3's contract let it count.

Three goal prompts have now shared one property: every one of them defined success as an emulator
outcome (a digest, a frame rate, a screenshot, a census) rather than **an interaction**. On
2026-09-14 the emulator reached the Wind Waker title screen, drew it correctly, played audio, and
sat there for 1,167 retraces — about 19 seconds of guest time — while no button press ever
reached it. The project has a working emulator and has never had a working controller.

## The one defining outcome

**A human presses a key on their own keyboard, and the game on screen answers.**

From a clean shell, with no exported variables and no absolute path argument:

1. one command opens a 960x720 window that shows the game;
2. sound comes out of the default output device;
3. **pressing J at the title screen makes the file-select screen appear**;
4. the same thing happens when a human double-clicks BlueWake.app.

Items 1, 2 and 4 have each been observed. Item 3 has never been observed, and until it is, nothing
else in the ledger is a product.

## The input chain, and the measurement that has never been made

A key press must survive five links:

    physical key -> macOS delivers keyDown to the key window -> SDL queues a key event
                 -> SDL's keyboard state array for that scancode becomes 1
                 -> PADRead maps the scancode to a GameCube button
                 -> the host merges the live pad into the SI transfer
                 -> the guest's g_mDoCPd_cpadInfo sees the button

Everything from the SI merge rightward is proven: the route card drives the same path with a
synthetic button, and the probe reads port 0 with `err=0` and installed bindings.

The links above SDL have never been observed. Nothing in the ledger contains a line that says
**"this process observed the J key go down"**. That single line is the missing measurement, and it
splits the remaining problem cleanly in half:

* if `J=1` never appears while a key is physically held, the fault is above SDL (window
  activation, focus, or synthetic-event delivery), and the fix is in the window layer;
* if `J=1` appears and the game still does not answer, the fault is below SDL (pad mapping or the
  SI merge), and the fix is in the pad layer.

The facts already measured, which bound those two halves:

| launch path | SDL_GetKeyboardFocus() | window flags | key received |
| --- | --- | --- | --- |
| app bundle via `open` | non-null | `0x20002220` (INPUT_FOCUS set) | not observed |
| raw binary from a shell, which is what `scripts/play.sh` execs | **null** | `0x0` | impossible |

Two SDL 3 facts do most of the explanatory work here. SDL 3 **does not activate the application
when a window is shown** unless `SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN` is set, and
`SDL_RaiseWindow` is the documented way to give a window `SDL_WINDOW_INPUT_FOCUS`. Aurora calls
neither, so on the shell path the window is drawn but never becomes the key window. Note also that
synthetic keystrokes sent by `osascript`/System Events are not a valid test of this: they may
never reach SDL at all. **A physical key press is the tiebreaker, and it is the one experiment
worth asking the user for.**

## Method: instrument once, fix once, measure once

One build carries both halves of the first iteration, because composite and Aurora rebuilds are
expensive and a rebuild spent on only a diagnostic is a rebuild wasted:

1. an opt-in input probe that prints a line **on change** (the first key-down, not the 20,000th
   pad read), so an input gap can never again be attributed by argument;
2. the window-layer fix: activate the application when the window is shown, and raise it.

Falsifier, stated before the run: if the probe never prints a down key while a physical key is
held, the window layer is still wrong and the pad layer is not to blame. If it prints the down key
and the title does not advance, the window layer is fixed and the pad layer is next. Either answer
ends the iteration with a key press or with the next concrete defect, never with a document.

## The iteration contract

An iteration counts only if a human can now see, hear or do something they could not before, **or**
one of the five governing numbers moved.

A key that reaches the game, a screen that changes when pressed, a button that moves Link, a louder
speaker and a faster frame are progress. A new tier, a new oracle, a new instrument, a longer
ledger, a reduced and explained failure, and a better-understood blocker are **costs you pay to
make a measurement trustworthy** — not progress. A diagnostic is allowed only when it ships in the
same build as the fix it evaluates.

If an iteration ends with no change to an observable and no change to a governing number, it
FAILED. Revert it and change the mechanism, not the constant. One failure is a signal. Three
failures of the same shape means stop, measure the layer underneath, and say so out loud. Never
write "no product gate advances" as a successful outcome; that sentence is the failure signature of
this project.

## The five governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reports all five. It
reproduces the 2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | `2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b` |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host (6.0 fps reference came from a faster Mac) |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers
measured on the host you are standing on, and record the host identity with every timing. A change
that moves fps but moves the digest is not a result; it is a behavior change.

## What is already done — do not rebuild it

Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, in about
16 minutes of wall time, and reaches the title screen with correct video and audio on the Aurora
window. The hybrid-O2 composite with the 256-cycle cap is the promoted configuration. The Aurora
backend already provides Metal video, SDL3 audio output and live pad input on port 0 with installed
keyboard bindings. `scripts/route_digest.py` and `scripts/cycle_invariance.py` are the
correctness oracles. `scripts/play.sh` already exists and already defaults the renderer, the
composite, the DOL, the RELs and the DSP ROMs. Read the ledger before you measure; do not re-derive
a fact that is already recorded.

## Where the speed is — measured, unchanged

Over the 700-retrace boot route, of 162,441,352 deadline evaluations, **97.90% were bounded by the
256-cycle cap and no device deadline at all**; DSP LLE cadence is 1.93%, audio 0.20%, VI 0.0025%,
decrementer 0.0008%, and the pending-interrupt one-cycle clamp only 0.0071%. Of 62,096,587
dispatch returns, 56.6% are cross-chunk, 31.2% are budget exits and 11.9% are same-chunk. **97.9%
of the time no device needs anything, and the host is still re-entered every 256 guest cycles
anyway.** Chaining has been attempted three times and failed identically three times, each at the
aggregate delivery hash after the retained 1,024-record window. None of this is the day's work
while a key still does not reach the game.

## Milestones — each one is something a human can check

**M1 — A REAL KEY MOVES THE GAME. Do this before any optimization.** With no exported variables
and no absolute path argument, one command opens BlueWake and **pressing J on the title screen
produces the file-select screen**. Evidence: the probe line showing the key going down, a
screenshot of the screen that the key press caused, and the same interaction working from a
double-clicked BlueWake.app. A screenshot of a screen nobody pressed anything to reach is not
evidence for M1.

**M2** Outset median ≥ 15 fps with p99 ≥ 10 fps, digest unchanged, **measured through the same
command a human uses** so the number is a number a human can feel.

**M3** Outset median ≥ 30 fps with p99 ≥ 20 fps, pacing stable, digest unchanged.

**M4** Link's hair correct at the GX owner, with a screenshot checkpoint and no regression to
opaque, transparent or depth-ordered geometry.

**M5** Continuous intelligible audio under human review; physical controller works including
reconnect; ten-minute unattended soak with bounded RSS and no crash.

**M6** One-command clean build and launch from a fresh clone plus the user's own disc, signed, no
Nintendo data in the artifact.

## Start with exactly this

1. Set `SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN` and `SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED` before
   the window is created, and raise the window when it is shown, in the Aurora window layer that
   the host actually links.
2. Repoint the existing `BLUEWAKE_INPUT_PROBE` at change, not cadence: report the first key-down,
   the first pad button, and any change in the pressed set. Ship the probe and the window fix in
   one build.
3. Launch once and press a real key while the window is up. Read the probe first, then the
   milestone. Record which of the two halves the answer assigns.
4. Make the one-command path a launch path that activates the application, and confirm the
   interaction there with no exported variables.
5. Only then H1: install the default-inert edge service, widen the retained delivery history, and
   name the first divergent delivery in one sentence. Then H2 (dispatch budget = the next actual
   device deadline; `bluewake_scheduler_interrupt_requires_host()` in
   `runtime/host/src/scheduler_contract.c` is the pure predicate), then H3 (GX frontend/sink and
   DSP).

If you find yourself producing a long honest document explaining why input cannot yet be
delivered, stop and treat that as the failure signal it is. Ship a key press.

## Fences (unchanged, non-negotiable)

- Never commit, publish or expose Nintendo data, original binaries, generated game code, saves,
  captures, device data, signing material or leaked source. The user-owned GZLE01 stays private
  and out of git.
- Preserve user data and unrelated work. One BlueWake process at a time.
- No silent stubs, no no-op substitutes for required services, no weakening or deleting a test
  because it exposes a failure, no captured emulator state as authentic initialization. A synthetic
  button press is a route driver, not evidence that the keyboard works.
- Never add a new tier, slice or patch for a unit that already compiles whole. New code must remove
  cost, not add diagnostics.
- The guest-visible cycle contract, the route digest, the card outcome and the paced-PCM
  fingerprint must survive every promotion. Never weaken, skip or re-record a digest to pass.
- Keep Route A's accepted artifacts and the dependency lock intact, and keep the private-data audit
  green.

## Budget and cadence

You have twenty iterations. An iteration is one hypothesis, one small change, one build, one
measurement, and at most one line appended to CURRENT.md. No new status document may be longer than
the change it describes. Report to the user every three iterations, in one paragraph: what a human
can now see, hear or do that they could not before, plus the five governing numbers. If a line of
work has not moved a key press, an observable or a number after three iterations, kill it and say
so.

## Note on scope

Nothing in v4 relaxes the PRD, the legal boundary, or the evidence rules that made this project
trustworthy. It changes two things: M1 becomes an interaction instead of a picture, and a
diagnostic is only allowed to ship in the same build as the fix it evaluates. The Route B freeze
and its reopening trigger are unchanged.

## The first report this goal owes the user

One sentence, in this shape: *"Press J at the title screen and the file menu appears — here is the
probe line that proves the key arrived, and here is the screenshot of what it caused."* Or: *"the
key still does not arrive, and the probe now proves it, so the defect is above/below SDL and the
next build fixes that."* Either is acceptable. Silence is not.
