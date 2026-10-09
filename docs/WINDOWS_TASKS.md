# Windows tasks

Work that needs a Windows PC, in priority order. Elliott owns Windows; anyone with the hardware can help.
Read [AGENTS.md](../AGENTS.md) first. One pull request per task, results in the linked issue.

**You need:** Windows 10 or 11 (x64), a CPU with AVX2, a Direct3D 12 GPU, your own USA `GZLE01` revision 0
disc, and the build tools in [BlueWake on Windows](WINDOWS.md#what-you-need).

**Where things stand (October 5, 2026):** BlueWake 0.5.0 is the published Windows download, built from
`main` (`0d1f821`) and checked on one PC: [Windows build for 0.5.0](status/WINDOWS_BUILD_0.5.0.md). Everything
in the table below is in it. Still to do by hand: the camera-stick invert, "Quit the game" and the option
notes, mouse buttons, and a controller.

## Shipped in 0.5.0: checks and remaining verification

These changes shipped in 0.5.0. Results below describe the original release checks; reporter updates
and remaining defects are tracked in [TECH_DEBT.md](TECH_DEBT.md). New unreleased changes belong in
"In `main`, waiting for a Windows build" below.

| Change | Issue | What to check | Windows result, October 5 |
| --- | --- | --- | --- |
| Pictobox photos no longer freeze the picture | #13 | Take a Pictobox photo; the game keeps drawing. | Not tested |
| Swap A and B, Swap X and Y | #55 | F1 › Controls; the swap takes effect at once. | Not tested |
| Camera no longer flips direction in water | #73 | Swim and turn the camera with the stick and the mouse. | Not tested |
| Controller button remapping | #66 | F1 › Controls › Controller buttons: change a button, confirm the game follows it, restart, confirm it was kept. | Not tested (no controller) |
| Portable mode: `portable.txt` beside `BlueWake.exe` keeps saves, settings and logs in a `user` folder beside it | #64 | With `portable.txt`, the log's `[windows] ... data=` line points to the `user` folder and saves land there; without it, `%APPDATA%\BlueWake` as before. | Partial: Aurora's caches and `imgui.ini` still go to `%APPDATA%\BlueWake` |
| Jump and Run off by default (0.4.0 has them always on) | #71 | A new install has no jump on Space or the left bumper; F1 › Mods › Jump and Run turns both on after a restart. | Default off; not pressed live |
| Smooth Motion off by default, and the "Smooth Motion paused" counter | #79 | A new install runs at 30 FPS; turning Smooth Motion on shows the counter when it pauses. | 30 FPS by default; counter not tested |
| `[music-stream]` log line | #65, #97 | A session log shows the line when the intro music starts. | Pass |
| Cutscene sound log: `[demo]`, `[demo-sound]`, `[audio-lost]` | #65, #97 | Play to the first cutscene; the log has a `[demo] end` line with `cues`, `sounds` and `missing`. | Pass: intro `cues=4 sounds=4 missing=0` |
| "Quit the game" in the settings menu; notes under Brisk Sail, Unrestricted boat and Invert camera left and right | Discord | F1: "Quit the game" next to Close ends BlueWake (and the next launch shows no recovery message); with a controller, the d-pad reaches it. F1 › Mods: the three options have a line saying what they do. | Not tested on 0.5.0 (F1 opens; menu clicks didn't reach the window in the test session) |
| "Exact" sound no longer crashes at launch (fixed October 1, after 0.4.0) | #58 | F1 › Sound and files › Exact, restart: the game starts and plays sound. | Pass: started and ran with Exact on, no crash |
| A launch that crashes is recovered: the next launch starts with Fast sound and mods off, backs up `settings.ini` and says so in plain words | #58 | Turn on a mod, end `BlueWake.exe` in Task Manager within a few seconds of starting, start it again: the message shows, a `settings.ini.before-safe-mode-*` file is beside `settings.ini`, and the next normal launch has no message. | Pass on 0.5.0: message, backup with the mod on, no message next launch |
| The camera-stick inversion now comes from a header shared with the Mac (no change intended on Windows) | #73 | F1 › Controls › "camera stick left and right inverted" on: the camera turns the same inverted way on land, swimming and on the boat. | Not tested (no controller) |
| Mouse buttons and keyboard keys can be changed | Discord | F1 › Controls › Mouse buttons: set the right button to B; click the game, right-click, and Link uses his sword. Keyboard keys for the GameCube buttons: give A another key; it works in game, J no longer does, and both survive a restart (`mouse_buttons=` and `key_map=` in `settings.ini`). | Keys pass on 0.5.0 (L as A, J off); mouse buttons and the menu by hand not tested |
| `gamecontrollerdb.txt` beside the saves adds controller mappings | #61 | Put the SDL_GameControllerDB file in `%APPDATA%\BlueWake`; the session log has `[pad] N controller mappings from ...` with N above 0, and a controller that worked before still works. | Partial on 0.5.0: `[pad] 578 controller mappings`; no controller to confirm |
| Older CPUs get a message instead of nothing | #77 | Task 2 below: `sde64 -nhm -- BlueWake.exe` shows "BlueWake can't run on this processor"; a normal launch is unchanged. | Pass on 0.5.0 under SDE `-nhm`; `-hsw` not run |
| The first-launch disc prompt offers .rvz only when `nodtool.exe` is beside `BlueWake.exe` | #120 | From the release zip (no `nodtool.exe`), with no disc chosen yet: the message says .iso or .gcm and how to convert an .rvz in Dolphin, and the picker lists only *.iso and *.gcm. A self-built folder still offers .rvz. | Pass on 0.5.0 from the zip; self-built folder not checked |
| Portable mode keeps Aurora's shader and pipeline caches in the `user` folder too | #64 | With `portable.txt` and no `%APPDATA%\BlueWake` folder before: after a run, `dawn_cache.db` and `pipeline_cache.db` are in `user`. `%APPDATA%\BlueWake` may still appear holding only `imgui.ini` (needs a RecompCore change). Without `portable.txt`, the caches stay in `%APPDATA%\BlueWake` as before. | Pass on 0.5.0; only `imgui.ini` in `%APPDATA%\BlueWake` |
| Molgera's arena in the Wind Temple has its sand floor (it was black) | #126 | Go through the boss door in the Wind Temple, or load a copy of a save moved there with `scripts/save_set_restart.py IN.gci OUT.gci kazeB 0 0`: the floor is sand, and the session log's `[gx-core] shutdown` line has `array_unresolved=0`. Elsewhere looks as before. | Pass on 0.5.0: sand floor, `array_unresolved=0` |


### The second Windows run (added October 5)

If the first BlueWake Windows build was made before these merged (from `c00397e` on, October 5),
rebuild from `main` and check only these rows of the table above, nothing else again (the 0.5.0 draft, built from `c56d6b6`, has none of them; Exact sound was already checked on it):

1. A launch that crashes is recovered, with the plain message (#58).
2. The camera-stick inversion from the shared header (#73).
3. Mouse buttons and keyboard keys can be changed.
4. "Quit the game" and the three option notes.
5. Older CPUs get a message instead of nothing (#77, task 2).
6. `gamecontrollerdb.txt` beside the saves adds controller mappings (#61).
7. The disc prompt offers .rvz only when `nodtool.exe` is there.
8. Portable mode keeps the shader caches in the `user` folder (#64).
9. Molgera's arena has its sand floor (#126).

Results on the 0.5.0 build (`0d1f821`): [Windows build for 0.5.0](status/WINDOWS_BUILD_0.5.0.md). Still to do by hand: items 2, 4, the mouse half of 3, and a controller for 6.

## In `main`, waiting for a Windows build

After 0.5.0: check these on the next Windows build.

For intro audio (#97), wait at the title until its music plays before starting a new scratch file.
The immediate-entry route can miss the failure. The Mac reproduction and callback-order diagnosis
are in [the October 6 record](status/TRIAGE_2026-10-06.md#title-to-intro-reproduction-and-callback-ordering).
The optional automated equivalent uses `BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1`,
`BLUEWAKE_PAD_BUTTONS=0x0100`, `BLUEWAKE_PAD_TITLE_DELAY=900` and `BLUEWAKE_MAX_RETRACES=3600`
with separate scratch card/settings/SRAM/cache paths. `[music-dvd]` reports the first four stream
read completions. This is a reproduction aid, not an audio fix; preserve the session log.

Merged after `0d1f821`, so 0.5.0 doesn't have them. Check only these on the next build:

| Change | Issue | What to check | Result |
| --- | --- | --- | --- |
| Opt-in cache-flush fallback for stale Pictobox preview | #13 | Set `$env:BLUEWAKE_CACHE_FLUSH_FALLBACK="1"` and use a copied save. Take a photo, cancel, turn toward a different view and take another without changing rooms. Both regular and Deluxe previews must show the new view; save/reload one photo. Check left-stick selection separately. For a short diagnostic only, set `DOL_GXCORE_TEX_VERIFY=1` and retain the log. Remove both variables afterward. | Matched Mac off/on runs reproduce/repair the second-photo failure. Off by default; iPad and Windows gameplay untested. [Evidence](status/PICTOBOX_CACHE_2026-10-06.md). |
| Deferred DVD completion for title-to-intro music: on by default on Mac, iPhone and iPad since October 7; Windows still opt-in | #97, related #65 | Launch from PowerShell with `$env:BLUEWAKE_DEFER_DVD_COMPLETION="1"`; wait for the title music, start a new scratch file and listen to the history intro without skipping. Check one later music change, room loading and a copied save-state reload. Keep the session log; it says `[dvd] deferred completion=on (BLUEWAKE_DEFER_DVD_COMPLETION)`. Then without the variable (`off (default)`) for comparison. If it passes, turn it on by default for Windows in `main.c`. | Mac: intro reproduced and repaired with captured audio, save-state reload, Windfall file and room loading, and the Builder's training route (October 6 and 7); physical iPad intro output passes. Windows untested. See [candidate evidence](status/TRIAGE_2026-10-06.md#deferred-completion-candidate). |
| Stream-state diagnostics include stop/play, decode/buffer and DVD state; the triage script flags brief playback | #65, #97 | Play the silent history intro once without skipping; keep the log through the track stopping. Run `python scripts/triage_session_log.py LOG`. Compare `[music-stream]` state changes and the new fields; a state-4 sample alone is not acceptance. | Pending Windows hardware; Mac build and targeted diagnostics checks recorded in [October 6 triage](status/TRIAGE_2026-10-06.md) |
| A controller recognized late, or left as player 2, plays as player 1 | #61 | With a controller SDL only knows from `gamecontrollerdb.txt` (in `%APPDATA%\BlueWake`), plugged in before starting: it moves Link. The session log has `[pad] 'NAME' is player 1 (it had no player slot)`. With two controllers, unplug the first; the second takes over (`... (it was player 2)`). An Xbox controller alone behaves as before. | Not built yet |
| The Wind Waker baton is no longer mirrored | #156 | With a controller and the fast right-stick camera on (the default): take out the Wind Waker and conduct a song; right on the C-stick is right. Swimming and sailing still turn the camera the stick's way (#73). | Not built yet |
| The builder works with Visual Studio 2022's clang | #153 | Build from `main` with Visual Studio 2022 (clang 19): the output says `note: building the app without its optimization profile (... unsupported instrumentation profile format version)` and the build finishes. With Visual Studio 2026 18.10 or newer there is no note. | Checked on a Mac with LLVM 18, 20 and 22; Windows untested |
| Option: left stick's up and down inverted when aiming (first person, items) | #154 | F1 › Controls › “Left stick up and down inverted when aiming” on: in first person and with the grappling hook, bow or boomerang, pushing the left stick up aims up. Walking is unchanged, and the setting is kept after a restart (`aim_invert_y=1` in `settings.ini`). Off by default. | Not built yet |
| Left stick: the game's own dead zone, no jump, full tilt at full travel (RecompCore patch 0159) | #138 | With an Xbox or other controller: tilt the left stick slowly from the center. Link starts to walk smoothly, with no sudden jump; full run comes near the end of the stick's travel; small diagonals aren't snapped to straight lines; Link stands still with the stick at rest. The session log has `[pad] player 1: the game's own stick dead zone`. | Not built yet |
| Dungeon maps draw their grid and rooms (eight texgens, sixteen TEV stages; RecompCore patch 0157) | #74 | Open the map in any dungeon you've reached (or a copy of a save moved into Dragon Roost Cavern with `scripts/save_set_restart.py IN.gci OUT.gci M_NewD2 0 0`): the grid and the rooms you've seen are drawn, not only the door marker. The session log's `[gx-core] shutdown` line has `unsupported_texgen=0` and `tev_stages_over=0`. A first launch logs `Seeded pipeline cache` and little shader compiling (`pipelines_made` in `[perf-summary]`) in places played before. | Not built yet |
| Portable mode keeps controller remaps, keyboard bindings and `imgui.ini` in the `user` folder (RecompCore patch 0160) | #64 | With `portable.txt` beside `BlueWake.exe`: remap a controller button and change a keyboard key, quit, start again. The remaps are kept, the `user` folder has `imgui.ini` and a `*.controller` file, and no new `imgui.ini` or `*.controller` appears in `%APPDATA%\BlueWake`. If `%APPDATA%\BlueWake` already had remaps, the first portable launch logs `[portable] copied ...` and keeps them. Without `portable.txt`, remaps made before still work. | Not built yet |
| The window opens in place: centred the first time, then where you left it, without appearing first and then jumping (RecompCore patch 0161) | #89 | Start BlueWake windowed: the window appears once, centred. Move it, quit, start again: it opens where you left it. Move it to a second monitor, quit, unplug that monitor, start: it opens centred, and the log says `[windows] window centred: ... is no longer on a monitor`. Fullscreen still starts fullscreen on its display. | Not built yet |

### Flickering capture (#136)

The NVIDIA report is still unreproduced on Mac. Use a copy of the affected save and keep the same
camera, resolution and original textures. Record the exact BlueWake version/commit, GPU and driver;
do not delete caches or player data. This is a short comparison, not an hours-long soak:

1. Record roughly ten seconds at original 30 FPS, then the same view at Smooth Motion 60 and 120.
   Note exactly which cloud/wave disappears and whether the camera is moving. Keep the session log.
2. Return to that view once with the existing caches warmed. If the flicker disappears, inspect the
   D3D12 fallback/specialized-shader transition; if it persists, prioritize matching, UV and colour
   interpolation. A cold/warm difference is evidence, not a cause by itself.
3. Only after finding a repeatable bad interval, use the existing renderer dump with a small range:
   `DOL_AURORA_FRAME_INTERP_DUMP` names a private output directory; `DOL_AURORA_FRAME_INTERP_DUMP_FROM`
   and `DOL_AURORA_FRAME_INTERP_DUMP_TO` must both be set, preferably for no more than 30 frames.
   Read frame numbers from a short run with `DOL_AURORA_FRAME_INTERP_LOG_FRAMES=1`. The dump is
   inactive at 30 FPS; regular host screenshots are needed for that control. Keep original images
   private for analysis and ask before publishing promotional footage.
4. Compare real and intermediate frames. If only intermediate frames fail, use
   `DOL_AURORA_FRAME_INTERP_TRACE=first-last` for that narrow range to identify the draw's match or
   rejection. If real frames fail too, investigate transform/texture/depth or backend behavior.
   Remove diagnostic environment variables after the run; do not use a traced run to claim speed.

No rendering change is proposed from the current Mac sample. [Exact findings and limits](status/TRIAGE_2026-10-06.md#matched-mac-capture-follow-up).

## 1. A Windows build from BlueWake `main`

**Why:** ships everything in the list above to Windows players.

1. Build from a clean checkout of `main`: `python scripts/windows/build.py "D:\path\to\GZLE01.iso"`.
2. Run checks 1 to 4 of the [Windows checklist](WINDOWS_ACCEPTANCE.md), and the checks in the list above.
3. Package it like Wind Waker Recomp's releases (its `scripts/windows/package_release.py` is a starting
   point; bring it over as a pull request): `BlueWake-vX.Y.Z-windows-x64.zip` holding the build folder,
   licenses and a `BuilderProvenance.json`, **without `nodtool.exe`** (it embeds Wii keys), plus a source zip.
4. Attach both to a **draft** release on BlueWake. Chris runs the release check and publishes; nobody else
   publishes releases.

**Done when:** the draft has both zips and the checklist results are posted.

**Status (October 5, 2026):** done. 0.5.0 was built from `0d1f821`, packaged with
`scripts/windows/package_release.py` (#119) and published with the Mac, iPhone and iPad files. The checks in
the table above that need a controller or the settings menu by hand are still open.

## 2. A clear message on CPUs without AVX2 (#77)

**Why:** on an older CPU, such as an Intel Core i7 860, `BlueWake.exe` exits silently.

**In `main` (October 5):** `windows/src/win_entry.c` checks the CPU from the C runtime's initializer table
(`.CRT$XIU`, before the C++ static initializers and `main`), in functions compiled for plain x86-64, and shows a
message box instead of stopping silently. Only builds for x86-64-v3 include it. On the Mac, the same code
compiled for Windows showed the entry in `.CRT$XIU` and no AVX instructions in the check; it hasn't run on Windows.

**Done when:** under Intel SDE emulating an older CPU (`sde64 -nhm -- BlueWake.exe`), the message appears
instead of nothing, and a normal launch (and `sde64 -hsw -- BlueWake.exe`) is unchanged.

## 3. Opening the window in place (PR #89)

Done in `main` (RecompCore patch 0161, saulob's change); its Windows check is in the table above.

## 4. Elliott's native functions and lean memory

**Why:** `--native-entries` and `--lean-memory` are in BlueWake's Windows builder but change nothing yet.
The natives hook only where the translated code matches what Wind Waker Recomp's builder produces (0 of 15
match today), and lean memory needs the deadline test in Wind Waker Recomp's newer `fast_blocks.py`.

1. Done October 9: `--lean-blocks` brings Elliott's original copies back as an option.
2. Build with `--lean-blocks` and compare speed with the default build in the same states
   ([PERFORMANCE.md](PERFORMANCE.md), phase 4, step 3). Then add `--lean-memory`, then `--native-entries` and
   check its log for how many certify.

**Done when:** the natives certify, the speedup is measured, and the defaults are decided from the numbers.

## 5. Windows-only reports

Use `python3 scripts/triage_session_log.py session-*.log` on any attached log.

| Issue | What to do |
| --- | --- |
| #80 HD pack shading on AMD | Try the Hypatia DDS pack on Windows, with and without it; compare with the PNG pack. |
| #61 8BitDo GameCube controller | Check whether SDL sees it and what it maps to. |
| #76 Forsaken Fortress soft lock | Try to reproduce on the tower with the Moblins; it may be the original game's behaviour. |
| #74 dungeon map without its drawing | Not Windows-only: reproduced on the Mac on October 5 (see [OPEN_ISSUES_2026-10-05.md](status/OPEN_ISSUES_2026-10-05.md#what-the-october-5-loop-found)). The cause is RecompCore's shader limits, so the fix is a runtime change; nothing to capture on Windows until it lands. |
| #65, #97 missing music or sound in cutscenes | Still reported on Windows 0.5.0 and Mac M4. The Windows log shows `1tale.afc` in playing state for two retraces, then idle. Check the history intro after naming Link separately from the title demo and bird scene. A cue count or one state=4 line does not prove sustained audible music. See [TECH_DEBT.md](TECH_DEBT.md). |
| #59, #72, #79, #86 slowdowns | Measure the scenes with the triage script. The #76 log already shows the Forsaken Fortress exterior limited by the GX worker (83 of 97 slow seconds). |

Close an issue only when the reporter confirms the fix, or with a clear explanation.

### Reading the cutscene sound lines (#65, #97)

Every build writes one `[demo] end` line per cutscene, and `[audio-lost]` when a cutscene, or the intro
story's streamed music, goes silent for 6 seconds.

| What the log shows | What it means | Where to look |
| --- | --- | --- |
| `cues=0` on a cutscene that should have sound | The cutscene never asked for its sounds: its sound track didn't run. | Scene/cue dispatch and timing. Record the actual `[chassis]` settings: Windows 0.5.0 enables supported native accelerators by default; the Mac comparison can have them off. |
| `missing` above 0, with `[demo-sound] ... no sound` lines | The cutscene asked, but the game couldn't start the sound, usually because its sound data wasn't loaded in time. | Slow disc or ARAM reads; compare the slow seconds (`[fps-dip]`) around the cue. |
| `sounds` equal to `cues` but `[audio-lost]` or a long `silent=` | The sounds started but nothing reached the speakers. | The audio output: Smooth Motion (on by default in 0.4.0, off in `main`), and drops when the game falls behind real time. |
| `[music-stream]` reaches `state=4` then quickly returns to idle | The track started but stopped early; the reason is not established by this line. | Track lifetime, explicit stop requests, stream reads/decoder and scene changes. |
| No `[music-stream]` line with `state=4` during the intro | The streamed music never started playing. | Reading `Audiores/Stream/*.afc` from the disc image. |

On Mac, iPad and Windows `main`, the audio pacing defaults are the same (`BLUEWAKE_WALL_PACE=1`,
`DOL_AUDIO_NO_THROTTLE=1`, `BLUEWAKE_CLOCK=now`), but that does not establish identical execution.
The affected Windows 0.5.0 log has the native accelerators on, while the bounded Mac comparison has
them off. The experimental 60 Hz gameplay option is absent, and Smooth Motion is off by default.
Record actual settings and module capabilities before comparing platforms; none of these differences
is established as the cause of the stream ending early.
