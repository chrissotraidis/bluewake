# Build your own BlueWake

BlueWake is distributed as source. You build it on your Mac from your own copy of *The Wind Waker*, and the
result is an app for your own iPhone or iPad. The game's code is translated from your disc during the
build, so **the app you build is yours alone: never share or upload it.**

## Mac personal app (migration candidate)

The reconciliation branch also provides an Apple Silicon Mac target:

```sh
scripts/builder/build.sh /path/to/your/GZLE01.iso --platform macos --out build/macos
```

It uses the same supported USA revision-0 disc checks, translation, mods and
local training as the device builder. The result is
`build/macos/packaged/BlueWake.app`. It contains your disc, extracted game files
and translated module. Keep the entire app personal; never share or upload it.
The app can be moved out of the checkout. Its saves and preferences stay under
`~/Library/Application Support/BlueWake`, so replacing the app does not replace
player data. Experimental desktop save states use that folder's `States`
subdirectory; existing explicit path overrides still work.

Use `--source-only` to verify extraction and translation first, then rerun the
same command without that flag. `--no-train` skips personal optimization training
and must not be treated as equivalent performance. The builder retains any
previous packaged app beside the new one. It signs locally with an ad-hoc
signature; `--identity NAME` selects an installed Mac signing identity. Device
options `--ipa`, `--app`, `--profile` and `--install` do not apply to Mac.

`--app-only --platform macos` builds the shell without game files. It is for
local packaging checks, not a playable build or an accepted public release.
PadMint's complete Mac path is not implemented or claimed here.

**Acceptance is in progress.** Fresh shell compilation and extraction/translation
pass; generated source matches the verified digest. A relocated launch using an
existing test module passes its bounded diagnostic route. Full clean module
compilation/training, player-app game save/reload, upgrades and sustained performance must pass before this route is
recommended for migration. See the
[reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).

## What you need

- A Mac with Apple silicon that runs the current Xcode, and at least 25 GB of free disk space
- Xcode from the App Store. Open it once, and under **Settings › Components** install the iOS platform.
- [Homebrew](https://brew.sh), then in Terminal:
  ```sh
  brew install cmake ninja
  ```
- Your disc image of *The Legend of Zelda: The Wind Waker*, GameCube USA (`GZLE01`, revision 0), as an
  `.iso`. The build checks it and refuses other versions.
- An iPhone or iPad with an A13 chip or newer, on iOS/iPadOS 17 or later

## 1. Build

```sh
git clone https://github.com/chrissotraidis/bluewake.git
cd bluewake
scripts/builder/build.sh "/path/to/The Legend Of Zelda The Wind Waker.iso" --source-only
```

`--source-only` checks your tools and disc and generates the base source in a few minutes. It stops
before mods, training, compilation and packaging; it produces no app. If it passes, build the app:

```sh
scripts/builder/build.sh "/path/to/The Legend Of Zelda The Wind Waker.iso" --ipa build/BlueWake.ipa
```

This includes [local optimization](#local-optimization). Matched physical-device frame-rate
acceptance of the current player-built candidate remains pending; training alone does not prove 30 FPS.
Expect well over an hour on a fast Mac and longer on smaller ones; the Mac stays busy. The terminal
shows each stage and its elapsed time. Rerunning the same command reuses compiled work and completed
matching profiles. Interrupted training playback starts again from an isolated card; it does not
resume mid-sequence. Keep the same output directory and build options to reuse completed work.

Generated game code and build outputs stay in `build/`; downloaded runtime and translator sources
live in `ref/`. Nothing is uploaded.
Keep the IPA there or anywhere else that is not synced: iCloud Drive (including a synced Desktop or
Documents folder), Dropbox and similar would upload it.

## Local optimization

The developer build reaches about 30 FPS in the measured iPad scenes because it is compiled with an
optimization profile: counts of which game code runs most. That profile comes from the game, so it
cannot be published. The builder makes yours instead. It builds a test version for your Mac, runs
a training sequence from your disc without a window or sound, records the counts, then compiles the
device version with them. You do not need to play or supply a save. The playback took 18 minutes
24 seconds on the development M3 Max. Logs are under `build/device/logs` and `build/device/pgo-local/logs`.

On the Mac, an app built this way ran the test route as fast as the developer build; iPad frame-rate
tests of it are still to come. To skip training, add `--no-train`: the build is faster, but the game
measured about 27.5 FPS instead of 30 at the Outset Island pier on an iPad Pro M2.

Training starts with a new private card by default; that is the route validated so far. You can add
`--training-save "/path/to/GZLE01.card"`
to train with a copy of your own BlueWake memory card; your original is not modified. This requires
a BlueWake `.card` container, not a Dolphin `.gci` or `.raw` file directly. Use quoted absolute paths.
The optional saved-card route has not been validated in this pass. Generated profiles and training saves remain private under the build directory. Matching completed profiles
are reused on later runs; changes to inputs invalidate them.

## 2. Install

The IPA is unsigned; it needs signing before installation. Common signing tools are listed below;
BlueWake-specific end-to-end installation has not been verified with every tool.

| Tool | Notes |
| --- | --- |
| [Sideloadly](https://sideloadly.io/) | Mac app: connect the device, drag the IPA in, sign in with your Apple ID |
| [AltStore](https://altstore.io/) | AltServer on the Mac installs AltStore on the device; add the IPA from AltStore's **My Apps** |
| [SideStore](https://sidestore.io/) | Like AltStore, but refreshes on the device without the Mac |
| Xcode | Sign and install from the Builder directly: `--identity`, `--profile` and `--install` ([DEVICE_BUILD.md](status/DEVICE_BUILD.md)) |

With a free Apple ID, apps expire after seven days; refresh them with the same tool (your saves stay).
The device needs **Developer Mode** on (Settings › Privacy & Security), and the first launch may ask you to
trust your Apple ID under Settings › General › VPN & Device Management.

## 3. Copy the disc to the device

The app reads the game's graphics, sound and world data from your disc. Copy the same `.iso` to the
device: in Finder, select the device, open **Files** and drag it onto **BlueWake**, or save it in
local **On My iPad** or **On My iPhone** storage in Files. Open BlueWake and pick it; the app checks
it and prepares the game. Keep the imported disc on the device: the game reads it while you play.
**⋯ › Game Data & Saves › Remove Disc Image…** removes the imported disc and extracted game files,
keeps your saves and settings, and requires importing the disc again before playing.

## Updating

```sh
git pull
scripts/builder/build.sh "/path/to/disc.iso" --ipa build/BlueWake.ipa
```

Compatible completed work is reused. Sign with the same Apple ID/team and app identifier, then install
the new IPA **over** the existing app. If your installer requires removing BlueWake, stop and resolve
the signing mismatch first. Never delete BlueWake to update it: deleting it deletes its saves.
Back up first with **⋯ › Game Data & Saves › Back Up Saves…**.

Use the same options as your first build (for example `--no-train` or `--training-save`) so
completed work is reused.

## Bringing your Dolphin saves

Export the save from Dolphin (**Tools › Memory Card Manager**, or the `.gci` file in its GC folder), copy
it to the device, then open **⋯ › Game Data & Saves › Import Dolphin Save…**. Choose the quest log and the
BlueWake slot for it; your current saves are backed up first. USA saves only.

## Mods

The build includes the Widescreen (16:9 and 16:10) and Better Wind Waker mods (`--no-mods` leaves them
out). Better Wind Waker's settings are built in: turn it on in **⋯ › Mods** and choose its settings in
**⋯ › Mods › Better Wind Waker Settings**. No patched disc is needed. See [MODS.md](MODS.md).

## If something fails

Each step writes a log under `build/device/logs`, and the error names it. Rerunning the same command
reuses compatible completed work. For help, ask on [Discord](https://discord.gg/xwHfUD2bxW) or
[open an issue](https://github.com/chrissotraidis/bluewake/issues), with the failing stage and relevant error excerpt.
Review logs for personal paths, signing details and device identifiers before posting. Never attach the IPA, disc, patched disc, game files, saves,
optimization profiles or signing/provisioning files, and do not upload the whole build directory.
