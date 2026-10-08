# Where BlueWake is going

The bar BlueWake holds itself to: how it ships, what every platform gets, and how it treats contributors.
[PRIORITIES.md](PRIORITIES.md) orders the work; this file says what "done" looks like. If a plan disagrees with
this file, raise it with Chris rather than quietly doing it another way. Owner: Chris. Written October 8, 2026.

## Why this file exists

[Wind Waker HD Recomp](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp), a static recompilation of the Wii U
version, started three days after BlueWake and is drawing more attention right now. On October 8:

| | BlueWake | Wind Waker HD Recomp |
| --- | --- | --- |
| Started | September 28 | October 1 |
| Stars | 266 | 287 |
| Release downloads, all time | about 6,000 | about 5,560 |
| Latest release, first day | 0.6.0: 267 | v0.2.7: about 1,090 |
| Releases from October 6 to 8 | 2 | 8 |
| Platforms you can download | Windows | Windows, macOS, Linux x86 and arm64; Android to build yourself |

Over all time the two are even. What has pulled ahead is how often they ship and how easy it is to start playing.
Their HD art and their Wii U-level renderer we can't copy. The rest of what they do, we can.

## 1. No game code in any release

Their release is a small zip for each platform. It holds no game code: the first start builds the game once from
the player's own dump, with a compiler it downloads, and every later start launches straight away. Everything
stays in the unzipped folder.

BlueWake's goal is the same on every platform: **one download per platform, no game code in it, the game built on
the player's machine the first time it starts, no terminal.**

| Platform | Today | To get there |
| --- | --- | --- |
| Windows | A ready-made build with the translated game (the October 4 exception) | Run `scripts/windows/build.py` from the app on first launch, with a progress screen; bundle or download its toolchain; no PowerShell |
| Linux | Not released; #107 builds from the player's disc | The same first-launch build, as an AppImage or a zip |
| Mac | Build from source in a terminal | An app that builds on first launch |
| iPhone and iPad | PadMint on a Mac adds the game to the app | PadMint stays: iOS can't compile on the device |
| Android | #93 builds an APK from the player's disc | A build tool on the computer, like PadMint, until a phone can build it |

The obstacle is build time. A full build takes 17 to 45 minutes on a Windows desktop, 90 to 105 minutes on an
M3 Max and three hours on a MacBook Air, much of it the local training run. Shipping a reviewed optimization
profile, so players skip training (PRIORITIES Tier 2, rank 4), comes first.

The October 4 exceptions in [AGENTS.md](../AGENTS.md) (ready-made Windows and Linux builds) stay until the
first-launch build works on that platform. Then the ready-made build is retired.

## 2. Every platform gets the same app

The same options, in the same places, on every platform. A player who moves from iPad to Windows to Android
should find each setting where they expect it.

**Android's bar is the iPhone and iPad app** (`apple/ios/src/BWGameOverlay.mm`, `touch_controls.cpp`,
`controller_settings.mm`). It doesn't ship as a supported platform until it has all of this:

- **The ⋯ menu, section by section:**
  - Touch Controls: show, move, settings.
  - Display: render resolution, texture filtering, aspect ratio, FPS, Smooth Motion.
  - Controller: camera stick, invert, button mapping, reset.
  - Gameplay: jump and sprint, fast transitions, quick doors.
  - Mods: widescreen, Better Wind Waker, HD texture pack, install a pack.
  - Game Data & Saves: back up, restore, import a Dolphin save, remove the disc image, where the files are.
  - Help & Feedback: report a problem, share the session log.
- **Touch controls that look like the iPhone's:** the same button art and transparency, every GameCube button in
  reach, a floating movement stick, nothing over the game's own HUD, and a layout editor, with separate layouts for
  phones and tablets.
- **Controller handoff:** a controller hides the touch controls, and unplugging it brings them back.
- **Saves:** saving and loading tested on a phone.
- **Speed:** a steady 30 FPS on Outset and at sea on a current phone, from the normal build steps.

The desktop F1 menu is fine for testing, but it isn't what Android players should see. The same goes for any new
platform.

**HD art.** The Wind Waker HD texture importer is in BlueWake (`docs/WWHD_TEXTURES.md`). It should become a menu
action on every platform: "Import HD textures from your Wii U game". Elliott Tate's HD renderer work (a GameCube
or HD switch, lighting, shadows, ambient occlusion) isn't on any public branch; ask him where it lives before
planning around it.

## 3. Ship small and often

- Release when a fix is proven on the platform it affects. Don't wait to batch.
- Each "What's new" line leads with what the player notices, then the evidence: a measured number, the issue
  number, the platform checked.
- Name every contributor whose work is in the release.
- Keep the checks that matter: `scripts/release/check_public_assets.sh` on every asset, and "a build that
  compiles is not a game that plays."

## 4. Contributors get a fast yes or a clear next step

Wind Waker HD Recomp's Android port, Windows build, a performance pass and rumble all came from community pull
requests, merged within days and credited in the notes. People notice that.

- Approve a fork's CI runs the same day (Actions › the run › Approve). Pull requests from forks don't run until
  someone does.
- Review within two days. Merge when CI is green, the change is scoped, and it says how it was checked.
- If a pull request stalls, offer to carry it on a branch that keeps the contributor's commits.
- A runtime change still goes to RecompCore `bluewake-next` first ([AGENTS.md](../AGENTS.md)).

## 5. Performance is measured and shown

The plan and the measuring method are in [PERFORMANCE.md](PERFORMANCE.md).

"It's slow" is the loudest complaint about BlueWake online. Every performance change comes with numbers from the
same benchmark spot, before and after, and the release notes say what changed.

- A **Copy performance report** action in every menu: version, system, GPU and driver, renderer, settings that
  aren't the default, and the last minute's `[perf-summary]` and `[fps-dip]` numbers. Players paste it into an
  issue. The session log already has the data.
- When the app falls back to a slower renderer, it says so on screen. A Linux laptop that silently fell back to
  OpenGL ES ran the bird scene at 4% speed; with Vulkan the same laptop's lowest was 74%.

## 6. Features worth taking

What Wind Waker HD Recomp players get that BlueWake players ask for, in rough order of value for effort:

| Feature | BlueWake today | Notes |
| --- | --- | --- |
| Copy performance report | Session log only | Section 5 |
| Gyro aiming (DualSense, Switch Pro, Steam Deck, a DSU server) | Requested in #188 | SDL3 reads controller gyros; aim in first person like the Wii U GamePad |
| 120 and 240 FPS, matched to the display, "keep game speed" | 60 and 120 | Smooth Motion's flicker (#136) and cost on small CPUs come first |
| A mod manager with installable packs that replace game files | Built-in mods and texture packs only (#152) | Needs a file replacement path in the disc reader |
| HD textures from the player's own Wii U game, from the menu | The importer is a script | Section 2 |
| Crash recovery: automatic save states plus recorded input, so a crash can be replayed | Save states exist | Turns crash reports into reproductions |
| Cheats (items, hearts, rupees) | None | Cheap; a spare save file only |

## What we don't copy

Wind Waker HD Recomp implements the Wii U's graphics library directly on Metal and Vulkan, and its main thread
needs about 4 ms a frame on an M3. BlueWake converts the GameCube's drawing commands on the CPU, which is where
much of its time goes (PRIORITIES Tier 2). Replacing that with native rendering, piece by piece, is the long-term
answer, not a quick port of their renderer.
