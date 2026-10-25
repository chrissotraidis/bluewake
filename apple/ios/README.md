# BlueWake for iPadOS and iOS

The iOS host builds the same host as macOS (`runtime/host/src`) against GXRuntime,
Aurora (Metal through Dawn) and SDL3 for UIKit, plus iOS-only pieces in `src/`: among them the entry
shim, the touch controls, the disc and save importers and a small DSP support shim. The runtime is the
RecompCore commit pinned in [scripts/builder/profiles/bluewake.sh](../../scripts/builder/profiles/bluewake.sh)
(`RECOMPCORE_SHA`), which the builder fetches; to build the app, see
[Build your own BlueWake](../../docs/BUILD_YOUR_OWN.md) and [the builder](../../docs/BUILDER.md). Audio uses Dolphin's high-level Zelda DSP by default
(BLUEWAKE_DSP_MODE=lle selects the interpreter). The console's SRAM is emulated on EXI channel 0
(runtime/host/src/ipl_sram.c, Dolphin's defaults: English, stereo) and kept in
Documents/BlueWake/sram.bin, so the game plays in stereo and its Stereo/Mono option persists;
without it the game read an empty SRAM and chose mono. BLUEWAKE_SRAM=0 turns it off (the macOS
host leaves it off unless BLUEWAKE_SRAM is set, so the certified route is unchanged). The console
clock starts at the local time (BLUEWAKE_CLOCK=now), so saves carry the real date and time. EFB depth
peeks (GXPeekZ) read Aurora's depth snapshot, so the sun's glare and lens flare draw as in Dolphin
(BLUEWAKE_EFB_PEEK=0 turns it off).

## Simulator build

Dawn publishes an `ios-arm64` package but no simulator package, so retag it once:

```bash
mkdir -p build/ios-deps && cd build/ios-deps
curl -LO https://github.com/encounter/dawn/releases/download/v20260618.032059/dawn-ios-arm64.tar.gz
mkdir dawn-ios && tar xzf dawn-ios-arm64.tar.gz -C dawn-ios && cp -R dawn-ios dawn-iossim
python3 ../../scripts/ios/retag_macho_platform.py --platform iossim \
    dawn-iossim/lib/libwebgpu_dawn.a dawn-iossim/lib/libwebgpu_dawn.a
ranlib dawn-iossim/lib/libwebgpu_dawn.a
cd ../..
```

Configure and build:

```bash
cmake -S apple/ios -B build/ios-sim -G Ninja \
    -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DPNG_SHARED=OFF \
    -DAURORA_DAWN_PROVIDER=system -DDawn_DIR=$PWD/build/ios-deps/dawn-iossim/lib/cmake/Dawn \
    -DAURORA_SDL3_PROVIDER=vendor -DAURORA_SDL3_LINKAGE=static -DAURORA_DAWN_LINKAGE=static \
    '-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local' -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON
cmake --build build/ios-sim --target BlueWake
```

## Run in one simulator

The macOS prerequisites (`generated/full`, the composite, the disc under `ref/`) must exist, as for
`scripts/play.sh`. Then:

```bash
scripts/ios/sim_run.sh --screenshot-after 45     # boot to the title
scripts/ios/sim_run.sh --route-full              # unattended to controllable Outset
scripts/ios/sim_run.sh --env DOL_FRAME_PACING_LOG=1 --device "iPad mini (A17 Pro)"
```

The script retags and signs the composite for the simulator, shuts down any other booted simulator,
installs, launches, and writes `stdout.log`, `stderr.log` and screenshots under
`local-research/ipad/<stamp>/`.

It refuses to start while Dolphin or the macOS host is running (`scripts/one_game_guard.sh`): one
game on the Mac at a time. Three modes use the app's own container instead of the repository:

```bash
scripts/ios/sim_run.sh --fresh                 # a new player's iPad: the first-run screen
scripts/ios/sim_run.sh --fresh --import-disc   # the same, importing the repo's disc unattended
scripts/ios/sim_run.sh --container             # a returning player: whatever the container holds
```

## First run

When the container lacks the disc or the files made from it, a first-run screen lists what is
missing, imports the disc through the document picker (or accepts one copied into BlueWake's folder
in Files as `GZLE01.iso`), validates GZLE01 USA rev 0 and makes `main.dol` and `rels/` on the
device (`src/disc_import.c`). The translated composite cannot be made on an iPad; it comes from a
Mac build of the same disc.

## Profile-guided build

The composite's hot chunks and the host both take a PGO profile (see `scripts/pgo_composite_hot.py`
and `scripts/pgo_host.sh`). For the iOS host, reconfigure with the host profile:

```bash
P=$PWD/build/host-pgo-gen/prof/merged.profdata
cmake build/ios-sim "-DCMAKE_C_FLAGS=-fprofile-instr-use=$P -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date" \
    "-DCMAKE_CXX_FLAGS=-fprofile-instr-use=$P -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"
```


## Acceptance

```bash
scripts/ios/sim_save_acceptance.sh   # new game, save, the guest's quit, reload, audio capture
scripts/ios/sim_heavy_view.sh --no-build        # the heaviest Outset view, retraces a second
scripts/ios/sim_village_view.sh --no-build      # the village walk from the pier save
scripts/ios/sim_run.sh --no-build --fresh --import-disc   # a new player's first run
scripts/ref_walk_corpus.sh                      # the same inputs through Dolphin, then BlueWake
```

Runs about 13 minutes in one simulator and prints PASS or the failing clauses.


## Controls

The touch shell is SunPad's (`src/BWGameOverlay.mm`; transfer record in
`docs/archive/SUNPAD_TRANSFER.md`): main stick, C-stick, grouped D-pad, A/B/X/Y/Z, Start, L and R over the
game view, and a three-dot menu at the top right with touch settings (opacity, size, hide with a
controller, move controls, reset), a layout editor, aspect ratio, game data and saves, and Report a
Problem. Opening the menu, a panel, the editor or an alert pauses the game until it closes. Touch
state goes into Aurora's virtual pad (`src/touch_controls.cpp`), merged with the keyboard and any
controller; hardware controllers hide the controls. `BLUEWAKE_TOUCH_CONTROLS=1` or `0` forces them
on or off. On an iPhone with the original 4:3 picture, the default layout puts the controls in the
black bars on each side of the picture (movement, D-pad, L and Start on the left; Z, R, the face
buttons and the camera stick on the right) so they do not cover the minimap or the item HUD; Fill
Screen, bars narrower than 84 points and any saved position fall back as before.
`BLUEWAKE_SHELL_DEMO=settings|layout` opens a panel in an unattended run, and
`BLUEWAKE_TOUCH_TAPS="8@A@0.15;45@move:0,1@4"` presses controls at those seconds after launch
(through the same UIKit handlers a finger uses) for unattended touch routes.

A hardware keyboard (on the simulator, the Mac's keyboard) plays with Aurora's bindings: W/A/S/D
the stick, F/H/T/G the C-stick, J/K/U/I A/B/X/Y, Q Z, E/R L/R, Return Start, the arrow keys the
D-pad. It combines with a bound controller and with the touch controls, the stronger input winning
on each axis (recompcore 0083). `BLUEWAKE_KEY_TAPS="36@w@3;41@j@1"` presses keys at those seconds
after launch through SDL's keyboard entry (SDL_AddKeyboard, SDL_SendKeyboardKey, as its GCKeyboard
handler does), and `BLUEWAKE_INPUT_PROBE=1` logs what the pad hands the guest.

## Device build

One command builds the device app from a fresh checkout and the user's disc image: it fetches the
pinned RecompCore and DolRecomp forks and the Dawn iOS package, extracts and translates the game,
generates and compiles the composite for arm64 iOS (A13 and newer, iOS 17), and builds, embeds and
signs the app:

~~~bash
scripts/ios/build_device.sh "/path/to/The Legend Of Zelda The Wind Waker.iso"            # ad hoc
scripts/ios/build_device.sh DISC.iso --identity "Apple Development: ..." \
    --profile dev.bluewake.BlueWake.mobileprovision --install <device id>             # on the iPad
~~~

Prerequisites, the signing and install steps, optional PGO profiles and what was verified are in
[docs/DEVICE_BUILD.md](../../docs/DEVICE_BUILD.md). On the device the first-run screen
asks only for the disc (the composite is embedded in the app's `Frameworks`); saves and the files
made from the disc live in the app's `Documents/BlueWake`, which Finder and the Files app can see.
What to check on the iPad is in [docs/archive/status/IPAD_STATE_2026-09-24.md](../../docs/archive/status/IPAD_STATE_2026-09-24.md).
