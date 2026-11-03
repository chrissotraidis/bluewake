# The Windows and Linux recipe on the Mac, on the benchmark tour (October 20, 2026)

The question: should Mac (and then iPhone and iPad) builds use the module preparations and behaviour timing that
the Windows and Linux builders have used by default since #277 and #335? Until now they are opt-in on Apple.

## How it was measured

- Both builds from `f2611b1` (0.8.0), the same disc (identical `main.dol`, composite digest `54f54434…`), local
  training, `scripts/builder/build.sh DISC --platform macos --jobs 8`, on an M3 Max.
- **Default:** the 0.8.0 candidate's Mac build from October 17.
- **Recipe:** the same, plus `--combined-optimizations --lean-blocks --lean-memory --lean-deadlines --behaviour-timing`.
  Its log: 461,975 lean block copies, 526,629 lean memory accesses, 589,378 lean deadline tests, and 401,618 block
  starts always prepaid.
- `scripts/bench_tour.py` from a copy of a card, each run in its own home, user and cache folders. Headless: three
  pairs, alternating default and recipe. Rendered: one default run, two recipe runs. Desktop apps were open (load
  average 4 to 6). The card was unchanged afterwards.

## Results

Headless game frames a second (medians of three), and the game thread's busy share in the rendered runs, where both
hold 30 game frames a second:

| Stop | Default | Recipe | Change | Rendered busy, default | Rendered busy, recipe (two runs) |
| --- | --- | --- | --- | --- | --- |
| Outset | 47.4 | 77.9 | +64% | 65% | 43% / 43% |
| Windfall | 48.6 | 82.2 | +69% | 64% | 41% / 40% |
| Dragon Roost Cavern | 64.2 | 113.7 | +77% | 53% | 33% / 35% |
| Forest Haven | 38.5 | 65.7 | +71% | 83% | 50% / 47% |
| sea by the Fortress | 35.3 | 59.8 | +69% | 89% | 53% / 51% |

Every pair was 64% to 79% faster at every stop. Each build repeats itself: two runs of one build are within 1.5% with
matching checkpoints. The recipe's game module is 447 MB instead of 527 MB, and it compiled in 24 minutes instead of 90.

The two builds' checkpoints differ from the first stop. That is expected: behaviour timing lets a timer land up to a
block later, so the game state isn't cycle-identical (see the October 12
[behaviour timing record](BEHAVIOUR_TIMING_2026-10-12.md#results): the same 468-place sweep outcomes and identical
audio). One rendered recipe run logged 30 checkpoints instead of 31 (8100 missing); the repeat logged all 31.

This run doesn't separate the preparations from behaviour timing. Earlier M3 Max rows in
[PERFORMANCE.md](../PERFORMANCE.md#results) put behaviour timing at 7 to 9% on top of the lean preparations, so most
of the gain is the preparations.

## Not checked yet

- Play by hand on the Mac: a cutscene with music, a save and load, the menus, Picto Box, the HD look.
- Play by hand on the iPad, and an older iPhone (the iPhone 14, A15, is the next device to try).
- The 468-place sweep on this build.

## The same on an iPad Pro (M2), October 20

- **Baseline:** the iPad's installed app, 0.8.0 build 7 as PadMint 0.5.4 builds it for players from v0.8.0
  (`f2611b1`), signed again with the same identity so it could be reinstalled between runs.
- **Recipe:** the published 0.8.0 app (`--app BlueWake-v0.8.0-ios-unsigned.ipa`, the same host app) with the game
  module built from the same disc and source with `--combined-optimizations --lean-blocks --lean-memory
  --lean-deadlines --behaviour-timing`. Training was skipped: `--composite-pgo` gave it the Mac recipe build's trained
  profile, which comes from the same composite source (iOS builds train on a Mac test build anyway). Module 447 MB
  instead of 527 MB; compiled in 20 minutes.
- The tour's route and settings, launched with `devicectl` (`DEVICECTL_CHILD_` variables), with its own card and
  SRAM files in `Documents/BlueWake`. The player's card was never opened and hashed the same after every install.
  Because `bench_tour.py` can't save and load a state on the device, each run continues from a copy of the card with
  quest log 1 on Outset; two continues of one build already differ in checkpoints, so checkpoints say nothing here.
  Two alternating pairs (baseline, recipe, baseline, recipe), headless and paced (rendered at the game's own speed,
  as players run it).

| Stop | Headless, baseline | Headless, recipe | Change | Busy at 30 FPS, baseline | Busy at 30 FPS, recipe |
| --- | --- | --- | --- | --- | --- |
| Outset | 52.2 / 52.1 | 92.2 / 92.2 | +77% | 65% / 65% | 44% / 44% |
| Windfall | 52.0 / 53.0 | 93.3 / 93.5 | +78% | 68% / 67% | 43% / 44% |
| Dragon Roost Cavern | 75.1 / 77.0 | 136.1 / 136.4 | +79% | 51% / 50% | 34% / 34% |
| Forest Haven | 42.3 / 42.6 | 76.7 / 76.8 | +81% | 84% / 82% | 49% / 49% |
| sea by the Fortress | 39.1 / 39.0 | 67.9 / 68.5 | +75% | 88% / 88% | 53% / 53% |

Repeats of one build were within 2.5%. Every stop loaded in every run, with no dips. At the game's own speed both
builds hold 30 game frames a second; the recipe leaves the game thread about half idle where the baseline is near its
limit (Forest Haven and the sea by the Fortress), which is the headroom slower iPhones and iPads need.
