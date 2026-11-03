# BlueWake goal prompt - v6 (2026-09-14)

This supersedes v5 ([GOAL_PROMPT_V5_2026-09-14.md](GOAL_PROMPT_V5_2026-09-14.md)) as the durable
goal given to the implementation agent. The PRD still defines what finished means. This document
defines what counts as a day's work.

## Where this actually stands

**M1 has been achieved and photographed.** A real J key press advanced the Wind Waker title screen
to File Selection, and the screenshot of that screen came out of the Aurora window's own
framebuffer. This is not a synthetic-pad run: BLUEWAKE_PAD_BUTTONS was unset, --route was off, no
pulse was armed. The window was activated and the key posted by hand.

v5's remaining complaint was that this happened once, unexplained and uncaptured. It is now both
captured and explained far enough to bisect. What is left is not whether it works, but **why the
first thirteen seconds of presses are silently eaten**, and that is a defect to fix, not a behavior
to document.

## The reproduction, with its evidence

Run /tmp/bw-m1.err (PID 17776, BLUEWAKE_INPUT_PROBE=1, retraces 1900, no synthetic input):

    [boot-milestone] title-ready        retrace=333  blocks=22413395
    [boot-milestone] name-scene-create  retrace=1204 blocks=193645745 scene=0x80ACC2FC
    [boot-milestone] memcard-check      retrace=1212 blocks=194445967
    [boot-milestone] file-select        retrace=1290 blocks=199649200

Two presses arrived, with the probe proving the key reached the pad layer both times:

    [input-probe] print=13 pressed=1 J=1 codes=13 port0_button=0x0100 err=0
    [input-chain] retrace=1141 cpad_hold=0x0100 cpad_trig=0x0100   <- dropped
    [input-probe] print=15 pressed=1 J=1 codes=13 port0_button=0x0100 err=0
    [input-chain] retrace=1201 cpad_hold=0x0100 cpad_trig=0x0100   <- ACCEPTED

Frame capture at retrace 1900 (--capture-frame, P6 PPM 1920x1440, nonblank=2725986,
hash=0xF748720DD69864BC) shows Quest Log 1 / New Game / Start / Copy / Erase / Return. The press at
1201 produced name-scene-create three retraces later and file-select 89 retraces later. Delivery
and commit both work.

## The two things v5 got wrong

**First, the window is not one opening. It closes and reopens.** Every press below is a real
keyboard press with no synthetic input; only the moment differs.

| run | press retrace | outcome |
| --- | --- | --- |
| burst | 365 | **accepted**, but name-scene-create only at 449 - buffered through the title fade |
| v6 | 739 | dropped |
| v4-c | ~805 | dropped |
| m1 | 1141 | dropped |
| m1 | 1201 | **accepted** (file-select 1290) |
| v5 | ~1204 | **accepted** (file-select 1368) |

So there is a **dead stretch roughly retraces 740-1150** - in guest time about 12 s to 19 s - during
which a discrete press is ignored, with acceptance on both sides of it. A human who presses once,
the moment they see PRESS START, lands in that stretch and concludes the port is broken.

**Second, the title is not listening yet cannot be the whole story**, and v5 leaned on it. The
shipping --route driver arms BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1, a synthetic A pulse fired at
title-ready - retrace 333, the very start of the stretch that supposedly ignores input - and that
route reaches controllable Outset and reproduces the accepted digest. The title consumes input at
333. The discriminator is therefore not *when* the press happens but **how the press presents
itself to the guest's own poll**.

## The defect, stated as the thing to measure

The guest does not read a level; it reads an **edge**. WW keeps two parallel pad records, both
byte-addressed and both probed successfully already:

* the mDoCPd controller record at 0x803A4E20 - hold word at +0, trigger word at +2
  (A = 0x0100, START = 0x0010);
* the JUT pad the game also consults, reached through the pointer at 0x803A4DE0, with hold at
  0x18 and trigger at 0x1C (see main.c:6549).

Observed states are telling: cpad_hold=0x0100 with cpad_trig=0x0000 means the button is *still down*
while the trigger edge has already been consumed. The trigger is the thing the title acts on.

The prime suspect is that the host latches the pad **more than once per guest frame**. It latches at
every VI retrace (main.c:2443, inside host_sync_vi_cycles) *and* again when the guest writes the SI
poll register (main.c:2062), plus on COMCSR TSTART. A short real press therefore has to survive two
or three latches and still be sitting between the guest's own read and its next read. A long
synthetic pulse does not care. Verify this before fixing it; if it is wrong, the next candidate is
the press *shape* the guest sees (a one-frame edge versus a sustained hold) and the mDoCPd async-SI
read window.

## The one defining outcome

**A human presses a key on their own keyboard, and the game on screen answers.** From a clean shell,
with no exported variables and no absolute path argument:

1. one command opens a 960x720 window that shows the game;
2. sound comes out of the default output device;
3. pressing J (A) at the title screen makes the file-select screen appear - **the first press, not
   the fifth**;
4. the same thing happens when a human double-clicks BlueWake.app.

Items 1, 2 and 4 have each been observed. Item 3 now works *late* and is photographed; what it owes
is being reliably *early*. That is the whole of M1's remainder.

## M1 - restated as two halves with a named gate

**M1a - reproduce, deterministically, with a screenshot.** A fresh run, a real key press, the file
menu, captured. Evidence: the press line, the screenshot, and the retrace at which name-scene-create
fired relative to the press.

**M1b - name and remove the dead stretch.** Find what makes retraces roughly 740-1150 swallow a
discrete press. Evidence: an instrumented trace of the two guest pad records across a dropped press
and an accepted press; the boundary adduced by bisection rather than argument; and, after the fix, a
press at retrace ~400 advancing the screen.

A screenshot of a screen nobody pressed anything to reach is not evidence for M1 - and a press the
title was not yet listening for is not evidence of delivery. Both caveats still apply.

## Instrument in the same build as the fix

Composite and Aurora rebuilds are expensive, so a diagnostic ships only in the same build as the fix
or the reproduction it evaluates. Host-only rebuilds are cheap and sufficient for main.c:
cmake --build build/runtime-host-dsp --target bluewake_host -j 8, about two minutes.

Extend the existing BLUEWAKE_INPUT_PROBE block (main.c:2390, host_input_chain_probe) - do not add a
parallel mechanism. It already reads 0x803A4E20; add the trigger side and the JUT record at
0x803A4DE0 so one line shows both records plus the SI response buffer on every change. Log the same
line when a *host* latch happens, so a dropped press can be read as a function of latch count per
frame rather than guessed at.

Falsifier, stated before the run: if a press well after retrace 1204 still fails to advance the
title, or if a press at ~400 works only in the presence of a sustained synthetic hold, the
latch-count reading is wrong and the next iteration measures press shape at the guest poll instead.

## The iteration contract (unchanged)

An iteration counts only if a human can now see, hear or do something they could not before, **or**
one of the five governing numbers moved. A diagnostic is a cost you pay to make a measurement
trustworthy, never progress by itself, and it ships only alongside the fix or reproduction it
evaluates. If an iteration ends with no change to an observable and no change to a number, it
FAILED: revert it and change the mechanism, not the constant. One failure is a signal; three
failures of the same shape means stop, measure the layer underneath, and say so out loud. Never
write no product gate advances as a successful outcome; that sentence is this project's failure
signature.

## The five governing numbers

scripts/bench.sh runs the shipping configuration on the Outset route and reports all five. It
reproduces the 2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers
measured on the host you are standing on, and record the host identity with every timing. A change
that moves fps but moves the digest is not a result; it is a behavior change.

## What is already done - do not rebuild it

Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, and
reaches the title screen with correct video and audio on the Aurora window. The hybrid-O2 composite
with the 256-cycle cap is the promoted configuration. Aurora provides Metal video, SDL3 audio output
and live pad input on port 0 with installed keyboard bindings. scripts/route_digest.py and
scripts/cycle_invariance.py are the correctness oracles. scripts/play.sh defaults the renderer, the
composite, the DOL, the RELs and the DSP ROMs, and already supports --capture-frame F
--capture-retrace N (PPM) and --route (synthetic A pulse).

The input chain is proven to the guest's controller record. Do not re-derive it.

## Milestones

**M1** above. Do this before anything else.

**M2** Outset median at least 15 fps with p99 at least 10 fps, digest unchanged, measured through the
same command a human uses.

**M3** Outset median at least 30 fps with p99 at least 20 fps, pacing stable, digest unchanged.

**M4** Link's hair correct at the GX owner, with a screenshot checkpoint and no regression to opaque,
transparent or depth-ordered geometry.

**M5** Continuous intelligible audio under human review; physical controller works including
reconnect; ten-minute unattended soak with bounded RSS and no crash.

**M6** One-command clean build and launch from a fresh clone plus the user's own disc, signed, no
Nintendo data in the artifact.

## Speed data - measured, unchanged, and not the day's work

Over the 700-retrace boot route, of 162,441,352 deadline evaluations, 97.90% were bounded by the
256-cycle cap and no device deadline at all. Chaining has been attempted three times and failed
identically three times, each at the aggregate delivery hash after the retained 1,024-record window.
None of this is the day's work while a real press does not reliably advance the title.

## Start with exactly this

1. **Reproduce M1a in one fresh run.** Launch with BLUEWAKE_INPUT_PROBE=1 and
   BLUEWAKE_FRAME_TIMING=1 so every retrace carries a wall-clock stamp, bounded at 2,600 retraces
   with --capture-frame F --capture-retrace 2000. Activate the window, then press **J** once the log
   shows the title is up. Expected: name-scene-create, then memcard-check, then file-select, and a
   PPM of File Selection.
2. **Bisect the boundary inside that same run** by pressing at several retraces on either side of the
   dead stretch (e.g. 700, 900, 1100, 1250, 1400). Use the frame-timing stamps to assign each press
   an exact retrace afterwards. One run, one boundary.
3. **Name the gate.** Extend host_input_chain_probe as above to show both guest pad records and the
   latch events, then state what makes a discrete press survive or vanish. Do not rule in or out the
   memory-card path (memcard-check fires at retrace 1212, *after* the accepted press caused
   name-scene-create at 1204) without a measurement.
4. **Then M2, M3 and H1/H2/H3** as recorded in v4.

If you find yourself producing a long honest document explaining why a press cannot yet be delivered,
stop and treat that as the failure signal it is. Ship the first press.

## Fences (unchanged, non-negotiable)

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves,
  captures, device data, signing material or leaked source. The user-owned GZLE01 stays private and
  out of git; the M1 PPM and PNG stay in /tmp. ref/ is gitignored, so any edit made there (window
  activation, pad bindings) must be recorded in the ledger, never committed.
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

Twenty iterations. An iteration is one hypothesis, one small change, one build, one measurement, and
at most one line appended to CURRENT.md. No status document may be longer than the change it
describes. Report to the user every three iterations, in one paragraph: what a human can now see,
hear or do that they could not before, plus the five governing numbers. If a line of work has not
moved a key press, an observable or a number after three iterations, kill it and say so.

## The first report this goal owes the user

One sentence, in this shape: *Press J at the title screen and the file menu appears - here is the
probe line that proves the key arrived, here is the screenshot of what it caused, and here is how
soon after the title appears the press starts being accepted.* Silence is not acceptable; neither is
a document about why it cannot be done.
