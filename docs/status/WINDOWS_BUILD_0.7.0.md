# Windows build for BlueWake 0.7.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop. Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.7.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (2 to 3 hours on the Ryzen 7 5700U), then
play and look for about 30 minutes when the agent asks (step 3). Nothing else.

**What happens after:** the agent uploads the Windows zip to the existing **draft** v0.7.0 release and opens a pull
request with the results. Codex on the Mac then adds the Linux AppImage and checks everything, and Chris publishes.

## Rules for the agent

- Read [AGENTS.md](../../AGENTS.md) first. Do only what this file says: no code edits, no refactors. If anything in
  "Stop and ask Chris" happens, stop, show the error with your best read of the cause, and wait.
- Use a checkout on a **local drive** (for example `C:\src\bluewake`). The 0.6.0 build failed from a network (UNC)
  path.
- Start the build and every game run **from a PowerShell window on the desktop**, never over SSH or from a background
  shell. From a background shell Windows compiles on the slow cores and game windows get no real display. The 0.6.0
  run's first shell-attached launch ended early.
- Never change Chris's real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode. Logs, the game
  module, the disc and saves stay on this PC. Quote summary lines from logs; never attach them.
- Ask Chris only for what needs a person: watching, listening, pressing buttons, or the disc path. One short
  instruction at a time, saying exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, `git checkout --detach c6094ec76c82700d5a56a28bf627c8179f686542`, `git status --short` | HEAD is `c6094ec`, the tree is clean, and `version.json` says 0.7.0 build 6. This is the "Candidate commit" in [RELEASE_0.7.0.md](RELEASE_0.7.0.md). |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | Python 3.10+, clang 22 or newer (0.6.0 used 22.1.3 from Visual Studio 2026), CMake and Ninja on PATH, and `gh` logged in with access to chrissotraidis/bluewake. |
| The disc | `C:\BlueWake-private\GZLE01.iso`, `%APPDATA%\BlueWake\GZLE01.iso`, or the `game\GZLE01.iso` of the 0.6.0 build folder | One exists. Otherwise ask Chris for the path. The builder checks it is GZLE01 revision 0. |
| Disk space | `Get-PSDrive C` | At least 40 GB free. |
| The draft release | `gh release view v0.7.0 --json isDraft,assets` | It is a draft and holds `BlueWake-v0.7.0-ios-unsigned.ipa`. Don't create another one. |

## The loop

**1. Build.** Close other programs, then from a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.7.0 --jobs 6
```

`--jobs 6` matters. Without it the builder allows one compile job per 2.5 GB of free memory, so 0.6.0 compiled with
one job and took 4 hours. If several large chunks run out of memory together, the builder retries them with fewer
jobs on its own. Expect 2 to 3 hours: 0.7.0's module is about a quarter bigger (lean block copies are now the
default), and training takes about 12 minutes instead of 30.

Check on it now and then. When it ends, note:
- the build time and the clang version it printed;
- the line `lean prepaid block copies: N blocks in M chunks` (it must say "lean");
- the SHA-256 of `build\windows-0.7.0\BlueWake\gGZLE01_recomp.dll` (`Get-FileHash`).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.7.0\BlueWake\BlueWake.exe`. Saves, settings and
logs then go to `build\windows-0.7.0\BlueWake\user\`. Start every run from PowerShell in that folder with
`.\BlueWake.exe`. After each run, summarize the newest file in `user\logs` with
`python scripts\triage_session_log.py LOG`.

Make a test save from a **copy** of a card. Look for one in this order: `%APPDATA%\BlueWake\GZLE01.card`, then
`build\windows\BlueWake\user\GZLE01.card` (the 0.6.0 portable test), then ask Chris. Then place quest log 1:

```
python scripts\card_set_restart.py CARD build\windows-0.7.0\BlueWake\user\GZLE01.card sea 41 0
```

`sea 41 0` is the sea at Forest Haven, one of the heaviest places. Run it again with `sea 44 0` for Outset and
`M_NewD2 0 0` for Dragon Roost Cavern when a check needs them. If slot 1 is empty, add the slot number Chris names as
a last argument. With no card at all, start a new file: checks a, b and e then happen on Outset, and c is skipped.

**3. Checks with Chris.** Quit each run with F1 › Quit the game. If a run starts with a recovery message, the run
before it was interrupted: note it and go on.

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Loads quest log 1 (Forest Haven), walks and turns the camera for two minutes. | The log has `[gx] draw fusion on`, `[renderer] D3D12` (or Vulkan), and at exit `[gx-core] draws fused onto the draw before:` with a large number. No fatal lines. Chris saw nothing missing, flickering or wrongly coloured: models, water, particles, the sky. |
| b | Fusion's speed | The same two minutes again with `$env:DOL_GX_FUSE="0"` set in that PowerShell window, then `Remove-Item Env:DOL_GX_FUSE`. | Recorded either way, for both runs: the median `game=` of the `[fps]` lines and the lowest `speed=` of the `[fps-dip]` lines. With fusion should be the same or faster. |
| c | Dragon Roost and Outset | With the card at `M_NewD2 0 0`: plays the cavern for two minutes, with a fight, and opens the map (D-pad right). Then at `sea 44 0`: walks Outset, saves (Start › Save), quits, starts again and loads. | Nothing missing on screen, the map shows its grid and rooms, the save loads, and the second launch has no recovery message. |
| d | Controllers, only if one is at hand | Plugs it in **before** starting; plays a minute. With two controllers, takes a hit and checks that only the one in use rumbles. Takes a Pictobox photo if the save has the Pictobox, and picks Yes with the left stick. | The stick is smooth with no dead zone jump; `[pad] player 1` is in the log; only player 1 rumbles; Yes can be chosen. Each matching row in [WINDOWS_TASKS.md](../WINDOWS_TASKS.md#in-main-waiting-for-a-windows-build). |
| e | Smooth Motion at 120 | F1 › Display: Smooth Motion on, 120 (or the display's rate). Plays Forest Haven for two minutes. | The log says `[interp-pace] 16 threads: a slow game keeps its in-between frames`, and the picture doesn't step down to 30 for seconds after a hitch. |
| f | Window, portable, FPS counter | Moves the window, quits, starts again. F1 › Display: Show FPS, then picks a corner. | The window opens where it was left; nothing new appears in `%APPDATA%\BlueWake`; the counter moves to the corner. |

If Chris has had enough, a and c are the ones that matter for the release; say which were skipped.

**4. Package.** Delete `user\GZLE01.card` and `portable.txt` from the build folder (`package_release.py` leaves saves and
logs out of the zip anyway; keep the logs until the Results table is filled in), then:

```
python scripts\windows\package_release.py 0.7.0 --app build\windows-0.7.0\BlueWake --out build\windows-0.7.0\release
```

Note the SHA-256 of `build\windows-0.7.0\release\BlueWake-v0.7.0-windows-x64.zip`. The source zip and `SHA256SUMS` it
also writes stay on this PC; the Mac makes the release's own.

**5. Hand-off.**

1. Upload only the Windows zip to the existing draft:
   ```
   gh release upload v0.7.0 build\windows-0.7.0\release\BlueWake-v0.7.0-windows-x64.zip --clobber
   gh release view v0.7.0 --json isDraft,assets
   ```
   It must still be a draft, with the IPA and the zip.
2. On a branch `codex/windows-0.7.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.7.0 build results", with no logs attached.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, or prints `building the app without its optimization profile` (it needs Visual Studio 2026 18.10
  or newer, #153).
- The build log says `prepaid block copies` without "lean".
- Check a shows missing or broken drawing that `DOL_GX_FUSE=0` fixes. Fusion then has to be turned off for Windows
  (one line in `runtime/host/src/main.c`), which needs a new commit.
- The game crashes, or a save doesn't load.

## Results

| | |
| --- | --- |
| Commit built | |
| Build time, jobs, clang | |
| Lean copies line | |
| Module SHA-256 | |
| a. Start and look (fusion on) | |
| b. Game rate with and without fusion | |
| c. Dragon Roost, map, Outset, save and reload | |
| d. Controllers | |
| e. Smooth Motion at 120 | |
| f. Window, portable, FPS counter | |
| Zip SHA-256, uploaded to the v0.7.0 draft | |

