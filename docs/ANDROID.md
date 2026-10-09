# BlueWake on Android

BlueWake also builds as a native Android (arm64) app. As on the other platforms, you build it yourself from
your own disc: the game's code is translated from that disc during the build, so **the APK you build is yours
alone: never share or upload it.**

It is the same static recompilation as the Windows and iOS builds: the same translator, the same pinned
runtime (RecompCore, GXRuntime, Aurora), the same generated game source verified against the same digest, and
the Windows builder's source steps. Only the target is different: the game module and the app are compiled for
arm64 Android with the NDK's clang, Aurora draws through Dawn on **Vulkan**, SDL3 runs the app (its Java
activity, input and audio), and a touch overlay stands in for a controller.

## Status

Brought up on 2026-09-30 on one device, a Samsung Galaxy Z Fold 7 (Snapdragon 8 Elite, Adreno 830, Android 16),
from a Redump-verified `.rvz` converted to ISO with Dolphin, built on a Windows 11 PC (i9-13900K) with NDK r29:

- The builder runs end to end on Windows: the generated source has the verified digest (`54f54434`), and the mods
  give the Windows builder's counts (813 chunks with the variants).
- The game boots, renders through Vulkan, plays sound and plays with the touch controls, on both of the Fold's
  screens; its memory card is created in the app's folder. A scripted route (the Windows builder's training route: boot, the
  title, a new file, the prologue, player control on Outset) reaches player control, and the owner of the device
  played from a new file onto Outset.
- Local optimization training runs on the device (`--device`, below); the trained module holds the title's
  flyover at full speed with the game thread 67 percent busy, where the untrained one ran at 83 to 98 percent speed.

Checked again on 2026-10-04 on BlueWake's `main` (f1915a1), on the same phone and PC, untrained (no `--device`):

- The builder runs end to end with the Windows builder's steps as they are now: the verified digest, its default
  source optimizations (`prepare_blocks`), and the game module compiled at `-O2` for `cortex-a78` in 29 minutes
  (18 jobs).
- The scripted route reaches player control on Outset at the same retrace and position as the build above (20255).
  At retrace 15000 the picture matches that build's frame for frame (the lookout, the sea, the island); sound plays and
  the memory card is created in the app's folder.
- Speed, with the phone already warm (surface 38 °C, charging): the title and the opening at 30 FPS, the prologue at
  23 FPS, Outset at 22 FPS with the game thread 98 percent busy. The earlier build trained on the phone with
  `--device` and compiled for `oryon-1` held Outset at 30 FPS on the phone the same night, so train on the device
  for play.

Measured with the scripted route, rendered and paced, the phone charging over USB:

| Build | Boot, title, file select | Prologue (the view over Outset) | Player control on Outset |
| --- | --- | --- | --- |
| No optimization profile, `-mcpu=cortex-a78` | 30.0 FPS | 26.4 FPS (lowest 21.4) | 28.3 FPS |

**Sustained speed is set by the phone's temperature.** Samsung's thermal manager lowers the CPU's frequency
limit in steps as the back of the phone warms, independently of the thermal zones' cooling devices. Measured on
the Fold 7 with the trained module, Link standing on Outset, the phone charging:

| Surface (SKIN) temperature | CPU limit (prime cores) | Game speed |
| --- | --- | --- |
| below 38 °C | 4.47 GHz (none) | 30 FPS |
| 38 °C | about 2.0 GHz | 30 FPS, game thread 84 to 89 percent busy |
| 40 °C | about 1.7 GHz | 24 to 28 FPS |
| 42.7 °C | about 1.4 GHz | about 21 FPS |

The game's work per frame (instructions counted with `simpleperf stat`) and its memory (about 850 MB once on
Outset) stay flat over a session: the slowdown after several minutes of play is the frequency limit, not a leak.
So on a phone, power is speed: the Android build asks for a 60 Hz display (not 120), leaves Smooth Motion off, and
lowering the render resolution (the GPU was 72 percent busy at 2x) or playing unplugged keeps the limit higher for
longer.

Not yet tried: saving and reloading a game on Android, other devices, Android versions and GPUs (Mali, older
Adreno), game controllers on Android (they go through SDL, as on Windows), the HD texture packs, and the later game.

## What you need

- A Windows PC (the builder runs the Windows side of the pipeline: the translator and the disc extractor)
  - Visual Studio 2022 or newer with the C++ workload (its headers and libraries), and a clang for Windows:
    Visual Studio's own clang component, or a portable LLVM (`clang+llvm-*-x86_64-pc-windows-msvc`), passed
    with `--llvm`
  - Python 3.10+, Git, CMake 3.25+ (Ninja from the Android SDK's CMake is used if none is on PATH)
- The Android SDK with the NDK (r27 or newer), build-tools and a platform, and a JDK 17
- An arm64 Android device on Android 13 (API 33) or newer with Vulkan
- Your disc image of *The Legend of Zelda: The Wind Waker*, GameCube USA (`GZLE01`, revision 0), as an `.iso`.
  Convert an `.rvz` with Dolphin first: `DolphinTool convert -i GAME.rvz -o GZLE01.iso -f iso`

## Build

```bash
python scripts/android/build.py PATH/TO/GZLE01.iso --llvm PATH/TO/llvm --sdk PATH/TO/android-sdk --jdk PATH/TO/jdk-17
```

The steps and their logs (`build/android/logs`) are the Windows builder's, then:

- **8 compile**: the game module `libgGZLE01_recomp.so` for arm64 (`-mcpu=cortex-a78` by default: any ARMv8.2
  device of the last few years, the Quest 3 included; `--cpu oryon-1` for a Snapdragon 8 Elite). About 25 minutes on
  the PC above, 35 with an optimization profile.
- **9 app**: `libmain.so` from `android/CMakeLists.txt`: the unchanged host (`runtime/host/src`), GXRuntime,
  Aurora with Dawn (Aurora's prebuilt `dawn-android-aarch64` package) and SDL3 (built from source), the donor DSP
  and the Android entry shim and touch bridge (`android/src`).
- **10 package**: the APK, built without Gradle: SDL's Java (from the SDL source Aurora fetched) and
  `android/java`, compiled with `javac` and `d8`, linked with `aapt2` with Aurora's bundled pipeline cache as an
  asset, the two libraries stripped of debug information, stored uncompressed, 16 KB aligned and signed with a
  local debug key (`build/android/debug.keystore`). Two changes are made to SDL's Java as it is copied: the thread
  that runs the game gets a 64 MB stack (as the Windows executable is linked with), and `SDLSurface` tells Aurora
  when its surface is ready and gone (`auroraNativeSetSurfaceReady`, which Aurora's Android window code waits for).

| Option | |
| --- | --- |
| `--device SERIAL` | Train the optimization profile on this adb device (below) |
| `--train-app` | With `--device`, also train the app (`libmain.so`) on the device, with a drawn playback (below) |
| `--no-app-profile` | Build the app without its trained profile |
| `--cpu CPU` | `-mcpu` for the module and the app (default `cortex-a78`) |
| `--profile FILE` | Compile with this optimization profile instead of training one |
| `--package ID`, `--label NAME` | The application id (default the profile's bundle id, `dev.bluewake.BlueWake`) and the app's name |
| `--app-only` | Reuse the compiled game module; rebuild the app and the APK |
| `--no-mods`, `--source-only` | As the Windows builder's |
| `--debuggable` | Mark the APK debuggable (it is always profileable by `simpleperf`) |

**Optimization training on the device.** With `--device SERIAL` the builder does what the Windows builder's
training does, on the phone: it compiles an instrumented game module (`-O0`, `-fprofile-instr-generate`, a few
minutes), installs it in a training APK, plays the opening to player control on Outset headless and unpaced (plain,
then with widescreen and Better Wind Waker's options, about 20 minutes each on the Fold 7), pulls the counts, merges
them with the NDK's `llvm-profdata` and compiles the real module with them. A short run first checks that a profile
is written at all. The runs use their own memory card; the player's saves are not touched. The profile is made from
the game, so it stays in `build/android/pgo-device` and is never shared. The training APK is left installed: put
the real one back with `install.py`.

**The app's own training.** The module's training is headless, so it never runs the GX worker's translation or
the render thread, the app's busiest code (the GX worker is most of a core on the sea). With `--train-app` the
builder also compiles an instrumented `libmain.so` (the host, GXRuntime, Aurora and SDL; Dawn is prebuilt),
installs it with the real game module and plays the same opening once, drawn and at the game's pace with the
player's settings (about 7 minutes), then compiles the app with those counts. The profile stays in
`build/android/pgo-app`, and every later app build uses it (`--no-app-profile` leaves it out). A later
`--train-app` reuses it while the module, the app's sources, RecompCore, the compiler and the CPU are unchanged
(`--retrain` trains again).

Measured on the Fold 7 over 20 seconds of the title's sea (600 game frames, `simpleperf stat --per-thread`), the
CPU cycles the app's two busy threads spend per frame (the game thread, about 70 M, is the game module's):

| App build | GX worker | Render thread |
| --- | --- | --- |
| Before | 51 M | 7.9 M |
| Linked with `-Bsymbolic-functions` | 49 M | 7.5 M |
| And trained (`--train-app`) | 47 M | 7.9 M |

`libmain.so` is linked with `-Bsymbolic-functions`: its exported functions (the host's, Dawn's and SDL's C entry
points) were called through the PLT even from within libmain.

## Install

```bash
python scripts/android/install.py
```

installs `build/android/WindWakerRecomp.apk` on the connected phone (`--serial` picks one when several devices
are connected) and pushes what the game reads to the app's folder,
`/sdcard/Android/data/<application id>/files/game`: `main.dol`, the 415 RELs and the disc image (1.4 GB,
pushed once). They come from your disc and are never inside the APK. The application id is the one the build used.

In that same folder (`files/`):

| | |
| --- | --- |
| `GZLE01.card` | the memory card: your saves |
| `sram.bin` | the console's settings (sound mode) |
| `Backups/` | the cards kept before Restore Saves or Import Dolphin Save replaced one |
| `Load/Textures/GZLE01/` | an HD texture pack (Mods > Install Texture Pack) |
| `launch.env` | optional launch settings, one `NAME=value` a line (`install.py --env NAME=value`) |
| `logs/session-*.log` | the newest eight sessions (also in logcat, tag `BlueWake`) |

**Uninstalling the app deletes this folder, saves included.** Back up the card first:
`python scripts/android/install.py --pull-saves backup/`. Installing a new build over the old one keeps it.

For testing over adb on a locked phone, `am start -n <application id>/dev.bluewake.android.BlueWakeActivity
--ez showWhenLocked true` shows the game over the lock screen (the training uses it); a launch from the home screen
never does.

## Play

The app is the iPhone and iPad app's, feature for feature (apple/ios/src/BWGameOverlay.mm): the same menu, the
same touch controls, the same settings under the same names.

- **The ⋯ button** (top right) opens the menu over the paused game: Display (Show FPS, Smooth Motion, Render
  Resolution, Texture Filtering, Aspect Ratio), Gameplay (Jump & Sprint, Fast Transitions, Quick Doors), Mods
  (Widescreen 16:9 and 16:10, HD Texture Pack, Better Wind Waker and its settings, Install Texture Pack),
  Controller (Camera Stick, Button Mapping), Touch Controls (show or hide, Touch Control Settings, Move Controls),
  Game Data & Saves (Back Up Saves, Restore Saves, Import Dolphin Save, Where Are My Files?, Remove Disc Image) and
  Help & Feedback (Report a Problem on GitHub, Share Session Log). Back works the menu too: out of a submenu, and
  with nothing open it opens the menu. It never closes the game.
- **Touch controls**: the iPhone app's buttons, colors and layouts, every GameCube button. On a phone, with the
  original 4:3 picture, they sit in two columns beside it, clear of the game's HUD; on a tablet or a foldable's
  inner screen they go over the picture, as on the iPad. The movement stick also floats: touch anywhere on the left
  half. Move Controls drags and resizes each one; phones and tablets keep separate layouts. They hide while a game
  controller is connected (Touch Control Settings > Hide with a controller); touching the screen brings them back
  until the controller is used again.
- **Controllers**: through SDL, as a GameCube pad (as on Windows), with the menu's camera and button mapping.
- The picture keeps the game's shape and renders at three times the GameCube's 480 lines by default; the mouse
  camera is off (touches would reach it as clicks).

## Measure

```bash
python scripts/android/measure.py --card-on-device /sdcard/Android/data/<id>/files/GZLE01.card --cool --label NAME
```

loads a save from a copy of that card, waits for retrace 3000, then counts the game thread's instructions and cycles
for 60 seconds with `simpleperf` and reads the `[perf]` lines over the same window. It prints the speed, how busy the
game thread was, and instructions and cycles per retrace, and writes them with the session log to
`build/android/measure`. Instructions per retrace is the number to compare between builds: it holds while the phone
heats and throttles. `--cool` waits for the phone to cool to under 34 °C at full clocks first; `--apk` installs a
build first; `--headless` measures without drawing or pacing; `--env` adds launch settings (a test warp, the
player's display settings). The card is only read.

## Files

| | |
| --- | --- |
| `android/CMakeLists.txt` | `libmain.so`: the host, GXRuntime, Aurora, SDL3 and Dawn (static), the donor DSP |
| `android/src/android_entry.c` | The entry shim: paths, the session log, `launch.env`, the training profile's write |
| `android/src/android_shell.cpp` | The shell's native side: the touch pad, pause reasons, FPS, settings and save glue (the iPhone app's `apple/ios/src` C++ is compiled in) |
| `android/src/profile_flush.c` | Compiled into training modules only |
| `android/java/dev/bluewake/android/` | The activity (SDL's, a 60 Hz display request), the ⋯ menu, touch controls and panels (`Overlay`, `MenuView`, `TouchControlsView`), the settings (`Settings`), saves and texture packs (`GameData`), and the log share (`LogProvider`) |
| `android/AndroidManifest.xml`, `android/res/` | The manifest and the icon (the iOS app's) |
| `scripts/android/build.py`, `install.py`, `measure.py` | The builder (on top of `scripts/windows/build.py`), the installer, and the speed measurement |

Outside these, the port changes a few lines of shared code: Android leaves out the desktop options menu as iOS
does (`runtime/host/src/settings_menu.h`), `cmake/composite` accepts extra sources for the training module, and
`.gitignore` ignores APKs, `.so` files and keystores.

The Android port was written with substantial AI assistance (Claude), like the rest of the project; the status
above records what was checked, and on what.
