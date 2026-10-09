# BlueWake priorities

The ranked backlog and the evidence behind it. What is being done on which day is in [GOAL_LOOP.md](GOAL_LOOP.md);
this page says what comes next and why. Owner: Chris. Updated October 9, 2026. The October 8 version, with the
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
| 1 | Controllers plugged in at launch miss the stick fix and player 1 | [#138](https://github.com/chrissotraidis/bluewake/issues/138), [#155](https://github.com/chrissotraidis/bluewake/issues/155) | Fixed in main (#195), ships in 0.6.1 | Reporters confirm on 0.6.1 |
| 2 | **Speed: the 30% instruction gap.** On the same phone BlueWake runs 173 M instructions a retrace where Wind Waker Recomp's translation runs 110 M; the lean block copies are the difference | [#59](https://github.com/chrissotraidis/bluewake/issues/59), [#137](https://github.com/chrissotraidis/bluewake/issues/137), [#159](https://github.com/chrissotraidis/bluewake/issues/159), [#86](https://github.com/chrissotraidis/bluewake/issues/86), [#93](https://github.com/chrissotraidis/bluewake/pull/93) | `--lean-blocks` in main (#196), off | Measured October 10, decided October 11 ([PERFORMANCE.md](PERFORMANCE.md#the-plan)) |
| 3 | Clouds, distant waves and fog flicker at 60 and 120 FPS on NVIDIA, new in 0.5.0 | [#136](https://github.com/chrissotraidis/bluewake/issues/136) | Suspected: RecompCore patches 0152 to 0155 | The reporters' runs with `DOL_AURORA_INTERP_ALL_VERTICES=1`, then `DOL_GX_TRANSFORM_VERIFY=1` |
| 4 | **Speed: how it feels.** Smooth Motion drops to 30 for seconds after a hitch on big CPUs and costs too much on small ones; a missing Vulkan silently falls back to a slow renderer | [#137](https://github.com/chrissotraidis/bluewake/issues/137), [#79](https://github.com/chrissotraidis/bluewake/issues/79), [#56](https://github.com/chrissotraidis/bluewake/issues/56) | Cause found, fixes written up (runbook phase 2) | October 11 |
| 5 | Pictobox: the left stick can't move the cursor at "keep this picture?"; rumble goes to every controller; the right stick also aims, always inverted | [#191](https://github.com/chrissotraidis/bluewake/issues/191), [#190](https://github.com/chrissotraidis/bluewake/issues/190), #186 | Cause found (`mouse_camera.c`, `haptics.c`) | October 11 |
| 6 | **Linux as a download.** It builds and plays from source on two laptops, a Steam Deck and a Gentoo desktop (#203: 99% lowest speed at Outset) | [#56](https://github.com/chrissotraidis/bluewake/issues/56), [#107](https://github.com/chrissotraidis/bluewake/pull/107), [#200](https://github.com/chrissotraidis/bluewake/pull/200), [#203](https://github.com/chrissotraidis/bluewake/issues/203) | In main from source | AppImage (#200), the release check, Chris's approval: 0.7.0 |
| 7 | **The benchmark tour:** one command that measures a build at fixed spots and proves two builds behave the same | none | Specified (runbook phase 1) | October 12 to 16 |
| 8 | **Android at the iPhone app's standard:** the ⋯ menu, touch layout and features match; speed is the gap (69% at Outset on a Fold 7) | [#75](https://github.com/chrissotraidis/bluewake/issues/75), [#93](https://github.com/chrissotraidis/bluewake/pull/93) | ⋯ menu done; up to date with 0.6.0 | Lean blocks, then the label decision |
| 9 | **Speed, horizon 2:** lighter timing, natives that remove round trips, cached display lists, the graphics thread on four-core CPUs | [#86](https://github.com/chrissotraidis/bluewake/issues/86), [#179](https://github.com/chrissotraidis/bluewake/issues/179) | Planned | 0.8 |
| 10 | **Releases without game code:** a maintainer-trained profile, then a first-launch build with a progress screen ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)) | none | Not started | After lean blocks settle the build options |
| 11 | Older single reports: a Forsaken Fortress soft lock, the pirate flag's missing texture, the Pictobox's stale photo fix on by default | [#76](https://github.com/chrissotraidis/bluewake/issues/76), [#69](https://github.com/chrissotraidis/bluewake/issues/69), [#13](https://github.com/chrissotraidis/bluewake/issues/13) | Unverified, needs info, opt-in | Reproduce from a copied save; a physical iPad for #13 |
| 12 | **Features,** in order: gyro aiming, HD textures from the menu, Elliott's HD renderer work, a controller picker, ultrawide, Wind Waker HD options, a mod folder, a graphics hotkey | [#188](https://github.com/chrissotraidis/bluewake/issues/188), [#155](https://github.com/chrissotraidis/bluewake/issues/155), [#70](https://github.com/chrissotraidis/bluewake/issues/70), [#152](https://github.com/chrissotraidis/bluewake/issues/152), [#108](https://github.com/chrissotraidis/bluewake/issues/108) | Requests | After the speed work |
| 13 | iPhone and iPad builds from Windows through PadMint | [#100](https://github.com/chrissotraidis/bluewake/pull/100), [#46](https://github.com/chrissotraidis/bluewake/pull/46) | Parked drafts | A maintainer decision |

Not planned now: the European disc (#60), Intel Macs (#48), Switch (#62), the Wii U-style interface (#57). CPUs
without AVX2 get a clear message and no build (#77).

## Waiting on someone else

| Who | What | Where |
| --- | --- | --- |
| bessian | Dungeon maps on his save, Saturday | #74 |
| Bighead-SMZ, MaLDox77 | The two flicker switches, one at a time | #136 |
| TheGameTuber, pdale-boop | The launch fix on 0.6.1 | #155, #138 |
| saulob | #106 brought up to date with `main` | #106 |
| jkoehler11 | #194 without its PERFORMANCE.md change | #194 |
| cforain | #200 narrowed to the disc chooser and the AppImage; a Steam Deck run and a video clip | #200, #56 |
| LiquidAzir | Android measured with a lean build | #93 |
| Elliott | Where the HD renderer work (lighting, shadows, the GameCube/HD switch) lives | Direct |
| A physical iPad | The Pictobox fallback, before it is on by default | #13 |

Shipped and waiting for the reporter to confirm: #156 (baton), #154 (aim invert), #64 (portable mode, in 0.6.1),
#55, #58, #66, #73.

## Contributors' pull requests

Review within two days ([DIRECTION.md](DIRECTION.md)). Open now: Android (#93, LiquidAzir), the FPS counter's
position (#106, saulob), the Linux test loader (#194, jkoehler11), the Linux AppImage (#200, cforain).

