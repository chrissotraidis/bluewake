# Rendered testing with an agent

How to let a coding agent (Claude Code, Codex and the like) test BlueWake's rendering on its own: build, warp to a
place, play scripted input, capture or dump frames, time the GPU, compare builds and settings, and show you the
result as images and numbers. Everything here was used on Windows 11 (PowerShell, Direct3D 12) while tracking down
#491's flickering shadows and the ambient occlusion halo. The game-side switches are the same on Linux and Mac; the
window and launch tricks are Windows-specific.

The short version: **a save state, a pad script and a frame dump give you the same frames on every run, so two
builds or two settings can be compared pixel for pixel.** The rest of this page is how to get there and what trips
agents up.

## The two scripts

[`scripts/rendered_run.py`](../scripts/rendered_run.py) runs the app the same way each time, once or once a
variant, and keeps what it made: frame captures, frame dumps, its log, and what the log says about speed and GPU
time (`results.json`). Each run gets its own copy of the card, its own settings and data folder, and none of the
caller's `BLUEWAKE_`/`DOL_` variables. [`scripts/frame_compare.py`](../scripts/frame_compare.py) reads the frames
(it needs `pip install numpy pillow`). Both print their full usage with `--help`; the sections below are what they
set and why. A run that ends at start-up leaves the app's launch marker, and the next run in that folder would start
in safe mode (no HD look, no mods, 4:3): `rendered_run.py` clears the marker before each run, and marks a run that
still started in safe mode (SAFE MODE on its line, `safe_mode` in `results.json`).

**Flicker, `main` against a build.** From a save state, two sword swings, frames dumped at 60 FPS and render scale 2,
sun shadows on and off on each build, plus a debug run whose frames mark where the shadows are:

```
python3 scripts/rendered_run.py build/new/BlueWake/BlueWake.exe out/flicker --card my.card \
    --state trees.bwstate --pad 1885:0x0200:6,1915:0x0200:6 --max-retraces 2035 \
    --dump 0:80 --smooth 1 --scale 2 --set BLUEWAKE_HD_LOOK=1 --set BLUEWAKE_HD_SUN_SHADOWS=1 \
    --variant new --variant new-nosun BLUEWAKE_HD_SUN_SHADOWS=0 \
    --variant main APP=build/main/BlueWake/BlueWake.exe \
    --variant main-nosun APP=build/main/BlueWake/BlueWake.exe BLUEWAKE_HD_SUN_SHADOWS=0 \
    --variant mask DOL_AURORA_SUN_SHADOW_MAP_DEBUG=1 DOL_AURORA_AO_DEBUG=1 BLUEWAKE_HD_AO=0
python3 scripts/frame_compare.py ratio out/flicker/main out/flicker/main-nosun --mask out/flicker/mask
python3 scripts/frame_compare.py ratio out/flicker/new out/flicker/new-nosun --mask out/flicker/mask
python3 scripts/frame_compare.py blinks out/flicker/new --sheet blinks.png
python3 scripts/frame_compare.py sheet out/flicker/main out/flicker/new --frames 26,28,30 --out hit.png
```

`ratio`'s "full" column is shadowed ground against the same ground without shadows: 0.65 is one shadow (1 - 0.35
strength). On `main`, while the hit's motion blur was on, it went 0.51, 0.56, 0.44, 0.61, 0.41; with the fix it held
0.65.

**Benchmarking an effect.** The same state, paced, render scale 3, twice each, with Aurora's GPU time per zone:

```
python3 scripts/rendered_run.py build/new/BlueWake/BlueWake.exe out/bench --card my.card \
    --state trees.bwstate --max-retraces 2700 --scale 3 --gpu-prof --repeat 2 \
    --set BLUEWAKE_HD_LOOK=1 --variant ao-on --variant ao-off BLUEWAKE_HD_AO=0
```

prints each run and each variant's mean (game frames a second, busy, GPU busy time); `results.json` has every zone
(ambient occlusion: 0.114 and 0.122 ms a frame on an RTX 5070 Ti). Paced, game frames a second stop at 30, so compare
busy and GPU time; `--unpaced` measures speed instead. The standard speed benchmark at fixed stops is
[BENCHMARK_TOUR.md](BENCHMARK_TOUR.md) (`scripts/bench_tour.py`).

## Rules to give the agent first

[AGENTS.md](../AGENTS.md) applies; for rendered tests in particular:

- Never touch the real user folder (`%APPDATA%\BlueWake`, `~/.local/share/BlueWake`, ...). Every run gets its own
  folder with `BLUEWAKE_DATA_DIR`, and a **copy** of a card in it.
- Never commit or upload save states, cards, frame dumps, disc files or the game module. States hold the game's
  memory; dumps hold the game's pictures. Keep them in a scratch folder outside the repository.
- Capture the game's own window or its own frames, never the screen: a screen grab of a live desktop picks up
  whatever else is open (it did, once, for us).
- Ask before sending real keyboard or mouse input to the desktop (see "Menus" below).
- Don't patch the app just to get a screenshot. Use its test hooks and input paths, so what you test is what ships.

## Builds

- **Host-only over a trained build.** Renderer work (RecompCore/Aurora, `runtime/host/src`) needs
  `scripts/windows/build.py DISC --out <build> --host-only --jobs 16`: it keeps the last build's training and
  recompiles only what changed. A RecompCore change: commit it locally, put its hash in
  `scripts/builder/profiles/bluewake.sh` (`RECOMPCORE_SHA`), check `ref/recompcore` out at that commit, build. The
  builder refuses a `ref/recompcore` with uncommitted changes.
- **A second build to compare against.** Copy a whole build folder (robocopy is fine) to keep a baseline. CMake keeps
  absolute paths inside it, so a copy only builds under its original folder name: rename the original aside, rename
  the copy into its place, build, rename both back.
- **Surprise full rebuilds.** Checking `ref/recompcore` out at another commit gives every header that differs a new
  timestamp, and a header every game-module file includes (`GXRuntime/include/core/cpu.h`) recompiles all ~850
  chunks. If the header's content matches what the module was built against, set its timestamp back before the
  module's objects and the rebuild drops to the app.

## Launching without anyone at the PC

- The PC can be locked; it must not sleep. Rendered runs work from a locked session.
- Start long jobs detached so they don't steal focus or open empty console windows:

  ```powershell
  $si = New-CimInstance -ClassName Win32_ProcessStartup -ClientOnly -Property @{ShowWindow=[uint16]0}
  Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
      CommandLine = 'python scripts\rendered_run.py ... > C:\scratch\out\console.txt 2>&1'
      CurrentDirectory = 'C:\src\bluewake'; ProcessStartupInformation = $si }
  ```

- `rendered_run.py` writes `status.txt` in its output folder as each run starts and ends: let the agent wait on
  that file (a loop that ends when the run's own line appears), not with fixed sleeps.
- Writing your own launcher instead: `BlueWake.exe` is a GUI program, so PowerShell's `& BlueWake.exe` returns at
  once (use `Start-Process $app -Wait -PassThru`), and `powershell -File script.ps1 -List a,b` passes `a,b` as
  **one** string.

## Getting to a place

The stage select can start a quest log and go straight somewhere ([STAGE_SELECT.md](STAGE_SELECT.md#for-tests-and-benchmarks)):

| Variable | |
| --- | --- |
| `BLUEWAKE_STAGE_SELECT=1` | the stage select on |
| `BLUEWAKE_WARP=stage:room:point[:layer]` | `sea:44:8:0` is Outset's hilltop, `sea:11:0:1` Windfall's pier at night, `M_NewD2:0:0` Dragon Roost Cavern; names are in `config/stage_select/names.tsv` |
| `BLUEWAKE_WARP_FILE=1` | which quest log on the card copy to start (pick one without effects you don't want, e.g. before the Master Sword's glow) |
| `BLUEWAKE_WARP_TIME=noon` | `0`-`23`, `morning`, `noon`, `evening`, `night` |
| `BLUEWAKE_WARP_STOP_AFTER=600` | quit that many play frames after Link arrives (the exit code is then 1; that's normal) |

To find a good spot, sweep a room's spawn points (one short run each, one frame each) and tile the frames into one
image for the agent to look at.

## Save states

- `BLUEWAKE_SAVE_STATE=C:\scratch\states\spot.bwstate@1935` saves at retrace 1935 (several, comma-separated).
- `BLUEWAKE_LOAD_STATE=C:\scratch\states\spot.bwstate` starts from it.
- **A state is what makes runs repeatable.** From boot, the game's randomness differs run to run (grass drops a
  random item, cut pieces fly differently), so two "identical" runs aren't. From a state, every run is frame-exact:
  we checked, the sky differed by 0.00 between two runs.
- Loading restores the retrace count, so everything timed in retraces (`BLUEWAKE_MAX_RETRACES`, the pad script,
  captures) counts on from the state's retrace. A limit below it ends the run at once.
- Make the state just before the thing you want to see happen. A state "at the trees" saved after the walk's last
  swing had a tree already falling.

## Input

`BLUEWAKE_PAD_SCRIPT="retrace:buttons:length[:x:y[:cx:cy]],..."` presses GameCube buttons (hex: A `0x0100`, B
`0x0200`, X `0x0400`, Y `0x0800`, Start `0x1000`) for that many retraces, with the stick if given (`0:80` is forward).
It is added to what else is pressing, and it works after a state load. Example, two sword swings from a state saved
at 1875: `BLUEWAKE_PAD_SCRIPT=1885:0x0200:6,1915:0x0200:6`.

## Pictures

**Frame captures** (cheap, unpaced):

```
BLUEWAKE_CAPTURE_OPENING_FRAME=C:\scratch\run1\frame.ppm
BLUEWAKE_CAPTURE_RETRACE=1300
BLUEWAKE_CAPTURE_INTERVAL=60
BLUEWAKE_WALL_PACE=0
BLUEWAKE_MAX_RETRACES=3200
```

writes `frame-<retrace>.ppm` beside that path: the game's picture with its HUD, no settings menu. Unpaced runs take
seconds. Smooth Motion makes no in-between frames unpaced.

**Frame dumps** (every real and in-between frame, paced, big):

```
DOL_AURORA_FRAME_INTERP=1
DOL_AURORA_FRAME_INTERP_STEPS=3        # 1 = 60 FPS, 3 = 120 FPS, display = the display's rate
DOL_AURORA_FRAME_INTERP_DUMP=C:\scratch\dumps\run1
DOL_AURORA_FRAME_INTERP_DUMP_FROM=0
DOL_AURORA_FRAME_INTERP_DUMP_TO=260    # game frames
DOL_AURORA_RENDER_SCALE=2
```

Files are `frameNNNNNN-real.ppm` and `-between.ppm`, `-between2`, `-between3`. **Order on screen: frame N's
in-betweens come before real N** (they run from N-1 toward N). Get that wrong and every real frame looks like a
flicker. Size: about 0.9 MB an image at render scale 1 (640x480), 3.5 MB at scale 2; 260 game frames at 120 FPS is
about 1 GB at scale 1, 3.5 GB at scale 2.

**Comparing builds and settings:** run the same state and pad script with only one thing changed: `main` against your
build, render scale 1 against 2, Smooth Motion 60 against 120, an effect on against off (`BLUEWAKE_HD_LOOK`,
`BLUEWAKE_HD_SUN_SHADOWS`, `BLUEWAKE_HD_AO`, ...). Launch variables win over the settings file for the run.

## Finding flicker and other differences in the frames

Let the agent do the counting and only show you the frames that matter (`frame_compare.py` does each of these:
`blinks`, `ratio`, `track`, `diff`, `sheet`):

- **One-image blinks:** an image that differs from both neighbours in the same direction (by 40 or more of 255)
  while the neighbours match each other (within 12). Motion doesn't do that; flicker does. Count blink pixels per
  image, rank, and look at the top few with the blinks painted red.
- **What an effect does on its own:** divide a frame with the effect by the same frame without it (the runs must be
  from a state). With sun shadows, shadowed ground sat at exactly 0.65 of its unshadowed brightness (1 - 0.35
  strength) and lit ground at 1.000; during the camera shake it dropped to 0.41, below what one shadow can do, which
  is how the doubled shadows were found.
- **Split by a debug view:** a debug run from the same state gives a mask (fully shadowed, fully lit) to measure the
  real frames with.
- **A brightness track:** the mean of a region, image by image, shows a pulse at the game's 30 FPS at a glance.
- Before believing a difference, rule out what moves anyway: the sword's trail, cut pieces, smoke, swaying ground
  plants, the camera.

## Debug views

| Variable | Shows |
| --- | --- |
| `DOL_AURORA_AO_DEBUG=1` | ambient occlusion alone |
| `DOL_AURORA_SUN_SHADOW_MAP_DEBUG=1` | the shadow map's term (with `DOL_AURORA_AO_DEBUG=1` and `BLUEWAKE_HD_AO=0` it replaces the picture: black shadow, white lit) |
| `DOL_AURORA_SUN_SHADOW_MAP_DEBUG=6` | each shadow cascade tinted (red, green, blue, yellow) |
| `DOL_AURORA_FAR_BLUR_DEBUG=1` | distance as brightness (log2 of near-plane units over 24), the sky blue |

[HD_SUN_SHADOW_MAP.md](HD_SUN_SHADOW_MAP.md) lists the shadow map's other tools.

## Menus

- `BLUEWAKE_SETTINGS_TEST_OPEN=10:60` opens the settings menu 10 seconds in, for 60, on its first tab.
- Capture the window itself with Win32 `PrintWindow(hwnd, hdc, 2)` (full content); it works with other windows in
  front.
- Posted key and mouse messages don't reach the game (SDL takes input only with focus). To press keys, bring the
  window in front (press and release Alt with `keybd_event`, then `ShowWindow(hwnd, 9)` and `SetForegroundWindow`),
  then send keys with `keybd_event`. That is real input on the desktop for a few seconds: get the PC owner's OK. With
  the menu opened by the hook, the cursor starts on the close button: the first Down lands on the first setting.
- `BLUEWAKE_BACKGROUND_INPUT=1` lets a controller in while the window is in the background (a virtual pad on ViGEm
  would do for scripted presses).

## GPU cost

`DOL_AURORA_GPU_PROF=1` (`rendered_run.py --gpu-prof`) prints each zone's average GPU time every 240 frames to stderr
(`Ambient occlusion 0.089 x2.0`, ...); `rendered_run.py` keeps the last report of each run in `results.json`. Measure
paced runs: unpaced, the whole frame's GPU time varied by tens of percent between runs; paced, two runs of the same
variant were within 2%. A zone is the steadier number.
On a Mac (Metal) with the HD look on, the zones and `frame_ms` can't be used until it's fixed in RecompCore: on an
M2 `frame_ms` read about 12,000 ms and each HD pass about 1.0 or 2.0 ms, adding up to four times `busy_ms`. `busy_ms`
was steady there (4.41 and 4.29 ms in two paced runs), and with the HD look off the whole report was (`frame_ms` 1.57
and 1.59).
`DOL_AURORA_RENDER_SCALE` takes effect at the first frame, after the startup log line that still reports scale 1.

## Agent pitfalls we hit

- Windows paths in shell heredocs lose their backslashes (`\b` became a control character in a path): write scripts
  with a file editor tool, not through `cat <<EOF`.
- A wait loop that greps a shared status file can match an older run's "done": wait for the run's own line.
- A shell whose working directory is inside a folder keeps Windows from moving or deleting that folder.
- Keep the agent's own changes for a screenshot or a test out of commits: one change per commit, and say plainly
  what was measured, on what, and what wasn't checked.
