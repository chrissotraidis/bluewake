# BlueWake 0.8.0 release record

The working record for 0.8.0, started as the next-release draft on October 11. Fill in each check with the device, the
commit and what was seen; a row stays "not yet" until someone runs it. The Windows build is
[WINDOWS_BUILD_0.8.0.md](WINDOWS_BUILD_0.8.0.md). Previous release: [0.7.0](RELEASE_0.7.0.md), October 10. Only Chris
publishes.

## Candidate

| | |
| --- | --- |
| Version | 0.8.0, build 7 (`version.json`, set by the freeze pull request) |
| Candidate commit | `f2611b1357215d1f426af9dbe90c7d7189320c99` (#376's merge, October 17): what the Windows build, the AppImage and the IPA are built from. Check it out with `git checkout --detach f2611b1` |
| RecompCore | `338a3dbcdd384142f0fe40db44e0cd1d1b0c61e4` (through patch 0182: the HD look and its sea fixes, the Picto Box fixes, the resize fix and the copy crop; 0179 belongs to draft #371 and 0181 to draft #342) |
| Previous release | 0.7.0, October 10 (`66b33be`) |

While the candidate is being built and checked, only docs and fixes for what the checks find go into `main`.

## What it contains

| Change | Issue | Ships to | Checked so far | Still to check |
| --- | --- | --- | --- | --- |
| **Linux download:** the first ready-made Linux build, an AppImage (cforain's setup, #217, #226; the shared build container, #254, merged October 12, with `--clang22` for the release build); the module about 50 MB smaller and quicker to load with RELR on glibc 2.36 or newer (pdale-boop, #268) | #56 | Linux | Source builds by five testers; the AppImage CI; RELR: all 2.2 million pointers the same and identical checkpoints on Arch | A contributor's own-disc AppImage, the release check, a Steam Deck run |
| **Faster:** the host's checks around each block of game code do less (pdale-boop, #242) | #59, #246 | Every platform | Linux: 5 to 8% on slow CPUs, identical checkpoints | The release build |
| **Faster:** returns into another chunk skip the dispatch's search, and direct calls skip the chassis for GXBegin (Elliott Tate, #234, #235) | #59 | Windows, Linux, Android builders | 811 of 811 dispatches on the prepared source; tests | The release build |
| **Faster on Linux:** draw fusion on, as on Windows (#247) | #215 | Linux | jkoehler11 on a Steam Deck: a tenth of the draws, nothing missing | Forest Haven on Linux |
| **Faster to build** on Mac, iPhone and iPad: code training never ran compiles at `-O1` (#252) | #104 | Mac, iPhone, iPad | M3 Max: 61 min of compile instead of 79; iPad Pro: the same speed in two places | PadMint's `player-check.sh` |
| Rebuilding after an app change takes about a minute (pdale-boop, #243) | none | Windows, Linux builders | Windows and Linux; Mac mods step byte-identical | none |
| Swap L and R (the triggers) under Controls (#248) | #244 | Windows, Mac, Linux | CI test | A controller |
| The developers' stage select, off by default (pdale-boop, #210, #250); once it's used, saving is off until a restart, F7 asks first, and picked cutscenes get their story flag (#266) | #251 | Windows, Mac, Linux | Played on Windows and Linux; no cost when off; #266 played on Windows, Mac build and card test | The 468-place sweep on the candidate; #266 on a Mac or Linux by hand |
| The Mac builder takes `--lean-blocks` and `--lean-memory`, the Windows builder's preparations, off by default (Elliott Tate's `lean_memory.py`, #257) | #59 | Mac builder | M3 Max: lean memory 5.5% faster at Outset and 6% in Dragon Roost Cavern, same blocks ([record](SPEED_PROFILE_2026-10-11.md)) | On by default in the Windows and Linux builders since October 12 (Chris's call); x86, Deck and phone numbers still wanted |
| `--lean-deadlines` in the Mac, Windows and Linux builders (#264; on by default on Windows/Linux since #277, opt-in on Apple), and Mac training at `-O1` with lean memory so the trained module links (#270) | #59, #215 | Every builder | M3 Max: 3.7 to 4.9% on top of lean memory untrained; trained, lean memory and lean deadlines 6.3 to 7.4% faster than lean blocks alone; same game state | The release build; Apple default remains Chris's decision |
| The lazy overlap watch, on by default since October 12 (`BLUEWAKE_LAZY_OVERLAP=0` turns it off): the host stops reading the scene's overlap phase at every direct call, and the game module answers the per-call question itself (#274) | #59, #215, #93 | Every platform | M3 Max: 3.9 to 4.7% faster on top of lean memory and lean deadlines; identical checkpoints, also through four scene changes; every one of 708 M inline answers agreed with the host | A Windows PC, a Deck and the Fold 7 |
| **The Wind Waker HD look:** one switch, off by default, in every desktop menu and on iOS: islands on the horizon from further away (#284), and ambient occlusion, sun and cloud shadows, edge light, torch and fire light, HD's far-distance blur and bloom (#287, RecompCore ee99bbc); **F2** switches it in the game on the desktop (#306); distant islands kept within the game's island heap, after a long sail with them on once hung a later load (#309, #308) ([WWHD_LOOK.md](../WWHD_LOOK.md)) | #57 | Every platform but Android | Mac (M3 Max, Metal), from save states: each piece's frames before and after; with the HD texture pack too; no measurable time added at Outset; sea rooms 11, 14, 20, 32 and 44 from the stage select: 13 to 15 island models, none outside the island heap | Windows (D3D12, Vulkan) and Linux by hand; a small GPU; an iPhone or iPad |
| **HD textures from your own Wind Waker HD dump:** the importer installs into the folders the Mac and Linux apps read (#295) | #57 | Mac, Linux | A decrypted USA dump on a Mac: 3,546 replacements, all loaded at Outset, with the HD look on and off | `--install` on a Mac's own settings; a Linux PC |
| Widescreen 21:9 for ultrawide screens, as a mod beside 16:9 (#279) | #70 | Every platform | CI | An ultrawide screen by hand |
| `--behaviour-timing`: every block prepaid, timers land at the next block's start, the turn's budget checked at loops and calls (#290); Windows/Linux default since #335, opt-in on Apple | #59, #215 | Every builder | M3 Max: 7.0% faster at Outset and 9.1% in Dragon Roost Cavern, the same 468-place sweep, identical audio, the module 8.5% smaller ([note](BEHAVIOUR_TIMING_2026-10-12.md)); PC, Deck and Fold 7 results in [PERFORMANCE.md](../PERFORMANCE.md#results) and the #335 row below | The release build; Chris decides the Apple default. Android remains experimental in #93 |
| Personal iPhone and iPad builds take `--combined-optimizations` and the lean options, off by default (#292); trained builds with the lazy overlap watch work again (#281) | #104 | iPhone, iPad, Mac builder | iPad Pro M2 comparison: Outset game-thread busy 81% to 54%, Dragon Roost Cavern 53% to 43%, saves unchanged ([record](GOAL_LOOP_2026-10-15.md#now-the-speed-sprint-from-october-11)) | Broader play acceptance and Chris's decision on the Apple default |
| **Controls:** the D-pad moves the cursor in Items and Quest Status (#298, #301), and gyro aiming with a DualSense, DualShock 4, Switch Pro or Steam Deck (#302, #304), both off by default | #245, #188 | Mac, Windows, Linux, iPhone, iPad | The D-pad: scripted presses on the Mac and an iPad Pro (the cursor moves as with the stick); gyro: its turn unit-tested from SDL's axes, compiled for the Mac and iOS | A controller's D-pad by hand; a gyro controller on any platform |
| **Faster trained builds:** training with the lazy overlap watch on, the full watch only until player control (pdale-boop, #297) | #59, #215 | Every builder that trains | M3 Max, two trained builds from one commit: 4.4% less time at Outset and 5.2% in Dragon Roost Cavern, same checkpoints; training passes on Linux and the Mac | A trained Windows build |
| Linux: portable mode as on Windows, `--portable` in both builders (#303), and missing parents of the data folder made (#311) (both pdale-boop) | #64 | Linux, Windows builder | Arch: portable data in `user/`, remaps copied once, rebuilds keep `user/`; a fresh home and a new `BLUEWAKE_DATA_DIR` start; the stage select sweep runs on Linux again | The Windows builder's `--portable` |
| CI actions brought up to date (Mrlinkwii, #273) | none | CI | Every check passes with them | none |
| **Fixed:** the sea chart shows each island's own map, and the Pictobox preview the new photo: the cache-flush fallback is on by default (`BLUEWAKE_CACHE_FLUSH_FALLBACK=0` turns it off), and a flush no longer throws away an image the GPU drew into game memory, which the reporter saw as a red plane at sea (#283, RecompCore ee99bbc) | #263, #13 | Every platform | The #263 reporter on Windows (opt-in, before the red-plane fix): every map right, frame rate and Pictobox unchanged; Mac: no lost GPU images in 5,613 at sea, Outset's frames identical with it on and off, with or without the HD look | The reporter on the next Windows build; a physical iPad |
| **Mods › Wind Waker HD textures**, first in the Mods tab or menu on every build: the switch, the texture count and adding a pack (#314); a red or green status and the chosen folder's texture count (jkoehler11, #345); the Linux folder picker opens (jkoehler11, #344) | #57, #336 | Windows, Mac, Linux, iPhone, iPad | Mac: the section, count and picker; jkoehler11 confirmed the Linux picker | Windows by hand |
| **The same menus and hotkeys on every desktop build** (pdale-boop, #324, #326 to #330, #332): F1 settings, F2 HD look, F5 and F8 save and load state, F7 stage select, F9 frame rate, F10 Smooth Motion, F11 or Alt+Enter fullscreen, with a status line; the Mac and Linux menu with a controller; Restart now and safe mode on Linux; Alt+Enter without pausing (#339) | #320 | Windows, Mac, Linux | pdale-boop on Windows and Linux; the Mac with a keyboard | A controller on the Mac |
| **Faster by default on Windows and Linux:** behaviour timing in both builders (#335; `--no-behaviour-timing` turns it off) | #59, #215 | Windows, Linux | 7 to 11% on Intel desktops, 9% on the Steam Deck; played on the Deck and on Windows; Outset at full speed on a Fold 7 (#93) | The release build |
| **Fixed:** the Picto Box credits the right subject, so Lenzo accepts correct photos (jslay88, #334, through #341 with RecompCore `57123c6`) | #267, #333 | Every platform | jslay88 on Linux and an M4 Pro; pdale-boop's three photos on Windows; an M3 Max | The #267 reporter |
| **Fixed:** the Greatfish merchant stays on his raft (jslay88's fix, #323) | #313 | Every platform | Confirmed by the reporter | none |
| Picto Box gallery photos fill their frames at 16:9, 16:10 and 21:9 (pdale-boop, #346) | none | Every platform | Windows, Linux and the Mac | none |
| The Wind Temple's R04 and R08: a switch, off by default, gives loose RELs a load time (pdale-boop, #331), and save states wait for it (#350) | #318 | Every platform | Sweeps on Windows, Linux and the Mac: only R04 and R08 change | F5 and F8 during a real wait; then the default |
| Safe startup turns the HD look off (#343) | none | Windows, Mac, Linux | Mac fixtures and gameplay | Windows and Linux by hand |
| **Fixed:** resizing the window keeps the existing surface and the picture drawing (pdale-boop, RecompCore#38, pinned by #353) | #325 | Windows, Mac, Linux | Arch/X11/NVIDIA, including F1 open; Mac/Metal zoom and fullscreen; Windows/D3D12 resizing and fullscreen; Windows and Linux CI | Reporter confirmation on `main`; Wayland; Windows resizing with F1 open |
| **Android, experimental:** an arm64 APK built from your own disc on a Windows PC (LiquidAzir, #93), and `--app-only` refuses a module built with other options (#372) | #75 | Android | Fold 7: Outset at full speed with behaviour timing in alternated cool-start runs; the app-only tests; CI | Other phones; a Bluetooth controller with the D-pad option; #238 |
| **HD textures made in the app:** Mods › Wind Waker HD textures › Import from Wind Waker HD…, from a decrypted HD folder or a WUD/WUX image with its key files (#315) | #57 | Windows, Mac, Linux, iPhone, iPad | Full imports on the Mac and on Linux (pdale-boop's own dump): 3,546 textures, byte for byte the script's; generated images on an Android emulator and the iOS simulator | A Windows import; an authentic WUD/WUX image; an iPhone or iPad |
| **Full speed on Wayland:** mailbox present, so KWin 6.8 no longer holds the game at about 70% (jslay88, #370; `BLUEWAKE_VSYNC=1` for FIFO) | #56, #215 | Linux | CachyOS, KWin 6.8, RTX 4090 at 120 Hz: retraces held 59.9 | GNOME; the Steam Deck's gamescope |
| Picto Box photos under widescreen keep their 4:3 shape, opt-in (`picto_crop=1` on Windows, `BLUEWAKE_PICTO_CROP=1` on Mac and Linux; pdale-boop, #338 with RecompCore#37) | none | Windows, Mac, Linux | Windows and Arch at 16:9, 16:10 and 21:9 | The Mac |
| Mac builds work with Mono or Homebrew SQLite installed (#367) | none | Mac builder | A full private Mac build with the conflicting paths; the regression fails without the fix | PadMint's `player-check.sh` |
| The benchmark tour (`scripts/bench_tour.py`, jslay88, #360); the native-entry tests compare guest state only (#359); a source-compiled `__ieee754_fmod`, not hooked (#364) | #179 | Tooling | Tests. First run on a disc: the candidate's Mac build (M3 Max, rendered), all five stops loaded with no dips, 31 checkpoints, the card unchanged (below) | A comparison against 0.7.0 |
| **Linux AppImage builds pass their own ABI audit:** `--appimage` builds the app with libc++ and Dawn from source, as CI does (#374; found from irregularthanis's build log on #371) | #56 | Linux builder | Contract test; the pinned Dawn package checked to link libstdc++ | An AppImage from a contributor's own disc passing `appimage-abi` |
| The HD import script picks the same sources on Windows as on Mac and Linux (pdale-boop, #375) | #57 | Windows | pdale-boop's Windows run matched the native pack in every PNG and report field | none |
| **The HD look, softer:** sun shadows off by default (their streaks), gentler ambient occlusion, cloud shadows, bloom and far blur (#321) | #57 | Every platform | Chris's iPad Pro (October 15); irregularthanis's Radeon screenshots on #371 showed the streaks and darkening it removes | A look on Windows and Linux |
| **The HD look on the sea:** cloud shadows without square steps, and no banding (#379, RecompCore#41) | #371 | Every platform | M3 Max: the straight-edged band across Outset's sea gone, before and after from the same host | Vulkan and Direct3D 12, where it was reported |
| **Windows: older GPUs without Direct3D 12 draw through Direct3D 11** instead of a black screen: the builder ships `d3dcompiler_47.dll` (#377) | none (a GTX 550 Ti on Discord) | Windows | Builder test; the player's log shows the cause | A PC without Direct3D 12 |
| Elliott Tate's seventh natives round (libm, collision blocks, rotations, geometry, JASystem) and `cache_ops.py`, behind `--native-entries` and `--cache-ops`, off by default; the replay generated at build time (jslay88, #361) | #179 | Windows, Linux builders | 60,000 leaf cases, no mismatches, on four leaves; 14 minutes of play | A measured build with it on |
| Two more native sets, linked only when asked: `--native-set4` (colour blends, key-frame animation, collision setup) and `--native-gx` (the GX FIFO writers); no hooks yet, so nothing in them runs (jslay88, #365, #366, through #381) | #179 | Windows, Linux builders | The sources and comparison tests compile; CI | A build with the switches on; hashes from a BlueWake module before any hook |
| Linux: a Docker layer error is explained as the build cache, and the container build no longer prints a "sensitive data" warning (#382) | #56 | Linux builder | The Linux CI job builds the Dockerfile | none |

Not in it yet: the Android HD import (#347, draft), the iOS menu-switch change (#316), the first cold-cache Picto Box
photo (#342), Smooth Motion without the flicker (#371) and one desktop menu (#368, coming in five parts). Their remaining checks and decisions are in [GOAL_LOOP.md](../GOAL_LOOP.md) and
[PRIORITIES.md](../PRIORITIES.md).

After the freeze: #383 (pdale-boop) fixes `tests/test_safe_mode_hd.py`, which failed at the candidate because it still
expected the HD look switch to turn sun shadows on (#321 leaves them off). Test only; the candidate's code is
unchanged.

## Release notes (draft, for the release page)

BlueWake 0.8.0 brings the Wind Waker HD look, HD textures you make from your own Wind Waker HD game inside the app,
more speed by default on Windows and Linux, the same menus on every computer, and an experimental Android build.
Every version needs your own USA (GZLE01) disc image. Nothing from the game is included except in the ready-made
Windows and Linux builds.

- **The Wind Waker HD look**, off by default: islands on the horizon from further away, bloom, the far-distance blur,
  ambient occlusion, cloud shadows, and edge and torch light, kept gentle so the game keeps its colours. Press **F2**
  on Windows, Mac and Linux, or turn it on under **Mods** on iPhone and iPad. Each piece can be turned off by itself.
- **HD textures from your own Wind Waker HD game.** **Mods › Wind Waker HD textures › Import from Wind Waker HD…**
  makes the texture pack, HD menus and HUD included, from your decrypted HD dump or a USA WUD/WUX image with its keys,
  on Windows, Mac, Linux, iPhone and iPad. No Python, and nothing from the Wii U game is included in BlueWake.
- **Faster by default on Windows and Linux.** Behaviour timing: 7 to 11% faster on Intel desktops and 9% on the Steam
  Deck, together with lighter checks around the game code (thanks to pdale-boop) and Elliott Tate's faster returns
  and calls.
- **The same menus and keys on every computer** (thanks to pdale-boop): F1 settings, F2 the HD look, F5 and F8 save
  and load state, F7 the stage select, F9 the frame rate, F10 Smooth Motion, F11 fullscreen. The Windows menu works
  with a controller, and Linux gets Restart now and safe mode.
- **Controls:** the D-pad moves the cursor in Items and Quest Status, gyro aiming with a DualSense, DualShock 4,
  Switch Pro or Steam Deck, swapping L and R, and 21:9 widescreen.
- **Fixed:** Lenzo accepts the right Picto Box photos (thanks to jslay88), each island's sea chart shows its own map,
  the Greatfish merchant stays on his raft (jslay88), and resizing the window on Linux no longer crashes
  (pdale-boop). On Wayland the game runs at full speed (jslay88).
- **New, off by default:** the developers' stage select, with 468 places to jump to from your own file (thanks to
  pdale-boop).
- **Linux:** portable mode, a working texture-pack folder picker (thanks to jkoehler11), and the build container
  (thanks to cforain).
- **Android, experimental:** build it yourself from your own disc on a Windows PC (thanks to LiquidAzir). See
  docs/ANDROID.md.

Known issues: with Smooth Motion above 60 FPS, clouds and the HD look can flicker on some PCs (#136; a fix is in
progress in #371). The Wind Temple's rooms R04 and R08 need an opt-in switch to load (#318).

## After publishing

Post on each issue that 0.8.0 has the change and ask the reporter to confirm. Close an issue only when the reporter
does (AGENTS.md).

| Issue | What to ask |
| --- | --- |
| #56 Linux support | Whether the AppImage starts and plays, on their distribution and on the Steam Deck |
| #57 The Wii U HD look and textures | Whether the HD look and the in-app import work for them |
| #70 Ultrawide | Whether 21:9 under Mods looks right |
| #75 Android | Point to ANDROID.md; it stays experimental |
| #108 A hotkey for the HD look | Whether F2 works |
| #136 Flickering clouds and waves | Keep open: the Smooth Motion fix is #371, not in 0.8.0 unless it lands first |
| #188 Gyro aim, #244 swap L and R, #245 the D-pad in menus | Whether each option works with their controller |
| #263 Each island's own sea chart | Whether every island shows its own map with no variable set |
| #267 Lenzo's Picto Box photos | Whether Lenzo accepts the photos |
| #320 The same hotkeys everywhere | Whether the keys match on their platform |
| #325 Resizing on Linux | Whether resizing and fullscreen keep drawing |

## Checks before publishing

| Check | Who | Result |
| --- | --- | --- |
| Freeze: `version.json`, the candidate commit here | Chris decides; Codex does it | Done: #376 merged as `f2611b1`, `version.json` 0.8.0 build 7 |
| Windows build from the candidate on Chris's PC, with the play checks in [WINDOWS_TASKS.md](../WINDOWS_TASKS.md) sections 1 and 2 | Chris's PC | Built: `f2611b1` on the Ryzen 7 5700U in 81 min (6 jobs, clang 22.1.3), module `b318ad1f…6bb24` ([WINDOWS_BUILD_0.8.0.md](WINDOWS_BUILD_0.8.0.md#results)). Start check only, at Chris's direction: Outset with scripted input for two minutes, Direct3D 12, draw fusion, no fatal lines, quit cleanly. The HD look, Dragon Roost and saving, the controller menu and the HD import were skipped, so the WINDOWS_TASKS.md rows stay open |
| Linux AppImage from a contributor's own disc; `check_appimage_abi.py` at 2.39; the release check | cforain or jkoehler11 | Not for 0.8.0 (Chris, October 17): Linux players build from source, and the notes say an AppImage will be added once one is built and checked |
| The app-only IPA, PadMint's `audit`, and PadMint's `scripts/player-check.sh` before tagging and after publishing | Codex, on this Mac | IPA: pass at `f2611b1` (M3 Max, October 17). `BlueWake-v0.8.0-ios-unsigned.ipa`, 5.7 MB, SHA-256 `c45a2e94…dd92335`, reports 0.8.0 build 7 and holds no game module; PadMint `audit` (0 address-named functions) and `check_public_assets.sh` pass. `player-check.sh` before tagging: pass (PadMint `main` `b9f4c3d`, 0.5.3; Apple Python 3.9.6, an empty PadMint folder, a fresh clone of `f2611b1`, this IPA as the app): translated, trained, compiled, signed and verified a personal IPA in 6 h 17 min, slow because `padmint build` compiles with 2 jobs (players' `padmint make` uses the cores). After publishing: not yet |
| Source zip and recipe | Codex, on this Mac | Pass at `f2611b1`: `git archive` with the `bluewake-0.8.0/` prefix (1,213 entries) and `padmint.json` as `BlueWake-v0.8.0-padmint.json`; `check_public_assets.sh` passes both |
| The 468-place warp sweep with the stage select (`scripts/stage_select_sweep.py`) | pdale-boop or Codex | On `main` `d73ac95` (not the candidate), a Mac, an early file: 446 of 468 arrive, the card unchanged after all 468; the rest are the noted test rooms and crash rooms and late-game cutscenes the file can't reach ([STAGE_SELECT.md](../STAGE_SELECT.md)). On the candidate `f2611b1` (Mac build, M3 Max, October 17, the same early file): 446 of 468 arrive and the card is unchanged after all 468. The other 22 are the names file's crash rooms, hangs and story cutscenes, plus Ganon's room cutscene and the ending, which an early file can't reach. A late-game file is still to do |
| The benchmark at Outset and Dragon Roost Cavern against 0.7.0 | Codex | Done on the Mac (M3 Max, October 17), `bench_tour.py` on both builds made the same way (`build.sh DISC --platform macos --jobs 8`/`6`, local training), alternating, with other work running (load average 7 to 9). **Checkpoints match** between 0.7.0 and the candidate in every pair: the same game state along the route. Headless speed, three pairs: Outset, Windfall and Dragon Roost Cavern within about 1% of 0.7.0 (−2.4% to +1.4%); Forest Haven −0.3% to −3.6% and the sea by the Fortress −1.2% to −4.7%. With `BLUEWAKE_CACHE_FLUSH_FALLBACK=0` (two runs) the candidate is +1.0% to +2.4% at the first three stops, even at Forest Haven and −2.1%/+1.6% by the Fortress, so most of the loss at the busy stops looks like the cost of the cache-flush fallback (on by default since #263's fix). Rendered, both hold 30 game frames a second at every stop. Not run on a slower Mac, an iPhone or an iPad |
| The draft release | Codex prepares; Chris publishes | A v0.8.0 **draft** targeting `f2611b1` (no tag until it's published) with the IPA, the source zip, the recipe, the Windows zip and `SHA256SUMS` for all four; GitHub's digests match. Windows zip (`56cc8458…588e2f`, 35 files, provenance `f2611b1` unmodified, verified digest `54f54434`): `check_public_assets.sh`'s only finding is `containsTranslatedGameCode: true`, the accepted one (October 18, Mac). Notes in Chris's voice. Ready for Chris to publish |
| Mac build from the candidate | Codex, on this Mac | Pass: `scripts/builder/build.sh DISC --platform macos --jobs 8` at `f2611b1`, with local training; the module compiled in 90 minutes; the app packaged and signed ad hoc. Played only by the tour and the sweep above; ten minutes with a controller is still to do |
