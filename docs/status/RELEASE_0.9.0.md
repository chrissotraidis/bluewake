# BlueWake 0.9.0 release record

The working record for 0.9.0, started October 24, 2026. It says what the next version includes, what waits, and every
check before publishing. Fill in each check with the device, the commit and what was seen; a row stays "not yet"
until someone runs it. Previous release: [0.8.0](RELEASE_0.8.0.md), October 18. Only Chris publishes.

**Frozen October 24, 2026 at `211abfd`** (#464's merge): `version.json` 0.9.0, build 8. Until the release is out,
only docs and fixes for what the checks find go into `main`. The Windows build follows
[WINDOWS_BUILD_0.9.0.md](WINDOWS_BUILD_0.9.0.md).

## The aim

Fix the bugs players hit most (the camera jitter with the direct right-stick camera, the Smooth Motion flicker),
make Mac, iPhone and iPad as fast as the PC builds, ship the Linux AppImage the Dawn fix unblocks, and carry the HD
texture importer and the one desktop menu that are already in `main`.

## Test candidate

| | |
| --- | --- |
| Version | 0.9.0, build 8 |
| Candidate commit | `211abfdecf13beb5b9f2120459a1a3cde50dd84e` (#464's merge, October 24): what the Windows build, the AppImage and the IPA are built from. `git checkout --detach 211abfd` |
| Draft release | v0.9.0, a draft targeting `211abfd` (no tag until it's published), with the IPA, the source zip and the recipe; the Windows zip and `SHA256SUMS` come next |
| Test candidate | `f55ffba5`, a local integration branch (October 24): `main` at `729298f` plus #455, #456, #398 (which carries #371 and #446), #431 and #450. It exists so the Mac and iPad can be played and measured before the merges; the release commit will be `main` after them |
| Test candidate 2 | `dee2a223` (October 24, evening): `main` at `c5ebeca` (with the rendering stack, #468) plus #455, #431 and #316. The Mac and iPad apps from the first candidate with only the app rebuilt from this tree; the game module is byte-identical. iPad Pro: the tour at the same speed (Outset 92.1, Windfall 93.2, Dragon Roost Cavern 133.4, Forest Haven 75.3, the sea by the Fortress 68.4), the card unchanged. Mac: the camera stand-still 0 of 1,813 frames off; with “The right stick aims too” off, first person ignores the right stick. This is the build for Chris's play |
| RecompCore | `1446450`, #398's pin: RecompCore#44 (the Dawn callbacks, on #43), then #40 (Smooth Motion), then #42 (sun shadows), on `bluewake-next` |
| Previous release | 0.8.0, October 18 (`f2611b1`) |

## What it contains

### Already in `main`

| Change | Issue | Ships to | Checked so far | Still to check |
| --- | --- | --- | --- | --- |
| **One desktop menu** for Mac, Linux and Windows, drawn from one table of every setting: the Controls tables, pausing under the menu as a setting, one control for every list, contrast and text size, greyed settings that say why, a Config tab and controller navigation (pdale-boop, #384 to #388) | #320 | Windows, Mac, Linux | CI on all three; merged October 20 | The menu with a controller on the Mac (Sunday); the Windows rows in [WINDOWS_TASKS.md](../WINDOWS_TASKS.md) |
| **Share Session Log** sends this session's log and the one before, so a crash can be reported after relaunching (#440) | none | Every platform | CI | A crash, a relaunch and a share on one platform by hand |
| **HD textures from a real Wind Waker HD disc image:** the importer reads the hashed content a WUD/WUX stores (#443), takes a key file, a pasted key or an emulator `keys.txt` as it is, keeps the form after a failure, and colours the result (#444) (jkoehler11) | #336 | Every platform but Android | Importer and parser tests; a USA WUX on Linux with the player's `keys.txt`: 3,546 textures imported, 124 skipped for a taken name, 17 unreadable | A Windows import from the menu; an iPhone or iPad import with a `keys.txt` |
| The Android builder runs on macOS too (benwolfram, #391), and Android training refuses a run that never reaches player control (#445) | #75 | Android (experimental, own-disc) | Tests; the Mac run to a signed APK | The builder on a Windows PC; fresh `--retrain` measurements on a phone |
| A sea compass switch, off by default: on, the red end points the way you're sailing, on a north-up dial (jslay88, #456, merged October 24) | none | Every platform | Linux screenshots on and off at all four headings | The clock hand; a Windows sail |
| **Controls:** a controller can play while another window is in front, off by default (#465); and “The right stick aims too (first person, items)”, on by default, so the right-stick camera can stay while first person and items aim with the left stick (#466) | #442, #402 | Windows, Mac, Linux | The settings table test; the host builds on the Mac. #466 on the Mac test candidate, scripted: the right stick turns first person only with the switch on | A controller by hand, the bow with the switch off; the Windows rows in WINDOWS_TASKS.md |
| **Smooth Motion without the flicker** in the HD look, clouds, wave crests and swimming (pdale-boop, #371, RecompCore#40; merged October 24 through #468) | #136 | Every platform | The contributor's platform checks; CI | The flicker cases above 60 FPS on the Mac and iPad; Windows |
| **Sun shadows from a shadow map,** so what is off screen still casts, in the HD look (off by default) (pdale-boop, #398, RecompCore#42; merged October 24 through #468) | #57 | Every platform but Android | The contributor's checks; CI. Mac test candidate, October 24: frames at all five tour stops with the HD look, with and without sun shadows, no artifacts; two runs of each are identical. Turning sun shadows on changes the guest state from the first checkpoint, the same way every run: on the high quality it widens the palm trees' cull boxes in game memory and the room model's culling, so shadows from off screen still cast. Off by default, so default builds are unaffected | The HD look on the Mac and iPad by eye, with the quality switches |
| **Linux builds at the player's CPU target again:** Dawn compiles with BMI2 (jslay88's RecompCore#43, with #44 for the pinned Apple and Android headers; #446 pins it and runs Linux CI with `-march=x86-64-v3`; merged October 24 through #468) | #433 | Linux (and every build, through the pin) | Linux/AppImage CI with the player's flags, Windows host, sanitizers; the callbacks compiled for Mac, iOS, tvOS and Android. Mac test candidate, October 24: the existing Dawn and pipeline caches (made by older builds on October 1) loaded and were written again over seven launches on Metal, with no cache errors | The restart check on Windows; a contributor's own-disc AppImage |

### In test candidate 2, waiting on Chris's play

#371 and #398 merged on October 24 because they only act when their settings are on (Smooth Motion, sun shadows),
both off by default, after the contributors' Windows and Linux play and the Mac checks; Chris's look at them on the Mac
and iPad is still in the checks below. These three change what every player gets, so they merge after Chris has
played them (AGENTS.md).

| Change | Issue | Ships to | Checked so far | Still to check |
| --- | --- | --- | --- | --- |
| **The camera no longer dips for a frame every few seconds** with Direct right-stick camera and aiming on: the feature hooks also run on the block a host turn starts on (jslay88, #455) | #401, #430 | Every platform | Linux: before, a dip every 2 to 15 seconds standing still, each one a frame where the camera hook did not run; after, none in a play session. The dispatch and handoff tests fail without the fix. Mac test candidate, October 24, a scripted stand-still from the tour's Outset state (the right stick nudged, then no input): 0 of 5,410 frames off the held camera view in three minutes; with only #455's host change reverted, 20 of 5,103, about one every 8.5 seconds, each a single frame at the game's own pitch. iPad Pro, the test candidate: 0 of 1,843 | Mac and iPad by hand: the camera in normal play, then climbing, quick doors and the draw tags. Windows: the row in WINDOWS_TASKS.md. Both reporters confirm before their issues close |
| **Mac, iPhone and iPad as fast as the PC builds:** the Windows and Linux module preparations and behaviour timing by default (`--conservative` keeps the plain translation) (#431) | #104 | Mac, iPhone, iPad | October 20: 64 to 77% more game frames a second on the M3 Max and 75 to 81% on the iPad Pro ([record](APPLE_RECIPE_TOUR_2026-10-20.md)). October 24 test candidate on the iPad Pro: the same gain, +75 to 79% over 0.8.0 (checks below) | Chris plays the Mac and the iPad by hand: a cutscene with music, saving and loading, the menus, the Picto Box, the HD look |
| **iPhone and iPad: the menu's switches keep the menu open** (Mods, Better Wind Waker Settings, Gameplay), so several settings can be changed in one visit (#316) | #104 | iPhone, iPad | A UI test on the iPadOS 27 simulator; signed for the iPad | Tap two switches in a row on the iPad: the menu stays open and the checkmarks move |

### Tools and records (nothing changes for players)

The benchmark tour keeps one guest state for repeated runs (#450) and will refuse a setup run that never reaches
gameplay (#459, for #458); the Steam Deck cache-flush and GX-worker records (#452, #453); what an Outset frame is made
of (#454); the Deck game-thread plan and its finished no-gain experiments (#421); the worker tests (#424); the
native and translated code guide (#435).

## Not in 0.9.0

| What | Why it waits |
| --- | --- |
| Android first-run screen and crash log (#460) | To be split into the screen and the log, with the disc image in its ready check, then run on a phone |
| Android HD import (#347) | A phone and lifecycle pass |
| The cold first Picto Box photo (#342, RecompCore#39) | Conflicts with `main` and its platform checks |
| iPhone and iPad menu switches that keep the menu open (#316) | A tap test on the iPad |
| The Mac's slow-chunk compile flags (#392) | With #431's defaults only 5 of 859 chunks take over three minutes and the module compiles in 36 minutes, so it would save little and cost about 1.5% of game speed; likely parked |
| Natives rounds 4, 5 and 7 (#179) | They cannot be generated from a current tree (#451); the set owners first |
| Clang 18 for the Deck (#437) | Its rendered and Game Mode measurements are still running |
| Building the iPhone and iPad app on Windows (#100) | Draft; its own platform checks |

## Release notes (draft, for the release page)

A draft in Chris's voice, for the changes Sunday's review accepts; drop any line whose pull request does not merge.

BlueWake 0.9.0 makes the Mac, iPhone and iPad builds as fast as the PC builds, fixes the camera dipping with the
right-stick camera, and stops the flicker with Smooth Motion. Every version needs your own USA (GZLE01) disc image.
Nothing from the game is included except in the ready-made Windows and Linux builds.

- **Much faster on Mac, iPhone and iPad.** Builds you make with PadMint or the builder now get the same module
  preparations and behaviour timing as the Windows and Linux builds: on the benchmark tour, 75 to 79% more game frames
  a second on an iPad Pro (M2) and 64 to 77% on an M3 Max, with a smaller game module and a much shorter build.
  `--conservative` builds it the old way.
- **Fixed: the camera dipping for a frame every few seconds** with Direct right-stick camera and aiming on (thanks to
  jslay88).
- **Fixed: flicker with Smooth Motion above 60 FPS** in the HD look, the clouds, wave crests and swimming (thanks to
  pdale-boop).
- **Sun shadows from a shadow map** in the HD look, so what is off screen still casts a shadow (pdale-boop). Off by
  default, under the HD look's settings.
- **One menu on every computer** (thanks to pdale-boop): Windows, Mac and Linux draw the same menu from one list of
  settings, with controller navigation, contrast and text size, a Config tab, greyed settings that say why, and
  pausing under the menu as a setting.
- **HD textures from a real WUD/WUX image:** the importer reads the content a real image stores, takes a key file, a
  pasted key or an emulator `keys.txt` as it is, keeps the form after a mistake, and says what it imported and
  skipped (thanks to jkoehler11).
- **iPhone and iPad:** the menu's switches keep the menu open, so you can change several settings in one visit.
- **Share Session Log** includes the session before, so a crash can be reported after relaunching.
- **Linux** builds at the player's CPU target again (thanks to jslay88).
- **Controls:** a controller can play while another window is in front (off by default), and the right-stick
  camera can stay while first person and items aim with the left stick: turn off “The right stick aims too”.
- **New, off by default:** a sea compass switch: the red end points the way you're sailing (jslay88).
- **Android, experimental:** the builder runs on macOS too (thanks to benwolfram).

Known issues: 0.9.0 doesn't make Windows, Linux, the Steam Deck or Android faster than 0.8.0. On slower CPUs
the game can still dip below full speed where its own thread is the limit (#215, #427); that's the next version's
focus. The Wind Temple's rooms R04 and R08 need an opt-in switch to load (#318). The Windows download isn't
code-signed; its SHA-256 goes here, with [If Windows says it's a virus](../WINDOWS.md#if-windows-says-its-a-virus).

## After publishing

Post on each issue that 0.9.0 has the change and ask the reporter to confirm. Close an issue only when the reporter
does, or under the 14-day rule (AGENTS.md).

| Issue | What to ask |
| --- | --- |
| #401, #430 The camera dip | Whether the camera holds still with Direct right-stick camera and aiming on |
| #136 Flickering clouds and waves | Whether the flicker is gone with Smooth Motion above 60 FPS |
| #433 Linux build at x86-64-v3 | Whether the container and AppImage build now finish |
| #104 Built BlueWake with PadMint? | How long the build took and how it runs, on which iPhone, iPad or Mac |
| #336 Texture pack status | Whether the import's result and the loaded count are clear |
| #57 The Wii U HD look | Whether the sun shadows look right with the HD look |
| #442 Background input | Whether the controller works with another window in front |
| #402 Camera and aiming apart | Whether the bow feels right with “The right stick aims too” off |


## Checks before publishing

| Check | Who | Result |
| --- | --- | --- |
| Chris plays test candidate 2 on the iPad | Chris | Played October 24: stable at 30 FPS, no dips, Smooth Motion and the touch controls fine, the mods fine. The other fixes are left to players' reports. The Mac by hand was not played |
| Merge in order | Codex | Done October 24: RecompCore#43, #44, #40 and #42 into `bluewake-next`; #468 (#446, #371, #398), #431, #316, #469, #471 (#455) and the freeze #464 into `main` |
| The benchmark tour on the candidate against 0.8.0 | Codex | iPad Pro (M2), October 24: the test candidate (game module `00028fb0…`, built from `f55ffba5` with #431's defaults and the Mac build's training profile) against the October 20 runs of 0.8.0 as players have it, with the same harness, card and settings. Headless game frames a second, two runs: Outset 91.1 / 91.9 (0.8.0: 52.2 / 52.1, +75%), Windfall 92.6 / 92.6 (52.0 / 53.0, +76%), Dragon Roost Cavern 134.3 / 135.8 (75.1 / 77.0, +78%), Forest Haven 75.6 / 76.1 (42.3 / 42.6, +79%), the sea by the Fortress 68.5 / 68.2 (39.1 / 39.0, +75%). Every stop loaded, no dips. At the game's own speed every stop holds 30 game frames a second with the game thread 34 to 52% busy (0.8.0: 51 to 88%). Within 1.2% of the October 20 recipe build, so the rendering stack and the camera fix cost nothing measurable here. The player's card hashed the same (`4b54ec25…`) before the install and after every run. The test candidate stays installed for Sunday; the 0.8.0 app is kept to reinstall |
| The 468-place warp sweep (`scripts/stage_select_sweep.py`) on the candidate | Codex | Mac (M3 Max), October 24: 446 of 468 arrive and the card is unchanged after all 468, the same outcome at every place as the 0.8.0 candidate on October 17 (an early file at Outset, a copy of the iPad's card). The other 22 are the names file's test-room hangs, Wind Temple R04 and R08 (#318), and late-game cutscenes the file can't reach. A rendered tour on the Mac loaded all five stops at 30 game frames a second without dips, with the standard look and with the HD look (with and without sun shadows) |
| Windows build from the frozen commit on Chris's PC, with the checks in [WINDOWS_BUILD_0.9.0.md](WINDOWS_BUILD_0.9.0.md) | Chris's PC | Not yet |
| Linux AppImage from a contributor's own disc, after #446, with `check_appimage_abi.py` | cforain or jkoehler11 | Asked on #433 on October 24, now that #446 is in `main`. Not yet |
| The app-only IPA, PadMint's `audit`, and PadMint's `scripts/player-check.sh` before tagging and after publishing | Codex, on this Mac | IPA from a clean checkout of `211abfd`: `BlueWake-v0.9.0-ios-unsigned.ipa`, 5.7 MB, SHA-256 `675afb81…0b375`, reports 0.9.0 build 8, no game module; PadMint `audit` (PadMint `main` `a723ae0`, Apple Python 3.9.6) and `check_public_assets.sh` pass, also for the source zip (`c72a3a0b…9499`, 1,253 entries) and the recipe (`a8508bd7…0375`). `player-check.sh` before tagging: started October 24, 23:21 JST. After publishing: not yet |
| `scripts/release/check_public_assets.sh` on every artifact | Codex | IPA, source zip and recipe pass. The Windows zip when it's uploaded |

