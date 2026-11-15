# Windows build for BlueWake 0.12.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.11.0
([WINDOWS_BUILD_0.11.0.md](WINDOWS_BUILD_0.11.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.12.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (about 1.5 hours, then about 2.5 hours for the
Linux AppImage in WSL), and look at the screen for about 15 minutes when the agent asks (step 4). The test card from
0.11.0 is already on the PC (`C:\BlueWake-private\GZLE01.card`).

**What happens after:** the agent uploads the Windows zip and the Linux AppImage to the existing **draft** v0.12.0
release and opens a pull request with the results. Codex on the Mac checks both with the release gate, and Chris
publishes.

## What is new on Windows and Linux in 0.12.0

- **Distance blur** (#605, RecompCore#61): the game's depth of field blurred almost everything (its depth copy was read
  in the wrong format). It now blurs only the distance. Display › **Distance blur** (Off, Light, Default, Strong,
  GameCube) and **Distance blur starts** (0 to 100) start as the game has them. This is the one change that alters the
  default picture everywhere.
- **Module art** (#583): the Earth Temple's banners and the pirate ship's sail draw their art instead of noise.
- **Native cutscene framing** (#599), Mods, off by default: widescreen cutscenes without black bars, and the opening
  legend unstretched.
- **Dolphin saves** (#604): Config › Dolphin saves › Import Dolphin Save… and Export Dolphin Save….
- **Save states** (#582): a state saved with the stage select off loads as "in play", so F7 works after it.
- **Shorter Triforce quest** (#589), Mods › Better Wind Waker, off by default.
- **The builder** retries the `translated.new` rename while antivirus has the files open (#596, padmint#255).
- Linux only: the AppImage carries the stage select's English names (#585); the first-run window's Cancel stops the
  preparation (#540).

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
| The commit | `git fetch origin`, then `git log origin/main --merges --grep "Freeze 0.12.0" -1 --format=%H` gives the freeze commit; `git checkout --detach COMMIT`, then `git status --short` | One commit is found, the tree is clean, and `version.json` says 0.12.0, build 12 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | As for 0.11.0: Python 3.10+, clang 22 or newer, CMake and Ninja on PATH, `gh` logged in |
| The disc | `C:\BlueWake-private\GZLE01.iso`, or the one 0.11.0's build used | One exists. Otherwise ask Chris for the path |
| The card | `C:\BlueWake-private\GZLE01.card` | It exists; it is the card 0.11.0's checks used |
| The 0.11.0 build and tour state | `build\windows-0.11.0\BlueWake\BlueWake.exe` in the checkout (or the unpacked 0.11.0 release zip), and `C:\BlueWake-private\tour-0.11.0.bwstate` | Both exist; they are the baseline for steps 3 and 4 |
| Disk space | `Get-PSDrive C` | At least 70 GB free |
| WSL and Docker | `wsl -l -v`; in Ubuntu `docker info` (without `sudo`) | As for 0.11.0 |
| The draft release | `gh release view v0.12.0 --json isDraft,assets` | It is a draft. Don't create another one |

## The loop

**1. Build**, into a new folder, so the first build's rename (#596) runs. From a desktop PowerShell window in the
checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.12.0 --jobs 6
```

No extra options. Note the build time and clang version; that it fetched RecompCore `2159e88`; whether the log has a line
about retrying the `translated.new` rename (either is fine; a `WinError 5` stop is not); that `BuilderProvenance.json`
has `"fp_checks": true` and `"direct_gate_training": true`; and the SHA-256 of
`build\windows-0.12.0\BlueWake\gGZLE01_recomp.dll`.

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.12.0\BlueWake\BlueWake.exe`. After each run Chris plays,
summarize the newest file in `user\logs` with `python scripts\triage_session_log.py LOG`. Place quest log 1 of a **copy** of
the card at Outset:

```
python scripts\card_set_restart.py C:\BlueWake-private\GZLE01.card build\windows-0.12.0\BlueWake\user\GZLE01.card sea 44 0
```

**3. Speed and exactness, on its own.** From the 0.11.0 tour state, four headless runs in this order: 0.11.0, 0.12.0,
0.12.0, 0.11.0:

```
python scripts\bench_tour.py CARD APP --mode headless --setup-state C:\BlueWake-private\tour-0.11.0.bwstate
```

Compare with `python scripts\bench_tour.py compare A.json B.json`. Passes when every run loaded all five stops, the two
0.12.0 runs' checkpoints match each other, guest memory (cycles, MEM1, MEM2, aliases) matches 0.11.0's (nothing that is
on by default in 0.12.0 changes what the game computes), and 0.12.0 is within 3% of 0.11.0 at every stop. No speed
change is expected.

**4. Pictures and checks.** The rendered checks use `scripts\rendered_run.py` (`--help`; each run gets its own folder and
a copy of the card) and `scripts\frame_compare.py`; the agent reads the frames itself. Quit each run Chris plays with
F1 › Quit the game.

| # | Check | How | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Chris loads quest log 1 at Outset and walks and turns the camera for two minutes | `[renderer] D3D12`, `[gx] draw fusion on`, `gate=on`, no fatal lines; Chris saw nothing missing, flickering or wrongly coloured |
| b | Distance blur (#605) | Rendered runs warped to Outset at noon (`--warp sea:44:0 --warp-time noon`), a capture every 300 retraces: 0.11.0 and 0.12.0 at `--scale 1`, then 0.12.0 at `--scale 3` with `--set DOL_AURORA_GAME_DOF=0`, `=2` and `=4`. Then Chris sails a minute at 1x render scale with the camera zoomed all the way in, and moves Display › Distance blur starts | At 1x, 0.11.0's frames are soft almost everywhere past Link; 0.12.0's keep Link and what is near sharp and blur only the distance. At 3x, `=4` blurs the distance about as 1x does and `=0` not at all. Chris sees Link and the boat sharp, and the slider move the blur out |
| c | Module art (#583) | Rendered runs, 0.11.0 and 0.12.0, warped to `M_Dai:5:6` and `A_umikz:0:204` at noon | 0.12.0's Earth Temple banners show red and gold cloth and the ship's sail its art where 0.11.0 shows noise or a flat colour; the rest of each frame matches |
| d | Cutscene framing (#599) | Rendered runs warped to `A_mori:0:202:8` (Tetra falls) with `--set BLUEWAKE_ASPECT=16:9`, with `BLUEWAKE_NATIVE_CUTSCENE_FRAMING=1` and without | On: no black bars in any captured frame. Off: the game's bars. No crash either way |
| e | Dolphin saves (#604) | In the portable build with the copied card, Chris opens F1 › Config › Dolphin saves › **Export Dolphin Save…** and saves `C:\BlueWake-private\dolphin\export.gci`; then **Import Dolphin Save…**, picks that file, picks its quest log 1 and Quest Log 3 to put it in, and presses the button | Export writes the file. Import lists the quest logs with hearts and rupees, makes `user\Backups\GZLE01-*.card`, and BlueWake restarts; the imported quest log loads and plays |
| f | Save states (#582) | A rendered run with the stage select off: quest log 1, F5 in play. The next launch with the stage select on: F8 before starting a file, then F7 | The log says `state loaded: a file is going`; F7 asks, then opens the stage select |

If Chris has had enough, a, b and e are the ones that need him; c, d and f the agent does alone.

**5. Package.** Delete `user\` and `portable.txt` from the build folder, then:

```
python scripts\windows\package_release.py 0.12.0 --app build\windows-0.12.0\BlueWake --out build\windows-0.12.0\release
```

Note the SHA-256 of `BlueWake-v0.12.0-windows-x64.zip`. The source zip and `SHA256SUMS` it also writes stay on this PC; the
Mac makes the release's own.

**6. The Linux AppImage, in WSL**, from the same commit and disc. Keep everything on WSL's own disk:

```
git clone https://github.com/chrissotraidis/bluewake ~/bluewake-0.12.0 && cd ~/bluewake-0.12.0
git checkout --detach COMMIT && git status --short
python3 scripts/linux/build.py ~/bw-private/GZLE01.iso --out build/linux-0.12.0 --ci-container --appimage --jobs 6
```

Passes when it fetched RecompCore `2159e88`, printed clang 22, and the AppImage audit's line reports GLIBC no higher than
2.39. Then the stage select's names (#585): in an empty folder,
`build/linux-0.12.0/BlueWake-x86_64.AppImage --appimage-extract 'usr/bin/stage_select/*'` gives
`squashfs-root/usr/bin/stage_select/names.tsv`, identical to `config/stage_select/names.tsv` (`cmp`). Then start the app folder
headless from an empty data folder; it reaches the title screen and quits by itself:

```
mkdir -p ~/bw-private/scratch-0.12
BLUEWAKE_DATA_DIR=~/bw-private/scratch-0.12 BLUEWAKE_DISC=~/bw-private/GZLE01.iso BLUEWAKE_RENDERER=headless BLUEWAKE_MAX_RETRACES=1500 build/linux-0.12.0/BlueWake/bluewake
```

Passes when the newest session log there has `[boot-milestone] title-ready`, `gate=on` and no fatal lines. Copy the
AppImage out as `/mnt/c/BlueWake-private/BlueWake-v0.12.0-linux-x86_64.AppImage` and note its SHA-256. #540's Cancel
needs the setup window on a Linux desktop; it is not checked here.

**7. Hand-off.**

1. Upload only the Windows zip and the AppImage to the existing draft:
   ```
   gh release upload v0.12.0 build\windows-0.12.0\release\BlueWake-v0.12.0-windows-x64.zip --clobber
   gh release upload v0.12.0 C:\BlueWake-private\BlueWake-v0.12.0-linux-x86_64.AppImage --clobber
   gh release view v0.12.0 --json isDraft,assets
   ```
   It must still be a draft.
2. On a branch `codex/windows-0.12.0-results` from `origin/main`, fill in the Results table below and open a pull request
   "Windows 0.12.0 build results", with no logs or frames attached. Mark the WINDOWS_TASKS.md rows each check covered.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, stops with `WinError 5`, or prints `building the app without its optimization profile`.
- The speed check shows a stop that doesn't load, checkpoints that differ between the 0.12.0 runs, guest memory that
  differs from 0.11.0's, or 0.12.0 more than 3% slower at a stop.
- A check shows missing or broken drawing beyond the changes above; the game crashes; a save or an import doesn't load.
- The AppImage build fails, its audit fails or reports GLIBC above 2.39, or the headless start doesn't reach the title.

## Results

Completed November 15, 2026 on Chris's Windows 11 PC (Ryzen 7 5700U, AMD Radeon graphics), with Ubuntu in WSL for Linux. Chris directed accepting the changes and finishing the build despite this PC's benchmark variance. The 3% speed gate is **waived, not passed**. These are automated runtime, native UI and captured-frame checks; Chris did not perform the requested hands-on visual acceptance.

| | |
| --- | --- |
| Commit built | Clean freeze `df9e46fe0be5c0f95d46a3b194f59a4b96186640`, version 0.12.0, build 12; both platforms |
| Build time, jobs, clang, RecompCore, provenance, rename retry | Windows: 80m14s, six jobs, clang 22.1.3; RecompCore `2159e88b10f1d4c77cfb67041382b623b4462205`. Fresh local training, `fp_checks: true`, `direct_gate_training: true`, clean source provenance and matching module hash. App optimization profile used. No rename retry or WinError 5 stop. Desktop PowerShell used process-only `-ExecutionPolicy Bypass` because its execution policy blocked the launcher script; no persistent policy change |
| Module SHA-256 | `59f1231f53cb3f085c3db3c86eb0540e3ab74337894e459646d4713ae2eec33f` |
| Speed: 0.11.0 and 0.12.0 per stop, checkpoints | Four four-run sets completed. Each loaded all five stops; all 31 checkpoints matched within and across versions, including cycles, MEM1, MEM2 and aliases. Final set, with Plex stopped, is below. Outset was 4.27% slower: Chris waived the speed gate rather than repeat it again. Results do not establish a regression cause |
| a. Start and look | Scripted Outset movement/camera run from imported quest log 3: 137s, exit 0, 12 captured frames, 29.97 game FPS, D3D12, draw fusion and gate on, no fatal lines. No human two-minute visual acceptance claimed |
| b. Distance blur | All five prescribed rendered variants exited 0 with seven captures each. Reviewed 3x captures show the stronger GameCube distance blur and sharp Link, with Off sharper in the distance. The 1x pair was closely matched in this view; it does not prove the full expected before/after softness. Sailing with zoom and moving the slider by hand remain unchecked |
| c. Module art | Both version pairs exited 0. Earth Temple captures confirm red/gold banner cloth in 0.12.0 versus noisy purple in 0.11.0, with surrounding scene closely matched. Ship warp reached the deck, but the sail was outside the captured view; sail art remains unverified |
| d. Cutscene framing | On/off rendered runs exited 0. This progressed card's forest warp did not trigger Tetra's cutscene, so the captures do not verify the black-bar change |
| e. Dolphin saves | Native UI automation exported a 98,368-byte GCI, displayed Link with six hearts and 180 rupees, imported source quest log 1 into empty Quest Log 3, made a 98,412-byte backup and restarted BlueWake. A separate rendered run loaded imported quest log 3 at Outset and ran scripted movement/camera input for 137s, exit 0. Only copied cards used; original card and tour-state hashes unchanged. RAW/damaged GCI cases and Dolphin's own import UI not tested |
| f. Save states | Native UI: F5 saved in play with stage select off; next launch with stage select on, F8 before starting a file restored play (161 host fields, none missing/mismatched). Log says `state loaded: a file is going`; F7 asked, then a second F7 opened stage select. Passed this regression case |
| Zip SHA-256, uploaded to the v0.12.0 draft | `3ff3915f5c893539a0ce9f1052d769d8c984826b140c907a61166508e6dc58b9` — `BlueWake-v0.12.0-windows-x64.zip`; uploaded, GitHub digest matches |
| AppImage: build time, clang, audit line, stage select names, headless start, SHA-256, uploaded | 2h02m58s, six jobs, clang 22.1.8. `appimage audit: 14 ELF files; maximum GLIBC_2.38; no private game files`. Extracted stage names match source byte for byte. Empty-data headless launch reached `title-ready` at retrace 328, gate on, exit 0 and no fatal lines. SHA-256 `d1cc82d1e173719f1da41cabbee53a18227c35c44498134fe4319768acd60a6a` — `BlueWake-v0.12.0-linux-x86_64.AppImage`; uploaded, GitHub digest matches. Linux graphical gameplay and setup Cancel not tested |

Final four-run set (0.11.0, 0.12.0, 0.12.0, 0.11.0), mean benchmark rates; higher is faster:

| Stop | 0.11.0 | 0.12.0 | Change |
| --- | ---: | ---: | ---: |
| Outset | 63.49165 | 60.78075 | -4.27% |
| Windfall | 65.82630 | 66.67725 | +1.29% |
| Dragon Roost Cavern | 86.89725 | 85.44170 | -1.68% |
| Forest Haven | 58.44040 | 57.80000 | -1.10% |
| Sea | 66.00985 | 65.88370 | -0.19% |

Packaging used the repository allowlist/private-file and DLL-dependency checks, with dependency license paths pointed at this build's own dependency directory. Portable test data was moved to private storage before packaging. The generated source ZIP and checksums stayed local; the existing three Apple/source draft assets were preserved. No logs, frames, discs or saves were uploaded or committed.

The existing v0.12.0 release remains a draft. This completes the Windows/WSL build handoff; the Mac's external release-content gate and Chris's publication decision remain outstanding, as assigned above. The visual coverage gaps are recorded rather than counted as passes.
