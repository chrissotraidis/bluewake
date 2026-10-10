# BlueWake 0.6.1 release record

The working record for 0.6.1. The steps are in [GOAL_LOOP.md](../GOAL_LOOP.md). Fill in each check with the device,
the commit and what was seen. A row stays "not yet" until someone runs it.

## Candidate

| | |
| --- | --- |
| Version | 0.6.1, build 6 (`version.json`, set at the freeze) |
| Candidate commit | not yet |
| RecompCore | `34fe2c18db4cd06bbf1b6d6d2b760be88a5c104c` (patches 0160 to 0162) |
| Previous release | 0.6.0, October 8 |

## What it contains

| Change | Issue | Ships to | Checked so far | Still to check |
| --- | --- | --- | --- | --- |
| Controllers plugged in at launch get the smooth stick and play as player 1 (pdale-boop, #195) | #138, #155 | Windows, Mac, Linux | pdale-boop on Windows with two controllers; a test in CI | Mac with a controller at launch |
| Portable mode keeps controller remaps, keyboard bindings and `imgui.ini` in the `user` folder (#184) | #64 | Windows | CI | A Windows PC ([WINDOWS_TASKS.md](../WINDOWS_TASKS.md)) |
| The window opens in place: centred, then where you left it (saulob, #197; RecompCore patch 0161) | #89 | Windows | CI (merged October 9) | A Windows PC |
| The FPS counter's position under Display (saulob, #106; runtime in #199, patch 0162) | none | Windows, Mac, Linux | Windows 11 by saulob; CI | Mac menu |
| A rare crash that stopped the game with `unmapped pc=0x8180fff0` when a disc read finished at the wrong moment (pdale-boop, #211) | none | Every platform | Windows: the crashing save runs, and the stage select's places that crashed run | Mac play; a long session |
| A second build into the same folder works instead of failing at `native-game-math` (pdale-boop, #209) | #59 | Windows and Linux builders | Tests; a real rerun on Windows | None |
| Linux: the game module compiles about twice as fast, and runs up to 10% faster on slower CPUs (pdale-boop, #218) | #215 | Linux builder | i5-12600KF: identical checkpoints at all 468 places | Steam Deck; an older 4-core PC |
| Linux builds from source (jkoehler11, #107) | #56 | Linux | Two laptops and a Steam Deck | A package audit before any Linux download |
| Building your own copy is faster: training takes about 12 minutes instead of 30 (pdale-boop, #202) | none | Windows and Linux builders | i5-12600KF and i5-6500, matched speeds and checkpoints | The build-day builds themselves |

Not in 0.6.1: `--lean-blocks` on by default (measured in #208, waiting for Chris's decision), the Smooth Motion
pacing and renderer-fallback fixes (the October 11 plan), Android (#93).

## Release notes (draft, for the release page)

BlueWake 0.6.1 is a small update with fixes for controllers and Windows, and a native Linux build you can make
from source. Every version needs your own USA (GZLE01) disc image. Nothing from the game is included.

- A controller that's already plugged in when BlueWake starts now gets the smooth stick and plays as player 1.
  Before, the 0.6.0 stick fix only reached controllers connected after launch (#138). Thanks to pdale-boop.
- Windows: the window opens where you left it, or centred the first time, instead of appearing and then jumping.
  Thanks to saulob (#89).
- Windows portable mode keeps controller remaps, keyboard bindings and window layout in the `user` folder, so
  nothing is written to `%APPDATA%\BlueWake` anymore (#64).
- Fixed a rare crash (`unmapped pc=0x8180fff0`) that could stop the game anywhere when a disc read finished at
  exactly the wrong moment. Thanks to pdale-boop, who tracked it down (#211).
- You can choose where the FPS counter sits, under Display. Thanks to saulob (#106).
- Building your own copy is faster: the optimization training now plays the tour of the game in parallel, about 12
  minutes instead of 30 on a 16-thread PC, with the same speed afterwards. Thanks to pdale-boop (#202).
- Linux: BlueWake builds and runs natively from your own disc, with saves, controllers, settings and HD packs
  (#107). Thanks to jkoehler11 (KongMing), and to fehnomenal and the Steam Deck testers. The steps are in
  `docs/LINUX.md`. The game module now compiles about twice as fast on Linux and runs up to 10% faster on slower
  CPUs (pdale-boop, #218). Make sure Vulkan is installed: without it, BlueWake falls back to a much slower renderer.

Speed is the next big focus. The plan, and how it's being measured, is in `docs/PERFORMANCE.md`.

## Checks

| Check | Platform and device | Commit | Result |
| --- | --- | --- | --- |
| Windows build from the commit | | | not yet |
| A controller connected at launch: smooth stick, player 1, the `[pad]` lines in the log | | | not yet |
| Portable mode: remaps kept in `user`, nothing new in `%APPDATA%\BlueWake` | | | not yet |
| The window opens in place; a removed monitor gives a centred window | | | not yet |
| App-only IPA, PadMint audit, release check | | | not yet |
| Source zip and recipe | | | not yet |
| `check_public_assets.sh` on every asset | | | not yet |

## The lean-blocks measurement (not released)

Done a day early by pdale-boop, from `acfaf32` on Windows and `9285e07` on Linux, so no lean build is needed on
build day. The rows are in [PERFORMANCE.md](../PERFORMANCE.md#results) (#208): 5 to 8% faster on an i5-12600KF's
P-cores, 8 to 10.5% on its E-cores, 10.6 to 12.4% on an i5-6500 under Linux with 9.9% fewer instructions, and 15½
minutes of play with no audio loss or fatal lines.
