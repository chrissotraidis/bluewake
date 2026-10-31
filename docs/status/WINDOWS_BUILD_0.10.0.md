# Windows build for BlueWake 0.10.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.9.0
([WINDOWS_BUILD_0.9.0.md](WINDOWS_BUILD_0.9.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.10.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (about 1.5 to 3 hours on the Ryzen 7 5700U, then
about as long again for the Linux AppImage in WSL), and play and look for about 20 minutes when the agent asks (step 4).
Nothing else.

**What happens after:** the agent uploads the Windows zip and the Linux AppImage to the existing **draft** v0.10.0
release and opens a pull request with the results. Codex on the Mac checks both with the release gate, adds the other assets, and Chris
publishes.

## Rules for the agent

- Read [AGENTS.md](../../AGENTS.md) first. Do only what this file says: no code edits, no refactors. If anything in
  "Stop and ask Chris" happens, stop, show the error with your best read of the cause, and wait.
- Use a checkout on a **local drive** (for example `C:\src\bluewake`), never a network path.
- Start the build and every game run **from a PowerShell window on the desktop**, never over SSH or from a background
  shell: from a background shell Windows compiles on the slow cores and game windows get no real display.
- Never change Chris's real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode. Logs, the game
  module, the disc, saves and save states stay on this PC. Quote summary lines from logs; never attach them.
- Ask Chris only for what needs a person: watching, pressing buttons, or the disc path. One short instruction at a
  time, saying exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, then `git log origin/main --merges --grep "Freeze 0.10.0" -1 --format=%H` gives the freeze commit; `git checkout --detach COMMIT`, then `git status --short` | One commit is found, the tree is clean, and `version.json` says 0.10.0, build 9 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | Python 3.10+, clang 22 or newer (Visual Studio 2026 18.10 or newer), CMake and Ninja on PATH, and `gh` logged in with access to chrissotraidis/bluewake |
| The disc | `C:\BlueWake-private\GZLE01.iso`, `%APPDATA%\BlueWake\GZLE01.iso`, or the `game\GZLE01.iso` of the 0.9.0 build folder | One exists. Otherwise ask Chris for the path. The builder checks it is GZLE01 revision 0. |
| The 0.9.0 build | `build\windows-0.9.0\BlueWake\BlueWake.exe` in this checkout, or the unpacked 0.9.0 release zip | One exists; it is the speed check's baseline (step 3). Without one, skip step 3 and say so |
| Disk space | `Get-PSDrive C` | At least 70 GB free (40 for Windows, 30 for the AppImage in WSL). |
| WSL and Docker, for the AppImage | In PowerShell `wsl -l -v`; then in the Ubuntu shell `docker info` (without `sudo`) and `python3 --version` | An Ubuntu distribution on WSL 2, and `docker info` works without `sudo` (Docker Desktop's WSL integration, or Docker inside Ubuntu). If not, ask Chris: installing Docker is his to do. Without them, do steps 1 to 5 and say the AppImage was skipped |
| The draft release | `gh release view v0.10.0 --json isDraft,assets` | It is a draft and holds `BlueWake-v0.10.0-ios-unsigned.ipa`. Don't create another one. |

## The loop

**1. Build.** Close other programs, then from a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.10.0 --jobs 6
```

`--jobs 6` matters: without it the builder allows one compile job per 2.5 GB of free memory. This build trains its
own optimization profile, which the new speed work needs (#480: with an older profile it reads as a loss). When it
ends, note:

- the build time and the clang version it printed;
- the lines for `lean prepaid block copies`, behaviour timing and the gather alias cache (the build must not say any
  was turned off);
- that it fetched RecompCore `78c266d`;
- the SHA-256 of `build\windows-0.10.0\BlueWake\gGZLE01_recomp.dll` (`Get-FileHash`).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.10.0\BlueWake\BlueWake.exe`. Saves, settings and
logs then go to `build\windows-0.10.0\BlueWake\user\`. Start every run from PowerShell in that folder with
`.\BlueWake.exe`. After each run, summarize the newest file in `user\logs` with
`python scripts\triage_session_log.py LOG`.

Make a test save from a **copy** of a card (`%APPDATA%\BlueWake\GZLE01.card`, then the 0.9.0 build's
`user\GZLE01.card`, then ask Chris), and place quest log 1 at Forest Haven:

```
python scripts\card_set_restart.py CARD build\windows-0.10.0\BlueWake\user\GZLE01.card sea 41 0
```

Use `sea 44 0` for Outset and `M_NewD2 0 0` for Dragon Roost Cavern when a check needs them. With no card at all,
start a new file: checks a, b, c and f then happen on Outset, and d and e are skipped.

**3. Speed against 0.9.0, on its own.** No one needs to watch. From a copy of the same card, outside the checkout
(the state holds guest memory):

```
python scripts\bench_tour.py CARD build\windows-0.10.0\BlueWake\BlueWake.exe --mode headless --setup-state C:\BlueWake-private\tour-0.10.0.bwstate
```

The first run makes the state. Then run the tour four more times from that same state, alternating the builds:
0.9.0, 0.10.0, 0.10.0, 0.9.0 (the 0.9.0 build's `BlueWake.exe` in place of the new one). Keep each run's
`summary.json` and compare a 0.9.0 run with a 0.10.0 run with `python scripts\bench_tour.py compare A.json B.json`.
Passes when every run loaded all five stops and the checkpoints match between the builds. Note each stop's game
frames a second for both builds; a slower 0.10.0 at any stop is a stop-and-ask.

**4. Checks with Chris.** Quit each run with F1 › Quit the game. If a run starts with a recovery message, the run
before it was interrupted: note it and go on. Plug a controller in **before** starting if one is at hand.

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Loads quest log 1 (Forest Haven), walks and turns the camera for two minutes. | The log has `[renderer] D3D12` and `[gx] draw fusion on`, no fatal lines, and the triage summary shows the game near full speed. Chris saw nothing missing, flickering or wrongly coloured. |
| b | The HD look's fixes for #491 | At Outset (`sea 44 0`): F2 turns the HD look on. F1 › Display › Wind Waker HD look settings: open it and turn on Sun shadows. Stands on the pier looking out to sea, then cuts a few small trees and bushes. | No dark outline around Link and no dark band on the sea; the shadows on the ground hold steady through each hit; the clouds keep their grey shading. Each switch in the pane turns its part off and on. F2 takes everything off again, sun shadows too. |
| c | The night's stars, with Smooth Motion | F1 › Display: Smooth Motion at 60 or the display's rate. Then a run started with `$env:BLUEWAKE_STAGE_SELECT="1"; $env:BLUEWAKE_WARP="sea:11:0:1"; $env:BLUEWAKE_WARP_FILE="1"; $env:BLUEWAKE_WARP_TIME="night"; .\BlueWake.exe`: looks up at the sky on Windfall's pier and turns the camera. | Stars show in the night sky and stay put while the camera turns (0.9.0 drew none). No crash. Remove the four variables afterwards (`Remove-Item Env:BLUEWAKE_*`). |
| d | Dragon Roost and saving | With the card at `M_NewD2 0 0`: plays the cavern for two minutes, opens the map (D-pad right), presses F5, then F8. Then at `sea 44 0`: saves (Start › Save), quits, starts again and loads. | The map shows its grid and rooms; "State saved" and "State loaded" show; the save loads; the second launch has no recovery message. |
| e | A closed game keeps its settings (#511's fix, which Windows already had) | Changes one setting (F1 › Display › Show FPS on), plays a minute, then ends BlueWake from Task Manager (End task) and starts it again. | No recovery message on the second launch, and Show FPS is still on. Turn it back off. |
| f | The menu with a controller | Presses Back: the menu opens. Moves through Controls with the D-pad, changes tabs with LB and RB, and presses Down past the last setting of Controls. F1 › Display shows "Smooth Motion under sustained load" with Automatic and Never pause. | Everything is reachable without the mouse; Down at the end of a tab scrolls to the notes under it; B closes the menu and the press doesn't reach Link. With a Switch controller: "Nintendo button layout" swaps A/B and X/Y when on. |

If Chris has had enough, a, b and d are the ones that matter for the release, then c; say which were skipped.

**5. Package.** Delete `user\GZLE01.card` and `portable.txt` from the build folder (and the rest of `user\`), then:

```
python scripts\windows\package_release.py 0.10.0 --app build\windows-0.10.0\BlueWake --out build\windows-0.10.0\release
```

Note the SHA-256 of `build\windows-0.10.0\release\BlueWake-v0.10.0-windows-x64.zip`. The source zip and `SHA256SUMS`
it also writes stay on this PC; the Mac makes the release's own.

**6. The Linux AppImage, in WSL.** From the same freeze commit and the same disc, after the Windows zip is packaged.
In the Ubuntu shell, keep everything on WSL's own disk (not `/mnt/c`, which is many times slower). Use the disc
path from "Before you start" and the commit from the first check:

```
mkdir -p ~/bw-private && cp /mnt/c/BlueWake-private/GZLE01.iso ~/bw-private/
git clone https://github.com/chrissotraidis/bluewake ~/bluewake-0.10.0 && cd ~/bluewake-0.10.0
git checkout --detach COMMIT && git status --short
python3 scripts/linux/build.py ~/bw-private/GZLE01.iso --out build/linux-0.10.0 --ci-container --appimage --jobs 6
```

Don't use `sudo`; if the container build fails, see
[If the container build fails](../LINUX.md#if-the-container-build-fails). It trains its own profile, like the Windows
build. Passes when it fetched RecompCore `78c266d` and ends with the AppImage audit's line (`appimage audit: N ELF
files; maximum GLIBC_2.3x; no private game files`, with GLIBC no higher than 2.39). Then start the app folder
headless from an empty data folder; it reaches the title screen and quits by itself:

```
mkdir -p ~/bw-private/scratch
BLUEWAKE_DATA_DIR=~/bw-private/scratch BLUEWAKE_RENDERER=headless BLUEWAKE_MAX_RETRACES=1500 build/linux-0.10.0/BlueWake/bluewake
ls ~/bw-private/scratch/logs
```

Passes when the newest session log there has a `[boot-milestone] title-ready` line and no fatal lines. Copy the
AppImage out under its release name and note its SHA-256:

```
cp build/linux-0.10.0/BlueWake-x86_64.AppImage /mnt/c/BlueWake-private/BlueWake-v0.10.0-linux-x86_64.AppImage
sha256sum /mnt/c/BlueWake-private/BlueWake-v0.10.0-linux-x86_64.AppImage
```

The `.zsync` file, the disc copy, the app folder and `scratch` stay in WSL.

**7. Hand-off.**

1. Upload only the Windows zip and the AppImage to the existing draft:
   ```
   gh release upload v0.10.0 build\windows-0.10.0\release\BlueWake-v0.10.0-windows-x64.zip --clobber
   gh release upload v0.10.0 C:\BlueWake-private\BlueWake-v0.10.0-linux-x86_64.AppImage --clobber
   gh release view v0.10.0 --json isDraft,assets
   ```
   It must still be a draft, with the IPA, the zip and the AppImage.
2. On a branch `codex/windows-0.10.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.10.0 build results", with no logs attached. Mark the WINDOWS_TASKS.md rows each check covered.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, or prints `building the app without its optimization profile` (it needs Visual Studio 2026 18.10
  or newer, #153).
- The speed check shows 0.10.0 slower than 0.9.0 at a stop, a stop that doesn't load, or checkpoints that differ.
- Check a or b shows missing or broken drawing, or b shows the outline or the flicker.
- The game crashes, or a save doesn't load.
- The AppImage build fails, its audit fails or reports GLIBC above 2.39, or the headless start doesn't reach the title.
- The AppImage build fails, its audit fails or reports GLIBC above 2.39, or the headless start doesn't reach the title.

## Results

| | |
| --- | --- |
| Commit built | `edd88f15750211cca7a4596c9c5aa348f124efe3`, clean freeze, version 0.10.0/build 9. Windows 11 x64, AMD Ryzen 7 5700U with Radeon Graphics. |
| Build time, jobs, clang | 95 min 59.5 s, 6 jobs, clang 22.1.3 (Visual Studio Build Tools 2026 18.10.2). Started from visible desktop PowerShell. Fresh local training accepted: 425 translated functions executed; final module used the resulting profile. The app used its committed profile and ThinLTO. |
| Lean copies, behaviour timing and gather alias cache lines, RecompCore | `lean prepaid block copies: 464979 blocks in 842 chunks`; `behaviour_timing: 404438 block starts in 842 chunks now always prepaid`; final CMake cache: `BLUEWAKE_GATHER_ALIAS_CACHE:BOOL=ON`, `BLUEWAKE_GATHER_PIPE:BOOL=ON`. Provenance also confirms both enabled. Fetched RecompCore `78c266d20059560fa5efe68be16062c36623af6a`. |
| Module SHA-256 | `2918a64b40d10ed2e4699a4ed2de105c114050e579fb2fcc621e7e8fbbe1aaae` |
| Speed against 0.9.0 (headless tour, per stop, both builds) | Unverified; skipped at Chris's direction after setup failed. The preserved test card started a new game: last milestone `new-game-intro`, `play_scene=0`; the setup guard rejected the state and exited 1. No five-stop measurement or cross-build checkpoint comparison completed. Chris then directed proceeding without a save file. No speed improvement is claimed. |
| a. Start and look | Partial: a 1.8-minute portable D3D12 Outset smoke run from this build's private new-file training state, with scripted movement, `[gx] draw fusion on`, zero fatal lines, and F1 > Quit exit 0. Triage: 108 watched seconds, 12 below target, median below-target game speed 85%, lowest 68%, 7 hitches, worst frame 134 ms, 550 pipelines made. The window and settings menu were inspected. Forest Haven, two minutes of human walking/camera input, sustained near-full-speed acceptance, audio and flicker/colour correctness remain unverified. |
| b. The HD look's fixes for #491 | Not performed; skipped at Chris's direction. Outline, sea band, cloud shading, shadow stability and each effect switch remain unverified on this PC. |
| c. The night's stars, with Smooth Motion | Not performed; skipped at Chris's direction. Night-sky stars and camera stability with Smooth Motion remain unverified on this PC. |
| d. Dragon Roost and saving | Skipped at Chris's direction when proceeding without a saved game. Dungeon map, F5/F8, in-game save, reload and clean second launch remain unverified. |
| e. A closed game keeps its settings | Not performed; skipped at Chris's direction. Forced-termination recovery and settings retention remain unverified. |
| f. The menu with a controller | Not performed; skipped at Chris's direction. Controller navigation, scrolling, button layout and sustained-load choices remain unverified. F1 opened the menu and its Quit button exited cleanly with the mouse. |
| Zip SHA-256, uploaded to the v0.10.0 draft | `c56325884ef6f3f4cc502dd6c129e16a6d66316082a546bb3a99476dcd668f66`; 35 files, 218 MB. Uploaded to the existing v0.10.0 draft; GitHub digest matches the local SHA-256. |
| AppImage: build time, audit line, headless start, SHA-256, uploaded | 2 h 42 min 20 s including container preparation, 6 jobs, Ubuntu 24.04 CI container with clang 18.1.3, fresh local profile (425 translated functions executed). `appimage audit: 14 ELF files; maximum GLIBC_2.38; no private game files`. Direct WSL Ubuntu headless start from an empty private data folder: `title-ready` at retrace 328, exits 0 at the 1500-retrace cap, zero fatal lines. SHA-256 `c4337ce4d9755368470fa47e46b78c23646c80fa3b52fbfc9ced968783ffc2ba`. Uploaded to the existing v0.10.0 draft; GitHub digest matches the local SHA-256. |

### Scope and private data

Chris directed proceeding without a save file. Manual acceptance was not completed; the skipped checks are not passes.
The first attempted rendered smoke launch used an incorrect renderer identifier and actually ran headless; it was stopped
and is excluded from rendered evidence. The corrected run showed a recovery notice from that interruption and then exited
cleanly through the menu. Its log confirms Direct3D 12. The build's new-file training state was used only for this smoke
check, not as a substitute for the required five-stop benchmark.

The Windows package passed the packager's allowlist and DLL checks, plus an independent ZIP integrity, unique/safe path,
private-file exclusion, required-binary and freeze-provenance check. Portable test data and logs were moved outside the app
folder before packaging and remain private on this PC. Real AppData saves were not modified. The source ZIP and SHA256SUMS
remain local. The Mac must hash-check both uploaded artifacts and run `scripts/release/check_public_assets.sh` before any
publication; only `containsTranslatedGameCode: true` is accepted for these Windows/Linux artifacts. Publication remains
Chris's decision.

### Linux environment and verification

The Linux checkout was clean at the same freeze commit, with RecompCore `78c266d20059560fa5efe68be16062c36623af6a`.
The build confirmed the same lean-copy and behaviour-timing counts as Windows and `BLUEWAKE_GATHER_ALIAS_CACHE:BOOL=ON`.
Provenance records local training, no source modifications and module SHA-256
`8d33248b24f66b17862810e60904455090f6dcf9339fbacf54a17040328eed42`.

Docker was already running; adding Chris's Ubuntu user to the existing docker group made `docker info` work without sudo.
The first direct app-folder starts stopped before game code because the WSL host lacked `libc++.so.1`, then
`libunwind.so.1`. After installing the host runtime packages `libc++1`, `libc++abi1` and `libunwind-18` (no upgrades or
removals), the direct headless title check above passed. The same binary also passed a separate empty-data headless title
check inside the build container. These are startup checks, not Linux rendering, audio, gameplay or Steam Deck acceptance.
The disc copies, app folder, training data, scratch data, logs and zsync file remain private in WSL.

### Hand-off verification

Both authorized assets are on the existing v0.10.0 draft, with matching GitHub digests; the IPA and the other existing
assets remain present. Only the Windows ZIP and Linux AppImage were uploaded in this run. Repository audit (WSL) and
`git diff --check` passed. No source-code changes were made. Windows CI on the results PR remains the pre-merge check;
this hand-off does not merge the PR or publish the release.

## Afterwards: repackage the AppImage with its licenses

The Mac's gate found that the AppImage carried only MbedTLS's license (#520 fixes `make_appimage.sh`). Chris only has
to say:

> Pull the latest chrissotraidis/bluewake and follow "Afterwards: repackage the AppImage with its licenses" in
> docs/status/WINDOWS_BUILD_0.10.0.md.

This repackages the app folder already built in WSL: no rebuild, so the game module and every binary stay exactly as
built from `edd88f1`; only the packaging script is newer. In the Ubuntu shell:

```
cd ~/bluewake-0.10.0
git fetch origin && git checkout --detach origin/main && git status --short
git log -1 --format='%h %s' -- scripts/linux/make_appimage.sh
docker run --rm --init --platform linux/amd64 --user "$(id -u):$(id -g)" --env HOME=/tmp \
  --volume "$PWD:$PWD" --workdir "$PWD" bluewake-linux-ci:local bash -c \
  'scripts/linux/make_appimage.sh build/linux-0.10.0 build/linux-0.10.0/BlueWake-x86_64.AppImage && python3 scripts/linux/check_appimage_abi.py build/linux-0.10.0/BlueWake-x86_64.AppImage 2.39'
cp build/linux-0.10.0/BlueWake-x86_64.AppImage /mnt/c/BlueWake-private/BlueWake-v0.10.0-linux-x86_64.AppImage
sha256sum /mnt/c/BlueWake-private/BlueWake-v0.10.0-linux-x86_64.AppImage
```

Passes when the `git log` line is "Linux AppImage: carry the licenses of everything it bundles", the script ends
without an error (it stops if a bundled library has no license), and the audit prints `no private game files` with
GLIBC no higher than 2.39. Then from PowerShell:

```
gh release upload v0.10.0 C:\BlueWake-private\BlueWake-v0.10.0-linux-x86_64.AppImage --clobber
gh release view v0.10.0 --json isDraft,assets
```

It must still be a draft with all five assets. Tell Chris the new SHA-256, and stop: the Mac compares every file in
the new image with the old one, and only the license folder may differ.
