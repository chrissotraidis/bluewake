# BlueWake on Linux

Build The Legend of Zelda: The Wind Waker (GameCube USA, GZLE01 revision 0)
from your own disc and play it on Linux. This is the same game as the Windows
build: a native host, the translated game module, and the DSP, packaged as a
folder you run with `./bluewake`.

## What you need

- An x86-64 Linux PC.
- Your own GZLE01 revision 0 disc image, as an uncompressed `.iso` or `.gcm`.
  (A Dolphin-compressed image like `.rvz`/`.wia` is not converted here; convert
  it in Dolphin: right-click the game, Convert File, format ISO.)
- clang (with lld and llvm-profdata), CMake 3.25+, Ninja, Python 3.10+, git:
  `sudo apt install clang lld llvm cmake ninja-build git` (or your distro's
  equivalent). Vulkan drivers for your GPU. The host and the game module both
  build with clang, matching the Windows build's optimization pipeline.

## Build

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux

The first build clones the pinned RecompCore and DolRecomp sources into
`ref/recompcore`, translates your disc, and compiles the game module (the long
step). Rerun the same command to continue or reuse an existing build. Your
disc, the extracted files and the translated module stay in `build/linux`,
which git ignores.

Useful options:

    --source-only  stop after translating: checks tools, disc and translation
                   in minutes, before the long compile
    --no-mods      skip the widescreen and Better Wind Waker variants
    --opt-level 1  faster to compile, a little slower in game
    --jobs N       parallel compile jobs (default: all cores, limited by memory)

## Play

    build/linux/BlueWake/bluewake

`--help` lists the options (widescreen, Smooth Motion, Better Wind Waker, fullscreen,
disc and module paths). Keyboard: arrows D-pad, J/K/U/I face buttons, W/A/S/D stick,
H/F/T/G C-stick, E/R L/R, Q Z, Return START; game controllers work. Mouse: click the
game and move to turn the camera, Esc releases it.

Your saves, settings and session logs live in `~/.local/share/BlueWake`, outside
the build, so rebuilding never touches them. The settings menu (Esc or F1) saves to
`~/.config/BlueWake/settings.ini`.

## The app folder

`build/linux/BlueWake/` is a personal build: `gGZLE01_recomp.so` is code
translated from your disc and `game/` holds your disc image. Never share or
upload it.

## The AppImage

`scripts/linux/make_appimage.sh` packages the built app folder as an AppImage
(the default `build/linux/BlueWake-x86_64.AppImage` plus its `.zsync` for delta
auto-update). It bundles the host, the translated game module, the DSP roms and
every shared library the host links except the glibc/libstdc++ baseline, so the
image runs on any x86-64 desktop. The player's disc is not bundled: on first run
the launcher asks for it and prepares it into the data dir (a disc and files
extracted from it are never distributed). Needs appimagetool, desktop-file-utils
and zsync-curl on the build host.

## Releases

A ready-made Linux build that includes the game code is published the same way as
Windows: it is made on a personal machine from the owner's disc and attached to the
release by hand. The disc, files extracted from it, and console keys never enter
GitHub or CI (a secret could not hold a 1.4 GB disc, and must not). CI builds and
tests everything that does not need the disc (`.github/workflows/linux-host.yml`),
and every published artifact passes `scripts/release/check_public_assets.sh`.

## Why clang

The host (Aurora, SDL3, the DSP) and the game module both build with clang, matching
the Windows build. Aurora uses C++20 designated-initializer field orders that gcc
rejects, so the host needs clang. The game module's translated chunks are each one
enormous generated function, and clang's optimizer is pathologically slow on them
(many minutes per chunk at `-O2`); the build defeats that with `-fno-slp-vectorize`
and `-mllvm -large-interval-freq-threshold=10` (the two superlinear passes), the same
flags the Windows builder uses, and then applies the certified native accelerators,
fixed CPU/RAM, direct calls, gather pipe and a locally trained PGO profile — the
pipeline that reaches 30 FPS (see docs/PERFORMANCE_OPTIMIZATIONS.md).
