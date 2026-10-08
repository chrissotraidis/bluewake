# Windows build, October 5, 2026

The first BlueWake Windows build from `main` is done and packaged as a **draft** release,
[`v0.5.0-windows`](https://github.com/chrissotraidis/bluewake/releases) ("BlueWake 0.5.0 for Windows (beta)"),
with `BlueWake-v0.5.0-windows-x64.zip`, `BlueWake-v0.5.0-source.zip` and `SHA256SUMS`. Chris still has to run the
release gate and publish it. The packaging script is in #119.

**Machine:** Chris's mini PC, Windows 11 Pro (build 26300), AMD Ryzen 7 5700U (Zen 2, 8 cores, AVX2), AMD Radeon
integrated graphics (driver 31.0.12027.9001), 12.9 GB RAM, about 180 GB free on C:. Tools: Visual Studio Build
Tools 2026 (18.10.2) with clang 22.1.3, CMake 4.4.4, Ninja 1.13.2, Python 3.12.

**Build:** `main` at `c56d6b6`, RecompCore `0b86924`, generated source digest `54f54434…` (the verified tree),
default optimizations, mods, local training (425 of 813 translated functions ran). Module SHA-256 `884559f8…`.
It took **132 minutes** in one run: 57 min of training playbacks and 58 min for the optimized compile. With only
about 2.5 GB of RAM free, the builder picked 1 compile job, so I restarted it at the start with `--jobs 3`.
No chunk ran out of memory.

## Checks (one try each, scripted pad input, `portable.txt`, a fresh card)

| Check | Result |
| --- | --- |
| Launch, title screen, into the game | Pass. The scripted run named Link, played the intro and reached player control on Outset. |
| Session log `Device:` line | Pass: `Device: AMD Radeon(TM) Graphics (IntegratedGPU)`, D3D12 |
| #64 portable mode | **Partial.** `data=...\BlueWake\user\`, and the card, `sram.bin`, settings and logs land there. But Aurora still creates `%APPDATA%\BlueWake` with `dawn_cache.db`, `pipeline_cache.db` and `imgui.ini`. |
| #79 Smooth Motion off by default | Pass: `[smooth-motion] off in_between=0 target_fps=30`. The "paused" counter wasn't tested. |
| #71 Jump and Run off by default | Pass for the default only (`movement_extras=false`, no override on a fresh install). Not pressed live. |
| `[music-stream]` line (#65, #97) | Pass: `path="Audiores/Stream/1tale.afc" ... state=4` |
| `[demo]` lines (#65, #97) | Pass, see below |
| #13 Pictobox, #55 swaps, #73 water camera, #66 remapping | Not testable here: no save past Outset, no controller, and no live input in this session |
| Package smoke test | Pass. Unpacked to a new folder with `portable.txt`, picked the ISO in the picker, it was prepared under `user\`, and the game reached the title screen and took START. |

## The three bug checks

- **Exact sound (#58): no crash.** With `lle_audio=1` in `settings.ini` and a restart, the log shows
  `[dsp-lle] authentic DSPCore shadow route enabled`. It ran 100 seconds through boot and the title to the
  scripted stop: `[audio] summary pushes=399872 dropped=0 dropped_frames=0 starved=144 stretched=13353`,
  `[perf-summary] exit minutes=1.7 ... lowest_speed=100%`. I didn't get into gameplay with it on. (My first try
  didn't count: the settings file wasn't written.)
- **Intro cutscene sound (#65, #97): plays.**
  `[demo] end stage=sea event=38 frames=3150 cues=4 sounds=4 missing=0 silent=0.4s of 105.2s`, the same as the
  Mac (`cues=4 sounds=4 missing=0 silent=0.4s of 104.7s`). There were no `[audio-lost]` or `[demo-sound]` lines.
- **Dungeon map (#74): not tested.** There's no save copy (`C:\BlueWake-private\GZLE01.card` doesn't exist).

**Triage** (`scripts/triage_session_log.py`, run 1, 7 min): 41 of 170 watched seconds below target, median
speed 89%, lowest 38%, all `game thread`, 39 of them at sea room 44 (Outset). 2 cutscenes with no missing
sound, 0 `[audio-lost]`, 21 hitches with the worst frame at 204 ms. The opening itself ran at 30 FPS.

**Not tested:** gameplay beyond Outset, controllers, the settings menu by hand, Smooth Motion on, HD textures,
`.rvz` discs, other GPUs, and long sessions.

## Suggested replies (Chris)

- **#13, #55, #66, #71, #73, #79:** "This fix is in BlueWake 0.5.0 for Windows, which is up on the Releases
  page now. It's the first Windows build from BlueWake's own code. Could you try it and tell me whether it's fixed
  for you? If not, please attach the newest `session-*.log` from `%APPDATA%\BlueWake\logs`."
- **#64:** "Portable mode is in 0.5.0 for Windows: with `portable.txt` beside `BlueWake.exe`, saves, settings and
  logs go to the `user` folder. The graphics caches still land in `%APPDATA%\BlueWake`, so I'm leaving this
  open until that's fixed too."
- **#58:** "I turned on Exact sound in 0.5.0 for Windows and restarted, and it started and played the opening
  without crashing on my PC. Could you try 0.5.0? If it still crashes, please attach the session log and the
  `crash-*.log` beside it."
- **#65, #97:** "On my Windows PC, 0.5.0 plays the intro after naming Link with all four sounds and the music.
  0.4.0 had Smooth Motion and native math on by default, and 0.5.0 turns both off. Could you try the same scene
  in 0.5.0 and send the log if the sound is still missing?"

## For the Mac agent (shared code)

- **Portable mode (#64) leaks Aurora's caches to `%APPDATA%\BlueWake`.** GXRuntime's Aurora backend never sets
  `AuroraConfig.userPath`, so Aurora uses its default folder for `dawn_cache.db`, `pipeline_cache.db` and
  `imgui.ini`. Passing the host's data folder through (RecompCore) would fix it. That folder didn't exist on this
  PC before the test, and holds only caches.
- `scripts/triage_session_log.py` counts the startup line `[crash] reports=...crash-NNNN.log` as a "fatal line"
  in every session, so every log reports one fatal line.
- Every exit logs `[aurora:warning:aurora::gpu] Device lost: Device was destroyed.`, which looks harmless.

## For Windows follow-ups

- The release leaves out `nodtool.exe`, but the first-launch disc prompt (`windows/src/win_disc.c`) still
  offers `.rvz`. It should say ".iso or .gcm", or point players to Dolphin's Convert File.
- `default_jobs()` in `scripts/windows/build.py` picks 1 job on a 13 GB PC with other apps open. Three jobs
  worked fine.
