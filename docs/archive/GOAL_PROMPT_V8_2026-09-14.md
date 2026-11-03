# BlueWake goal prompt - v8 (2026-09-14)

Supersedes v7 ([GOAL_PROMPT_V7_2026-09-14.md](GOAL_PROMPT_V7_2026-09-14.md)). The PRD still defines what
finished means. This document defines what counts as a day of work, and it records the finding that ended
the ten-day stall for real.

## The state, plainly

Double-clicking **BlueWake.app** boots the game with video and audio, plays the opening cutscene, and one J
press opens File Selection. That path is now proven end to end, and it is proven on the path a human
actually uses rather than on the convenient one.

    open build/runtime-host-dsp/BlueWake.app
    (opening cutscene plays; PRESS START appears)
    J
    -> [boot-milestone] name-scene-create retrace=1405
       [boot-milestone] name-scene-execute retrace=1413
       [boot-milestone] memcard-check   retrace=1413
       [boot-milestone] file-select     retrace=1492
    -> window readback at retrace 1502: File Selection, Quest Log 1 / New Game

The press line that proves the key arrived is
`[input-probe] print=13 read=2693 pressed=1 J=1 codes=13,0,0,0,0,0,0,0 port0_button=0x0100 kbFocus=0x102712e30`,
and the guest line is
`[input-chain] retrace=1343 cpad_hold=0x0100 cpad_trig=0x0100 jut_hold=0x0100 jut_trig=0x0100` with the
edge cleared on the next frame. Evidence: /tmp/bw-dq.err, /tmp/bw-dq.ppm. Kept in /tmp by the fences below.

## The finding: the double-click path was dying on a budget default, not on input

main() defaulted `BLUEWAKE_MAX_BLOCKS` to 50,000,000 whenever the variable was unset. Both
scripts/play.sh and scripts/bench.sh set 100,000,000,000 explicitly. So **every terminal-invoked
experiment in ten days passed, and the app a human double-clicks died early.** Launching the bundle the way
Finder does, through LaunchServices, reproduced it exactly: BlueWake took the foreground, the window opened
at 960x720, the card line showed the user save slot, the logo cutscene played, and the process hard-stopped
at retraces=502 with `[trace] 50M blocks` still on the opening movie. A player saw the logo and then the
window vanished before the title screen existed.

Fix, in `runtime/host/src/main.c`: an unset budget now means no cap, zero and unset are the same, and the
run loop guards on `max_blocks == 0 || blocks < max_blocks`. This is inert for everything that was
measured, because the scripts set the budget explicitly and keep the budget they were measured with.

**The lesson to carry forward is methodological, not mechanical.** Ten days were spent instrumenting a
process that no human would ever launch, and reading evidence from it as if it were the product. The app a
player double-clicks is the artifact under test. Exercise that one.

## The correction v7 carries forward, still true

The bare Mach-O `build/runtime-host-dsp/bluewake_host` is not frontmost-eligible on macOS 14+, so macOS
refuses to activate it and no key event is ever delivered. That is why osascript frontmost, AXRaise and
CGEventPostToPid silently no-opped in every earlier session. The app bundle is eligible. Nothing in the pad
or latch path was ever at fault, and the pad-latch theory stays dead: dropped and accepted presses print
byte-identical pad records and an identical SI word.

## Falsified, and do not reopen

* **The "dead stretch" of retraces 740-1150 swallowing presses.** The framebuffer at retrace 700 shows the
  opening logo movie over the 2002/2003 copyright line. `title-ready` at retrace 333 is the *cutscene*
  running, not the interactive title. A cutscene ignoring input is correct behavior.
* **Multiple pad latches per guest frame.** Byte-identical pad records on dropped and accepted presses.
* **A press-shape defect.** Press-to-file-select latency was 149 retraces in the m1d direct-launch run and
  149 retraces in the launch-services run with the press 100 retraces later. The accepted press behaves
  identically however late it arrives.
* **macOS refusing activation.** It was always the bare binary.

## What is actually left

### Already proven on the player path, this session

The app was driven from a double-click through the new-game flow with OS-delivered presses only, and every
screen renders correctly:

| screen | reached at retrace | press that caused it |
| --- | --- | --- |
| opening cutscene | 333 onwards | none, it is a movie |
| File Selection | 1324 | J at 1175 |
| quest-log detail panel | ~1900 | J at 1697 |
| Name Entry | ~3000 | J at 2945 |

File Selection takes **two** presses, not one: the first selects the quest log and reveals the
Start/Copy/Erase/Return row, the second takes Start. That is the game's design. Screens are pixel-stable
while the game waits for input, so a still frame is the game holding rather than the app hanging.

Instrument note: [input-probe] can show msFocus=0x0 while kbFocus is non-zero and the press still lands,
which happens when the BlueWake window is not the main window while another app is frontmost. Trust
kbFocus non-zero plus [input-chain] cpad_trig=0x0100; do not gate on msFocus.

In order.

1. **Finish the new-game flow to gameplay.** Enter a name in the Name Entry grid, confirm END, and reach
   controllable Outset with a frame captured at each step (new-game intro, Opening.arc, Room44). Two
   things this settles at once: whether the game logic still advances past the file menu, and whether the
   name grid answers d-pad and A the way the rest of the menus do. If a synthetic-pad route reaches Outset
   but the keyboard path does not, the difference is the defect, and it is in input, not in game logic.
2. **M1c - the human press.** The one press above is CGEventPost, which enters the same OS input stack a
   physical key does but is still synthesized. One press by a human hand, on a physical keyboard, on a
   window opened by double-clicking BlueWake.app, closes M1. Nothing else does. Ask for it and report it.
3. **Confirm the press timing a player will experience.** PRESS START becomes reachable around retrace
   950-1000 and a delivered press is accepted from roughly retrace 1050-1110; the boundary is bracketed,
   not pinned. That matters only if a press made *after* the prompt is visible is ignored, and it was not:
   a press at 1343 advanced on the first try. So the honest player instruction is "press J once PRESS
   START is on screen," and the prompt's 3-minute cutscene at the current 6 fps is an M2 speed problem,
   not an input problem. Do not spend iterations pinning a number that no longer gates anything.
4. **M2, M3 and H1/H2/H3** exactly as recorded in v4: Outset median at least 15 fps then 30 fps with the
   digest unchanged; Link hair correct at the GX owner with a screenshot and no geometry regression;
   continuous intelligible audio under human review; physical controller including reconnect; a ten-minute
   unattended soak with bounded RSS and no crash; one-command clean build and launch from a fresh clone
   plus the user's own disc, signed, with no Nintendo data in the artifact.

## Launch recipe, both paths

The bundle is the product; the bare binary is a debugging convenience that cannot take focus.

    # the human path - exactly what a double-click does
    open build/runtime-host-dsp/BlueWake.app

    # the instrumented path - LaunchServices semantics, with probes
    open --stdout /tmp/bw.out --stderr /tmp/bw.err \
         --env BLUEWAKE_INPUT_PROBE=1 --env BLUEWAKE_FRAME_TIMING=1 \
         --env BLUEWAKE_MAX_RETRACES=1600 build/runtime-host-dsp/BlueWake.app

    # the bounded experiment path
    env BLUEWAKE_INPUT_PROBE=1 BLUEWAKE_FRAME_TIMING=1 \
        BLUEWAKE_PLAY_HOST=$PWD/build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake \
        scripts/play.sh --retraces 1700 --capture-frame /tmp/bw.ppm --capture-retrace 1400 \
        >/tmp/bw.out 2>/tmp/bw.err

Deliver a press only when the window owns the foreground, and re-check immediately before each press,
because the machine is in active use by its owner:

    /tmp/bwfocus $pid 400        # policy=0 name=BlueWake / activate=1
    /tmp/bwkey 0 38 700          # pid 0 = global HID post; keycode 38 is J
    lsappinfo info -only name $(lsappinfo front)    # expect BlueWake

A press counts only when [input-probe] reports pressed=1 J=1 with a non-zero kbFocus AND [input-chain]
shows cpad_trig=0x0100. If /tmp/bwkey is gone: `clang -O2 -o /tmp/bwkey /tmp/bwkey.c -framework
ApplicationServices`. The computer-use pressKey path is unavailable here (AXError.cannotComplete, the
emulator saturates its main thread). Do not spend iterations there.

## Instrument in the same build as the fix

Host-only rebuilds are cheap: `cmake --build build/runtime-host-dsp --target bluewake_host -j 8`, about two
minutes, and it re-assembles and re-signs BlueWake.app in the same step. Composite and Aurora rebuilds are
expensive and ship only alongside the change they evaluate. Extend `host_input_chain_probe`
(`main.c:2390`) rather than adding a parallel mechanism. Known weak spot, worth one iteration and no more:
`name_scene` reads 0x00000000 in every line so far, including on frames where the name scene is provably
current, so `host_find_scene_by_proc_name(cpu, 0x000Eu)` is returning nothing during the title.

## The iteration contract

An iteration counts only if a human can now see, hear or do something they could not before, **or** one of
the five governing numbers moved. A diagnostic is a cost you pay to make a measurement trustworthy, never
progress by itself. If an iteration ends with no change to an observable and no change to a number, it
FAILED: revert it and change the mechanism, not the constant. One failure is a signal; three failures of
the same shape means stop, measure the layer underneath, and say so out loud. Never write that no product
gate advanced as a successful outcome; that sentence is this project's failure signature.

Twenty iterations. One hypothesis, one small change, one build, one measurement, and at most one line
appended to CURRENT.md. No status document may be longer than the change it describes. Report to the user
every three iterations in one paragraph: what a human can now see, hear or do that they could not before,
plus the five governing numbers.

## The five governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reports all five; it reproduces
the 2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |

fps is host-dependent; the digest, the turn count and the card are not. A/B only against numbers measured
on the host you are standing on. A change that moves fps but moves the digest is not a result, it is a
behavior change. The budget fix above is provably outside all five: the scripts set the budget explicitly.
A full bench re-run is the confirming check, not the change.

## What is already done - do not rebuild it

Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, and reaches the
title screen with correct video and audio on the Aurora window. The hybrid-O2 composite with the 256-cycle
cap is the promoted configuration. Aurora provides Metal video, SDL3 audio output and live pad input on
port 0 with installed keyboard bindings. scripts/route_digest.py and scripts/cycle_invariance.py are the
correctness oracles; scripts/bench.sh and scripts/play.sh are the measurement and play paths. The input
chain from the OS key event to the guest controller record is proven, and it must not be re-derived.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git;
  every PPM and PNG from this work stays in /tmp. `ref/` is gitignored, so any edit made there ships as a
  patch under `patches/aurora/`, never as committed vendored source.
* Preserve user data and unrelated work. One BlueWake process at a time.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. A synthetic button press is a
  route driver, **not** evidence that the keyboard works.
* Never add a new tier, slice or patch for a unit that already compiles whole. New code must remove cost,
  not add diagnostics.
* The guest-visible cycle contract, the route digest, the card outcome and the paced-PCM fingerprint must
  survive every promotion. Never weaken, skip or re-record a digest to pass.
* Keep Route A accepted artifacts and the dependency lock intact, and keep the private-data audit green.

## The first report this goal owes the user

One sentence, in this shape: *Double-click BlueWake.app, wait for PRESS START, press J once, and the file
menu appears - here is the probe line that proves the key arrived, here is the screenshot of what it
caused, and here is the human press that closed it.* Silence is not acceptable; neither is a document
about why it cannot be done.
