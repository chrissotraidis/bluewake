# BlueWake priorities

The ranked list of what to fix and build next. Read this first if you are picking up work in this
repository, then [DIRECTION.md](DIRECTION.md) for what "done" looks like (releases, platform parity, contributors)
and [AGENTS.md](../AGENTS.md) for the rules.

Updated October 8, 2026 (JST, evening): reordered around performance on `main` and the
[direction](DIRECTION.md) taken from Wind Waker HD Recomp, with the day's reports. Owner: Chris.
Evidence: [the October 8 triage record](status/TRIAGE_2026-10-08.md), [the October 7 fix pass](status/FIXES_2026-10-07.md)
and the dated files in [status/](status/).

## Right now

0.6.0 is out, and most of its fixes are confirmed by players. The loudest complaint online is speed: low frame
rates and slowdowns in busy scenes. So the next builds focus on making `main` run better, while the way BlueWake
ships catches up with [DIRECTION.md](DIRECTION.md).

1. **Speed on `main`**, by the plan in [PERFORMANCE.md](PERFORMANCE.md): measured with one scripted tour, biggest lever first.
2. **Controllers that miss the 0.6.0 fixes at launch** (#138): a small fix with a big reach.
3. **Releases without game code** on every platform, built on first launch ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)).
4. **Linux** (#107) merged and released, then **Android** (#93) at the iPhone app's standard.
5. **Contributors' pull requests**, reviewed within two days.

## What's next after 0.6.0

### The plan, in order

Each item says why it is here (the evidence), its first step, and what it is waiting on. Re-rank when the evidence
changes.

1. **Controllers connected at launch miss the 0.6.0 fixes** ([#138](https://github.com/chrissotraidis/bluewake/issues/138), Tier 1).
   *Evidence:* pdale-boop, on Windows with two controllers: the left stick fix and the player 1 handoff (#61) apply
   only to a controller connected after BlueWake starts. With one already connected at launch, the log has no
   `[pad] player 1: the game's own stick dead zone` line, and the old dead zone and jump are back. The handler in
   `runtime/host/src/mouse_camera.c` is installed after Aurora has added the controller. Most players start with
   their controller plugged in, so most of them don't get the fix. *First step:* run the same handoff for
   controllers already connected when the handler is installed. pdale-boop offered the pull request; review it the
   same day. Ship it in the next build.

2. **Speed: carry out [the runbook in PERFORMANCE.md](PERFORMANCE.md#the-runbook).** Phases 1 to 4 are written to
   be done in order by any agent: the benchmark tour, three small runtime fixes, the `--no-cold` test, then lean
   block copies. The file also holds the evidence (17 player logs) and what is ruled out. In short:
   - **A benchmark tour** (`scripts/bench_tour.py`, to write): the tester's own card, Outset and the training
     tour's stops unpaced, with checkpoint hashes that show two builds behave the same. Every speed change is measured with it.
   - **The 30% instruction gap.** BlueWake's game thread runs about 158 M instructions a retrace where a build from
     Wind Waker Recomp's translation runs 110 M on the same phone. Elliott's lean block copies, kept out by BlueWake's
     cycle-exact comparison, are the likely cause. It needs Chris's decision on the acceptance standard
     ("plays the same"), then one build each on Linux and Windows.
   - **Code the training skipped is compiled for size** on Windows and Linux: cutscenes, combat, bosses. One
     `--no-cold` build (added October 8) tests it.
   - **Smooth Motion's pacing** steps down to 30 and stays there for up to minutes on CPUs where its frames don't
     compete with the game; fix it in the shared runtime.
   - **Say when the renderer falls back** (OpenGL ES ran the bird scene at 4%, Vulkan at 74% or more).
   - **The graphics thread** on four-core CPUs, after those.

   The native entries (#179) are worth re-certifying sometime, but they were measured at about 1% of the game
   thread together, so they are not a speed fix.

3. **Cloud, wave and fog flicker** ([#136](https://github.com/chrissotraidis/bluewake/issues/136), Tier 1).
   *Evidence:* two NVIDIA players; clean at 30 FPS, flickering at 60 and 120, the forest's fog too; new in 0.5.0.
   *First step:* their runs with `DOL_AURORA_INTERP_ALL_VERTICES=1` and `DOL_GX_TRANSFORM_VERIFY=1`, one at a
   time (asked on #136); whichever stops it names the patch (0155 or 0152).

4. **Releases without game code** ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)). *First step:* a
   reviewed optimization profile so players skip the training run, then a Windows app that runs the builder on first
   launch with a progress screen. The ready-made Windows and Linux builds stay until that works.

5. **Linux** ([#107](https://github.com/chrissotraidis/bluewake/pull/107)). Two laptops and a Steam Deck have run it.
    Merged into `main` on October 8 (its shared change, a cheaper direct-call check, was shown byte-identical
    from a save state). Next: say when Vulkan is missing ([PERFORMANCE.md](PERFORMANCE.md), lever 4), then the package audit and Chris's approval for
    a release.

6. **Android at the iPhone app's standard** ([#93](https://github.com/chrissotraidis/bluewake/pull/93)). *Status:*
    up to date with 0.6.0 and playing on a Galaxy Z Fold 7; controller handoff works. Not started: the ⋯ menu and the
    touch controls, which must match the iPhone app ([the bar](DIRECTION.md#2-every-platform-gets-the-same-app)).
    It holds full speed when cool and drops to 20 to 23 FPS when the phone throttles. Its game thread runs about
    158 M instructions a retrace against 110 M in the port based on Wind Waker Recomp's translation, the same gap as
    the lean block copies in item 2.

7. **Smaller bugs from the 0.6.0 checks:** the Pictobox left stick can't move the cursor at "keep this picture?" and
    in the gallery (the zoom takeover in `mouse_camera.c` stays on); rumble goes to every connected controller; the
    right stick also aims, and is always inverted (pdale-boop, #186). The Pictobox fallback
    (`BLUEWAKE_CACHE_FLUSH_FALLBACK`) now passes on the Mac and Windows; turn it on by default once a physical iPad
    agrees.

8. **Features from [DIRECTION.md](DIRECTION.md#6-features-worth-taking):** gyro aiming (#188), HD textures from
    the menu, then a mod manager (#152).

9. **iPhone and iPad builds from Windows through PadMint** (#100, draft).

10. **Long term: native rendering**, replacing the CPU-side drawing conversion piece by piece with the decompilation.
    The largest gain and the most work.

No promise of 30 FPS on a Steam Deck yet: it takes the lean block copies and the cold-code fix together ([PERFORMANCE.md](PERFORMANCE.md)).

### Waiting on someone else

| Who | What | Where |
| --- | --- | --- |
| pdale-boop | The pull request for controllers connected at launch | #138 |
| Bighead-SMZ, MaLDox77 | The two flicker switches, one at a time | #136 |
| DonatelloEsq | The bird scene's sound on 0.6.0 (fixed for minibeas and pdale-boop) | #65 |
| ncarson9 | B and X fixed under Controller buttons with his one-line mapping | #61 |
| TheGameTuber | A 0.6.0 log with the controller in port 1 | #155 |
| saulob | Whether to bring #106 and #89 up to date, or have them carried | #106, #89 |
| LiquidAzir | The ⋯ menu and touch controls on Android | #93 |
| Elliott | Where the HD renderer work (lighting, shadows, GameCube/HD switch) lives | Direct |
| A physical iPad | The Pictobox fallback, before it is on by default | #13 |

### How well each 0.6.0 fix is proven

Close an issue when its reporter confirms.

| Fix | Proven by | Still needs |
| --- | --- | --- |
| Dungeon maps (#74) | Mac; Windows from pdale-boop's copied save in Dragon Roost Cavern | The original reporter |
| Intro music (#97) | Confirmed by KTroopA9 (Windows) and minibeas (Mac); heard by pdale-boop | Done |
| Bird scene and later cutscene sound (#65) | minibeas's Mac log (`silent=0.1s of 46.2s`); pdale-boop heard it and three later scenes on Windows | DonatelloEsq |
| Controller as player 1 (#61) | pdale-boop on Windows, for a controller connected after launch | The launch case (plan item 1) |
| Baton (#156) | pdale-boop on Windows | The reporter |
| Left stick (#138) | pdale-boop: "a big improvement", for a controller connected after launch | The launch case (plan item 1) |
| Aim invert (#154) | pdale-boop on Windows | The reporter |
| VS2022 builder (#153) | Confirmed by the reporter | Done |

## How this list is ordered

1. **Game-breaking bugs first:** a crash, a soft lock, a controller that can't play, missing sound or
   drawing that players need, or a regression in the current release.
2. **Performance next:** low or uneven frame rates are the most common complaint, on every platform.
3. **Everything else after:** narrower bugs, then requested features, then new platforms.

Within a tier, what affects the most players and has the clearest fix comes first. Re-rank as soon as
evidence changes: a new crash or data-loss report goes straight to the top.

Keep the states apart: *suspected* (a hypothesis), *cause found* (shown in code, logs or a reproduction),
*fixed in main* (merged), *shipped* (in a release) and *confirmed* (the reporter says it works). Only
close an issue when its reporter confirms, or it is a clear duplicate. When you change a row, update the
issue, this page and, for Windows checks, [WINDOWS_TASKS.md](WINDOWS_TASKS.md) in the same pull request.

## Shipped in 0.6.0

Published October 8, 2026 from `806d65c` (Windows zip built from `4eb41f0`, the same game and app code). What each
still needs is a confirmation, not a release:

| Shipped in 0.6.0 | Issue | Still to confirm |
| --- | --- | --- |
| Dungeon maps draw their grid and rooms (RecompCore patch 0157) | [#74](https://github.com/chrissotraidis/bluewake/issues/74) | Windows check from the [task list](WINDOWS_TASKS.md#in-main-waiting-for-a-windows-build) |
| A controller recognized late, or left as player 2, plays as player 1 (October 7) | [#61](https://github.com/chrissotraidis/bluewake/issues/61) | A physical controller that needs `gamecontrollerdb.txt` |
| The Wind Waker baton is no longer mirrored (October 7) | [#156](https://github.com/chrissotraidis/bluewake/issues/156) | Conducting with a controller |
| No big dead zone or jump on the left stick (RecompCore patch 0159, October 7) | [#138](https://github.com/chrissotraidis/bluewake/issues/138) | Slow aiming, diagonals and drift on a physical controller |
| Intro music after the title music: on by default everywhere ([#163](https://github.com/chrissotraidis/bluewake/pull/163), [#172](https://github.com/chrissotraidis/bluewake/pull/172)) | [#97](https://github.com/chrissotraidis/bluewake/issues/97) | Players on Windows waiting for the title music, then a new file |
| Windows builder: Visual Studio 2022 builds again (October 7) | [#153](https://github.com/chrissotraidis/bluewake/issues/153) | The reporter's Visual Studio 2022 build |
| Option: invert the left stick's up and down when aiming (October 7, [#166](https://github.com/chrissotraidis/bluewake/pull/166)) | [#154](https://github.com/chrissotraidis/bluewake/issues/154) | Menu check on Windows |
| Linux port's shutdown fix for every platform (RecompCore patch 0158, [#167](https://github.com/chrissotraidis/bluewake/pull/167)) | [#56](https://github.com/chrissotraidis/bluewake/issues/56) | Windows quit check |
| The session log names the controller mapping in use ([#165](https://github.com/chrissotraidis/bluewake/pull/165)) | [#61](https://github.com/chrissotraidis/bluewake/issues/61) | None (a log line) |
| Touch controls no longer stay held after Apple menus open | none (found in testing) | Physical iPhone/iPad touch check |
| Opt-in Pictobox fix, `BLUEWAKE_CACHE_FLUSH_FALLBACK=1` (still off by default) | [#13](https://github.com/chrissotraidis/bluewake/issues/13) | Windows and physical iPad check; then decide the default |

Only Chris publishes releases. Follow [RELEASE.md](status/RELEASE.md) and run the release audit.

## Tier 1: bugs that break the game

| Rank | Problem | Who it hits | What we know | Fix and next step |
| --- | --- | --- | --- | --- |
| 1 | **Intro and cutscene music missing** ([#97](https://github.com/chrissotraidis/bluewake/issues/97), [#65](https://github.com/chrissotraidis/bluewake/issues/65)) | All platforms | *Fixed in 0.6.0 and confirmed*: disc reads complete after the game marks them pending. KTroopA9 and minibeas confirmed the intro; the bird scene plays its music on 0.6.0 (minibeas's Mac log, `silent=0.1s of 46.2s`), and pdale-boop heard it and the Forsaken Fortress arrival, Gohma and the Tower of the Gods rising on Windows. | Close #97. Close #65 when DonatelloEsq confirms. |
| 2 | **Some controllers don't work at all** ([#61](https://github.com/chrissotraidis/bluewake/issues/61) 8BitDo GameCube mod kit, [#155](https://github.com/chrissotraidis/bluewake/issues/155) GameCube controller on a Mayflash adapter) | Windows and Mac, any controller SDL doesn't know without `gamecontrollerdb.txt`, and anyone with several controllers | *Detection shipped in 0.6.0*: a connected controller takes player 1 whenever nobody has it ([details](status/FIXES_2026-10-07.md#controllers-and-player-1-61)); the reporter confirmed that connecting late gets it detected. *Layout suspected*: its buttons and sticks are scrambled with both his line and the stock file. SDL uses a mapping with a `crc:` field only on an exact CRC match, and the stock 8BitDo line puts the right stick on a2/a3 and L/R on a5/a4 where his tool found a3/a4 and b6/b7. The log now names the mapping in use (#165). #155 has no log yet. | #61: his one-line `gamecontrollerdb.txt` works with B and X swapped (October 7); asked him to fix those under Controller buttons on 0.6.0, then close. #155: the 0.5.0 log shows the adapter as four GameCube controllers, all removed 12 seconds in; asked for a 0.6.0 log with the controller in port 1 ([details](status/TRIAGE_2026-10-08.md#controllers-61-155)). |
| 3 | **Wind Waker baton left and right reversed** ([#156](https://github.com/chrissotraidis/bluewake/issues/156)) | Windows and Mac with the fast right-stick camera on (the default); new in 0.5.0 | *Shipped in 0.6.0* (fixed October 7): #44's C-stick flip for the game's own camera is skipped while the game's conducting flag is set, confirmed live on the Mac ([details](status/FIXES_2026-10-07.md#the-wind-waker-baton-156)). | Reporter confirmation; play each song with a controller. |
| 4 | **Clouds and distant waves flicker** ([#136](https://github.com/chrissotraidis/bluewake/issues/136)) | Windows on NVIDIA at 60 and 120 FPS; a regression from 0.4.0 | *Suspected*: 0.5.0 brought in water/HUD interpolation, transform reuse, texture mip and blending changes (RecompCore patches 0152 to 0155). Not reproduced on the Mac (Metal). | *Cause narrowed* (October 8): a second reporter (RTX 5070 Ti) shows it clean at 30 and flickering at 120, and the Outset forest's fog flickers too. Look at how the in-between frames carry moving-texture draws (patches 0152 to 0155). [Capture steps](WINDOWS_TASKS.md#flickering-capture-136). |
| 5 | **Left stick dead zone, then a jump to about 22%** ([#138](https://github.com/chrissotraidis/bluewake/issues/138)) | Every controller player | *Shipped in 0.6.0, confirmed for a controller connected after launch* (pdale-boop: no jump, smooth ramp, no drift). *Cause found* for the rest: a controller already connected at launch never gets the fix or the player 1 handoff, because the handler is installed after Aurora adds it. | Plan item 1: pdale-boop's pull request. |
| 6 | **Soft lock in the Forsaken Fortress** ([#76](https://github.com/chrissotraidis/bluewake/issues/76)) | One Windows report | *Unverified*: a Moblin knocked off a ledge mid-capture. May be the original game. | Reproduce with a copied save placed in the fortress (`scripts/save_set_restart.py`); compare with Dolphin before changing anything. |
| 7 | **HD texture packs: dark shading and orange hair** ([#80](https://github.com/chrissotraidis/bluewake/issues/80)) | Hypatia's pack | *Not a BlueWake bug*: the reporter sees the same shading in Dolphin with the same pack, and is contacting the pack's author. | Closed. |
| 8 | **Pictobox shows the previous photo** ([#13](https://github.com/chrissotraidis/bluewake/issues/13)) | All platforms | The 0.5.0 freeze is fixed. The stale second preview is repaired by the opt-in flag above in Mac and iPad simulator runs. | Physical iPad and Windows checks, then decide the default (see "Shipped in 0.6.0"). |
| 9 | **Pirate ship flag has no texture** ([#69](https://github.com/chrissotraidis/bluewake/issues/69)) | Windows 0.3.0 report | *Needs info*: not rechecked on 0.5.0. The sail module keeps its texture in its own data, like Molgera's floor (#126, fixed). | Load a save near the ship and check on the Mac; if it is blank, trace the sail's texture address as #126 was traced. |

## Tier 2: performance

Players on older or mid-range CPUs see 20 to 25 FPS on Outset and in heavy scenes, whatever their GPU.
This is the most visible problem after the bugs above, and it is on BlueWake's side.

| Rank | Bottleneck | Reports | What we know | Next step |
| --- | --- | --- | --- | --- |
| 1 | **Game thread (the translated game code)** | [#137](https://github.com/chrissotraidis/bluewake/issues/137) (Ryzen 7 2700), [#59](https://github.com/chrissotraidis/bluewake/issues/59) (bird scene), [#159](https://github.com/chrissotraidis/bluewake/issues/159) (i7-6500U laptop: 10 to 20 FPS where Dolphin gets 30 to 40), Steam Deck on the Linux port (22 FPS on average, CPU-bound, about 30% short of 30) | In #137's log, 35 of 83 seconds ran below full speed (lowest 65%); 27 name the game thread. On the Mac, the host's per-block bookkeeping costs about a sixth as much as the game code itself, so fewer block boundaries is the lever. The Windows release has BlueWake's conservative prepaid block copies, which skip nearly every block that touches memory: Elliott's `lean_memory.py` then changes 0 accesses and his native entries certify 0 of 15. His full transform failed BlueWake's strict boot-route comparison on October 2 ([findings](status/FIXES_2026-10-07.md#performance-findings)). | Follow [PERFORMANCE.md](PERFORMANCE.md): the benchmark tour, the acceptance standard, then Elliott's lean block copies behind a builder flag. James Koehler-Killeen (KongMing) offered to measure on the Steam Deck first (`perf record`, `[fps-dip]` lines), then send small RecompCore PRs with before/after numbers ([plan](https://github.com/chrissotraidis/bluewake/pull/107)). October 8: denormals ruled out (the game sets FPSCR NI at boot, so flush-to-zero is on all session); jkoehler11's `perf record` on the Deck puts the time in collision filtering, J3D model walking and display-list emission, the native entries' functions, which save only about 1% together. The Linux bird scene's 4% was an OpenGL ES fallback; with Vulkan its lowest is 74% ([details](status/TRIAGE_2026-10-08.md#the-bird-scene-one-cutscene-two-problems-65-59)). #179's 0 of 15 native entries is the conservative copies, not drift; the comparison tests need a Linux loader to be rerun ([details](status/TRIAGE_2026-10-08.md#native-entries-certify-0-of-15-179)). |
| 2 | **Graphics thread (GX worker) converting the game's drawing to GPU work on the CPU** | [#86](https://github.com/chrissotraidis/bluewake/issues/86) (i7-6950X, RTX 3080), Steam Deck reports on Discord | In Outset the GX worker is 92 to 98% busy and the game drops to 55 to 90% speed; resolution barely matters. The title flyover is about 20,000 draws a frame. The app's own optimization profile cut the GX worker's time per frame from about 16.9 to 14 ms on an i9 (October 2), and older Visual Studio builds now skip that profile with a note (#153). | Profile the GX worker in the same Outset spot with warm caches; find the hot paths in command conversion and vertex decoding. |
| 3 | **Smooth Motion's own cost** | #137, [#79](https://github.com/chrissotraidis/bluewake/issues/79) | At 120 FPS the in-between frames cost CPU time and pause after every slowdown (six step-downs in #137's two minutes); its helper thread used 62 to 70% of a core at the title. | Compare the same spot at 30, 60 and 120 to measure the cost; make sure the pause is not triggered by the interpolation's own work. |
| 4 | **Build time** | [#104](https://github.com/chrissotraidis/bluewake/issues/104) (3 hours on an M4 MacBook Air), [#153](https://github.com/chrissotraidis/bluewake/issues/153) (45-minute compile on an i5-12600KF) | The Windows builder assumes 2.5 GB per compile job, but #153 measured about 0.3 GB per clang process with 19 GB free. Training takes about 30 minutes of the Windows build. | Recheck the memory-per-job estimate to allow more jobs; see whether a reviewed profile could let players skip training. |

Use matched comparisons: same scene, settings, warm caches and hardware, one change at a time. Do not
tell players to lower settings as the fix; the logs show the limit is CPU time per game frame.

## Tier 3: smaller bugs, easy wins and support

| Problem | Issue | Fix |
| --- | --- | --- |
| Windows build fails with Visual Studio's clang older than 22 (`app.profdata` format) | [#153](https://github.com/chrissotraidis/bluewake/issues/153) | *Fixed in 0.6.0, confirmed by the reporter* with VS 2022's clang 19. A way to choose the Visual Studio install when several are present would help testers. |
| Portable mode still writes `imgui.ini` to `%APPDATA%` | [#64](https://github.com/chrissotraidis/bluewake/issues/64) | Fixed in [#184](https://github.com/chrissotraidis/bluewake/pull/184) (RecompCore patch 0160): `imgui.ini`, controller remaps and keyboard bindings follow the portable folder. Waiting for a Windows run. |
| CPUs without AVX2 can't run the Windows build | [#77](https://github.com/chrissotraidis/bluewake/issues/77) | 0.5.0 now says so instead of closing silently. No build for those CPUs is planned. |
| Waiting on the reporter to confirm a shipped fix | [#55](https://github.com/chrissotraidis/bluewake/issues/55), [#58](https://github.com/chrissotraidis/bluewake/issues/58), [#66](https://github.com/chrissotraidis/bluewake/issues/66), [#73](https://github.com/chrissotraidis/bluewake/issues/73) | Close each when its reporter confirms. |

## Enhancements

In order. None of these should displace a Tier 1 or Tier 2 item.

1. **Controller picker** for player 1 in both menus ([#155](https://github.com/chrissotraidis/bluewake/issues/155)), once its log shows whether the Mayflash adapter needs it.
2. **Stick dead zone setting** ([#138](https://github.com/chrissotraidis/bluewake/issues/138)), only if players with worn sticks still need one after the October 7 fix.
3. **Invert the left stick's up and down while aiming** ([#154](https://github.com/chrissotraidis/bluewake/issues/154)): an option in both menus, October 7 ([#166](https://github.com/chrissotraidis/bluewake/pull/166)).
4. **Ultrawide** ([#70](https://github.com/chrissotraidis/bluewake/issues/70)): needs the widescreen code's camera and HUD values extended past 16:9.
5. **Wind Waker HD-style options** ([#152](https://github.com/chrissotraidis/bluewake/issues/152)): Hero Mode, moving while aiming, the shorter Triforce quest. These would be built-in options like Better Wind Waker's.
6. **Graphics hotkey** ([#108](https://github.com/chrissotraidis/bluewake/issues/108)): waits for Wind Waker Recomp's HD renderer to be brought over.
7. **Mod folder for game files and models** ([#152](https://github.com/chrissotraidis/bluewake/issues/152)): BlueWake reads files from the disc image and has no file replacement path yet. Large.
8. **Wii U-style interface** ([#57](https://github.com/chrissotraidis/bluewake/issues/57)): not planned now.

Open contributor pull requests: FPS overlay position ([#106](https://github.com/chrissotraidis/bluewake/pull/106)) and centering
the window ([#89](https://github.com/chrissotraidis/bluewake/pull/89)) need review against current `main`.

## Community ideas (Discord, October 7)

| Idea | From | How it fits |
| --- | --- | --- |
| Rework parts of RecompCore for speed; about 30% is needed for 30 FPS on a Steam Deck | KongMing | Tier 2, rank 1. Measure first, then small pull requests to RecompCore `bluewake-next` with matched before/after numbers and identical game behavior (the boot-route comparison). |
| Replace the CPU-side GX conversion with native rendering bit by bit, using the decomp (about 70% done) | wowjinxy | Long-term. It extends what the certified native J3D, skinning and math replacements already do; each swap needs a comparison test. Tier 2 rank 1 (game thread) comes first. |
| A save editor | Dale | Welcome as a separate tool. It has to keep both copies of each quest log and their checksums right, as `scripts/save_set_restart.py` and the Dolphin importer do, and work on copies. |
| Test saves at key scenes: a card in Ganon's Castle with New Game+, and about 45 save states from Outset to Ganon | Discord testers | Reproduce scene bugs without playing there: #65's bird scene, #76's fortress, #69's ship, dungeon maps. Keep them private; never commit or attach them. |

## Platforms

| Platform | Where it stands | What it needs |
| --- | --- | --- |
| Linux ([#56](https://github.com/chrissotraidis/bluewake/issues/56), [PR #107](https://github.com/chrissotraidis/bluewake/pull/107)) | jkoehler11 (KongMing on Discord) checked saves, controllers, settings, HD packs and quitting on his desktop and a Steam Deck. His shutdown fix is in RecompCore and pinned in `main` (#167). His Ryzen 9 5900X holds 30; the Steam Deck averages 22, the same CPU limit Windows players hit (Tier 2). | Bring current `main` in and drop the PR's own pin, CI green, a package audit. Steam Deck speed is shared performance work, not a Linux-only gate. Chris approves a Linux release. |
| Android ([#75](https://github.com/chrissotraidis/bluewake/issues/75), [PR #93](https://github.com/chrissotraidis/bluewake/pull/93)) | LiquidAzir's port builds an APK from the player's disc. Quiet since October 5; on October 8 he was asked whether he's still on it, with an offer to carry it forward on a branch that keeps his commits. | The iPhone app's menu and touch layout, current `main`, and performance evidence, as asked on the PR. The Mac has the Android SDK and NDK; an Android device is needed to test. |
| Windows through PadMint ([PR #46](https://github.com/chrissotraidis/bluewake/pull/46), [PR #100](https://github.com/chrissotraidis/bluewake/pull/100)) | Parked. | A maintainer decision; see [PROPOSALS.md](PROPOSALS.md). |
| European disc ([#60](https://github.com/chrissotraidis/bluewake/issues/60)), Intel Mac ([#48](https://github.com/chrissotraidis/bluewake/issues/48)), Switch ([#62](https://github.com/chrissotraidis/bluewake/issues/62)) | Not planned now. | Revisit after Tier 1 and 2. |
