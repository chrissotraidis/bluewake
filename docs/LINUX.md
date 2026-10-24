# BlueWake on Linux

Build The Legend of Zelda: The Wind Waker (GameCube USA, GZLE01 revision 0)
from your own disc and play it on Linux. This is the same game as the Windows
build: a native host, the translated game module, and the DSP, packaged as a
folder you run with `./bluewake`.

## Choose a build method

BlueWake offers two ways to produce the same Linux app. Use the **container
build** for a reproducible or SteamOS-targeted AppImage. Use the **native
build** when you prefer your installed Linux toolchain and do not need a
portable ABI baseline.

Container builds use Clang 18 by default, matching Linux CI. Pass
`--ci-container --clang22` to override that default with Clang 22; native builds
continue to use the supported Clang selected by the host's `PATH`.

| | Container build (recommended) | Native build |
| --- | --- | --- |
| Host | Linux, macOS, or Windows through WSL2 | x86-64 Linux |
| Requirements | Python 3.10+, Git, Docker with Linux containers | The complete compiler and development package list below |
| Advantages | Same Ubuntu 24.04 environment as CI; no host compiler setup; AppImage targets CI's SteamOS-compatible `GLIBC_2.39` ceiling | No Docker or container image; fastest on a configured x86-64 Linux workstation |
| Tradeoffs | Downloads and stores a Docker image; slower under `linux/amd64` emulation on Arm hosts | Depends on host package versions; an AppImage built on a newer distribution may exceed SteamOS's glibc ABI and fail its audit |

Both methods use the same builder, options, optimizations, PGO training and
output paths. They require your own GZLE01 revision 0 disc as an uncompressed
`.iso` or `.gcm`. Convert Dolphin `.rvz`/`.wia` images to ISO in Dolphin first.

## Container build (recommended)

Install and start Docker, then run:

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux \
      --ci-container

To create the personal AppImage as part of the same build, add `--appimage`:

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux \
      --ci-container --appimage

Clang 18 is the default, matching Linux CI. To evaluate Clang 22 explicitly,
add `--clang22` and use a separate output directory from an existing Clang 18
build:

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux-clang22 \
      --ci-container --clang22

The version-controlled [`scripts/linux/Dockerfile`](../scripts/linux/Dockerfile)
supplies Clang 18 and 22, CMake, development libraries and AppImage tools.
Clang 18 remains the default and CI also selects it explicitly; `--clang22` is
an opt-in for container builds. Either compiler uses Ubuntu's libstdc++, which the
AppImage links statically. Docker caches unchanged layers,
mounts the disc read-only and writes normal outputs to `build/linux`; the disc
never enters the image or CI. The result is always an x86-64 Linux build. Arm
hosts use Docker's `linux/amd64` emulation, which is substantially slower.

### If the container build fails

- **Don't run it with `sudo`.** Files a root run creates make Git refuse the checkout ("dubious ownership") and
  block later runs. Give them back with `sudo chown -R "$USER": path/to/bluewake`, add yourself to the
  `docker` group (`sudo usermod -aG docker "$USER"`, then log out and back in), and check that `docker info`
  works without `sudo`.
- **"failed to extract layer", "content digest ... not found" or another image error:** Docker's build cache is
  damaged, often after a run with `sudo`. Your disc image is fine. Run `docker builder prune -af` and build again.
- **Bazzite, Fedora Atomic and other read-only systems:** build inside a Distrobox (for example Ubuntu 24.04) with the
  native steps below, or use the ready-made AppImage once it is published.

## Native build

Install CMake 3.25+, Ninja, Git, `pkg-config`, Clang/Clang++ 17 or newer, LLD,
`llvm-profdata` and the graphics/audio development libraries. Ubuntu
24.04 package names are:

    sudo apt install clang lld llvm cmake ninja-build \
      git build-essential pkg-config libwayland-dev libxkbcommon-dev \
      libx11-xcb-dev libx11-dev libxrandr-dev libxext-dev libxi-dev \
      libxcursor-dev libgl1-mesa-dev libvulkan-dev libasound2-dev libpulse-dev \
      libxss-dev libxtst-dev libdbus-1-dev zlib1g-dev libpng-dev libfreetype6-dev \
      libsqlite3-dev libzstd-dev libfmt-dev libbz2-dev libbrotli-dev libicu-dev

Use your distribution's equivalent packages, then run:

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux

The builder checks command-line tools before reading the disc; CMake identifies
missing development libraries during `app-configure`. Vulkan drivers are needed
to run the result. Native `--appimage` additionally requires the packaging tools
listed under [Build the AppImage](#build-the-appimage).

## Build behavior

The first build clones the pinned RecompCore and DolRecomp sources into
`ref/recompcore`, translates your disc, and compiles the game module (the long
step). Rerun the same command to continue or reuse an existing build. Your
disc, extracted files and translated module stay in `build/linux`, which Git
ignores.

## Outputs

Both build methods write to the host directory selected by `--out`; container
artifacts are not left inside Docker. With the default `--out build/linux`, the
final outputs are:

| Output | Created by | Host path | Contents |
| --- | --- | --- | --- |
| Playable app folder | Every complete build | `build/linux/BlueWake/` | `bluewake`, the translated module, DSP files and your disc under `game/` |
| AppImage | A build with `--appimage` | `build/linux/BlueWake-x86_64.AppImage` | Portable Linux app and translated module; the disc itself is excluded |
| Update metadata | A build with `--appimage` | `build/linux/BlueWake-x86_64.AppImage.zsync` | Delta-update metadata for that AppImage |
| Build logs | Every build step | `build/linux/logs/` | Compiler, training, packaging and ABI-audit logs |

Run the folder build with `build/linux/BlueWake/bluewake`, or copy the
AppImage and make it executable. Both the folder and AppImage are personal
artifacts because they contain code translated from your disc; do not commit
or publish them. A different `--out PATH` replaces the `build/linux` prefix in
every path above.

Useful options:

    --clang22     use Clang 22 for a container build (requires --ci-container;
                  Clang 18 remains the default)
    --source-only  stop after translating: checks tools, disc and translation
                   in minutes, before the long compile
    --no-mods      skip the widescreen and Better Wind Waker variants
    --no-behaviour-timing
                   check the turn's budget after every block, as before behaviour
                   timing (on by default since October 14); about 9% slower
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

| Key | |
| --- | --- |
| Settings | F1 or Esc after releasing the mouse |
| HD look | F2 |
| Save state / load the latest | F5 / F8 |
| Stage select | F7, once it's on in settings |
| Frame rate | F9 |
| Smooth Motion | F10 |
| Fullscreen | F11 or Alt+Enter |

A few words at the bottom left say what each key did.

The optional controller zoom is under **Controls** in the settings menu. Enable it,
then hold the right-stick click and move that stick up or down; tapping the click
still enters or leaves first person. Its speed is adjustable beside the option.

The settings menu's **Display** tab has what the Windows one has: a **Fullscreen** button for this session
(F11), **Start in fullscreen** for the next launch (as `--fullscreen`), **Reset the window**, **Keep the
picture's shape** (black bars rather than stretching a 4:3 picture; a widened one always keeps its shape),
**Pause while the window is in the background** and **Compile shaders before playing**. The settings
that are new on Linux are off until you turn them on. The widescreen choice is under **Mods**, after HD
textures. Under **Gameplay**, Jump & Sprint (with a **Sprint speed**) and Fast transitions are one
switch each, as on the iPad.

**Config** is the last tab: **Exact sound** (off: Dolphin's high-level Zelda sound, the default and faster;
on: the sound chip's own program, slower; next launch), the menu's own settings (**Pause the game while the
menu is open**, **Menu text size** and **Menu contrast**), and where your saves and session logs are, with
buttons that open
the folder and the logs. The menu opens at the size and place you left it. The menu's buttons are the same on every
desktop build: Resume, Save state (F5), Load latest state (F8) and Quit the game.

A setting that applies at the next launch shows **Restart now** beside its note: BlueWake saves your
settings, closes and starts again by itself.

**Wayland:** when `WAYLAND_DISPLAY` is set, BlueWake presents with mailbox instead of FIFO (#370, jslay88). FIFO
waits for the compositor on the game thread, and on KWin 6.8 that wait held the game at about 70% speed.
Mailbox doesn't tear. `BLUEWAKE_VSYNC=1` brings back FIFO. Checked on KWin 6.8; GNOME and the Steam Deck's
gamescope haven't been checked yet.

**Safe mode:** if the last launch never reached a clean exit (it crashed or was killed), the next one starts
in safe mode, as on Windows: Fast sound, 4:3, in a window, with the mods, Better Wind Waker's settings, the
texture pack, the HD look, the gameplay extras and the stage select off. Your earlier `settings.ini` is copied beside it
first (`settings.ini.before-safe-mode-...`), and a note says so for fifteen seconds. Turn things back
on in the settings menu. `--safe-mode` starts in safe mode on purpose.

Your saves and session logs live in `~/.local/share/BlueWake`, outside the
build, so rebuilding never touches them. The settings menu (Esc or F1) saves
preferences separately to `~/.config/BlueWake/settings.ini`.

**Portable mode:** create an empty file named `portable.txt` next to the
`bluewake` executable, and BlueWake keeps the memory card, settings, save
states, logs, texture packs and shader caches in a `user` folder beside it
instead, as on Windows. Controller remaps made in the usual folder are copied
over the first time; nothing is moved. To bring your saves along, copy
`~/.local/share/BlueWake/GZLE01.card` (and `sram.bin`) into that `user` folder
first. `BLUEWAKE_DATA_DIR` still wins over both. `scripts/linux/build.py
--portable` puts `portable.txt` in the app folder for you; a later rebuild
leaves the `user` folder and `portable.txt` alone.

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

These are packaging-only dependencies; end users do not need them. A native
package build needs the [current `appimagetool`](https://github.com/AppImage/appimagetool/releases),
`desktop-file-validate` (`desktop-file-utils`), `zsyncmake` (`zsync` or
`zsync-curl`), `readelf` (`binutils`) and ImageMagick (`magick` or `convert`)
on `PATH`. The builder checks all of them before starting when `--appimage` is
present. Do not use the obsolete tool from the old AppImageKit release page;
it embeds a legacy runtime that requires FUSE 2. After creating the Linux build
from your disc as described above, run:

    scripts/linux/make_appimage.sh

Alternatively, `--appimage` keeps packaging explicit while performing it after
the normal app-folder build. Combine it with `--ci-container` to use the shared
CI image and avoid installing packaging tools on the host:

    python scripts/linux/build.py path/to/GZLE01.iso --out build/linux \
      --ci-container --appimage

The shared Dockerfile pins appimagetool, the Type 2 runtime and CMake. It uses
Clang 18 by default and includes optional Clang 22 tools.
The completed AppImage is automatically audited for prohibited private files,
accidental libstdc++ linkage and a maximum `GLIBC_2.39` requirement.
To pass that audit, `--appimage` links libstdc++ into the app statically
(`-static-libstdc++`), as Linux CI does, so the AppImage needs no `GLIBCXX_*`
from the player's system and uses the same prebuilt Dawn package as a folder
build. It uses its own `app-appimage` build folder; folder builds keep
libstdc++ shared. (AppImages used LLVM libc++ with Dawn built from source
before; on four slow cores that drew frames about 3% slower.)

The outputs are:

    build/linux/BlueWake-x86_64.AppImage
    build/linux/BlueWake-x86_64.AppImage.zsync

To use another build directory or output name:

    scripts/linux/make_appimage.sh BUILD_DIR OUT.AppImage

These are generated artifacts and are ignored by Git. Do not commit them. The
AppImage contains the translated game module, so only a maintainer may publish
it under the Linux release exception, after it passes the release asset check:

    scripts/release/check_public_assets.sh build/linux/BlueWake-x86_64.AppImage

### Run the AppImage

No ready-made Linux AppImage has been published yet; these steps work for one you built.

1. Make the AppImage executable (you only need to do this once):

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

4. **Start BlueWake later.** Selecting **Start BlueWake** records the original
   disc path in `$XDG_DATA_HOME/BlueWake/disc.txt` (normally
   `~/.local/share/BlueWake/disc.txt`) and keeps the prepared game files under
   that same BlueWake data directory. The optional texture-pack path is saved
   in `$XDG_CONFIG_HOME/BlueWake/settings.ini` (normally
   `~/.config/BlueWake/settings.ini`). The disc is not copied, so keep the ISO
   or GCM at the selected path.

   Every normal launch reads those saved paths. If the disc still exists and
   its prepared files are usable, BlueWake skips setup and starts the game.
   This works identically when using the application-menu launcher, Desktop
   shortcut, double-clicking the AppImage, or running:

       ./BlueWake-x86_64.AppImage

   Installed shortcuts contain the AppImage's absolute path. If you move or
   rename the AppImage, launch it from its new location with `--setup`, leave
   shortcut installation enabled, and select **Start BlueWake** to update them.
   If the disc or texture pack moves, use the same setup window to select its
   new location. A missing remembered disc automatically returns you to setup.

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
