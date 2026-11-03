# BlueWake goal prompt — v5 (2026-09-14)

This supersedes v4 ([GOAL_PROMPT_V4_2026-09-14.md](GOAL_PROMPT_V4_2026-09-14.md)) as the durable
goal given to the implementation agent. The PRD still defines what "finished" means. This document
defines what counts as a day's work.

## What v4 got right, and the one sentence that was wrong

v4 asked for the right measurement: a line proving that a key reached the game. It got it, and then
some. The chain is now proven end to end, on a live run:

    real key -> SDL -> PADRead -> SI merge -> guest g_mDoCPd_cpadInfo[0]

Holding **J** produced `port0_button=0x0100` (A) inside `PADRead`, and the guest's own controller
record at `0x803A4E20` showed, on the same press:

    [input-chain] retrace=739 cpad_hold=0x0100 cpad_trig=0x0100
                  si_word=0x09008080 si_poll=0x00F602F0 si_status=20202020
                  cpad_words=0x00000100,0x00000000
    [input-chain] retrace=745 cpad_hold=0x0000 cpad_trig=0x0000   (clean release)

A key, a mapping, an SI merge and a trigger edge. **Delivery is not the defect any more.**

v4's iteration still failed, because the conclusion written into the ledger was over-generalized:

> "the press reaches the game's own controller record and the title ignores it."

That sentence is **false as stated.** It was drawn from runs whose presses landed *before the title
was ready for them*. A run that was already on disk had done the exact thing M1 asks for, and the
ledger never noticed it.

## The evidence, side by side

All three runs below are real-keyboard runs with **no synthetic pad input at all**
(`BLUEWAKE_PAD_BUTTONS` unset, `--route` off, no pulse armed). Same card, same composite, same
bindings, same window flags (`0x20002220`, keyboard focus set). Only the moment of the press differs.

| run | press retrace | what the probe saw | outcome |
| --- | --- | --- | --- |
| v4-c | ~805 (held ~14 retraces) | `pressed=1 J=1 port0_button=0x0100 err=0` | no advance through retrace 1500 |
| v6 | 739 (held 6 retraces) | `cpad_hold=0x0100 cpad_trig=0x0100` in `cpadInfo`, release at 745 | no advance through retrace ~1400 |
| v5 | ~1204 (held ~2 retraces) | `pressed=1 J=1 port0_button=0x0100 err=0` | **name-scene-create 1281, memcard-check 1289, file-select 1368** |

The third row is M1, already achieved once, already in evidence, and never reported:

    [boot-milestone] title-ready       retrace=333  blocks=22413395
    [boot-milestone] name-scene-create retrace=1281 blocks=211519809 scene=0x80ACC2FC
    [boot-milestone] memcard-check     retrace=1289 blocks=212312077 scene=0x80ACC2FC
    [boot-milestone] file-select       retrace=1368 blocks=217495560 scene=0x80ACC2FC

## The remaining defect, stated precisely

The title screen has an **input window**. Two presses that arrived before it opened were dropped; one
press that arrived after it opened advanced the screen. The boundary lies between retrace ~805
(~13.4 s of guest time after boot) and retrace ~1204 (~20 s). Everything *downstream* of an accepted
press works: name-scene-create 77 retraces after it, file-select 164 retraces after it.

So M1 splits into two measurable halves, and both are cheap:

1. **Reproduce it.** A press after the window opens must produce the file menu *deterministically*,
   with a screenshot of the screen it caused. This is the deliverable the project has owed for ten days.
2. **Name the gate.** Find what opens the window, and decide whether it should exist. A title screen
   that says PRESS START and drops the first 13 seconds of presses is a defect to explain, not a
   behavior to document.

## The one defining outcome

**A human presses a key on their own keyboard, and the game on screen answers.**

From a clean shell, with no exported variables and no absolute path argument:

1. one command opens a 960x720 window that shows the game;
2. sound comes out of the default output device;
3. **pressing J (A) at the title screen, once the title is ready to accept it, makes the file-select
   screen appear**;
4. the same thing happens when a human double-clicks BlueWake.app.

Items 1, 2 and 4 have each been observed. Item 3 has now been observed once, unexplained and
uncaptured. It is not a product until it is reproducible and photographed.

## M1 — restated, with its diagnosis attached

**A real key press produces the file menu, reproducibly, with a screenshot, and the arming boundary
is named.** Evidence, all four parts:

* the probe line showing the press arrive (`[input-probe]` or `[input-chain]`);
* a screenshot of the file-select screen that the press caused;
* the earliest retrace at which a press is accepted, adduced by bisection, not by argument;
* the same interaction from the double-clicked bundle, with no exported variables.

A screenshot of a screen nobody pressed anything to reach is not evidence for M1 — and neither is a
press that the title was not yet listening for.

## Method: one build, one hypothesis, one measurement

Composite and Aurora rebuilds are expensive, so a diagnostic is allowed only when it ships in the
same build as the fix or the reproduction it evaluates. Host-only rebuilds (`bluewake_host`) are
cheap: `cmake --build build/runtime-host-dsp --target bluewake_host -j 8`, about two minutes.

Falsifier, stated before the run: if a press that lands well after retrace 1204 still fails to
advance the title, the input-window reading is wrong and the discriminator is something else —
press shape, focus at the instant of the press, or the card check — and the next iteration measures
that instead. If it advances, the window is real, the boundary is bisectable, and the next question
is what opens it.

## The iteration contract (unchanged from v4)

An iteration counts only if a human can now see, hear or do something they could not before, **or**
one of the five governing numbers moved. A diagnostic is a cost you pay to make a measurement
trustworthy, never progress by itself. A diagnostic ships only in the same build as the fix or the
reproduction it evaluates.

If an iteration ends with no change to an observable and no change to a governing number, it FAILED.
Revert it and change the mechanism, not the constant. One failure is a signal; three failures of the
same shape means stop, measure the layer underneath, and say so out loud. Never write "no product
gate advances" as a successful outcome; that sentence is the failure signature of this project.

## The five governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reports all five. It
reproduces the 2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | `2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b` |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers
measured on the host you are standing on, and record the host identity with every timing. A change
that moves fps but moves the digest is not a result; it is a behavior change.

## What is already done — do not rebuild it

Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, and reaches
the title screen with correct video and audio on the Aurora window. The hybrid-O2 composite with the
256-cycle cap is the promoted configuration. Aurora provides Metal video, SDL3 audio output and live
pad input on port 0 with installed keyboard bindings. `scripts/route_digest.py` and
`scripts/cycle_invariance.py` are the correctness oracles. `scripts/play.sh` defaults the renderer,
the composite, the DOL, the RELs and the DSP ROMs, and already supports `--capture-frame F
--capture-retrace N` (PPM) and `--route` (synthetic A pulse).

The input chain is proven to the guest's controller record. Do not re-derive that.

## Speed data — measured, unchanged, and not the day's work

Over the 700-retrace boot route, of 162,441,352 deadline evaluations, 97.90% were bounded by the
256-cycle cap and no device deadline at all. Chaining has been attempted three times and failed
identically three times, each at the aggregate delivery hash after the retained 1,024-record window.
None of this is the day's work while a real press does not reliably advance the title.

## Milestones

**M1** — above. Do this before anything else.

**M2** Outset median >= 15 fps with p99 >= 10 fps, digest unchanged, measured through the same
command a human uses.

**M3** Outset median >= 30 fps with p99 >= 20 fps, pacing stable, digest unchanged.

**M4** Link's hair correct at the GX owner, with a screenshot checkpoint and no regression to
opaque, transparent or depth-ordered geometry.

**M5** Continuous intelligible audio under human review; physical controller works including
reconnect; ten-minute unattended soak with bounded RSS and no crash.

**M6** One-command clean build and launch from a fresh clone plus the user's own disc, signed, no
Nintendo data in the artifact.

## Start with exactly this

1. **Reproduce the accepted press.** Launch with `BLUEWAKE_INPUT_PROBE=1` and a bounded retrace
   count, wait until the log shows the title is up, and press **J** after the window opens
   (well past retrace 1200). Capture the frame the press produces
   (`--capture-frame ... --capture-retrace ...`). Expected: `name-scene-create`, then
   `memcard-check`, then `file-select`, within ~200 retraces of the press.
2. **Bisect the boundary** in a single run by pressing at several retraces (e.g. 900, 1000, 1100,
   1200, 1300) and recording the retrace of each press against the retrace at which
   `name-scene-create` fires. One run, one boundary.
3. **Name the gate.** Then instrument the title's own input read in the same build as the fix, and
   say what opens the window. The existing `[title-execute]` trace is gated on canonical PC
   `0x81E01B88` and printed zero occurrences in a full title run — confirm that address before
   trusting it. Rule out the memory-card path only with evidence.
4. **Send a genuine held START** as a second data point: START is `0x0010` in `cpadInfo`. `key code
   36` is a no-op on this host, so a real held Return needs a posted CGEvent (a small Swift helper
   works; PyObjC/Quartz is not installed).
5. Then M2, M3 and H1/H2/H3 as recorded in v4.

If you find yourself producing a long honest document explaining why a press cannot yet be
delivered, stop and treat that as the failure signal it is. Ship a key press.

## Fences (unchanged, non-negotiable)

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves,
  captures, device data, signing material or leaked source. The user-owned GZLE01 stays private and
  out of git. `ref/` is gitignored, so any edit made there (window activation, pad bindings) must be
  recorded in the ledger, never committed.
* Preserve user data and unrelated work. One BlueWake process at a time.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test
  because it exposes a failure, no captured emulator state as authentic initialization. A synthetic
  button press is a route driver, not evidence that the keyboard works.
* Never add a new tier, slice or patch for a unit that already compiles whole. New code must remove
  cost, not add diagnostics.
* The guest-visible cycle contract, the route digest, the card outcome and the paced-PCM fingerprint
  must survive every promotion. Never weaken, skip or re-record a digest to pass.
* Keep Route A's accepted artifacts and the dependency lock intact, and keep the private-data audit
  green.

## Budget and cadence

You have twenty iterations. An iteration is one hypothesis, one small change, one build, one
measurement, and at most one line appended to CURRENT.md. No status document may be longer than the
change it describes. Report to the user every three iterations, in one paragraph: what a human can
now see, hear or do that they could not before, plus the five governing numbers. If a line of work
has not moved a key press, an observable or a number after three iterations, kill it and say so.

## The first report this goal owes the user

One sentence, in this shape: *"Press J at the title screen and the file menu appears — here is the
probe line that proves the key arrived, here is the screenshot of what it caused, and here is how
soon after the title appears the press starts being accepted."* Silence is not acceptable; neither
is a document about why it cannot be done.
