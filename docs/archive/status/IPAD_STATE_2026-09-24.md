# Where BlueWake stands, and how to put it on an iPad (2026-09-24)

This is the state at the end of the iPadOS simulator loop (goal v56, opened 2026-09-23). The next step
the user chose is a physical iPad run before any change of route. Nothing here has run on hardware yet.

## What works on the iPad simulator

Every item below was run on the iOS simulator (iPadOS 26.5, one simulator at a time) with the build
at this commit; the evidence is in [CURRENT.md](CURRENT.md) under the dates given.

| Area | State | Evidence |
| --- | --- | --- |
| First run | Pass | A fresh install shows the first-run screen, imports the user's GZLE01 disc, prepares main.dol and rels/, and boots (2026-09-24 night, 6) |
| Boot to play | Pass | Dolby screen, title, file select, prologue, Outset play scene; control admitted on the macOS retrace |
| Input | Pass | SunPad touch controls (move, camera, buttons, D-pad, pause menu driven by touch), hardware keyboard, controllers (face buttons by position, R-shoulder Z, analog L/R, right stick C) |
| Saves | Pass | The save/reload acceptance script: pause-menu save, card written, guest quit, reload into control; saves carry the real date and time |
| Audio | Pass | The game's own mix, stereo (console SRAM emulated; it was mono before), the game's Stereo/Mono option persists; gameplay audio matches Dolphin in shape, level and stereo image |
| Graphics | Pass, bounded | Against Dolphin from shared inputs: title, file select, pause menu pages, text, the pier walk, swimming, the beach, the village, Link's shading, the sun's glare (EFB depth peeks) |
| Lifecycle | Pass | Home-screen round trips, audio interruptions, windowed mode and rotation on iPadOS 26 |
| Memory | Pass | 21,000- and 54,000-retrace soaks end normally; resident size flat around 640-770 MB |
| Phones | Pass, bounded | iPhone 17 and 17e simulators play the Outset save with the touch controls in the 4:3 letterbox bars |

## Speed (60 retraces a second is the game's authentic 30 fps)

On this M2 MacBook Air (fanless), with the Mac rested:

| View | Retraces a second |
| --- | --- |
| Prologue, title | 60 |
| Outset play, dialogue | 58-60 |
| Interiors (Link's house) | 60 |
| The village walk | 59.9 |
| The heaviest view (pier facing the island) | 51-54 |

The heavy view is bound by the translated game's CPU work (about 112 M cycles a retrace). Cheaper
routes to it are exhausted and recorded (QoS, render resolution, native leaf copies, the
leader-guard fold, exact FP fast paths). Back-to-back runs slow as the fanless Mac heats up; a
fanless iPad will show the same in long sessions. An M2 iPad Pro has the same CPU cores as this Mac
and no simulator in the way, so it should do at least as well; that is what the hardware run will
tell.

## Open

- **Hardware run** (next, by the user): install on an iPad and repeat the checks above.
- **The route decision after it:** the play-scene source port is the only lever that brings the
  heavy view to 60; the alternative is keeping the speed and widening coverage (sailing, Forsaken
  Fortress, a dungeon). See [GOAL_PROMPT_V56](../GOAL_PROMPT_V56_2026-09-23.md) item 6.
- On the 11-inch iPad the default move stick and D-pad sit over the minimap's corner (SunPad's
  default; the layout editor moves them).

## Building the app for a device

Updated 2026-09-25: the steps that were here assumed this Mac's `build/` and `generated/` trees and
an unpublished RecompCore commit. A fresh checkout now builds the device app from the disc image
with one command, `scripts/ios/build_device.sh DISC.iso`; see [DEVICE_BUILD.md](../../DEVICE_BUILD.md).

## Installing on the iPad (not done yet)

The signing certificate, Developer Mode, the provisioning profile for `dev.bluewake.BlueWake`,
`build_device.sh --identity ... --profile ... --install` and copying the disc to the iPad are in
[DEVICE_BUILD.md](../../DEVICE_BUILD.md#signing-and-installing-on-the-ipad).

## What to check on the iPad

- It reaches the title and PRESS START, and a new game or the pier save reaches play.
- Touch controls, a keyboard or controller if at hand; the three-dot menu and layout editor.
- Save from the pause menu, quit, reopen, continue.
- Sound in stereo; a call or Siri pauses the game.
- Speed: the pier facing the island and the village; how it holds after 15-30 minutes (heat).
- Report a Problem (three-dot menu) writes a diagnostic report to share.
