# BlueWake priorities

The ranked backlog and the evidence behind it. What is being done on which day is in [GOAL_LOOP.md](GOAL_LOOP.md);
this page says what comes next and why. Owner: Chris. Updated October 10, 2026 (evening). The October 8 version, with the
0.6.0 tables and full evidence, is kept in [status/PRIORITIES_2026-10-08.md](status/PRIORITIES_2026-10-08.md).

## How it's ordered

1. **Bugs that break the game first:** a crash, a soft lock, a controller that can't play, missing sound or drawing,
   or a regression in the current release.
2. **Performance next:** low and uneven frame rates are the loudest complaint, on every platform.
3. **Everything else after:** narrower bugs, then features, then new platforms.

Within a tier, what reaches the most players with the clearest fix goes first. A new crash or data-loss report goes
straight to the top. Keep the states apart: *suspected*, *cause found*, *fixed in main*, *shipped*, *confirmed*. Close
an issue only when its reporter confirms, or it is a clear duplicate.

## The list

| # | What | Issues | State | Next |
| --- | --- | --- | --- | --- |
| 1 | Controllers plugged in at launch miss the stick fix and player 1 | [#138](https://github.com/chrissotraidis/bluewake/issues/138), [#155](https://github.com/chrissotraidis/bluewake/issues/155) | Fixed in main (#195), ships in 0.6.1. #155's adapter shows four controllers and player 1 goes to the first port, empty or not | Reporters confirm on 0.6.1; player 1 to the first controller that presses a button (October 11) |
| 2 | **Rare crashes.** `unmapped pc=0x8180fff0` when a disc read finished at the wrong moment, found by warping to all 468 places | none | Fixed in main (#211), ships in 0.6.1 | A crash sweep of all 468 places on every release candidate, once #210 costs nothing when off |
| 3 | **Speed: the 30% instruction gap.** On the same phone BlueWake runs 173 M instructions a retrace where Wind Waker Recomp's translation runs 110 M; the lean block copies are the difference | [#59](https://github.com/chrissotraidis/bluewake/issues/59), [#137](https://github.com/chrissotraidis/bluewake/issues/137), [#159](https://github.com/chrissotraidis/bluewake/issues/159), [#86](https://github.com/chrissotraidis/bluewake/issues/86), [#93](https://github.com/chrissotraidis/bluewake/pull/93) | Lean blocks on by default (#220); hidden symbols on Linux (#218); Elliott's draw fusion in #225. His other work since October 3 (faster loads, natives rounds 4, 5 and 7, a smaller upload) roughly closes the rest | 0.7.0, then the port in [GOAL_LOOP.md](GOAL_LOOP.md) |
| 4 | Clouds, distant waves and fog flicker at 60 and 120 FPS on NVIDIA, new in 0.5.0 | [#136](https://github.com/chrissotraidis/bluewake/issues/136) | Suspected: RecompCore patches 0152 to 0155 | The reporters' runs with `DOL_AURORA_INTERP_ALL_VERTICES=1`, then `DOL_GX_TRANSFORM_VERIFY=1` |
| 5 | **Speed: how it feels.** Smooth Motion drops to 30 for seconds after a hitch on big CPUs and costs too much on small ones; a missing Vulkan silently falls back to a slow renderer | [#137](https://github.com/chrissotraidis/bluewake/issues/137), [#79](https://github.com/chrissotraidis/bluewake/issues/79), [#56](https://github.com/chrissotraidis/bluewake/issues/56) | Smooth Motion on 8+ threads and the renderer notice in #224 (in CI); Smooth Motion is already off by default everywhere | 0.7.0 |
| 6 | Pictobox: the left stick can't move the cursor at "keep this picture?"; rumble goes to every controller; the right stick also aims, always inverted | [#191](https://github.com/chrissotraidis/bluewake/issues/191), [#190](https://github.com/chrissotraidis/bluewake/issues/190), #186 | Fixed in #222 and #223 (in CI); the right stick's aim is unchanged | 0.7.0; reporters confirm |
| 7 | **Linux as a download.** It builds and plays from source on two laptops, a Steam Deck and a Gentoo desktop (#203: 99% lowest speed at Outset) | [#56](https://github.com/chrissotraidis/bluewake/issues/56), [#200](https://github.com/chrissotraidis/bluewake/pull/200), [#203](https://github.com/chrissotraidis/bluewake/issues/203), [#214](https://github.com/chrissotraidis/bluewake/issues/214), [#215](https://github.com/chrissotraidis/bluewake/issues/215), [#217](https://github.com/chrissotraidis/bluewake/pull/217) | In main from source; compiles twice as fast (#218). An AppImage built on a new desktop fails on SteamOS (glibc); the Steam Deck runs about 20 FPS, CPU-bound | #217 (glibc 2.39 ceiling), then #200, the release check, Chris's approval: 0.7.0 |
| 8 | **The benchmark tour:** one command that measures a build at fixed spots and proves two builds behave the same | none | Specified (runbook phase 1); #210's `BLUEWAKE_WARP` makes it simpler | October 12 to 16 |
| 9 | **Android at the iPhone app's standard:** the ⋯ menu, touch layout and features match; speed is the gap (69% at Outset on a Fold 7) | [#75](https://github.com/chrissotraidis/bluewake/issues/75), [#93](https://github.com/chrissotraidis/bluewake/pull/93) | ⋯ menu done; up to date with 0.6.0 | Rebuilt on `main` with lean blocks and hidden symbols, measured on the Fold 7; then the label decision |
| 10 | **Speed, horizon 2:** lighter timing, natives that remove round trips, cached display lists, the graphics thread on four-core CPUs | [#86](https://github.com/chrissotraidis/bluewake/issues/86), [#179](https://github.com/chrissotraidis/bluewake/issues/179) | Planned | 0.8 |
| 11 | **Releases without game code:** a maintainer-trained profile, then a first-launch build with a progress screen ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)) | none | Not started | After lean blocks settle the build options |
| 12 | Older single reports: a Forsaken Fortress soft lock, the pirate flag's missing texture, the Pictobox's stale photo fix on by default | [#76](https://github.com/chrissotraidis/bluewake/issues/76), [#69](https://github.com/chrissotraidis/bluewake/issues/69), [#13](https://github.com/chrissotraidis/bluewake/issues/13) | Unverified, needs info, opt-in | Reproduce from a copied save; a physical iPad for #13 |
| 13 | **Features,** in order: gyro aiming, HD textures from the menu, Elliott's HD renderer work, a controller picker, ultrawide, Wind Waker HD options, a mod folder, a graphics hotkey | [#188](https://github.com/chrissotraidis/bluewake/issues/188), [#155](https://github.com/chrissotraidis/bluewake/issues/155), [#70](https://github.com/chrissotraidis/bluewake/issues/70), [#152](https://github.com/chrissotraidis/bluewake/issues/152), [#108](https://github.com/chrissotraidis/bluewake/issues/108) | Requests | After the speed work |
| 14 | iPhone and iPad builds from Windows through PadMint | [#100](https://github.com/chrissotraidis/bluewake/pull/100), [#46](https://github.com/chrissotraidis/bluewake/pull/46) | Parked drafts | A maintainer decision |

Not planned now: the European disc (#60), Intel Macs (#48), Switch (#62), the Wii U-style interface (#57). CPUs
without AVX2 get a clear message and no build (#77).

## Waiting on someone else

| Who | What | Where |
| --- | --- | --- |
| Bighead-SMZ, MaLDox77 | The two flicker switches, one at a time | #136 |
| TheGameTuber, pdale-boop | The launch fix on 0.6.1; for TheGameTuber, which adapter port his controller is in | #155, #138 |
| pdale-boop | The stage select with no cost when it's off | #210 |
| cforain | #217's CI result; a Steam Deck run with lean blocks and #218; a video clip | #217, #215, #56 |
| LiquidAzir | Android rebuilt on `main` with `--lean-blocks`, measured at Outset | #93 |
| Elliott | Where the HD renderer work (lighting, shadows, the GameCube/HD switch) lives | Direct |
| A physical iPad | The Pictobox fallback, before it is on by default | #13 |

Shipped and waiting for the reporter to confirm: #156 (baton), #154 (aim invert), #64 (portable mode, in 0.6.1),
#55, #58, #66, #73. Confirmed and closed October 10: #74 (dungeon maps).

## Contributors' pull requests

Review within two days ([DIRECTION.md](DIRECTION.md)). Merged October 9 and 10: #106, #194, #207, #208, #209, #211,
#216, #217, #218, the zoom from #213 (as #221) and, in CI, the AppImage setup from #200 (as #226). Open: Android
(#93, LiquidAzir), the stage select (#210, pdale-boop, waiting for no cost when off).
