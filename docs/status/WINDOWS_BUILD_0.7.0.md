# Windows build for BlueWake 0.7.0

Written so the Codex agent on Chris's Windows PC can run it on its own as a goal loop. Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.7.0.md as a goal loop until its
> hand-off is done.

When the hand-off is done, Codex on Chris's Mac checks the zip, adds the Linux AppImage and the iPhone/iPad app,
and Chris publishes. The zip travels as a file on a **draft** GitHub release; the results travel as a pull request.

## Rules for the Windows agent

- Read [AGENTS.md](../../AGENTS.md) first. Use the BlueWake checkout already on this PC (the 0.6.0 one), or clone
  `https://github.com/chrissotraidis/bluewake`.
- Do only what this file says: no refactors, no code edits. If a step fails, stop, show the error with your best
  read of the cause, and wait for Chris.
- Start the build and every game run **from a PowerShell window on the desktop**, not over SSH or from a background
  shell: Windows then runs the compile on the efficiency cores and takes three times as long, and rendered runs get
  no real display.
- Never touch the real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode. Logs, the game
  module, the disc and saves stay on this PC; quote summary lines from logs, never attach them.
- Ask Chris only for what needs a person: watching, listening, pressing buttons, or the disc path. One short
  instruction at a time, saying exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## What to build

| | |
| --- | --- |
| Commit | The "Candidate commit" in [RELEASE_0.7.0.md](RELEASE_0.7.0.md). If it still says "not yet", stop and tell Chris: the freeze hasn't happened. |
| RecompCore | `ddd031b2a7f45fcbda37a49daac9b308b8d95da1` (the builder fetches it) |
| Version | 0.7.0, build 6 (check `version.json`) |
| New since 0.6.0 | Lean block copies are the default, so the module is about a quarter bigger and the compile about half again as long. Draw fusion is on for Windows. |

## The loop

**1. Sync and build.** `git fetch origin`, `git checkout --detach COMMIT`, and confirm `git rev-parse HEAD` matches.
Find the disc: `C:\BlueWake-private\GZLE01.iso` or `%APPDATA%\BlueWake\GZLE01.iso`; otherwise ask Chris. Then:

```
python scripts\windows\build.py DISC --out build\windows-0.7.0
```

It takes about 40 minutes on a 16-thread desktop, longer on a laptop. Note the build time, the clang version, that
the log says lean block copies were prepared, and the SHA-256 of `build\windows-0.7.0\BlueWake\gGZLE01_recomp.dll`.
If it prints `building the app without its optimization profile`, stop and tell Chris (#153).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.7.0\BlueWake\BlueWake.exe`: saves, settings and
logs then go to `user\` in that folder. Start every run from PowerShell in that folder with `.\BlueWake.exe`. After
each run, summarize the newest file in `user\logs` with `python scripts\triage_session_log.py LOG`.

For a test save, copy Chris's card and move quest log 1 (his card is only read):

```
python scripts\card_set_restart.py %APPDATA%\BlueWake\GZLE01.card build\windows-0.7.0\BlueWake\user\GZLE01.card sea 41 0
```

That starts quest log 1 at Forest Haven. Use `sea 44 0` for Outset and `M_NewD2 0 0` for Dragon Roost Cavern. If the
card is missing or slot 1 is empty, ask Chris which slot to use; with no save, start a new file and do what you can.

**3. Checks.**

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and play | Loads quest log 1 at Forest Haven, walks around for two minutes, then quits with F1 › Quit the game. | The log has `[gx] draw fusion on`, `[renderer] D3D12` (or Vulkan) and, at exit, `draws fused onto the draw before` with a large number. No fatal lines. Chris saw no missing, flickering or wrongly coloured models, water or particles. |
| b | Fusion's speed | The same two minutes, once with `$env:DOL_GX_FUSE="0"` (then `Remove-Item Env:DOL_GX_FUSE`). | Recorded either way: the median `[fps]` game rate and the lowest `speed=` of each run, from the logs. Fusion should be the same or faster. |
| c | Dragon Roost and Outset | Moves the card to `M_NewD2 0 0`, plays the cavern for two minutes with a fight; then `sea 44 0` and walks Outset. Saves once, quits, starts again and loads. | No missing drawing, the save loads, no recovery message on the second launch, the dungeon map (D-pad right) shows its grid. |
| d | Controllers, only if one is at hand | The rows in [WINDOWS_TASKS.md](../WINDOWS_TASKS.md#in-main-waiting-for-a-windows-build) that mention 0.7.0's changes: connected at launch, rumble on player 1 only, the Pictobox prompt, the optional zoom, and a GameCube adapter if there is one. | Each row as written there. |
| e | Smooth Motion at 120 | F1 › Display: Smooth Motion on, 120. Plays Forest Haven for two minutes. | The log says `[interp-pace] N threads: a slow game keeps its in-between frames` on an 8-thread-or-more PC, and the picture doesn't step down to 30 for seconds after a hitch. |
| f | Window, portable mode, FPS counter | Moves the window, quits, starts again; F1 › Display: Show FPS, then a corner. | The window opens where it was left; nothing new in `%APPDATA%\BlueWake`; the counter moves to the corner. |

**If a shows missing or broken drawing that `DOL_GX_FUSE=0` fixes:** stop and tell Chris. Fusion then has to be
turned off for Windows (one line in `runtime/host/src/main.c`), which only rebuilds the app.

**4. Package.** Delete `user\GZLE01.card` and `portable.txt` from the build folder, then:

```
python scripts\windows\package_release.py 0.7.0 --app build\windows-0.7.0\BlueWake --out build\windows-0.7.0\release
```

Note the SHA-256 of `build\windows-0.7.0\release\BlueWake-v0.7.0-windows-x64.zip` (`Get-FileHash`). The source zip and
`SHA256SUMS` it also writes stay on this PC; the Mac makes the release's own.

**5. Hand-off.**

1. Upload the zip to the draft release, creating the draft if it doesn't exist:
   ```
   gh release view v0.7.0 || gh release create v0.7.0 --draft --title "BlueWake 0.7.0" --notes "Draft. The Windows build is from Chris's PC; the Mac finishes the release."
   gh release upload v0.7.0 build\windows-0.7.0\release\BlueWake-v0.7.0-windows-x64.zip --clobber
   ```
   Check with `gh release view v0.7.0` that it is still a draft. Upload only this zip.
2. On a branch `codex/windows-0.7.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.7.0 build results" with no log files attached.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Results

| | |
| --- | --- |
| Commit built | |
| Build time, clang | |
| Module SHA-256 | |
| a. Start and play (fusion on) | |
| b. Game rate with and without fusion | |
| c. Dragon Roost, Outset, save and reload | |
| d. Controllers | |
| e. Smooth Motion at 120 | |
| f. Window, portable, FPS counter | |
| Zip SHA-256, uploaded to the v0.7.0 draft | |

