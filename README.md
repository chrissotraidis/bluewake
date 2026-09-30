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
> **Get it with PadMint.** Each release has the BlueWake app without any game code. On an Apple
> silicon Mac, [PadMint](https://github.com/chrissotraidis/padmint/releases/latest) builds your game
> module from your own disc and adds it to that app. The build tunes the game on your Mac; the
> developer build tuned this way reaches 30 FPS on an iPad Pro (M2). See [Getting started](#getting-started).
>
> **AI disclosure:** BlueWake is developed with substantial AI assistance for code, testing,
> documentation and debugging. The status log records what has actually been checked, and on what.

**Questions or bugs?** Join the [Discord](https://discord.gg/xwHfUD2bxW) or
[open an issue](https://github.com/chrissotraidis/bluewake/issues).

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
- **The ⋯ menu:** FPS display, Smooth Motion (60 or 120 FPS), render resolution up to 4×, texture
  filtering up to 16× anisotropic, aspect ratio, mods, save backup and restore, and Report a Problem
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

**Smooth Motion** (⋯ › Display, off by default) draws in-between frames from the game's own 30, so the
picture moves at 60 FPS, or 120 FPS on a ProMotion screen, while the game itself keeps its normal
speed. It costs one extra half frame of display latency and backs off on its own when the device falls
behind. On the iPad Pro (M2) it held a steady 60 and 120 FPS at full speed through the title scenes
and Outset Island play, with the game thread near its limit outdoors. On the iPhone 14, which is
already below full speed in the busiest Outset views, it costs a few more points of game speed, so
leave it off there.

<p align="center">
  <img alt="BlueWake on an iPhone 14, with the touch controls in the black bars beside the picture" src="docs/images/bluewake-iphone-title.jpg" width="720">
</p>

## Getting started

You need:

- a Mac with Apple silicon, Xcode, CMake and Ninja, and at least 12 GB of free disk space
  (25 GB recommended)
- your `GZLE01` revision 0 disc image
- an A13 or newer iPhone or iPad on iOS/iPadOS 17 or later, with Developer Mode on
- an Apple ID for signing (a free one works; its apps expire after seven days)

**The easy way:** download [PadMint](https://github.com/chrissotraidis/padmint/releases/latest),
unzip it, double-click `PadMint.command`, choose BlueWake and drag in your disc image. PadMint gets
the tools it needs and the app from the [latest release](https://github.com/chrissotraidis/bluewake/releases/latest)
(which contains no game code), builds your game module from your disc and adds it, and saves your
BlueWake IPA in your Downloads folder.

**From this repository,** one command builds your own app from a fresh checkout:

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

## Mods

Open **⋯ › Mods**. Each mod applies the next time BlueWake starts and never changes your saves.

| Mod | What it does | How to add it |
| --- | --- | --- |
| **Widescreen 16:9** | A wider view with the HUD placed for 16:9 | Built in; turn it on |
| **HD Texture Pack** | Replaces the game's textures, for example with Hypatia's HD pack | **Install Texture Pack…** and pick the pack's folder in Files |
| **Better Wind Waker** | Wind Waker HD's quality-of-life changes: Swift Sail, instant text and more | Patch your disc on a Mac, then **Install Better Wind Waker…** |

Code mods cannot be applied to a statically recompiled game at runtime, so they are translated and built
into the app. Details are in [docs/MODS.md](docs/MODS.md).

## Your saves and game data

- Saves live in the app's memory card file, **On My iPad › BlueWake › BlueWake › GZLE01.card** in Files.
- **⋯ › Game Data & Saves › Back Up Saves…** exports a copy; **Restore Saves…** brings one back and
  keeps a copy of your current saves in a Backups folder first.
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

The app is on the [latest release](https://github.com/chrissotraidis/bluewake/releases/latest), without
the game code. The game code is translated from the game, so everyone makes their own from their own
disc: PadMint does it on an Apple silicon Mac and adds it to the released app (see
[Getting started](#getting-started)). Without it, the app opens and shows "Translated game code: Missing".

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
speed; raising the speed would make gameplay run faster too. For a smoother picture, turn on
**⋯ › Display › Smooth Motion**, which adds in-between frames without changing the game's speed.

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

Yes. Export the save from Dolphin (Tools, Memory Card Manager, or the `.gci` file in its GC folder), copy
it to your device, then open **⋯ › Game Data & Saves › Import Dolphin Save…**. Pick the quest log you want
and the BlueWake slot to put it in; your current saves are backed up first. USA saves only.

## Documentation

- [Current status](docs/status/CURRENT.md): the engineering log, newest first
- [Build your own BlueWake](docs/BUILD_YOUR_OWN.md): the player's guide
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
- [elliotttate](https://github.com/elliotttate), for Smooth Motion's in-between frames and the
  viewport and outline fixes in [his RecompCore work](https://github.com/elliotttate/RecompCore)
- [zeldaret/tww](https://github.com/zeldaret/tww), the Wind Waker decompilation, for research
- [Better Wind Waker](https://github.com/WideBoner/betterww) by WideBoner
- HD texture pack authors, including
  [Hypatia](https://forums.dolphin-emu.org/Thread-hypatia-s-tloz-the-wind-waker-hd-pack-v2-0001a)
- SunPad, whose touch control overlay BlueWake adapts

## License and legal

BlueWake is licensed under the [GNU GPL, version 3 or later](LICENSE), the license its Dolphin-derived
runtime and SunPad-derived touch controls allow together. See
[RIGHTS_AND_LICENSES.md](RIGHTS_AND_LICENSES.md) for the details and for game content.

BlueWake is an independent fan project, not affiliated with or endorsed by Nintendo. *The Legend of
Zelda*, *The Wind Waker* and GameCube are trademarks of Nintendo. You need your own legally obtained
disc and are responsible for following the laws that apply to it.
