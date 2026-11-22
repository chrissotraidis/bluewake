# Windows build for BlueWake 0.13.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.12.0
([WINDOWS_BUILD_0.12.0.md](WINDOWS_BUILD_0.12.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.13.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (about 1.5 hours, then about 2 hours for the
Linux AppImage in WSL), and look at the screen for about 15 minutes when the agent asks (step 4). The test card from
0.12.0 is already on the PC (`C:\BlueWake-private\GZLE01.card`).

**What happens after:** the agent uploads the Windows zip and the Linux AppImage to the existing **draft** v0.13.0
release and opens a pull request with the results. Codex on the Mac checks both with the release gate, and Chris
publishes.

## What is new on Windows and Linux in 0.13.0

- **Smooth Motion** (#615, #625, #594; RecompCore #62, #64, #58): the heat haze in lava rooms stays with the scene when
  the camera turns; the last heart keeps its size between frames; with the HD look, no one-frame shading jumps when the
  camera speeds up or slows down. Display › Smooth Motion under sustained load gets **Step down (experimental)** for
  120 Hz and up (#595, RecompCore #59), off by default.
- **The Picto Box in rooms with heat haze** (#617, RecompCore #63): its window follows the camera in Dragon Roost
  Cavern's lava rooms instead of freezing. This is on for everyone, with or without Smooth Motion.
- **The HD look** (#533, RecompCore #51): with HD's sky and light on, HD's sunlight at gamma's 1/2.2 by default, cloud
  shadows off by default, Windfall's arch no longer dims the whole screen, and HD's light on characters. **Wind Waker
  HD's own light and shadows** is a new experimental switch in the HD look's settings, off by default; it needs a pack
  imported from an HD disc. The HD look itself stays off by default.
- **Config › Performance** (#586, RecompCore #54), all off: Faster cloud shadows, Faster ambient occlusion, Present
  without waiting for the display, Present without gamescope's frame layer (Linux).
- **Mods › HD textures** (#627, RecompCore #65): how many textures loaded at start; **Keep the game's shading tables**
  and **Save the textures the pack lacks**, both off.
- **Widescreen** (#623): two more presets, **21:9 · 2560x1080** (64:27) and **21:9 · 3440x1440** (43:18). The builder
  now makes four more mod variants, so the mods step takes longer.
- **Better Wind Waker › No heart pulsing** (#628), off.
- **Gameplay › Sailing › Sail on Z** (#634), off; the controller zoom works on the boat too (#633).
- **Save state slots** (#632): five slots in the menu's footer, beside F5 and F8.
- **The Windows zip carries the stage select's English names** (#616).

## Rules for the agent

- Read [AGENTS.md](../../AGENTS.md) first. Do only what this file says: no code edits, no refactors. If anything in
  "Stop and ask Chris" happens, stop, show the error with your best read of the cause, and wait.
- Use a checkout on a **local drive** (for example `C:\src\bluewake`), never a network path.
- Start the build and every game run **from a PowerShell window on the desktop**, never over SSH or from a background
  shell: from a background shell Windows compiles on the slow cores and game windows get no real display.
- Never change Chris's real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode with copies. Logs,
  the game module, the disc, saves, save states and frames stay on this PC. Quote summary lines from logs; never attach
  them.
- Ask Chris only for what needs a person: looking, pressing buttons, or a path. One short instruction at a time, saying
  exactly what to look for. Read the logs and frames yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, then `git log origin/main --merges --grep "Freeze 0.13.0" -1 --format=%H` gives the freeze commit; `git checkout --detach COMMIT`, then `git status --short` | One commit is found, the tree is clean, and `version.json` says 0.13.0, build 14 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | As for 0.12.0: Python 3.10+, clang 22 or newer, CMake and Ninja on PATH, `gh` logged in |
| The disc | `C:\BlueWake-private\GZLE01.iso`, or the one 0.12.0's build used | One exists. Otherwise ask Chris for the path |
| The card | `C:\BlueWake-private\GZLE01.card` | It exists; it is the card 0.12.0's checks used |
| The 0.12.0 build and tour state | `build\windows-0.12.0\BlueWake\BlueWake.exe` in the checkout (or the unpacked 0.12.0 release zip), and `C:\BlueWake-private\tour-0.11.0.bwstate` | Both exist; they are the baseline for steps 3 and 4 |
| Disk space | `Get-PSDrive C` | At least 80 GB free (four more mod variants than 0.12.0) |
| WSL and Docker | `wsl -l -v`; in Ubuntu `docker info` (without `sudo`) | As for 0.12.0 |
| The draft release | `gh release view v0.13.0 --json isDraft,assets` | It is a draft. Don't create another one |

## The loop

**1. Build**, into a new folder. From a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.13.0 --jobs 6
```

No extra options. Note the build time and clang version; that it fetched RecompCore `c0f5f91`; that the mods step made
`widescreen6427` and `widescreen4318` and their combos; that `BuilderProvenance.json` has `"fp_checks": true` and
`"direct_gate_training": true`; and the SHA-256 of `build\windows-0.13.0\BlueWake\gGZLE01_recomp.dll`.

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.13.0\BlueWake\BlueWake.exe`. After each run Chris plays,
summarize the newest file in `user\logs` with `python scripts\triage_session_log.py LOG`. Place quest log 1 of a **copy** of
the card at Outset:

```
python scripts\card_set_restart.py C:\BlueWake-private\GZLE01.card build\windows-0.13.0\BlueWake\user\GZLE01.card sea 44 0
```

**3. Speed and exactness, on its own.** From the 0.11.0 tour state, four headless runs in this order: 0.12.0, 0.13.0,
0.13.0, 0.12.0:

```
python scripts\bench_tour.py CARD APP --mode headless --setup-state C:\BlueWake-private\tour-0.11.0.bwstate
```

Compare with `python scripts\bench_tour.py compare A.json B.json`. Passes when every run loaded all five stops, the two
0.13.0 runs' checkpoints match each other, guest memory (cycles, MEM1, MEM2, aliases) matches 0.12.0's, and 0.13.0 is
within 3% of 0.12.0 at every stop. Nothing on by default should change what the game computes. One change could cost a
little speed: #533 watches two more draw functions (`J3DDrawBuffer::drawHead` and `drawTail`) for everyone, so note any
stop that is slower and by how much.

**4. Pictures and checks.** The rendered checks use `scripts\rendered_run.py` (`--help`; each run gets its own folder and
a copy of the card) and `scripts\frame_compare.py`; the agent reads the frames itself. Quit each run Chris plays with
F1 › Quit the game.

| # | Check | How | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Chris loads quest log 1 at Outset and walks and turns the camera for two minutes | `[renderer] D3D12`, `[gx] draw fusion on`, `gate=on`, no fatal lines; Chris saw nothing missing, flickering or wrongly coloured |
| b | Heat haze and the Picto Box (#615, #617) | Chris warps (F7) into a Dragon Roost Cavern room with heat haze, Smooth Motion at the display's rate, and swings the camera; then raises the Picto Box there and turns it, then takes a photo. Once more with Smooth Motion off | The haze moves with the scene; the Picto Box's window follows the stick and the photo shows what it showed, with Smooth Motion on and off |
| c | The heart (#625) | Rendered runs, 0.12.0 and 0.13.0, warped to Outset with `--smooth display` and `--dump 600:900` | In 0.13.0's in-between frames the last heart keeps its size through each beat; in 0.12.0's it is drawn big for a few once a beat |
| d | Ultrawide presets (#623) | Rendered runs warped to Outset, `--set BLUEWAKE_ASPECT=43:18` and `=64:27`, in a 16:9 window | Black bars above and below the picture, each about 12.8% of the window's height at 43:18 and 12.5% at 64:27 (the presets' own shapes in a 16:9 window); the HUD sits inside the picture; no crash. With a 21:9 monitor, Chris picks the preset for it and sees no bars |
| e | No heart pulsing (#628) | Rendered runs warped to Outset with `--set BLUEWAKE_MODS=betterww`, `BLUEWAKE_OPTIONS=no_heart_pulse` and without it, `--capture 900:7` | With it the last heart's glowing copy is never drawn in any frame; without it, it is |
| f | Save state slots (#632) | Chris opens F1, presses **Save state slots…**, saves slot 2, quits; starts again, loads slot 2, then tries Load on an empty slot; then F5 and F8 | Slot 2 shows its time after the restart and loads where it was saved; the empty slot says so; F5 and F8 work as before |
| g | The stage select's names (#616) | After step 5, unpack the zip into an empty folder | It holds `stage_select\names.tsv`, identical to `config\stage_select\names.tsv` |

If Chris has had enough, a, b and f are the ones that need him; c, d, e and g the agent does alone.

**5. Package.** Delete `user\` and `portable.txt` from the build folder, then:

```
python scripts\windows\package_release.py 0.13.0 --app build\windows-0.13.0\BlueWake --out build\windows-0.13.0\release
```

Note the SHA-256 of `BlueWake-v0.13.0-windows-x64.zip`. The source zip and `SHA256SUMS` it also writes stay on this PC; the
Mac makes the release's own.

**6. The Linux AppImage, in WSL**, from the same commit and disc. Keep everything on WSL's own disk:

```
git clone https://github.com/chrissotraidis/bluewake ~/bluewake-0.13.0 && cd ~/bluewake-0.13.0
git checkout --detach COMMIT && git status --short
python3 scripts/linux/build.py ~/bw-private/GZLE01.iso --out build/linux-0.13.0 --ci-container --appimage --jobs 6
```

Passes when it fetched RecompCore `c0f5f91`, printed clang 22, and the AppImage audit's line reports GLIBC no higher than
2.39. Then start the app folder headless from an empty data folder; it reaches the title screen and quits by itself:

```
mkdir -p ~/bw-private/scratch-0.13
BLUEWAKE_DATA_DIR=~/bw-private/scratch-0.13 BLUEWAKE_DISC=~/bw-private/GZLE01.iso BLUEWAKE_RENDERER=headless BLUEWAKE_MAX_RETRACES=1500 build/linux-0.13.0/BlueWake/bluewake
```

Passes when the newest session log there has `[boot-milestone] title-ready`, `gate=on` and no fatal lines. Copy the
AppImage out as `/mnt/c/BlueWake-private/BlueWake-v0.13.0-linux-x86_64.AppImage` and note its SHA-256.

**7. Hand-off.**

1. Upload only the Windows zip and the AppImage to the existing draft:
   ```
   gh release upload v0.13.0 build\windows-0.13.0\release\BlueWake-v0.13.0-windows-x64.zip --clobber
   gh release upload v0.13.0 C:\BlueWake-private\BlueWake-v0.13.0-linux-x86_64.AppImage --clobber
   gh release view v0.13.0 --json isDraft,assets
   ```
   It must still be a draft.
2. On a branch `codex/windows-0.13.0-results` from `origin/main`, fill in the Results table below and open a pull request
   "Windows 0.13.0 build results", with no logs or frames attached. Mark the WINDOWS_TASKS.md rows each check covered.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, stops with `WinError 5`, or prints `building the app without its optimization profile`.
- The speed check shows a stop that doesn't load, checkpoints that differ between the 0.13.0 runs, guest memory that
  differs from 0.12.0's, or 0.13.0 more than 3% slower at a stop.
- A check shows missing or broken drawing beyond the changes above; the game crashes; a save or a state doesn't load.
- The AppImage build fails, its audit fails or reports GLIBC above 2.39, or the headless start doesn't reach the title.

## Results

To be filled in by the agent.

| | |
| --- | --- |
| Commit built | |
| Build time, jobs, clang, RecompCore, provenance, mod variants | |
| Module SHA-256 | |
| Speed: 0.12.0 and 0.13.0 per stop, checkpoints | |
| a. Start and look | |
| b. Heat haze and the Picto Box | |
| c. The heart | |
| d. Ultrawide presets | |
| e. No heart pulsing | |
| f. Save state slots | |
| g. The stage select's names | |
| Zip SHA-256, uploaded to the v0.13.0 draft | |
| AppImage: build time, clang, audit line, headless start, SHA-256, uploaded | |

