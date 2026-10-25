# Community questions and requests, October 17, 2026

What players and contributors raised in the Kahris Discord (#bluewake, #bluewake-dev and the BlueWake posts in #help)
from October 9 to 17, checked against `main` and GitHub. Only what is still open, or fixed but not yet in a download,
is listed. Players are described rather than named; contributors are named by their GitHub handle. Nothing here was
answered on Discord from this record.

## Fixed in `main`, in 0.8.0

| What people said | Where it stands |
| --- | --- |
| Lenzo won't accept Picto Box photos (#help) | Fixed by #341 (jslay88's #334 with RecompCore#36); the player has seen the news. Ships in 0.8.0 |
| Ultrawide 21:9 without the black bars (#bluewake) | Widescreen 21:9 under Mods (#279, #70) |
| The Mac build fails on SQLite after a Homebrew or Mono install (#bluewake) | #367: the Mac builds use the pinned static SQLite; no workaround needed |
| Rebinding an Xbox 360 controller's buttons, and keyboard keys (#help) | Controls › Controller buttons and Keyboard keys on every desktop build |
| Cutscenes missing sound: the intro, the bird dropping Tetra, the launch to the Forsaken Fortress (#help, October 7) | Fixed in 0.6.0 ([#65](https://github.com/chrissotraidis/bluewake/issues/65), [#97](https://github.com/chrissotraidis/bluewake/issues/97)), confirmed by those reporters. The Forsaken Fortress launch's sound effects weren't checked by name |
| The Mac build's local training fails on SQLite with Mono installed (#help, 17 messages; workarounds edited CMake caches) | #367 (October 16): no workaround needed; BUILD_YOUR_OWN.md says so |
| PadMint failing while compiling for iOS (#help, October 5) | The player confirmed it built with PadMint 0.4.8 |
| The HD look's lighting looks off: dark streaks, darker overall, square cloud shadows and banding on the sea (Chris, jslay88, #bluewake-dev; a player asked how to turn off ambient occlusion) | #321 turns the sun shadows off by default and softens the rest; #379 (RecompCore#41) places the cloud shadows from the sea's plane with gradient noise and dithers the shading passes. Checked in Mac renders; Vulkan and Direct3D 12 still to look at. Each piece can be turned off ([WWHD_LOOK.md](../WWHD_LOOK.md#turning-one-piece-off)) |
| Black screen with sound on a GTX 550 Ti (#help; also asked for DirectX 11) | The log shows Direct3D 12 refused, no Vulkan, and Direct3D 11 failing to load `d3dcompiler_47.dll`. #377 ships that DLL with the Windows build; it still needs a check on a PC without Direct3D 12 |

## Being fixed for 0.8.0

| What people said | Where it stands |
| --- | --- |
| Clouds flicker at 60 FPS on AMD, at 120 and 180 elsewhere, with the HD look (#bluewake-dev) | pdale-boop's draft #371 (RecompCore#40), not in `main` ([#136](https://github.com/chrissotraidis/bluewake/issues/136)). On October 17 jslay88 found his squares came from the HD look, not #371, and it looked right on NVIDIA; pdale-boop still saw the whole sky flicker on Arch when sweeping the camera. It goes in 0.8.0 only if it's done before the release build |
| No ready-made Linux build: a Bazzite player can't build one, and the Windows build under Proton has heavy input lag; a Steam Deck player gets 20 FPS under Proton (#bluewake) | The AppImage is planned for 0.8.0. #374 fixed AppImage builds failing their own ABI check; a contributor's AppImage from their own disc is the remaining step |
| The container build failed after running it with `sudo` ("dubious ownership"), then a Docker layer error (#bluewake) | [LINUX.md](../LINUX.md#if-the-container-build-fails) now says how to recover. On "bad rom?": no, `content digest ... not found` is Docker's damaged build cache; #382 says so in LINUX.md and stops the misleading "sensitive data" warning printed above it |
| An IPA built on a borrowed Mac fails to sideload with error -80045 (#help) | That code is the sideloading tool's Apple ID sign-in failing, not the IPA. [BUILD_YOUR_OWN.md](../BUILD_YOUR_OWN.md#if-something-fails) says what to try |

## Open requests

| Request | Where it stands |
| --- | --- |
| Switches or sliders for each piece of the HD look (pdale-boop; a player asked to turn off ambient occlusion) | Settings-file keys and variables work today ([WWHD_LOOK.md](../WWHD_LOOK.md#turning-one-piece-off)); menu switches suggested for #368's Experimental section |
| CISO and RVZ disc images (an Android tester) | Not supported; Dolphin converts both to ISO. Windows explains this when given an RVZ |
| Other languages (#bluewake) | [#60](https://github.com/chrissotraidis/bluewake/issues/60); menus could be translated sooner than the game |
| Analog stick sensitivity (#help; the question was deleted, Chris answered "Yep, we can!") | The camera and aiming stick speeds are under Controls; movement-stick sensitivity isn't. Needs the player's exact ask |
| The Tingle Tuner (#help, October 5) | Not supported; Chris gave the reason in #announcements |
| Save states from a controller | [#357](https://github.com/chrissotraidis/bluewake/issues/357), after #368 |
| A GameCube controller adapter on Windows 11 set up with Zadig (#bluewake-dev) | Unclear what fails; needs the session log's `[pad]` lines |
| Play on iPhone or iPad without a computer (#bluewake) | Not possible: the app is built from the player's own disc on a Mac, or on Windows through PadMint once #100 lands |
| Slow on budget Android phones (a Galaxy A16, Mali) (#bluewake-dev) | The game thread is the limit; Android is experimental, checked on a Fold 7 |
| "When is Android coming?", asked again on October 17 (#bluewake-dev) | The Android port is in `main` (#93) and in 0.8.0 as an experimental build made from your own disc on a Windows PC ([ANDROID.md](../ANDROID.md)). No ready-made APK, because it would contain the game (AGENTS.md) |
| An AppImage built automatically when a release is tagged (#bluewake-dev) | CI can't do it: a disc must never be in CI (AGENTS.md). The AppImage is built from a contributor's own disc |
| A written direction for the decompilation (#bluewake-dev; Chris asked for proposed notes) | Open. [DIRECTION.md](../DIRECTION.md) and PERFORMANCE.md's third horizon are the starting point |
| A write-up of rendering tests run by agents with the stage select (#bluewake-dev) | pdale-boop offered to write one. #380 (draft) is a related guide to measuring performance |

## Questions for Chris

- Whether the right-stick-click zoom (#221) should be on by default; it is opt-in under Controls. Asked on #bluewake-dev.
- A gameplay video of the Linux and Android builds for promotion, asked for on October 10 and 11; none posted yet.

