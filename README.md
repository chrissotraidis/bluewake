# BlueWake

<p align="center">
  <strong>The Legend of Zelda: The Wind Waker, running natively on iPhone and iPad.</strong><br>
  A static recompilation of the GameCube original, with Metal rendering, touch controls, controllers and mods.
</p>

<p align="center">
  <img alt="iPhone and iPad" src="https://img.shields.io/badge/platform-iPhone%20%2F%20iPad-0A84FF?logo=apple">
  <img alt="Metal renderer" src="https://img.shields.io/badge/renderer-Metal-5E5CE6">
  <img alt="Ahead-of-time static recompilation" src="https://img.shields.io/badge/PowerPC-static%20recompilation-FF9F0A">
  <img alt="Developer build: 30 FPS on iPad Pro M2" src="https://img.shields.io/badge/developer%20build%20(M2)-30%20FPS-30D158">
  <img alt="Game data not included" src="https://img.shields.io/badge/game%20data-not%20included-FF453A">
  <img alt="License: GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-lightgrey">
  <img alt="Status: source preview" src="https://img.shields.io/badge/status-source%20preview-FFD60A">
  <a href="https://discord.gg/xwHfUD2bxW"><img alt="Join the community on Discord" src="https://img.shields.io/badge/Discord-Join%20the%20community-5865F2?logo=discord&amp;logoColor=white"></a>
</p>

![BlueWake at the Wind Waker title screen on an iPad Pro, running at 30 FPS and full speed, with the touch controls visible](docs/images/bluewake-ipad-title.jpg)

> [!IMPORTANT]
> **Bring your own disc.** BlueWake needs your own legally obtained copy of *The Wind Waker* for
> GameCube, USA version (`GZLE01`, revision 0). This repository contains no disc image, playable game
> assets or saves. Your personal app contains code translated from your disc; you also import the
> disc on your device.
>
> **Public app releases are paused.** Build privately from source using your own disc; do not
> expect a downloadable BlueWake app. Publication requires clearance in the maintainer's private
> release audit. See [Getting started](#getting-started) and the [candidate checklist](docs/status/RELEASE.md).
>
> **AI disclosure:** BlueWake is developed with substantial AI assistance for code, testing,
> documentation and debugging. The status log records what has actually been checked, and on what.

**Questions or bugs?** Join the [Discord](https://discord.gg/xwHfUD2bxW) or
[open an issue](https://github.com/chrissotraidis/bluewake/issues).

The next stability candidate is still being validated on the draft integration branch, not main.
The measurements below describe earlier developer builds; they are not acceptance of the latest
changes or every personal player build. Windows and Apple TV remain experimental. See the
[platform evidence](docs/FORK_INTEGRATION.md) for completed checks and remaining gates.

## What is BlueWake?

BlueWake translates the game's PowerPC code, the main executable and all 415 of its modules, into
native arm64 code ahead of time, on your Mac. The app runs that code with a compatibility runtime for
the GameCube's hardware: memory and timing, disc and memory card, controllers, DSP audio, and graphics
through Metal. Nothing is compiled on the device while you play, so it needs no JIT; the few rare instructions the translator does not handle fall back to an interpreter.

The runtime is derived from [Dolphin](https://dolphin-emu.org/) and keeps an interpreter fallback, so
BlueWake is a static recompilation with a hardware compatibility layer, not an "emulation-free" rewrite.

## What works

- **Tested areas and features:** the opening and prologue, Outset Island, sailing the
  Great Sea, Windfall, a late-game Hyrule save, menus, and saving and reloading through the game's own
  memory card
- **Developer build: 30 FPS at full speed** on an iPad Pro (M2), the game's native frame rate, with stereo audio
- **Touch controls** with a layout editor, opacity and size settings
- **Game controllers and keyboards**, with camera inversion and button remapping
- **The ⋯ menu:** FPS display, render resolution up to 4×, texture filtering up to 16× anisotropic,
  aspect ratio, mods, save backup and restore, and Report a Problem
- **Mods:** 16:9 widescreen, Dolphin-format HD texture packs and
  [Better Wind Waker](https://github.com/WideBoner/betterww)

Later dungeons and boss fights are still largely untested. If you find a problem there, please
[report it](#getting-help).

## Performance

| Device | Result |
| --- | --- |
| iPad Pro 12.9" (M2) | Steady 30 FPS at full speed; a 44-minute session had 14 seconds below 29 FPS, all brief dips at area loads |
| iPhone 14 (A15) | 30 FPS in most play; dips to about 25-27 FPS in the busiest scenes and the title-screen flyover |
| Older devices | A13 or newer is required; slower chips have not been measured |

These measurements come from the developer build. Much of its speed comes from an optimization
profile: a record of which parts of the game's code run most, which the compiler uses to arrange
that code for speed. That profile is made from the game itself, so it cannot be published. Instead,
the builder makes your own: it runs the game briefly on your Mac, records the same kind of counts,
then compiles your app with them. On the Mac, an app built this way ran the same test route slightly
faster than the developer build; iPad frame-rate tests of it are still to come. Without the profile
(`--no-train`), the game runs at about 27.5 FPS instead of 30 at the Outset Island pier on the iPad Pro (M2).

In the measured slow scenes, the CPU is the main limit, so lowering render resolution alone has
not recovered full speed. Other scenes and HD texture packs can have different limits. Performance
on smaller devices is active work.

<p align="center">
  <img alt="BlueWake on an iPhone 14, with the touch controls in the black bars beside the picture" src="docs/images/bluewake-iphone-title.jpg" width="720">
</p>

## Getting started

You need:

- a Mac with Apple silicon, Xcode, CMake and Ninja, and at least 25 GB of free disk space
  (PadMint's required disk-space check, not a measured peak-space guarantee)
- your `GZLE01` revision 0 disc image
- an A13 or newer iPhone or iPad on iOS/iPadOS 17 or later, with Developer Mode on
- Or an Apple TV on tvOS 17 or later, with Developer Mode on ([Apple TV build guide](docs/status/TVOS_BUILD.md))
- an Apple ID for signing (a free one works; its apps expire after seven days)

PadMint's BlueWake workflow uses an app-only release as its starting point. Public app releases
are currently paused, so use the source builder below instead of relying on that release workflow.

**From this repository,** one command builds your own personal app from a fresh checkout:

~~~bash
scripts/builder/build.sh "/path/to/The Legend Of Zelda The Wind Waker.iso" --ipa build/BlueWake.ipa
~~~

It fetches the pinned runtime and translator, checks your disc, translates the game from it, tunes it
on your Mac, compiles it for iOS and writes an unsigned IPA. Install that with Sideloadly, AltStore, SideStore or Xcode, then
copy the same disc image to your device (Finder › your device › Files › BlueWake); BlueWake imports it on
first launch. Expect a first build of well over an hour on a fast Mac, longer on smaller ones; the
terminal shows each stage and its elapsed time, and completed work is reused if you stop and rerun
the same command. Run it with `--source-only` first to check your tools and disc in a few minutes.

**The IPA you build contains code translated from your disc: it is yours alone. Never share or upload it.**

Step by step, with updating and troubleshooting: [Build your own BlueWake](docs/BUILD_YOUR_OWN.md).
Signing with your own identity and other options: [docs/status/DEVICE_BUILD.md](docs/status/DEVICE_BUILD.md).
For Apple TV, use [the tvOS build guide](docs/status/TVOS_BUILD.md); it builds and installs directly with `devicectl`.

### Windows

An experimental native Windows x86-64 source port targets Direct3D 12, using the same disc and verified
game source. The runtime/app compile and link pass native Windows CI; end-to-end personal-module
build and Direct3D gameplay validation remain pending. To try it
with Visual Studio's C++ workload and its Clang component, Python, Git, CMake and Ninja:

~~~bash
python scripts/windows/build.py "D:\Games\The Legend of Zelda - The Wind Waker (USA).rvz"
~~~

It accepts an `.iso` or a Dolphin `.rvz` and writes `build\windows\BlueWake\BlueWake.exe`, a personal build like
the IPA: never share it. See [BlueWake on Windows](docs/WINDOWS.md), or
[Windows launch troubleshooting](#bluewakeexe-does-nothing-when-i-open-it) below.

## Mods

On iPhone/iPad, open **⋯ › Mods**; on Windows, press **F1** and open **Mods**.
Each mod applies the next time BlueWake starts and never changes your saves.

| Mod | What it does | How to add it |
| --- | --- | --- |
| **Widescreen 16:9** | A wider view with the HUD placed for 16:9 | Built in; turn it on |
| **Widescreen 16:10** | A wider view sized for 16:10 displays | Built in; select it instead of 16:9 |
| **HD Texture Pack** | Replaces the game's textures, for example with Hypatia's HD pack | See [texture-pack setup](#where-do-i-put-hypatias-texture-pack) below |
| **Better Wind Waker** | Swift Sail, instant text, faster animations and other individual settings | Built into new personal modules; enable it and open **Better Wind Waker Settings** |

Code mods cannot be applied to a statically recompiled game at runtime, so they are translated and built
into the app. Details are in [docs/MODS.md](docs/MODS.md).

**Smooth Motion (Experimental)** adds in-between rendered frames while gameplay stays at 30 updates
per second. It is off by default; see [how interpolation works](#how-does-smooth-motion-work).
**⋯ › Gameplay** adds optional Jump & Sprint, Fast Transitions and Quick Doors.
These apply at the next launch and start off. Jump and Run appear as editable touch buttons when
movement extras are enabled; keyboards use Space and Shift, and controllers use the left bumper
and left-stick click. Jump uses the game's ledge jump and respects its movement restrictions.

The rendering, camera, movement, transition and game-option additions come from
[elliotttate's source fork](https://github.com/elliotttate/Wind-Waker-Recomp), with original commit
authorship retained. [Integration status](docs/FORK_INTEGRATION.md) distinguishes BlueWake's checks
from the fork's reported measurements. Rebuild your personal game module for the new Better Wind
Waker settings and 16:10 variants; an older module does not gain those variants from an app update.

## Your saves and game data

- Saves live in **GZLE01.card**: **On My iPad/iPhone › BlueWake › BlueWake** in Files, or
  **`%APPDATA%\BlueWake`** on Windows (paste that path into File Explorer's address bar).
- On iPhone/iPad, **⋯ › Game Data & Saves › Back Up Saves…** exports a copy; **Restore Saves…** brings one back and
  keeps a copy of your current saves in a Backups folder first.
- On Windows, close BlueWake before copying or replacing the card, and keep a backup outside its data folder.
- Moving from Dolphin? See [Dolphin save import](#can-i-use-my-dolphin-saves) below; renaming a `.gci` is not conversion.
- Install updates over the existing app. Deleting BlueWake deletes its saves, so back them up first.
- **Remove Disc Image…** frees the space used by the disc and the files made from it. Saves, mods and
  settings stay. You must import your disc again before playing.

## Known issues

- **Loading hitches.** Changing areas can briefly stall; remaining loading and rendering costs are
  still being investigated.
- **First visits to new areas.** A bundled cache covers the areas tested so far; elsewhere, some
  objects may take a moment to appear the first time.
- **Busy scenes on iPhone** drop below 30 FPS on chips older than the M-series iPads.

## Getting help

- **Discord:** [discord.gg/xwHfUD2bxW](https://discord.gg/xwHfUD2bxW) for questions, testing and updates
- **Bug reports:** **⋯ › Help & Feedback › Report a Problem on GitHub** opens a prefilled issue. Say
  which device, where in the game it happened and what you saw, and attach the session log
  (**Share Session Log**, same menu). Please do not attach game files or disc images.

## Frequently asked questions

### Can I download it?

Public app releases are paused. You can build a personal app from source on an Apple silicon Mac
with your own disc (see [Getting started](#getting-started)). Never share that app: it contains
translated game code. An app-only build without your module shows "Translated game code: Missing".

### Why does it need my disc?

The game's code is translated from your disc during the build, and the game's graphics, sound and
world data are read from your disc on the device. Neither is included here.

### Which version of the game works?

Only the GameCube USA release, `GZLE01` revision 0. The build checks the disc and refuses others. The
Wii U *Wind Waker HD* is a different game and is not supported.

### Is this an emulator?

Not in the usual sense. The game's code is translated to native arm64 ahead of time and there is no
JIT; only a few rare instructions fall back to an interpreter. The hardware around the CPU (graphics, audio, memory
card, timing) comes from a Dolphin-derived runtime.

### Why 30 FPS?

That is the game's own frame rate on the GameCube. BlueWake targets that frame rate at 100% game
speed; raising the speed would make gameplay run faster too.

### Where do I put Hypatia's texture pack?

Extract the pack first. Its **`GZL` folder contains the textures**; keep the `tex1_…` PNG/DDS filenames
and the subfolders inside it.

- **Windows:** press **F1 › Mods › Open the texture folder**, or open
  `%APPDATA%\BlueWake\Load\Textures\GZLE01`. Copy `GZL` into it, giving
  `…\GZLE01\GZL\…\tex1_….dds` (or `.png`). Enable **HD texture pack** and restart BlueWake.
- **iPhone/iPad:** use **⋯ › Mods › Install Texture Pack…** and select `GZL` in Files, then enable
  **HD Texture Pack** and restart. For manual copying, put `GZL` under
  **On My iPad/iPhone › BlueWake › BlueWake › Load › Textures › GZLE01**.

Subfolders are searched recursively, so an extra `Hypatia WWHD Mod…` folder does not itself prevent
loading. Keep unused **Optional Textures** outside the installed texture folder, though: they can
contain competing replacements for the same texture. Add only the variants you choose. The pack's
**WideScreen Patch** folder is not needed; use BlueWake's built-in widescreen setting.
For a smaller pack on iPhone/iPad, see [Mods](docs/MODS.md#installing).

### BlueWake.exe does nothing when I open it

The symptom alone does not identify the cause. These steps apply to this repository's experimental
Windows port; include the repository/fork and exact build version when reporting it.

1. Run the executable from the **complete personal build folder**, not by itself or from inside an
   archive. Keep the builder's runtime DLLs, `gGZLE01_recomp.dll` and `game` folder together.
2. Open `%APPDATA%\BlueWake\logs` in File Explorer. Attach the newest `session-*.log` and, if present,
   the matching `crash-*.log`. If no new log appears, say so; Windows may be failing before BlueWake's
   logging starts. Include any Windows error message, your CPU/GPU and Windows version.
3. Check the [Windows requirements](docs/WINDOWS.md#what-you-need). If Windows names a missing DLL,
   restore the complete build output; do not collect DLLs from random download sites. A missing
   `gGZLE01_recomp.dll` requires building your own game module from your disc.

Do not delete `%APPDATA%\BlueWake` to troubleshoot: it holds your saves and settings.

### How does Smooth Motion work?

BlueWake matches objects between consecutive game frames, interpolates their transforms and supported
vertex motion, then renders extra views of the scene. **Game logic stays at 30 updates per second.**
This is geometry-based rendering interpolation, in the same broad category as other Zelda ports'
renderer interpolation; it does not mean their implementations are identical. BlueWake uses no AI
image generation or NVIDIA DLSS, and it does not simply crossfade two finished screenshots.

On iPhone/iPad, **⋯ › Display › Smooth Motion (Experimental)** offers 60 FPS and, on ProMotion screens,
120 FPS. The desktop settings also offer **Match the display**, up to 240 FPS. These are presentation
targets, not guarantees or faster game logic. Unmatched objects and camera cuts can skip interpolation.
It is experimental and off by default; disable it if you see artifacts or worse performance.

### Does it work on iPhone?

Yes, on an A13 or newer. The touch controls sit in the black bars beside the picture. The busiest scenes
dip below 30 FPS on an iPhone 14; see [Performance](#performance).

### Do controllers work?

Yes. Controllers that iOS supports work, with camera inversion and button remapping under
**⋯ › Controller**, and the game's rumble is passed to the controller.

### Will updates keep my saves?

Yes, as long as you install over the existing app. Use **Back Up Saves…** before anything else, and
never delete the app to update it.

### Can I use my Dolphin saves?

Yes, for **USA (`GZLE01`) in-game saves**. Export a `.gci` from Dolphin's **Tools › Memory Card Manager**
or use its existing `.gci` file. Dolphin save states are not supported by this importer.

- **iPhone/iPad:** copy the save to your device and open
  **⋯ › Game Data & Saves › Import Dolphin Save…**. Pick the source quest log and destination
  BlueWake slot; the app backs up your current card first. The importer also accepts Dolphin `.raw` cards.
- **Windows:** this port currently has no **Import Dolphin Save** screen. A `.gci` must be imported
  into BlueWake's `GZLE01.card` container; renaming it to `.card` will not work. Source users can use
  the repository's [save-import command-line helper](tests/dolphin_save_import_cli.c), which uses the
  same importer as iOS. Community converters are separate tools, not required by the iOS app or
  validated by this README.

Always close BlueWake and back up the destination card before a manual import. Work on a copy, check
the resulting quest logs, then put the converted card at `%APPDATA%\BlueWake\GZLE01.card` on Windows.
Importing into an existing game save replaces only the selected slot; a card with no Wind Waker save
receives the whole imported save, including all three slots.

## Documentation

- [Current status](docs/status/CURRENT.md): the engineering log, newest first
- [Build your own BlueWake](docs/BUILD_YOUR_OWN.md): the player's guide
- [BlueWake on Windows](docs/WINDOWS.md): building and playing on a Windows PC
- [Apple TV build](docs/status/TVOS_BUILD.md): build and install the controller-first tvOS app
- [The Builder](docs/BUILDER.md): how the build works, and reusing it for other ports
- [Device build](docs/status/DEVICE_BUILD.md): signing, installing and build options
- [Mods](docs/MODS.md): the three mods and how code mods are built
- [History](docs/HISTORY.md): the project's earlier README, from macOS prototype to iPad
- [Porting history](docs/PORTING_HISTORY.md) and [legal and provenance](docs/research/LEGAL_AND_PROVENANCE.md)

## Credits

- [Dolphin](https://dolphin-emu.org/): the compatibility runtime, DSP audio and texture-pack format
  come from it, and its GZLE01 game settings supply the widescreen code
- [RecompCore](https://github.com/chrissotraidis/RecompCore) and
  [DolRecomp](https://github.com/chrissotraidis/DolRecomp), forked for BlueWake, with the Aurora
  renderer and Dawn
- [zeldaret/tww](https://github.com/zeldaret/tww), the Wind Waker decompilation, for research
- [Better Wind Waker](https://github.com/WideBoner/betterww) by WideBoner
- HD texture pack authors, including
  [Hypatia](https://forums.dolphin-emu.org/Thread-hypatia-s-tloz-the-wind-waker-hd-pack-v2-0001a)
- SunPad, whose touch control overlay BlueWake adapts
- [Elliott Tate (@elliotttate)](https://github.com/elliotttate), for the rendering, desktop, gameplay
  and performance enhancements being consolidated into BlueWake from his Wind Waker fork
- [Ian MacFarlane (@iannotian)](https://github.com/iannotian), for the controller-first Apple TV contribution

## License and legal

BlueWake is licensed under the [GNU GPL, version 3 or later](LICENSE), the license its Dolphin-derived
runtime and SunPad-derived touch controls allow together. See
[RIGHTS_AND_LICENSES.md](RIGHTS_AND_LICENSES.md) for the details and for game content.

BlueWake is an independent fan project, not affiliated with or endorsed by Nintendo. *The Legend of
Zelda*, *The Wind Waker* and GameCube are trademarks of Nintendo. You need your own legally obtained
disc and are responsible for following the laws that apply to it.
