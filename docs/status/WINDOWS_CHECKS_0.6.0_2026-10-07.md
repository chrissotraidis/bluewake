# Windows checks for 0.6.0 on a second PC (October 7, 2026)

I ran the checks from [WINDOWS_BUILD_0.6.0.md](WINDOWS_BUILD_0.6.0.md) on my own Windows PC, from my own disc,
on the same commit as the published 0.6.0 Windows download. The hand-off run on Chris's PC skipped checks b,
c and d and had no listening confirmation; this run covers them, by ear and from the logs. I also played five
cutscenes from test saves. My build was not uploaded anywhere; the published zip is Chris's.

Logs, saves and the build stay on my PC. Lines below are quoted from `scripts/triage_session_log.py` and
the session logs.

## Build

| | |
| --- | --- |
| Device | Intel Core i5-12600KF, NVIDIA GeForce RTX 5070 Ti (D3D12), Windows build 26200 |
| Commit | `4eb41f0d30e0c2b1694b23082b94411327a2d809` (#172, the published 0.6.0); RecompCore `35e037f` |
| Build | `python scripts\windows\build.py DISC`, 52 min (the disc files and translation were reused from an earlier build; training and the module were redone); clang 22.1.3; optimization profile used |
| Module SHA-256 | `4b2d7dc289ab6b5f1addce5f6b5ba14d5efce31f7d54569cf6bb96bdc3900d6c` |
| Packaged zip | `c7e2cb387481667de4877963dcdea00c0e112d7251d4faae8c554d50f12bcf93` (33 files, no `user`, card, `portable.txt` or `nodtool.exe`); not uploaded |
| Controllers | A PowerA and a PDP wired controller; both report as `Xbox One Controller` (`vid 045e, pid 02ff`) |

All runs were in portable mode, started from PowerShell with `.\BlueWake.exe`.

## The release checks

| # | Check | Result |
| --- | --- | --- |
| a | Intro music, default | **Pass.** I heard music through the whole history intro without skipping. `[dvd] deferred completion=on (default)`; `1tale.afc` played from retrace 4440 to 17381 and decoded all 6,900,928 samples; `streamed playback ended within 60 retraces: 0`; no fatal lines. Quit from the settings menu with the controller (Select, then Quit the game): `[run] stopped: quit`, empty crash report. |
| b | Intro, fix off (`BLUEWAKE_DEFER_DVD_COMPLETION=0`) | **Silent, as expected.** No music under the scrolls. `deferred completion=off (BLUEWAKE_DEFER_DVD_COMPLETION)`; `1tale.afc: playing for 2 retraces, then state=0` with `dvd_pending=1`, the 0.5.0 signature. With a and b, the fix is what brings the music back on Windows. |
| c | Load, dungeon map, save, quit | **Pass, except the door.** Test card from my own card with `scripts/card_set_restart.py ... M_NewD2 0 0`. Dragon Roost Cavern loaded; the map showed its grid and rooms; `unsupported_texgen=0`, `tev_stages_over=0`, `array_unresolved=0`. I saved, quit from the menu and loaded the save again in a new launch: no stall, no recovery message (no `settings.ini.before-safe-mode-*` file), clean quits. One slow second on the first load while shaders compiled (`cause=shader-compile`). I did not go through a door. |
| d | Controller | **Each row passes once the stick fix is on, but it is not on for a controller connected at launch: see below.** Baton (#156): right is right. Aim option (#154): the left stick inverts as described, off again after. Player 1 (#61): with A and B plugged in, unplugging A gave B control at once (`'Xbox One Controller' is player 1 (it was player 2)`), and plugging A back in left B in control. Left stick (#138), after replugging: no jump from the center, full run near the end of the travel, no drift at rest; aiming with items, and with the shield most of all, is much better. Not tested: a controller known only through `gamecontrollerdb.txt`. |

## Found: the controller changes skip a controller connected at launch

The left-stick dead zone (#138), giving player 1 to a waiting controller (#61) and the `[pad] mapping in use`
line run only from the controller-added event in `runtime/host/src/mouse_camera.c` (`observe`). Aurora adds a
controller that is already connected while it starts, before BlueWake installs that observer, so none of them
run for it. In every one of my launches:

```
22:09:06.015 [aurora:info:aurora::input] Added controller 'Xbox One Controller' (instance 3, ...)
22:09:06.033 [pad] platform input initialized; live input merged at SI        <- no [pad] lines for it
22:10:34.916 [aurora:info:aurora::input] Added controller 'Xbox One Controller' (instance 4, ...)   <- unplugged and plugged back in
22:10:34.916 [pad] mapping in use: 0300fa675e040000ff02000000007801,...
22:10:34.917 [pad] player 1: the game's own stick dead zone
```

The difference in play is plain: with the controller connected at launch the stick has the old dead zone and
jump, and after replugging it doesn't. Plugging in any second controller also turns it on. Wired Xbox One
controllers that must be switched on after plugging in get the fix only if they are switched on after
BlueWake starts. The code is shared with the Mac, so it may behave the same there; I only saw it on Windows.
This is in the published 0.6.0. A fix would apply the same three steps to the controllers already connected
once the observer is installed.

## Other observations

- **Rumble goes to every connected controller,** not only player 1 (`runtime/host/src/haptics.c`, `send`
  loops over all gamepads). With two plugged in, the idle one on the desk vibrated off it during the
  Forsaken Fortress launch. Low priority.
- **The right stick also aims** in first person and with items, always inverted up and down, and the new
  "Left stick up and down inverted when aiming" option does not change it. I don't know whether the
  original game does the same; recorded for a decision, not as a bug.

## Pictobox preview (#13), on the same build

From the list in [WINDOWS_TASKS.md](../WINDOWS_TASKS.md#in-main-waiting-for-a-windows-build). A test card from
my playthrough (Windfall dock, `sea 11 0`): quest log 1 with the regular Pictobox, quest log 2 the same save
with the Deluxe one.

| Run | Result |
| --- | --- |
| Without the variable (`[texture-cache] fallback writeback=off`) | **Bug reproduced:** after a cancelled photo, the second photo's preview shows the old view. |
| `BLUEWAKE_CACHE_FLUSH_FALLBACK=1` (`fallback writeback=on (experimental)`), regular and Deluxe | **Pass:** the second preview shows the new view on both. A kept photo is still there after saving, quitting and loading again. No fatal lines. |
| Choosing Yes at "keep this picture?" | **Fails with the left stick and the D-pad,** with the controller connected at launch and after power-cycling it. The keyboard's A and D keys work (left is Yes). The gallery behaves the same. |

With `BLUEWAKE_PAD_TRACE=1`, the game gets `stick=0,0` the whole time the left stick is held at the prompt,
and `stick=127,0` and `-127,0` from D and A. The cause looks like `runtime/host/src/mouse_camera.c:599-600`:
in a zooming view (Pictobox, telescope) the right-stick camera gives the zoom to the left stick's up and down
and zeroes the left stick for the game, so it doesn't also aim. That is still in effect while the prompt and
the gallery are open. This is knapman's report on #13 ("pressing left on the gamepad doesn't work so you
can't save"). I didn't change any code.

## The builder with Visual Studio 2022 (#153), on `main`

At `e019f1f`, with Visual Studio 2022 Build Tools (clang 19.1.5) beside 2026 (clang 22.1.3). The builder
always picks the newest Visual Studio, so for this check only I added one uncommitted line to a separate
worktree's `build.py` that kept just the 2022 install. Run with `--source-only --no-train` into a separate
`--out`:

- `Visual Studio: ...\2022\BuildTools`, clang 19.1.5.
- `note: building the app without its optimization profile (... unsupported instrumentation profile format version)`.
- The app configured without the profile (the step that failed before), and translation and source generation
  finished with `composite source digest 54f54434...: the verified tree`. Exit 0, 6 min 45 s.

Not checked: compiling the module and the app with clang 19, and running the result.

## Cutscenes on 0.6.0 (beyond the release checks)

In my own 0.5.0 playthrough no cutscene from the start to the credits had music or sound effects, only
ambience. Test saves made with my save editor (from my playthrough's saves; private, not committed) put
each scene a few steps away. Each was watched without skipping, fix on (the default).

| Scene | Heard | Log |
| --- | --- | --- |
| History intro (check a) | Music to the end | Above |
| The Helmaroc King carries Tetra, the catapult fires, she falls into the forest (`sea` event 251) | Everything: the catapult, the bird's wings and cry, Tetra falling into the leaves | `demo01.afc` played in full (decoded = playback samples); `[demo] end stage=sea event=251 frames=1384 cues=0 sounds=0 missing=0 silent=0.1s of 46.2s`. The reports in [the October 8 triage](TRIAGE_2026-10-08.md#the-bird-scene-one-cutscene-two-problems-65-59) have `silent=12.4s` here without the fix. The scene itself ran at full speed; two slow seconds (51%, 97%) came just before it, on arriving at sea. |
| Arriving at the Forsaken Fortress with the pirates | Everything | `MajyuE` event 69: `cues=4 sounds=4 missing=0 silent=0.0s of 145.6s`; `mj_demo1.afc`, `go_maju.afc` |
| Gohma, then Valoo and Komali after Dragon Roost | Everything | `btdstart.afc` and `b_clear.afc` reached playing state; `Adanmae` event 62: `cues=3 sounds=3 missing=0`; `sea` event 134: `cues=4 sounds=4 missing=0`. Gohma's room had 6 slow seconds, lowest 68%. |
| The Tower of the Gods rises (Din's Pearl, the statues) | Everything, including the statues' hum and the explosion, which were missing in my playthrough | `ADMumi` event 74: `cues=3 sounds=3 missing=0 silent=0.0s of 103.2s`; `demo19.afc`. Inside the tower the map drew (`unsupported_texgen=0`, `tev_stages_over=0`). |

No run had a fatal line, an `[audio-lost]` line or a stream that ended within 60 retraces. So on Windows,
0.6.0's fix brings back cutscene sound through the bird scene and beyond, not only the history intro. One
PC, one disc; the scenes were reached from test saves, not by playing from a new file.
