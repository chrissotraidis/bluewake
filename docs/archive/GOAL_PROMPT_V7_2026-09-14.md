# BlueWake goal prompt - v7 (2026-09-14)

Supersedes v6 ([GOAL_PROMPT_V6_2026-09-14.md](GOAL_PROMPT_V6_2026-09-14.md)). The PRD still defines what finished
means. This document defines what counts as a day of work, and it carries a correction that v6 earned
the hard way.

## The correction: v6 diagnosed the wrong layer

v6 declared a "dead stretch" of retraces roughly 740-1150 during which a discrete press is swallowed,
and named multiple pad latches per guest frame as the prime suspect. Both halves were falsified in one
session, from the game own framebuffer and the guest own controller record.

**title-ready is not the title.** The milestone at main.c:6518 fires when the canonical PC reaches
0x81E01B88u. On this build it fires at retrace 333 - and a frame captured from the Aurora window readback
at retrace 700 shows the opening logo movie: the Zelda title card over a blue sky above the 2002, 2003
Nintendo copyright line. The interactive screen that answers an A press comes much later. v5 was nearer
the truth than v6: the title really is not listening yet, because it is not the title yet. It is a
cutscene, and a cutscene ignoring input is correct behavior, not a defect.

**The press that was dropped was dropped correctly.** A real J press reached the guest at retrace 397
([input-chain] retrace=397 cpad_hold=0x0100 cpad_trig=0x0100) and produced no milestone. It landed inside
the movie. That is the movie doing its job.

**The pad latch is not implicated.** Dropped and accepted presses print byte-identical pad records and an
identical SI word (si_word=0x09008080 on every line, every run). There is nothing in the latch path that
differs between a press that advances and one that does not. Do not reopen this without a measurement that
contradicts the above.

## What is established, and it is the thing that was missing

**A real key press now reaches the game, and the delivery path is proven end to end.** The gate was never
in the pad or the guest: it was that the process could not be made frontmost. The bare Mach-O binary
(build/runtime-host-dsp/bluewake_host) is not a frontmost-eligible application, so macOS 14+ refuses to
activate it and no key event is ever delivered to it. The app bundle is eligible.

Verified this session, launch recipe at the bottom:

    [input-probe] print=13 read=801 kbFocus=0x1066ed980 msFocus=0x1066ed980 flags=0x20002620
                  pressed=1 J=1 RETURN=0 codes=13,0,0,0,0,0,0,0 mappingsSet=1 port0_button=0x0100 err=0
    [input-chain] retrace=397 cpad_hold=0x0100 cpad_trig=0x0100 cpad_words=0x01000100,0x00000000
    [input-chain] retrace=399 cpad_hold=0x0100 cpad_trig=0x0000 cpad_words=0x01000000,0x00000000
    [input-chain] retrace=401 cpad_hold=0x0000 cpad_trig=0x0000 cpad_words=0x00000000,0x00000000

The chain runs from the OS key event, through SDL and the Aurora port-0 binding, into the SI word the guest
polls, and lands in the guest own mDoCPd record at 0x803A4E20 with a clean one-frame trigger edge. Nothing
between the keyboard and the game is in question any more.

Be honest about what this is not. The press above was posted with CGEventPost, which enters the same OS
input stack a physical key does, unlike BLUEWAKE_PAD_BUTTONS which injects inside the emulator and proves
nothing about input. It is still a synthesized event, so it upgrades the claim from "undelivered" to
"delivered on demand", and it does not yet discharge the human-standard requirement. The final M1 evidence
must include one press made by a human hand on a physical keyboard.

## The defect, restated as what is actually open

Everything now reduces to one measurable question:

**When does the interactive title appear, and does the first press after that advance it?**

Known press outcomes, all real deliveries through the OS input stack, only the moment differing:

| run | press retrace | screen state | outcome |
| --- | --- | --- | --- |
| burst | 365 onward, dense | opening movie | title advanced at 445 |
| m1c | 397 | opening movie | dropped (correct) |
| m1 | 1141 | unknown, movie or fade | dropped |
| m1 | 1201 | interactive title | **accepted**, file-select at 1290 |
| v5 | ~1204 | interactive title | **accepted**, file-select at 1368 |

Two readings fit the data and they are not yet separated. Either (a) the interactive title simply does not
exist before roughly retrace 1150 and every earlier press is legitimately ignored, or (b) the title is up
earlier and something about how a lone discrete press presents to the guest poll is wrong. The burst run
leans toward (a) with a wrinkle worth explaining: dense presses beginning at 365 did not advance the screen
until 445, so whatever admits input was already open by 445 in that run but had not opened by 1141 in m1.
A card, timing or pacing difference between those two runs is the first thing to rule in or out.

Do not theorize past that. Capture frames across the window, read the guest record, and let the boundary
fall out of the measurement.

## M1, restated

**M1a - the first press works.** From a fresh run, the first A press made once the interactive title is on
screen advances to File Selection, captured as a screenshot from the window readback. Evidence: the press
line, the retrace at which the next milestone fired relative to the press, and the frame.

**M1b - the boundary is named.** The retrace at which the interactive title becomes able to answer input is
stated as a number, with frames on both sides of it. If a lone press fails after that retrace while a dense
or held press succeeds, the remaining defect is press shape at the guest poll and it gets its own iteration.

**M1c - the human press.** One press made by a human hand, on a physical keyboard, on a window opened by
double-clicking BlueWake.app. This is the milestone the PRD actually names, and CGEventPost cannot satisfy
it.

A screenshot of a screen nobody pressed anything to reach is not evidence for M1. Neither is a synthetic
pad pulse. The m1 run is still the reference precisely because it had no synthetic input at all.

## The one defining outcome, unchanged

A human presses a key on their own keyboard and the game answers. From a clean shell, no exported variables,
no absolute path argument: one command opens a 960x720 window showing the game; sound comes out of the
default output device; pressing J at the title screen makes the file menu appear **on the first press, not
the fifth**; and the same thing happens when a human double-clicks BlueWake.app.

## Launch recipe - solve this first, it was the whole blocker

Launch the **app bundle**, never the bare binary. The bundle is frontmost-eligible and takes focus when
launched from a terminal; the bare Mach-O never does.

    env BLUEWAKE_INPUT_PROBE=1 BLUEWAKE_FRAME_TIMING=1 \
        BLUEWAKE_PLAY_HOST=$PWD/build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake \
        scripts/play.sh --retraces 1700 --capture-frame /tmp/bw.ppm --capture-retrace 1400 \
        >/tmp/bw.out 2>/tmp/bw.err

Then confirm the window owns the foreground:

    lsappinfo info -only name $(lsappinfo front)    # expect BlueWake

Then post a real J (keycode 38) through the OS input stack:

    /tmp/bwkey 0 38 700          # pid 0 = global HID post; keycode 38 is J

A press counts only when [input-probe] reports pressed=1 J=1 with a non-zero kbFocus AND [input-chain] shows
cpad_trig=0x0100. If /tmp/bwkey is gone, rebuild it: clang -O2 -o /tmp/bwkey /tmp/bwkey.c
-framework ApplicationServices. Note that the computer-use pressKey path is unavailable here: it fails
AXError.cannotComplete because the emulator saturates its main thread and the accessibility server cannot
complete a request. Do not spend iterations on it.

## Instrument in the same build as the fix

Composite and Aurora rebuilds are expensive, so a diagnostic ships only in the same build as the fix or the
reproduction it evaluates. Host-only rebuilds are cheap and sufficient for main.c:
cmake --build build/runtime-host-dsp --target bluewake_host -j 8, about two minutes.

Extend the existing BLUEWAKE_INPUT_PROBE block (main.c:2390, host_input_chain_probe) rather than adding a
parallel mechanism. It already reads 0x803A4E20. Add the JUT record reached through the pointer at
0x803A4DE0 (hold +0x18, trigger +0x1C), and add the active-scene identity, so one line says which scene the
guest is in when the press arrives. The single most valuable addition is that scene identity: it converts
"the press was dropped" into "the press arrived while scene X was current", which is the boundary this goal
is asking for.

Falsifier, stated before the run: if the interactive title can be shown to be on screen and stable before
retrace 1141, then reading (a) is dead, the remaining defect is real, and the next iteration measures press
shape at the guest poll. If the title only appears after roughly 1150, reading (a) holds and there is no
defect at all: the earlier presses were simply early, and M1a is done the moment a first press after that
boundary is photographed.

## The iteration contract

An iteration counts only if a human can now see, hear or do something they could not before, **or** one of
the five governing numbers moved. A diagnostic is a cost you pay to make a measurement trustworthy, never
progress by itself, and it ships only alongside the fix or reproduction it evaluates. If an iteration ends
with no change to an observable and no change to a number, it FAILED: revert it and change the mechanism,
not the constant. One failure is a signal; three failures of the same shape means stop, measure the layer
underneath, and say so out loud. Never write that no product gate advanced as a successful outcome; that
sentence is this project failure signature.

## The five governing numbers

scripts/bench.sh runs the shipping configuration on the Outset route and reports all five. It reproduces the
2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers measured on
the host you are standing on, and record the host identity with every timing. A change that moves fps but
moves the digest is not a result; it is a behavior change.

## What is already done - do not rebuild it

Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, and reaches the
title screen with correct video and audio on the Aurora window. The hybrid-O2 composite with the 256-cycle
cap is the promoted configuration. Aurora provides Metal video, SDL3 audio output and live pad input on
port 0 with installed keyboard bindings. scripts/route_digest.py and scripts/cycle_invariance.py are the
correctness oracles. scripts/play.sh defaults the renderer, the composite, the DOL, the RELs and the DSP
ROMs, and already supports --capture-frame F --capture-retrace N (PPM), --route (synthetic A pulse), and
--bundle. The input chain from the OS to the guest controller record is proven. Do not re-derive it.

## Milestones

**M1** above. Do this before anything else. M2, M3, M4, M5 and M6 stand as written in v6: Outset median at
least 15 fps then 30 fps with the digest unchanged; Link hair correct at the GX owner with a screenshot and
no geometry regression; continuous intelligible audio under human review, physical controller including
reconnect, and a ten-minute unattended soak with bounded RSS and no crash; one-command clean build and launch
from a fresh clone plus the user own disc, signed, with no Nintendo data in the artifact.

## Speed data - measured, unchanged, and not the day of work

Over the 700-retrace boot route, of 162,441,352 deadline evaluations, 97.90 percent were bounded by the
256-cycle cap and no device deadline at all. Chaining has been attempted three times and failed identically
three times, each at the aggregate delivery hash after the retained 1,024-record window. None of this is the
day of work while a first press does not reliably advance the title.

## Start with exactly this

1. **Find the boundary.** One run, BLUEWAKE_INPUT_PROBE=1 and BLUEWAKE_FRAME_TIMING=1, frames captured on
   both sides of the logo-to-title transition. State the retrace at which the interactive title is on screen
   and stable. Sample the window directly with screencapture while the run is live; that is cheaper than
   repeated runs and gives the frame timing to convert a wall-clock sample into an exact retrace.
2. **Press once, late, and photograph it.** Press A once, after the boundary, and require the first press to
   produce name-scene-create then memcard-check then file-select, with a PPM of File Selection from the
   window readback.
3. **Then close M1c by hand.** Ask the human to double-click BlueWake.app and press J once. That single act
   is the milestone the PRD names, and no synthesized event substitutes for it.
4. **Then M2, M3 and H1/H2/H3** as recorded in v4.

If you find yourself producing a long honest document explaining why a press cannot yet be delivered, stop
and treat that as the failure signal it is. The press is delivered now; ship the first-press advance.

## Fences (unchanged, non-negotiable)

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git; every
  PPM and PNG from this work stays in /tmp. ref/ is gitignored, so any edit made there (window activation,
  pad bindings, patch files under patches/aurora) must be recorded in the ledger and shipped as a patch,
  never as committed vendored source.
* Preserve user data and unrelated work. One BlueWake process at a time.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. A synthetic button press is a
  route driver, not evidence that the keyboard works.
* Never add a new tier, slice or patch for a unit that already compiles whole. New code must remove cost,
  not add diagnostics.
* The guest-visible cycle contract, the route digest, the card outcome and the paced-PCM fingerprint must
  survive every promotion. Never weaken, skip or re-record a digest to pass.
* Keep Route A accepted artifacts and the dependency lock intact, and keep the private-data audit green.

## Budget and cadence

Twenty iterations. An iteration is one hypothesis, one small change, one build, one measurement, and at most
one line appended to CURRENT.md. No status document may be longer than the change it describes. Report to
the user every three iterations, in one paragraph: what a human can now see, hear or do that they could not
before, plus the five governing numbers. If a line of work has not moved a key press, an observable or a
number after three iterations, kill it and say so.

## The first report this goal owes the user

One sentence, in this shape: *Press J at the title screen and the file menu appears - here is the probe line
that proves the key arrived, here is the screenshot of what it caused, and here is how soon after the title
appears the press starts being accepted.* Silence is not acceptable; neither is a document about why it
cannot be done.

