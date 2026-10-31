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
| Commit built | |
| Build time, jobs, clang | |
| Lean copies, behaviour timing and gather alias cache lines, RecompCore | |
| Module SHA-256 | |
| Speed against 0.9.0 (headless tour, per stop, both builds) | |
| a. Start and look | |
| b. The HD look's fixes for #491 | |
| c. The night's stars, with Smooth Motion | |
| d. Dragon Roost and saving | |
| e. A closed game keeps its settings | |
| f. The menu with a controller | |
| Zip SHA-256, uploaded to the v0.10.0 draft | |
| AppImage: build time, audit line, headless start, SHA-256, uploaded | |
| AppImage: build time, audit line, headless start, SHA-256, uploaded | |
