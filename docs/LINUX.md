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
- clang (with lld and llvm-profdata), CMake 3.25+, Ninja, Python 3.10+, git,
  and the development files SDL3 and the host build against (X11, Wayland,
  Vulkan, ALSA, PulseAudio). On Debian and Ubuntu, the same packages the Linux
  CI installs:

      sudo apt install clang lld llvm cmake ninja-build git build-essential \
        libwayland-dev libxkbcommon-dev libx11-xcb-dev libx11-dev libxrandr-dev \
        libxext-dev libxi-dev libxcursor-dev libgl1-mesa-dev libvulkan-dev \
        libasound2-dev libpulse-dev libxss-dev libxtst-dev

  (or your distro's equivalent). Without them the build stops at
  `app-configure` with SDL's "Couldn't find dependency package for …", which
  names the one missing. Vulkan drivers for your GPU. The host and the game
  module both build with clang, matching the Windows build's optimization
  pipeline.

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
    --host-only    after pulling a change to the app's own code (runtime/host/src,
                   linux/src): keep the last build's optimization profile instead
                   of training again, so only what the change reaches recompiles

A rebuild recompiles only the parts of the game module whose source changed.
After a change to the app alone, such as a new setting or mod hook, a normal
rebuild still trains again (the training plays the game with the app), which takes
most of the time; `--host-only` skips that. On an i5-12600KF the stage select's
hooks went in this way in 3½ minutes instead of 33: 16 of the 825 source files
changed, and the game module those steps make is byte-identical to a full
build's from the same profile. Run it from the same checkout and `--out` as the
build it updates.

## Play

    build/linux/BlueWake/bluewake

On an AppImage's first launch, **BlueWake Setup** opens automatically when no
usable disc has been remembered. Choose the USA revision-0 disc image and,
optionally, a Dolphin-format HD texture-pack folder, then press **Start
BlueWake**. Display, gameplay, controls and other preferences remain in the
shared F1/Esc settings menu used on every desktop platform. The setup file and
folder buttons use SDL's native dialog through an XDG
portal or Zenity. Dragging an ISO/GCM onto the setup window and entering paths
directly also work.

Later AppImage launches start the game directly. Use `--setup` to reopen the
populated setup window, or `--iso` to replace the remembered disc using only
the native file picker. A development-folder build keeps the simple picker as
its default when its disc is missing.

`--help` lists the options (widescreen, Smooth Motion, Better Wind Waker, fullscreen,
disc and module paths). Keyboard: arrows D-pad, J/K/U/I face buttons, W/A/S/D stick,
H/F/T/G C-stick, E/R L/R, Q Z, Return START; game controllers work. Mouse: click the
game and move to turn the camera, Esc releases it.

The optional controller zoom is under **Controls** in the settings menu. Enable it,
then hold the right-stick click and move that stick up or down; tapping the click
still enters or leaves first person. Its speed is adjustable beside the option.

Your saves, settings and session logs live in `~/.local/share/BlueWake`, outside
the build, so rebuilding never touches them. The settings menu (Esc or F1) saves to
`~/.config/BlueWake/settings.ini`.

## The app folder

`build/linux/BlueWake/` is a personal build: `gGZLE01_recomp.so` is code
translated from your disc and `game/` holds your disc image. Never share or
upload it.

## The AppImage

`scripts/linux/make_appimage.sh` packages the built app folder as an AppImage
(the default `build/linux/BlueWake-x86_64.AppImage` plus its `.zsync` metadata
for delta-capable update tools). It bundles the host, the translated game
module, the DSP roms and every shared library the host links except the
glibc/libstdc++ baseline. Build release artifacts on a suitably conservative
Linux system: like other AppImages, compatibility is limited by the glibc and
CPU baseline of the build host. The player's disc is not bundled: on first run
the launcher asks for it and prepares it into the data dir (a disc and files
extracted from it are never distributed).

### Build the AppImage

These are packaging-only dependencies; end users do not need them. The build
machine needs the [current `appimagetool`](https://github.com/AppImage/appimagetool/releases),
`desktop-file-validate` (usually from `desktop-file-utils`), `zsyncmake`
(usually from `zsync` or `zsync-curl`) and ImageMagick on `PATH`. Do not use
the obsolete tool from the old AppImageKit release page; it embeds a legacy
runtime that requires FUSE 2. After creating the Linux build from your disc as
described above, run:

    scripts/linux/make_appimage.sh

The outputs are:

    build/linux/BlueWake-x86_64.AppImage
    build/linux/BlueWake-x86_64.AppImage.zsync

To use another build directory or output name:

    scripts/linux/make_appimage.sh BUILD_DIR OUT.AppImage

These are generated artifacts and are ignored by Git. Do not commit them. The
AppImage contains the translated game module, so only a maintainer may publish
it under the Linux release exception, after it passes the release asset check:

    scripts/release/check_public_assets.sh build/linux/BlueWake-x86_64.AppImage

### Run the downloaded AppImage

1. Make the download executable (you only need to do this once):

       chmod +x BlueWake-x86_64.AppImage

2. Double-click or run the AppImage. If BlueWake does not have a usable
   remembered disc, it opens the graphical setup automatically:

       ./BlueWake-x86_64.AppImage

3. In **BlueWake Setup**:

   - Select your USA GZLE01 revision-0 `.iso` or `.gcm` with **Browse disc...**,
     by typing its path, or by dragging it onto the window.
   - To use an HD texture pack, enable it and select the pack's `GZL` or
     `GZLE01` folder with **Browse textures...**.
   - Leave **Add or update application-menu and Desktop shortcuts** enabled if
     you want both launchers. The application-menu entry also gets a
     **Configure BlueWake** action that reopens this window.
   - Once every enabled selection validates, select **Start BlueWake** to save
     the choices, install the requested launchers, and start the game. The
     launchers point to this AppImage, so keep it at the same path afterward.

4. On later launches, BlueWake remembers these choices. Start it normally from
   either installed launcher, by double-clicking the AppImage, or by running:

       ./BlueWake-x86_64.AppImage

To change the saved setup later, reopen the same window; its disc and texture
fields are filled with the current remembered values:

    ./BlueWake-x86_64.AppImage --setup

To use the former standalone ISO picker instead:

    ./BlueWake-x86_64.AppImage --iso

You can also bypass setup and supply a disc for only this launch:

    ./BlueWake-x86_64.AppImage --disc "/path/to/Wind Waker.iso"

If your system cannot mount AppImages with FUSE, open setup in extract-and-run
mode instead. Launchers installed during this run remember the fallback:

    APPIMAGE_EXTRACT_AND_RUN=1 ./BlueWake-x86_64.AppImage --setup

#### End-user dependencies

The setup feature adds no mandatory runtime library to the AppImage: SDL and
Dear ImGui are bundled. Its **Browse** buttons use one of these system
file-dialog providers when available:

- an XDG desktop portal plus a backend for your desktop (for example,
  `xdg-desktop-portal` and `xdg-desktop-portal-gtk`); or
- Zenity.

Most desktop Linux installations already provide an XDG portal. The provider
is optional because paths can also be typed or dragged into the setup window.
If the buttons do not open, install the appropriate backend or use either of
those alternatives. FUSE is also optional; use
`APPIMAGE_EXTRACT_AND_RUN=1` as shown above when it is unavailable.

### Steam Deck

Run the AppImage setup in Steam Deck's Desktop Mode. It creates application-menu
and Desktop launchers there; if extract-and-run mode was needed during setup,
the generated launchers preserve that fallback. This improves Desktop Mode
installation but does not add BlueWake to Steam's Gaming Mode library.

## Releases

When a ready-made Linux build is published, it follows the same exception as
Windows: it is made on a maintainer's or contributor's personal machine from
their disc and attached to the release by hand. The disc, files extracted from
it, and console keys never enter GitHub or CI (a secret could not hold a 1.4 GB
disc, and must not). CI builds and tests everything that does not need the disc
(`.github/workflows/linux-host.yml`). The same Ubuntu 24.04 (`glibc 2.39`) job
runs the host tests, then uses a synthetic empty module to exercise AppImage
packaging, recursively audit every ELF's ABI, and smoke-test the image. That
non-playable fixture is never uploaded. A playable release still comes from a
permitted personal build, and every published artifact passes
`scripts/release/check_public_assets.sh`.

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
