# Windows build for BlueWake 0.9.0

Written for the agent on Chris's Windows PC to run on its own as a goal loop, as for 0.8.0
([WINDOWS_BUILD_0.8.0.md](WINDOWS_BUILD_0.8.0.md)). Chris only has to say:

> Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.9.0.md as a goal loop until its
> hand-off is done.

**What Chris does:** keep the PC plugged in and awake while it builds (about 1.5 to 3 hours on the Ryzen 7 5700U), then
play and look for about 20 minutes when the agent asks (step 3). Nothing else.

**What happens after:** the agent uploads the Windows zip to the existing **draft** v0.9.0 release and opens a pull
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
- Ask Chris only for what needs a person: watching, pressing buttons, or the disc path. One short instruction at a
  time, saying exactly what to look for. Read the logs yourself.
- The release stays a draft. Don't publish it, don't comment on issues, don't push to `main`.

## Before you start

Check each line; if one fails, stop and ask Chris.

| Check | How | Passes when |
| --- | --- | --- |
| The commit | `git fetch origin`, then `git log origin/main --merges --grep "Merge pull request #464" -1 --format=%H` gives the freeze commit; `git checkout --detach COMMIT`, then `git status --short` | One commit is found, the tree is clean, and `version.json` says 0.9.0, build 8 |
| The tools | `python --version`, `clang --version`, `cmake --version`, `ninja --version`, `gh auth status` | Python 3.10+, clang 22 or newer (Visual Studio 2026 18.10 or newer), CMake and Ninja on PATH, and `gh` logged in with access to chrissotraidis/bluewake |
| The disc | `C:\BlueWake-private\GZLE01.iso`, `%APPDATA%\BlueWake\GZLE01.iso`, or the `game\GZLE01.iso` of the 0.8.0 build folder | One exists. Otherwise ask Chris for the path. The builder checks it is GZLE01 revision 0. |
| Disk space | `Get-PSDrive C` | At least 40 GB free. |
| The draft release | `gh release view v0.9.0 --json isDraft,assets` | It is a draft and holds `BlueWake-v0.9.0-ios-unsigned.ipa`. Don't create another one. |

## The loop

**1. Build.** Close other programs, then from a desktop PowerShell window in the checkout:

```
python scripts\windows\build.py DISC --out build\windows-0.9.0 --jobs 6
```

`--jobs 6` matters: without it the builder allows one compile job per 2.5 GB of free memory. When it ends, note:

- the build time and the clang version it printed;
- the lines for `lean prepaid block copies` and behaviour timing (the build must not say it was turned off);
- that it fetched RecompCore `1446450`;
- the SHA-256 of `build\windows-0.9.0\BlueWake\gGZLE01_recomp.dll` (`Get-FileHash`).

**2. Prepare.** Put an empty `portable.txt` beside `build\windows-0.9.0\BlueWake\BlueWake.exe`. Saves, settings and
logs then go to `build\windows-0.9.0\BlueWake\user\`. Start every run from PowerShell in that folder with
`.\BlueWake.exe`. After each run, summarize the newest file in `user\logs` with
`python scripts\triage_session_log.py LOG`.

Make a test save from a **copy** of a card (`%APPDATA%\BlueWake\GZLE01.card`, then the 0.8.0 build's
`user\GZLE01.card`, then ask Chris), and place quest log 1 at Forest Haven:

```
python scripts\card_set_restart.py CARD build\windows-0.9.0\BlueWake\user\GZLE01.card sea 41 0
```

Use `sea 44 0` for Outset and `M_NewD2 0 0` for Dragon Roost Cavern when a check needs them. With no card at all,
start a new file: checks a, b, c and e then happen on Outset, and d is skipped.

**3. Checks with Chris.** Quit each run with F1 › Quit the game. If a run starts with a recovery message, the run
before it was interrupted: note it and go on. Plug a controller in **before** starting if one is at hand.

| # | Check | Chris does | Passes when |
| --- | --- | --- | --- |
| a | Start and look | Loads quest log 1 (Forest Haven), walks and turns the camera for two minutes. | The log has `[renderer] D3D12` and `[gx] draw fusion on`, no fatal lines, and the triage summary shows the game near full speed. Chris saw nothing missing, flickering or wrongly coloured. |
| b | The camera standing still (#455), with a controller | Direct right-stick camera and aiming is on by default. Nudges the right stick, lets go, and stands still for a minute without touching anything. | The view holds still: no one-frame dip and snap back. Then climbing a ledge and going through a door look normal. |
| c | Smooth Motion with the HD look (#371) | At Outset (`sea 44 0`): F2 turns the HD look on, F10 turns Smooth Motion on (F1 › Display: Smooth Motion at the display's rate or 120). Looks at the clouds and the sea for a minute, turning the camera, then jumps in the water and swims. | No flicker in the clouds, the wave crests, the HD look's shadows or the rings around a swimming Link; the log has `[hd-look]` lines; no crash. F10 and F2 turn each off again. |
| d | Dragon Roost and saving | With the card at `M_NewD2 0 0`: plays the cavern for two minutes, opens the map (D-pad right), presses F5, then F8. Then at `sea 44 0`: saves (Start › Save), quits, starts again and loads. | The map shows its grid and rooms; "State saved" and "State loaded" show; the save loads; the second launch has no recovery message. |
| e | The menu and the new controller options, with a controller | Presses Back: the menu opens. Moves through Controls with the D-pad, changes tabs with LB and RB. Turns on "Controller works while another window is in front" and turns off "Pause while the window is in the background", closes the menu with B, clicks another window, and moves Link with the controller. Then turns off "The right stick aims too", takes out the bow (or first person with the right-stick click) and aims with the left stick. | Everything is reachable without the mouse; B closes the menu and the press doesn't reach Link; the controller plays with another window in front; with "aims too" off, the right stick doesn't aim and the left stick does. Turn both options back to their defaults afterwards. |

If Chris has had enough, a and d are the ones that matter for the release, then b; say which were skipped.

**4. Package.** Delete `user\GZLE01.card` and `portable.txt` from the build folder (and the rest of `user\`), then:

```
python scripts\windows\package_release.py 0.9.0 --app build\windows-0.9.0\BlueWake --out build\windows-0.9.0\release
```

Note the SHA-256 of `build\windows-0.9.0\release\BlueWake-v0.9.0-windows-x64.zip`. The source zip and `SHA256SUMS`
it also writes stay on this PC; the Mac makes the release's own.

**5. Hand-off.**

1. Upload only the Windows zip to the existing draft:
   ```
   gh release upload v0.9.0 build\windows-0.9.0\release\BlueWake-v0.9.0-windows-x64.zip --clobber
   gh release view v0.9.0 --json isDraft,assets
   ```
   It must still be a draft, with the IPA and the zip.
2. On a branch `codex/windows-0.9.0-results` from `origin/main`, fill in the Results table below and open a pull
   request "Windows 0.9.0 build results", with no logs attached. Mark the WINDOWS_TASKS.md rows each check covered.
3. Tell Chris it's done, with the results table and the pull request link. Then stop.

## Stop and ask Chris

- Any check in "Before you start" fails.
- The build fails, or prints `building the app without its optimization profile` (it needs Visual Studio 2026 18.10
  or newer, #153).
- Check a or c shows missing or broken drawing, or check b shows the dip.
- The game crashes, or a save doesn't load.

## Results

| | |
| --- | --- |
| Commit built | `211abfdecf13beb5b9f2120459a1a3cde50dd84e` (detached, clean); version 0.9.0, build 8. |
| Build time, jobs, clang | Windows PC, desktop PowerShell; exit 0 in 85m 8s, 6 jobs, clang 22.1.3. App compiler flags use `windows/pgo/app.profdata`; module flags use the locally trained profile. |
| Lean copies and behaviour timing lines, RecompCore | `lean prepaid block copies: 464979 blocks in 842 chunks`; `behaviour_timing: 404438 block starts in 842 chunks now always prepaid`; RecompCore `1446450c79046bd34e43abadc27522eeab768a6e`. |
| Module SHA-256 | `795d19375c01df3c8fb7b6b005bd2fd512c7f587d670cb72e91c0b737d58985a` |
| a. Start and look | Partial: portable title/opening session for 479.3 minutes, `[renderer] D3D12`, `[gx] draw fusion on`, zero fatal lines; F1 > Quit the game exited cleanly. Triage: median game speed 100%, 28 seconds below target (lowest 74%), 8 hitches, worst frame 106 ms. Chris confirmed the window was visible and accepted proceeding without further validation. Forest Haven walking/camera and visual correctness were not verified. |
| b. The camera standing still | Skipped at Chris's direction to assume it is fine and proceed; no controller, idle-camera dip, ledge or door check verified. |
| c. Smooth Motion with the HD look | Skipped at Chris's direction; clouds, water, shadows and swimming with both options on remain unverified. |
| d. Dragon Roost and saving | Skipped at Chris's direction; a valid earlier card was copied and prepared, but Dragon Roost map, F5/F8, saving, reload and clean second launch remain unverified. Real AppData saves were not changed. |
| e. The menu and the new controller options | Skipped at Chris's direction; controller navigation, background input and separate stick aiming remain unverified. F1 menu opening and Quit the game were checked with desktop input. |
| Zip SHA-256, uploaded to the v0.9.0 draft | `3c0edc36241fc1783693e1a9fa942995f4f32efec14fb3a9afc889a595da61ce`; Windows ZIP uploaded, GitHub digest matches. Release remains a draft with the existing IPA, source ZIP and PadMint recipe unchanged. |


Local `scripts/audit_repo.sh` and `git diff --check` passed. The freeze commit's Windows source-only host, repository audit, attribution, Linux host/AppImage and game-thread sanitizer workflows passed. CI does not establish hardware gameplay acceptance. Logs, disc, test cards and training data remain on this PC. The Mac release content gate remains required before publication.

Packaging produced 35 files and passed its allowlist and DLL import checks. ZIP integrity, unique members, private-file exclusions, required binaries, freeze provenance and module hash were checked independently. Only the Windows ZIP was uploaded; the locally generated source ZIP and SHA256SUMS stayed on this PC. The portable test folder and logs were retained privately outside the build app folder before packaging.

Chris accepted proceeding with unverified gameplay checks on October 24, 2026 ("let sjust assume its fine"). This acceptance is not a gameplay pass. The external `release_gate.py` is absent from both Windows and WSL homes: the Mac must verify the Windows ZIP hash and run `scripts/release/check_public_assets.sh` before publication, accepting only `containsTranslatedGameCode: true`. No release was published, no tag was created, and no issue comments were posted.
