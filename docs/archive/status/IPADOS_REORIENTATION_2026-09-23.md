# iPadOS reorientation, 2026-09-23

Opened at the user's direction on 2026-09-23: *reorient the project so the game actually works,
make it run on iPadOS, use the simulators one at a time, and do not use a hardware iPad yet.*
That instruction is the explicit approval PRD section 1 requires for changing platform order:
iPadOS is no longer deferred behind macOS. Nothing else in the PRD is weakened. The definition of
done, the authenticity rules and the data boundaries are unchanged.

## 1. The decision, and why

**The product track is Route A (ahead-of-time translated retail code) hosted on iPadOS now.
Route B (the source port) continues as the long-term speed track and does not gate iPadOS.**

The deciding facts, measured this session:

| Question | Route A | Route B |
|---|---|---|
| Reaches controllable gameplay today | Yes, on macOS since 2026-08-30 | No. The composed probe stops in the room scene's actor decode (40th iteration) |
| What an iOS port needs | A host build for iOS; the composite runs unchanged | Everything Route A has, plus the remaining port coverage |
| The game module's platform dependencies | 11 libc imports from `libSystem` (`calloc`, `memcpy`, `fprintf`, ...) | n/a |
| Cost per guest instruction | 5.8 to 27.2 host instructions | 0.91 (ROUTE_DECISION section 8) |

Route B is still the only route whose speed is set by compiled C, and the 2026-09-22 decision to
fund it stands. What changes is the order of work. Route A already plays the game, and nothing in
its host is macOS-specific in a way that matters. So the fastest path to "the game works on
iPadOS" is to host Route A there, then close its speed gap on the same device class, with Route B
maturing behind it.

## 2. What was built

- **`apple/ios/`**: a CMake project that builds `BlueWake.app` for the iOS simulator from the
  unchanged `runtime/host/src` sources plus GXRuntime/Aurora, SDL3 (built from source), Dawn,
  abseil, fmt, zstd and freetype. The only iOS-specific sources are:
  - `src/ios_entry.m`: gives `main()` to SDL so UIKit starts first, fills in default paths
    (`Documents/BlueWake/...` in a real container, or the repository via `BLUEWAKE_ROOT` in the
    simulator dev loop), line-buffers stdout and stderr, and schedules the touch controls;
  - `src/touch_controls.cpp`: on-screen stick, C-stick, A/B/X/Y/Z, L/R, Start and D-pad, drawn
    with ImGui inside the game's own Aurora frame and driven by SDL finger and pointer events.
    State goes into Aurora's virtual pad channel (`PADSetVirtualStatus`), so touch, keyboard and
    controllers merge on port 0. The controls hide when a controller button is pressed and come
    back on the next touch;
  - `src/dsp_common_shim.cpp`: the nine Dolphin `Common` helpers the donor DSP interpreter
    needs, so the DSP compiles from source without Dolphin's log, config and file layers.
- **`scripts/ios/retag_macho_platform.py`**: rewrites the platform field of `LC_BUILD_VERSION` in
  place for arm64 objects, archives and dylibs. Apple arm64 code is ABI-identical across macOS,
  iOS and the simulator. This retargets Dawn's `ios-arm64` package (643 objects) and the 487 MB
  translated composite to the simulator without recompiling either. Device builds should use real
  platform toolchains; the retag is a simulator bridge.
- **`scripts/ios/sim_run.sh`**: builds, retags and signs the composite, boots **one** simulator
  (it shuts down any other booted device first), installs, launches with the repository's
  prepared inputs, and writes logs and screenshots to `local-research/ipad/<stamp>/`. It takes
  `--route`, `--route-full`, `--retraces`, `--screenshot-after` and `--env KEY=VALUE`.

## 3. Defects found on the way, and where they lived

1. **The DSP adapter is required, not optional.** Without it, JAS waits forever for the DSP
   microcode (`lod_done=0`, pc parked at `0x8028E708`). The iOS build turns it on by default.
2. **`runtime/host/src/main.c` did not compile without the DSP adapter**, and line 5499 carried a
   literal NUL byte where `'\\0'` was meant. Both are fixed; the macOS build is unaffected.
3. **Black window on macOS and iOS alike: recompcore patch 0056.** `dol_aurora_initialize()`
   opened the first Aurora frame but never opened it to the FIFO translation worker added on
   2026-09-22. Every batch fell back to the packet sink, the core sink never saw the display copy
   that triggers the first present, and nothing was ever presented (`gx-core submitted=0`,
   zero `[frame-pacing]` lines, confirmed with a macOS control run). One call fixes it.
4. **The picture froze in the Outset play scene** while the game kept running (`gx-core failed=1`):
   the translation worker handed the front end whole batches (60,483 bytes measured), which
   overflowed its 8,192-event per-flush trace; the failure was sticky and silent. Recompcore patch
   0057 feeds batches in the 1 KiB slices the single-threaded path always used and names any
   front-end failure. The next full route ended with `failed=0` and 27.0 M draws submitted.
5. **A UIKit overlay never showed.** A transparent view over SDL's Metal view (or its window)
   was attached, sized and frontmost, yet never composited, because the game loop owns the main
   thread. Recompcore patch 0058 adds host overlay and event hooks to the Aurora backend, and the
   controls now draw inside the game's frame with ImGui, as Dusklight's do. The same patch fits the
   picture to the guest's aspect on iOS instead of stretching it.
6. **The simulator delivers Mac clicks as pointer events.** SDL reports them as mouse input, so
   the controls treat the left button as one more finger. That path also serves iPad trackpads.

## 4. Evidence: the iPad simulator runs the retail route

iPad Pro 11-inch (M5) simulator, iOS 26.5, the unattended `--route-full` input schedule,
`local-research/ipad/20260923-090831/` (private, ignored):

| Milestone | Retrace (iPad simulator) | Retrace (macOS route) |
|---|---|---|
| title ready | 333 | 333 |
| file select | 535 | 535 |
| new-game intro | 773 | 773 |
| opening complete | 13,850 | 13,850 |
| play scene (Outset) | 13,910 | 13,910 |
| player control admitted | 20,256 | 20,256 |

Rendering: Aurora on Metal through Dawn (`Apple iOS simulator GPU`, Compatibility feature level,
BGRA8 surface, Fifo present), 904,218 draws submitted by gxcore, 48,700 texture uploads. Audio: the
donor DSP runs (1.83 M DSP DMAs, first non-zero sample at retrace 354) into an SDL audio stream.
Screenshots show the title sky, the woodcut prologue with its text, Outset's cliff and sea, Aryll
on the lookout, and Link on the deck with the HUD after control is admitted.

**Touch input drives the game.** With the on-screen controls, tapping START and A on the title
screen took the game to file select (`name-scene-create` at retrace 999, `file-select` at 1,086),
and the log shows the pointer and finger events that did it
(`local-research/ipad/20260923-093944/`).

**A save plays end to end by touch.** Starting from the 2026-09-22 acceptance card
(`scripts/ios/sim_run.sh --card local-research/acceptance/20260922-144732/save.card`), in
landscape: the file screen lists the save in Quest Log 1, two taps on the on-screen A load it, the
Outset play scene is up at retrace 1,510 with the full HUD, and dragging the on-screen stick walks
Link down the pier and into the sea (player proc 4 to 5; screenshots in
`local-research/ipad/20260923-094710/`). Taps first failed on menus because a click lasts less than
a guest frame; buttons now hold for at least 120 ms. In landscape the picture fills the height and
is pillarboxed at 4:3. The player probe's `pos` field did not change while Link visibly moved, so
it is not the live position and should not be used as movement evidence.

**The macOS host benefits too.** After patch 0056 a windowed macOS control run presents again: its
frame at retrace 701 has 2.74 M non-blank pixels and gxcore reports `failed=0`.

Speed on the M2 host: about 58 retraces a second through the prologue and about 35 in the Outset
play scene, which is the same as the macOS play window (31 to 33). The simulator adds no measurable
cost, and the speed gap is Route A's known cost (ROUTE_DECISION section 1).

## 5. Loop iteration 2: saves, audio and lifecycle on the simulator

**Save, the guest's own quit, and reload pass on the iPad simulator.**
`scripts/ios/sim_save_acceptance.sh` runs the macOS acceptance clauses through the iOS app, with
Metal and the SDL audio device live (`local-research/ipad/acceptance-20260923-095148/`, PASS):
certified route (opening 13,850, play scene 13,910); the pause menu's save screen; the save step
(proc 18) with 3 card writes; the guest unmounting the card for its own reset; the card differing
from the start card by 1,218 of 98,304 bytes; and a second launch on that card reaching the play
scene without the new-game intro, with control at retrace 833.

**Audio is the game's own mix.** The same run captured 400 s of the paced output at 32 kHz
stereo: 93% of samples non-zero, RMS 2,837. That shows the DSP mix reaches the output path; it is
not a listening test.

**Backgrounding keeps the game.** Going to the home screen and back used to leave a white screen
while the guest kept running: begin_frame failed while the surface was gone, and nothing opened a
frame again. Recompcore patch 0059 retries begin_frame once batches arrive with no frame open and
lets the host hold the guest while it is inactive. After a home-screen round trip the log shows
the app resigning and regaining active and the frame reopening, and the picture returns. The
simulator suspends the process in the background, so the guest stops for that time (a 10.3 s gap
in the retrace clock) instead of running unseen.

**The app exits when the host returns.** SDL's iOS main used to return into a UIKit app with no
game; a bounded run now ends the process, which the acceptance script relies on.

**Where the time goes on the simulator** (8 s sample of the Outset play scene, busy threads only):
translated game code 47.8%, the host binary 37.0% (the DSP interpreter 12.7% and the GX front end
and draw construction 8.0% of all busy samples), memcpy/memset 6.9%. That matches the macOS owner
profile, so the speed work belongs to the shared route, not to iOS.


## 6. Loop iteration 3: speed

Measured Outset speed on the simulator: 27-33 retraces a second in lighter views and about 18 in
the heaviest (the save's pier view facing the island), against 60. Recompcore patch 0060 turns the
guest-alias lookup (5.7% of the game thread) into a binary search: -8.0% play-window cycles on the
macOS instruction bench with the digest unchanged, and +13% in that heavy view on the simulator
(17.6/18.4 -> 20.2/20.4 retraces a second). Details in [CURRENT.md](CURRENT.md). The next lever is the
DSP (16.7% of the game thread).

## 7. Loop iteration 4: the DSP

Dolphin's high-level Zelda ucode replaces the DSP interpreter on iOS: -14.8% play-window cycles,
the same milestones within 11 retraces, control at the same retrace, an audio envelope that
matches the interpreter's at r = 0.999, and a passing simulator save acceptance. Details in
[CURRENT.md](CURRENT.md). Simulator wall-clock numbers on this Mac vary run to run by more than
these gains (other processes compete for the cores), so the decisions rest on the macOS
instruction bench and the simulator runs confirm behaviour.

## 8. Loop iteration 5: rendering waits

With the DSP cheap, the game thread waited 66% of the time on the GX translation worker, which
waited on the GPU because every other frame overflowed Aurora's staging buffers. Recompcore 0062
reuses identical uniform blocks (79% of them) and sizes the vertex area for a whole frame: the
simulator's heavy Outset view went from about 21 to 34.5 retraces a second with no splits and no
spikes, and the macOS windowed bench window from 25 to 40.6. Frame captures are identical with and
without reuse. Details in [CURRENT.md](CURRENT.md).

## 9. Loop iteration 7: profile-guided optimization

PGO of the 100 hottest chunks (95% of game-code time) cuts the play window by 19.6% of cycles with
the certified digests unchanged, lifting the macOS headless play-window median to 53.6 retraces a
second and the simulator's heavy Outset view to 40.7. Gather-pipe batching (recompcore 0065)
removes the per-write worker lock with identical frame captures. Details in [CURRENT.md](CURRENT.md).

## 10. Loop iteration 8: inline helpers and wider training

Inline FP-availability and paired-single fast paths in the generated header, rebuilt through a PGO
cycle trained on the certified route, the full route into control and play from a save: -5.8%
instructions per play retrace with the digests unchanged; the simulator's heavy Outset view reaches
46.3 retraces a second. Details in [CURRENT.md](CURRENT.md).

## 11. Where the day ends

On the same unattended route in the same simulator, the Outset play scene went from 30.6 to 48.4
retraces a second, the stretch after control from 35.5 to 56.3, and the heaviest view from about 18
to 46.3, against 60 for authentic speed. Refuted along the way: -mcpu=apple-m1 (already the
default) and -O3 (neutral). The table is in [CURRENT.md](CURRENT.md).

## 12. Loop iterations 9-11: first run, the picture, the SunPad shell

The evening's work turned from speed to what a person sees. A new player's iPad now shows a
first-run screen that imports the disc through the document picker and prepares the game files on
the device. The title logo, minimap and HUD icons drew with the wrong palettes: the texture event
was built before the SETTLUT register the SDK writes last (recompcore 0066). The title now matches
the Dolphin reference. The touch controls are SunPad's, with the three-dot menu, settings and
layout editor, and each of those pauses the game (recompcore 0067). A capture of Link then showed
the 3D scene at 640x480, because gxcore made EFB copies at the guest's size; the fix makes them at
the render scale, as Aurora's own path does. Runtime-object PGO took another 2.2% of play-window
instructions. Details and evidence paths are in [CURRENT.md](CURRENT.md).

## 13. Loop iteration 12: the GX worker, and a title glitch

A sample of the simulator showed the GX translation worker spending a quarter of its time clearing
and copying memory it did not need, and re-uploading the same HUD images about 23 times a frame
because their cache identity carried a palette they never use. recompcore 0073 and 0074 remove both
(-4.6% rendered instructions; uploads 28,144 -> 194 over 1,800 retraces), and the game thread's wait
for the worker fell from 4.5% to 1.7%. The simulator save acceptance passes on the build. A report of
the title logo drawing as outlines is open: it has not reproduced on captures, live screenshots, the
START route or other FIFO batch sizes, and the shell's pause and resume paths are next. Audio stutter
is the speed gap by another name; the heavy view is still about 47 retraces a second against 60.

## 14. Loop iteration 13: host work off the game thread's hot path

Two host-side changes, each proven exact by the certified digest, took the play window from 334.0 to
291.8 M instructions per retrace: GX FIFO writes no longer pay for a full device sync (-10.0%), and
the per-block edge service answers its common case without a stack frame (-3.0%). The simulator's
heavy Outset view went from 47.7 to about 52 retraces a second, and the audio stretcher (recompcore
0077) now covers the rest: starved audio pushes fell from 39% to under 1%. Touch controls survive a
home-screen round trip (recompcore 0076), and a player who takes the controls during a scripted run
turns the script off.

## 15. Loop iteration 14: the pause menu's text

Two causes behind "still clear text issues", both found against a Dolphin reference of the same
pause menu. The A/B action labels are small textures the game rewrites in place without anything
our cache notices, so "Charts" stayed where "Choose" and "Return" belong; small textures are now
re-hashed once a frame, at no measurable cost (recompcore 0080). The SAVE tab was blank because its
palette load carried junk in address bits the hardware ignores; we rejected it and drew the tab with
an arrow icon's palette. Masking the address as the hardware and Dolphin do fixes that and about
1,700 other menu palette loads (recompcore 0081). The cursor's corner brackets drew with a black
outline because the game loads a second texture's palette into the same slot before drawing, which
the hardware honours and we did not; a palette load now refreshes the textures bound to that slot
(recompcore 0082). The simulator save acceptance passes on the build.
The same fault may explain the title logo the user once saw drawn as outlines, but a title run
showed no junk loads, so that report stays open.

## 16. Loop iteration 15: the actor search off the chassis

A new count of chassis round trips per guest address showed half of all of them in play come from
one loop: the game searching its actor list by id, 68,000 nodes a retrace, three round trips per node
to compare one word. The host now runs those iterations itself between two of the loop's own
boundaries, charging the same cycles and leaving the same registers and stack, and only where the
translated code would not have stopped. The certified play window fell from 280.0 to 218.7 M host
instructions a retrace (-21.9 percent) with the route digest unchanged, and the simulator's heavy
Outset view reached 57.4 retraces a second with a median frame of 16.8 ms, against 60 and 16.7 ms
for full speed. Over the whole unattended route the simulator now plays the prologue at 60, the
play-scene start at 59.9, Outset play at 57.7 and the opening dialogue at 58.9 retraces a second,
with 0.14 percent of audio pushes starved.

A hardware keyboard now walks Link too: the simulator binds a controller to player 0, and Aurora's
pad read had let that controller's stick replace the keyboard's; the two now combine (recompcore
0083), tested through SDL's own keyboard entry point.

## 17. Loop iteration 16: long runs, phones, and 3D play against Dolphin

The steered route runs on the fast path again and reaches the house interior on the simulator,
which plays interiors at 60 retraces a second; soaks of 54,000 and 30,000 retraces end normally with
memory flat at about 637 MB. The village walk is the heaviest stretch (45-60 on the simulator), and
the kernel's counters show why: the simulator and the Mac do the same work per retrace (about 380 M
instructions, IPC 4.2), and the simulator simply got fewer cycles a second on a busy fanless M2 Air.

The app runs on the iPhone 17 simulator, and on phones the touch controls now default into the black
bars beside the 4:3 picture, where they no longer cover the minimap and item HUD. iPadOS 26's
windowed mode and a rotation to full screen both resize the picture and controls. The user's broken
title logo predates the palette fixes of recompcore 0080-0082 and does not reproduce from a normal
launch of the current build.

Moving 3D play is now checked against Dolphin from shared inputs. scripts/ref_walk_corpus.sh sends
one input schedule through Dolphin and then BlueWake: off the pier, swimming, the beach, the grass,
drowning and respawn all match. scripts/pad_trace_to_dtm.py replays BlueWake's own steered route in
Dolphin (two SI polls a retrace): the village exterior matches, and where the two parted at a ramp,
BlueWake one retrace later does exactly what Dolphin did, so collision agrees as well.

The first audio reference against Dolphin then found the game had played in mono all along: the
host never emulated the console's SRAM, so the game read an empty one and chose mono. A small SRAM
device on EXI channel 0, with Dolphin's defaults, gives stereo that matches Dolphin's (side/mid
0.241 against 0.215, the same level), on by default in the iOS app.

## 18. Where the simulator loop ends

By the night of 2026-09-24 the simulator app covers the objective's list: the user's disc imported
on first run, title to controllable Outset play, touch, keyboard and controller input, stereo audio,
saves that reload, and 58-60 retraces a second everywhere measured except the densest view (51-54),
with the cheap speed levers exhausted and recorded. The user chose a physical iPad run next; the
state and install steps are in [IPAD_STATE_2026-09-24.md](IPAD_STATE_2026-09-24.md).

## 19. The loop

See [GOAL_PROMPT_V56_2026-09-23.md](../GOAL_PROMPT_V56_2026-09-23.md). One game on the Mac at a time
(simulator, macOS host or Dolphin reference, enforced by scripts/one_game_guard.sh), no edits to
scripts a running job uses, every iteration ends with a run and its evidence, and graphics changes
are judged against a private Dolphin reference of the same moment. The queue is ordered by what
stops a person from playing on an iPad, with the picture first.
