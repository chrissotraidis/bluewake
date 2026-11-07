# Windows build for BlueWake 0.11.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.10.0
([WINDOWS_BUILD_0.10.0.md](WINDOWS_BUILD_0.10.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.11.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** before starting, copy the memory card from the Mac to `C:\BlueWake-private\GZLE01.card` (the Mac
keeps it as `~/BlueWake-0.10.0-check/cards/ipad-20261024.card`; copy it over a USB stick or AirDrop, never through GitHub).
Keep the PC plugged in and awake while it builds (about 1.5 to 3 hours, then about as long again for the Linux AppImage in
WSL), and play and look for about 20 minutes when the agent asks (step 4).

**What happens after:** the agent uploads the Windows zip and the Linux AppImage to the existing **draft** v0.11.0
release and opens a pull request with the results. Codex on the Mac checks both with the release gate, adds the other
assets, and Chris publishes.

## What is new on Windows and Linux in 0.11.0

- **Speed:** cheaper floating-point code (#553), graphics commands handed over in runs (#531), cheaper direct-call
  checks (#503), and **Faster calls inside the game's code** on by default (#552, #567): +12.3% on a Ryzen 9 9900X with
  Windows 11 in pdale-boop's tour. The Windows builder now trains with that gate open (#555).
- **The first run** shows its four stages in the progress window (#525). Nobody has seen that window on Windows yet.
- **F9** walks off → frame rate → a full performance panel (#537).
- **Save states** keep the stage select, the saving lock, HD sky and mod settings working (#543).
- **Efficient soft effects** in Settings › Display, off by default (#504).
- Linux only: Clang 22 in the container (#550); game code on large memory pages, opt-in (#554).

## Rules for the agent

- Read [AGENTS.md](../../AGENTS.md) first. Do only what this file says: no code edits, no refactors. If anything in
  "Stop and ask Chris" happens, stop, show the error with your best read of the cause, and wait.
- Use a checkout on a **local drive** (for example `C:\src\bluewake`), never a network path.
- Start the build and every game run **from a PowerShell window on the desktop**, never over SSH or from a background
  shell: from a background shell Windows compiles on the slow cores and game windows get no real display.
- Never change Chris's real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode with copies. Logs,
  the game module, the disc, saves and save states stay on this PC. Quote summary lines from logs; never attach them.
- Ask Chris only for what needs a person: watching, pressing buttons, or a path. One short instruction at a time, saying
  exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, then `git log origin/main --merges --grep "Freeze 0.11.0" -1 --format=%H` gives the freeze commit; `git checkout --detach COMMIT`, then `git status --short` | One commit is found, the tree is clean, and `version.json` says 0.11.0, build 10 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | Python 3.10+, clang 22 or newer (Visual Studio 2026 18.10 or newer), CMake and Ninja on PATH, and `gh` logged in with access to chrissotraidis/bluewake |
| The disc | `C:\BlueWake-private\GZLE01.iso`, `%APPDATA%\BlueWake\GZLE01.iso`, or the `game\GZLE01.iso` of the 0.10.0 build folder | One exists. Otherwise ask Chris for the path |
| The card | `C:\BlueWake-private\GZLE01.card` | It exists and `python scripts\card_set_restart.py --help` runs. Without it, ask Chris to copy it from the Mac (see above). Don't use a card that starts a new game: that is what blocked 0.10.0's speed check |
| The 0.10.0 build | `build\windows-0.10.0\BlueWake\BlueWake.exe` in this checkout, or the unpacked 0.10.0 release zip | One exists; it is the speed check's baseline (step 3) |
| Disk space | `Get-PSDrive C` | At least 70 GB free (40 for Windows, 30 for the AppImage in WSL) |
| WSL and Docker, for the AppImage | `wsl -l -v`; in the Ubuntu shell `docker info` (without `sudo`) and `python3 --version` | As for 0.10.0, which worked on this PC |
| The draft release | `gh release view v0.11.0 --json isDraft,assets` | It is a draft (the Mac adds the iPhone/iPad app and the source to it). Don't create another one |

## The loop

**1. Build.** Close other programs, then from a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.11.0 --jobs 6
```

No extra options: 0.11.0's defaults prepare the FP checks and train with the faster-calls gate open. It trains its own
optimization profile, which this speed work needs. When it ends, note:

- the build time and the clang version it printed;
- the lines for `lean prepaid block copies`, behaviour timing, `fp-checks` and the gather alias cache (none turned off);
- that it fetched RecompCore `f9e3f4e`;
- that `BuilderProvenance.json` has `"fp_checks": true` and `"direct_gate_training": true`;
- the SHA-256 of `build\windows-0.11.0\BlueWake\gGZLE01_recomp.dll` (`Get-FileHash`).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.11.0\BlueWake\BlueWake.exe`. Start every run from
PowerShell in that folder with `.\BlueWake.exe`. After each run, summarize the newest file in `user\logs` with
`python scripts\triage_session_log.py LOG`. Place quest log 1 of a **copy** of the card at Forest Haven:

```
python scripts\card_set_restart.py C:\BlueWake-private\GZLE01.card build\windows-0.11.0\BlueWake\user\GZLE01.card sea 41 0
```

Use `sea 44 0` for Outset and `M_NewD2 0 0` for Dragon Roost Cavern when a check needs them.

**3. Speed, on its own.** No one needs to watch. From a copy of the card, outside the checkout:

```
python scripts\bench_tour.py CARD build\windows-0.11.0\BlueWake\BlueWake.exe --mode headless --setup-state C:\BlueWake-private\tour-0.11.0.bwstate
```

The first run makes the state; if it reports that setup didn't reach gameplay, stop and ask Chris. Then, from that same
state, six more runs in this order: 0.10.0, 0.11.0, 0.11.0 with the gate closed, 0.11.0 with the gate closed, 0.11.0,
0.10.0. "With the gate closed" means adding `--env BLUEWAKE_DIRECT_GATE=0` to the tour's command line (the tour clears any
`BLUEWAKE_` variable set in PowerShell, so that would not reach the game). Compare runs with
`python scripts\bench_tour.py compare A.json B.json`. Passes when every run loaded all five stops, the 0.11.0 runs' checkpoints
match each other with the gate open and closed, guest memory (cycles, MEM1, MEM2, aliases) matches 0.10.0's (the CPU hash
may differ from 0.10.0's: #553 drops a scratch field from it), and 0.11.0 is not slower than 0.10.0 at any stop. Note each
stop's game frames a second for all three. For reference, the Mac (M3 Max) measured 0.11.0 about 10% faster than 0.10.0
with the gate closed, and the gate a further +12.9%.

**4. Checks with Chris.** Quit each run with F1 › Quit the game. Plug a controller in **before** starting if one is at hand.

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Loads quest log 1 (Forest Haven), walks and turns the camera for two minutes. | The log has `[renderer] D3D12`, `[gx] draw fusion on` and `gate=on` on the `[chassis]` line, no fatal lines, and the game near full speed. F1 › Config shows "Faster calls inside the game's code" on. Chris saw nothing missing, flickering or wrongly coloured. |
| b | The first run (#525) | The agent unpacks the packaged zip from step 5 into an empty folder `C:\BlueWake-private\firstrun\` with an empty `portable.txt` beside `BlueWake.exe`, then Chris starts it and picks the disc when asked. | The progress window lists four stages, marks each as it finishes, shows the free space, and the game reaches the title. Delete the folder afterwards. |
| c | F9 (#537) | Presses F9 three times, a few seconds apart, at Outset. | Frame rate, then the full panel in the top right (frame times with a graph, CPU, memory, graphics card), then off. The game's CPU share stays at or under 100%. |
| d | Save states (#543) | Starts BlueWake and, before starting a file, presses F8 to load the state made in e's first run; then presses F7. Then at Outset presses F5, F8, and saves in the game (Start › Save), quits, starts again and loads. | F7 opens the stage select instead of "Start a file first". "State saved" and "State loaded" show; the save loads; the second launch has no recovery message. (First time through, press F5 once in play to make the state.) |
| e | Dragon Roost with soft effects (#504) | With the card at `M_NewD2 0 0`: plays the cavern for a minute, turns on F1 › Display › Efficient soft effects, plays another minute, turns it off. | The smoke and haze look the same or nearly so; nothing disappears or flickers. |

If Chris has had enough, a, b and d are the ones that matter for the release; say which were skipped.

**5. Package.** Delete `user\` and `portable.txt` from the build folder, then:

```
python scripts\windows\package_release.py 0.11.0 --app build\windows-0.11.0\BlueWake --out build\windows-0.11.0\release
```

Note the SHA-256 of `build\windows-0.11.0\release\BlueWake-v0.11.0-windows-x64.zip`. The source zip and `SHA256SUMS`
it also writes stay on this PC; the Mac makes the release's own. (Check b uses this zip, so package before b.)

**6. The Linux AppImage, in WSL.** From the same freeze commit and the same disc, after the Windows zip is packaged.
Keep everything on WSL's own disk:

```
mkdir -p ~/bw-private && cp /mnt/c/BlueWake-private/GZLE01.iso ~/bw-private/
git clone https://github.com/chrissotraidis/bluewake ~/bluewake-0.11.0 && cd ~/bluewake-0.11.0
git checkout --detach COMMIT && git status --short
python3 scripts/linux/build.py ~/bw-private/GZLE01.iso --out build/linux-0.11.0 --ci-container --appimage --jobs 6
```

The container image is rebuilt the first time, because 0.11.0 adds LLVM 22 to it (#550); that is expected. Don't use
`sudo`. Passes when it fetched RecompCore `f9e3f4e`, printed clang 22, and ends with the AppImage audit's line
(`appimage audit: N ELF files; maximum GLIBC_2.3x; no private game files`, with GLIBC no higher than 2.39). Then start the
app folder headless from an empty data folder; it reaches the title screen and quits by itself:

```
mkdir -p ~/bw-private/scratch-0.11
BLUEWAKE_DATA_DIR=~/bw-private/scratch-0.11 BLUEWAKE_DISC=~/bw-private/GZLE01.iso BLUEWAKE_RENDERER=headless BLUEWAKE_MAX_RETRACES=1500 build/linux-0.11.0/BlueWake/bluewake
ls ~/bw-private/scratch-0.11/logs
```

Passes when the newest session log there has a `[boot-milestone] title-ready` line, `gate=on` on the `[chassis]` line and
no fatal lines. (0.11.0's app folder no longer carries the game files, #525: `BLUEWAKE_DISC` names the disc, and the first run prepares them into the empty data folder, as a player's first run does.) Copy the
AppImage out under its release name and note its SHA-256:

```
cp build/linux-0.11.0/BlueWake-x86_64.AppImage /mnt/c/BlueWake-private/BlueWake-v0.11.0-linux-x86_64.AppImage
sha256sum /mnt/c/BlueWake-private/BlueWake-v0.11.0-linux-x86_64.AppImage
```

**7. Hand-off.**

1. Upload only the Windows zip and the AppImage to the existing draft:
   ```
   gh release upload v0.11.0 build\windows-0.11.0\release\BlueWake-v0.11.0-windows-x64.zip --clobber
   gh release upload v0.11.0 C:\BlueWake-private\BlueWake-v0.11.0-linux-x86_64.AppImage --clobber
   gh release view v0.11.0 --json isDraft,assets
   ```
   It must still be a draft, with the zip and the AppImage (and whatever the Mac has added).
2. On a branch `codex/windows-0.11.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.11.0 build results", with no logs attached. Mark the WINDOWS_TASKS.md rows each check covered.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, or prints `building the app without its optimization profile`.
- The speed check shows 0.11.0 slower than 0.10.0 at a stop, a stop that doesn't load, or checkpoints that differ.
- Check a or b shows missing or broken drawing, or b doesn't reach the title.
- The game crashes, or a save doesn't load.
- The AppImage build fails, its audit fails or reports GLIBC above 2.39, or the headless start doesn't reach the title.

## Results

November 7, 2026, Chris's Windows 11 Pro PC (Ryzen 7 5700U, Radeon integrated graphics, 12.9 GB RAM).
Both builds and every game launch ran from visible desktop PowerShell windows. The builds used the clean freeze
below and the same private disc. Testing used portable folders and copies of an existing populated test card;
real AppData saves were not changed. Logs, cards, states and disc files remain private.

**Acceptance remains incomplete:** the automated checks below passed, but Chris's Forest Haven play/visual check,
F9 at Outset, interactive save/reload and Dragon Roost soft-effects comparison have not been observed. No gameplay
acceptance is inferred from compilation, headless benchmarks or title rendering. The release remains a draft;
the Mac's content-gate check and Chris's publication decision are still required.

| | |
| --- | --- |
| Commit built | `0d84fa1969a1ac9a9e9333d85dd425816ea3ce23`; version 0.11.0, build 10; clean source |
| Build time, jobs, clang | Windows: 1h 19m 37s, six jobs, clang 22.1.3; exit 0; trained build and app optimization profile used |
| Lean copies, behaviour timing, fp-checks, gather alias cache, RecompCore, provenance | Lean copies: 464,979 blocks / 842 chunks; behaviour timing: 404,438 block starts / 842 chunks; FP checks: 296,188 tests removed / 811 chunks; gather alias cache on. RecompCore `f9e3f4e59be4d49f10130bf6e68bc5d5fa0451e6`. Provenance: `fp_checks=true`, `direct_gate_training=true`, `local_training=true`, `source_modified=false` |
| Module SHA-256 | `5141850ee3f426ce4b07b3e95d2650f8e2736774502806d760daee53daf7d6fb` |
| Speed: 0.10.0, 0.11.0, 0.11.0 gate closed (per stop) and checkpoints | Pass for default 0.11.0: 14.8–19.6% faster at every stop. See the table below. All six runs loaded five stops and produced 31 checkpoints. Full checkpoints match within each version, including 0.11.0 gate open/closed; cycles, MEM1, MEM2, aliases, spans and bytes match across versions. CPU hashes differ across versions as allowed for #553. Closed runs explicitly passed `--env BLUEWAKE_DIRECT_GATE=0` |
| a. Start and look | Partial: D3D12, draw fusion on, `gate=on`, title rendered, no fatal lines in startup; Config visibly shows Faster calls on. Forest Haven two-minute movement/camera, sustained speed and Chris's visual acceptance pending |
| b. The first run | UI automation observed the disc picker, four named progress stages with completion checkmarks, required/free disk space, then the rendered title; F1 Quit exited cleanly. Second run: 12 MB required, 67.8 GB free, zero fatal lines. Chris's observation was not collected. Temporary folder retained because cleanup was rejected by automatic approval review (`blocked by policy`) |
| c. F9 | Pending: three-state panel at Outset and CPU share at or below 100% not observed |
| d. Save states | Pending: F5/F8 messages, F8 before starting a file then F7, in-game save/restart/load and absence of recovery message not checked interactively. Benchmark state loading is not acceptance of this check |
| e. Soft effects | Pending: Dragon Roost minute off/on comparison not performed. Config shows Efficient soft effects off by default |
| Zip SHA-256, uploaded to the v0.11.0 draft | `000e165cd780fdf3f17c8cc1359c0e2952f4ebcc18148d1d3134f1f73c5a7c24`; uploaded to the existing v0.11.0 draft; GitHub SHA-256 matches |
| AppImage: build time, clang, audit line, headless start, SHA-256, uploaded | 2h 19m 15s, six jobs, clang 22.1.8, same freeze/RecompCore and fresh local training. `appimage audit: 14 ELF files; maximum GLIBC_2.38; no private game files`. Empty-data-folder headless start: title-ready at retrace 328, `gate=on`, zero fatal lines, exit 0. SHA-256 `b72fbb993b84b01c9369d50450bae952c06bece1c2660aa8c31789546f1e4462`; uploaded to the existing v0.11.0 draft; GitHub SHA-256 matches. Linux gameplay not checked |

Game frames per second, headless, mean of two runs per variant in the prescribed six-run order:

| Stop | 0.10.0 | 0.11.0 default | 0.11.0 gate closed | Default gain |
| --- | ---: | ---: | ---: | ---: |
| Outset | 54.77 | 64.50 | 56.03 | 17.8% |
| Windfall | 56.98 | 68.18 | 60.26 | 19.6% |
| Dragon Roost Cavern | 76.52 | 87.82 | 76.47 | 14.8% |
| Forest Haven | 50.23 | 58.06 | 52.03 | 15.6% |
| Sea by the Fortress | 58.70 | 67.78 | 58.73 | 15.5% |

These measure headless throughput, not displayed frame rate. Closed-gate Dragon Roost was effectively flat
(-0.06% versus baseline), not faster. Baseline Windfall varied about 6% between its two runs; default 0.11.0
repeats varied at most 1.9%. The default build improved at all five stops.

Packaging used a clean allowlisted app copy so the live portable test session could keep its copied card.
The ZIP contains 35 files, including 17 license files; its module hash matches the build. The packager's fixed
SDL license lookup was satisfied with a local junction to this build's dependency directory; no source changes
were needed. Only the Windows ZIP and Linux AppImage were uploaded; the release was verified still a draft with its pre-existing assets unchanged; generated source ZIP and sums
stay local. The Linux module SHA-256 is `5f0210c0352e3378fbd8270ac8317d61cc87890f09a869f7dd9317d93a893d17`.

