# Windows build for BlueWake 0.8.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.7.0
([WINDOWS_BUILD_0.7.0.md](WINDOWS_BUILD_0.7.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.8.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (about 2 to 3 hours on the Ryzen 7 5700U), then
play and look for about 30 minutes when the agent asks (step 3). Nothing else.

**What happens after:** the agent uploads the Windows zip to the existing **draft** v0.8.0 release and opens a pull
request with the results. Codex on the Mac checks the zip with the release gate, adds the other assets, and Chris
publishes.

## Rules for the agent

- Read [AGENTS.md](../../AGENTS.md) first. Do only what this file says: no code edits, no refactors. If anything in
  "Stop and ask Chris" happens, stop, show the error with your best read of the cause, and wait.
- Use a checkout on a **local drive** (for example `C:\src\bluewake`), never a network path.
- Start the build and every game run **from a PowerShell window on the desktop**, never over SSH or from a background
  shell: from a background shell Windows compiles on the slow cores and game windows get no real display.
- Never change Chris's real saves in `%APPDATA%\BlueWake`: read them only, and test in portable mode. Logs, the game
  module, the disc and saves stay on this PC. Quote summary lines from logs; never attach them.
- Ask Chris only for what needs a person: watching, listening, pressing buttons, or the disc path. One short
  instruction at a time, saying exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, then check out the candidate commit named in [RELEASE_0.8.0.md](RELEASE_0.8.0.md#candidate) with `git checkout --detach COMMIT`, then `git status --short` | The tree is clean, and `version.json` says 0.8.0, build 7 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | Python 3.10+, clang 22 or newer (Visual Studio 2026 18.10 or newer), CMake and Ninja on PATH, and `gh` logged in with access to chrissotraidis/bluewake |
| The disc | `C:\BlueWake-private\GZLE01.iso`, `%APPDATA%\BlueWake\GZLE01.iso`, or the `game\GZLE01.iso` of the 0.7.0 build folder | One exists. Otherwise ask Chris for the path. The builder checks it is GZLE01 revision 0. |
| Disk space | `Get-PSDrive C` | At least 40 GB free. |
| The draft release | `gh release view v0.8.0 --json isDraft,assets` | It is a draft and holds `BlueWake-v0.8.0-ios-unsigned.ipa`. Don't create another one. |

## The loop

**1. Build.** Close other programs, then from a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.8.0 --jobs 6
```

`--jobs 6` matters: without it the builder allows one compile job per 2.5 GB of free memory. When it ends, note:

- the build time and the clang version it printed;
- the lines for `lean prepaid block copies` and behaviour timing (on by default since #335; the build must not say it
  was turned off);
- the SHA-256 of `build\windows-0.8.0\BlueWake\gGZLE01_recomp.dll` (`Get-FileHash`).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.8.0\BlueWake\BlueWake.exe`. Saves, settings and
logs then go to `build\windows-0.8.0\BlueWake\user\`. Start every run from PowerShell in that folder with
`.\BlueWake.exe`. After each run, summarize the newest file in `user\logs` with
`python scripts\triage_session_log.py LOG`.

Make a test save from a **copy** of a card (`%APPDATA%\BlueWake\GZLE01.card`, then the 0.7.0 build's
`user\GZLE01.card`, then ask Chris), and place quest log 1 at Forest Haven:

```
python scripts\card_set_restart.py CARD build\windows-0.8.0\BlueWake\user\GZLE01.card sea 41 0
```

Use `sea 44 0` for Outset and `M_NewD2 0 0` for Dragon Roost Cavern when a check needs them. With no card at all,
start a new file: checks a, b and d then happen on Outset, and c is skipped.

**3. Checks with Chris.** Quit each run with F1 › Quit the game. If a run starts with a recovery message, the run
before it was interrupted: note it and go on.

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Loads quest log 1 (Forest Haven), walks and turns the camera for two minutes. | The log has `[renderer] D3D12` and `[gx] draw fusion on`, no fatal lines, and the triage summary shows the game near full speed. Chris saw nothing missing, flickering or wrongly coloured. |
| b | The HD look | At Outset (`sea 44 0`), presses F2, looks for a minute, presses F2 again. | The look comes on and goes off with each press, with a message at the bottom left; the log has `[hd-look]` lines; no crash. Note what Chris thinks of it (for #321). |
| c | Dragon Roost and saving | With the card at `M_NewD2 0 0`: plays the cavern for two minutes, opens the map (D-pad right), presses F5, then F8. Then at `sea 44 0`: saves (Start › Save), quits, starts again and loads. | The map shows its grid and rooms; "State saved" and "State loaded" show; the save loads; the second launch has no recovery message. |
| d | The menu with a controller, only if one is at hand | Plugs it in **before** starting. Presses Back: the menu opens. Moves through a tab with the D-pad, changes tabs with LB and RB, closes it with B, then plays a minute. | Everything is reachable without the mouse, B closes the menu, and the press that closes it doesn't reach Link. |
| e | Import HD textures, only if Chris has his own Wind Waker HD dump on this PC | F1 › Mods › Wind Waker HD textures › Import from Wind Waker HD…, chooses the dump's folder, waits, then Restart now. | The log after the restart says `Loaded 3546 texture replacement registrations` (or the count the menu showed). |

If Chris has had enough, a and c are the ones that matter for the release; say which were skipped.

**4. Package.** Delete `user\GZLE01.card` and `portable.txt` from the build folder, then:

```
python scripts\windows\package_release.py 0.8.0 --app build\windows-0.8.0\BlueWake --out build\windows-0.8.0\release
```

Note the SHA-256 of `build\windows-0.8.0\release\BlueWake-v0.8.0-windows-x64.zip`. The source zip and `SHA256SUMS`
it also writes stay on this PC; the Mac makes the release's own.

**5. Hand-off.**

1. Upload only the Windows zip to the existing draft:
   ```
   gh release upload v0.8.0 build\windows-0.8.0\release\BlueWake-v0.8.0-windows-x64.zip --clobber
   gh release view v0.8.0 --json isDraft,assets
   ```
   It must still be a draft, with the IPA and the zip.
2. On a branch `codex/windows-0.8.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.8.0 build results", with no logs attached.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, or prints `building the app without its optimization profile` (it needs Visual Studio 2026 18.10
  or newer, #153).
- Check a or b shows missing or broken drawing.
- The game crashes, or a save doesn't load.

## Results

| | |
| --- | --- |
| Commit built | `f2611b1357215d1f426af9dbe90c7d7189320c99`, detached, clean; 0.8.0 build 7 |
| Build time, jobs, clang | 81 min 26 s, 6 jobs, clang 22.1.3 (Visual Studio Build Tools 2026 18.10.2); exit 0; app optimization profile and ThinLTO enabled |
| Lean copies and behaviour timing lines | `lean prepaid block copies: 461975 blocks in 838 chunks`; `behaviour_timing: 401618 block starts in 838 chunks now always prepaid` |
| Module SHA-256 | `b318ad1f18bd4fb0365e0fb74210a0e8452902e333057330cb4931531306bb24` |
| a. Start and look | New file on Outset in portable mode. At Chris's direction, completed with scripted controller input and sampled window inspection instead of Chris's hands-on check. Movement and camera turns ran for over two minutes; sampled views showed no obvious missing geometry or incorrect colours. Intermittent flicker and audible sound were not independently verified. Log: `[renderer] D3D12`, `[gx] draw fusion on`, 0 fatal lines; 3 below-target seconds of 321 monitored (lowest dip 72%; 95% median among dips, not the session median), generally 30 FPS. Quit through F1 > Quit the game; process exited and shutdown recorded. |
| b. The HD look | skipped at Chris's direction |
| c. Dragon Roost and saving | skipped at Chris's direction |
| d. The menu with a controller | skipped at Chris's direction |
| e. Import HD textures | skipped at Chris's direction |
| Zip SHA-256, uploaded to the v0.8.0 draft | `56cc845823ec7e6af19f5306ab7517a511d6bbcaa222cc7f84a437b38c588e2f`; uploaded only `BlueWake-v0.8.0-windows-x64.zip`; GitHub's digest matches; release remains a draft with the IPA |

Checked October 17, 2026 on Chris's Windows PC: AMD Ryzen 7 5700U with Radeon Graphics,
Windows build 26300. The build and both game launches ran from visible desktop PowerShell windows.
No card existed in either documented location; the test used a new portable file. The first session
remained at the title/attract sequence while awaiting input and was also quit cleanly through F1.
The second session used the existing `BLUEWAKE_PAD_SCRIPT` and title/dialogue confirmation controls,
normal D3D12 rendering and normal pacing; no game or build code was changed.

The 35-file Windows zip passed the packager's private-file and DLL-dependency checks plus ZIP integrity
validation. Its contents include `BlueWake.exe`, `gGZLE01_recomp.dll` and `d3dcompiler_47.dll`.
The test card and `portable.txt` were removed before packaging. No logs, disc, saves, local source zip or
local `SHA256SUMS` were uploaded. The previously uploaded release assets were preserved.

Mac hand-off: download the draft Windows zip, verify the SHA-256 above, and run
`scripts/release/check_public_assets.sh` before publication. The external release gate is not installed
on this Windows PC and was not run here. The only accepted content finding is
`containsTranslatedGameCode: true`. Chris retains publication; this hand-off does not publish or tag.
