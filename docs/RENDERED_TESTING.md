# Rendered testing with an agent

How to let a coding agent (Claude Code, Codex and the like) test BlueWake's rendering on its own: build, warp to a
place, play scripted input, capture or dump frames, time the GPU, compare builds and settings, and show you the
result as images and numbers. The single-run tools were first used on Windows 11 (PowerShell, Direct3D 12) while
tracking down #491's flickering shadows and the ambient occlusion halo; `render_bench` was used on Linux and a Steam
Deck. The game-side switches are the same on Linux and Mac; the window and launch tricks below are
Windows-specific.

The short version: **a save state, a pad script and a frame dump give you the same frames on every run, so two
builds or two settings can be compared pixel for pixel.** The rest of this page is how to get there and what trips
agents up.

## The scripts

[`scripts/rendered_run.py`](../scripts/rendered_run.py) runs the app the same way each time, once or once a
variant, and keeps what it made: frame captures, frame dumps, its log, and what the log says about speed and GPU
time (`results.json`). Each run gets its own settings and data folder, a copy of the card if one is given (`--card`;
a run from a save state doesn't need one), and none of the caller's `BLUEWAKE_`/`DOL_` variables. On Linux the app
keeps which disc to read in its data folder (`disc.txt`, `game.txt`), so each run gets a copy of the player's. [`scripts/frame_compare.py`](../scripts/frame_compare.py) reads the frames
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

## Benchmarking builds rendered: `render_bench`

[`scripts/render_bench`](../scripts/render_bench) benchmarks one build, or build A against build B, **rendered**,
on a Steam Deck (or any Linux machine) over SSH, or here:

```
render_bench [user@host] BUILD_A [BUILD_B] [--states DIR] [--card PATH] [--pairs 2] [--runs 3] [--headless]
             [--cpu-profile]
```

Every path you give (the builds, `--states`, `--card`, `--out`) is on the machine you run it from; what the target
needs is copied there for the run, and **everything comes back**: the target keeps only the tour's start state for
the next run. A build is an AppImage or a folder with one; its commit is read from `build_script`'s paths
(`build/ALIAS-COMMIT/`, `build/artifacts/ALIAS/COMMIT/`). A Linux folder build, the portable builder's `bluewake`
beside `gGZLE01_recomp.so`, is its folder, its `BlueWake/` subfolder or its `bluewake`; its commit is the folder's
`BuilderProvenance.json`, and on a target the folder is copied there without its `user/` (your settings and saves)
and `logs/`. On a Mac it runs here only: a build is a `BlueWake.app` or
a build folder holding `packaged/BlueWake.app` (`build/macos`), its commit the app's own (`BuilderProvenance.json`),
and the card `~/Library/Application Support/BlueWake/GZLE01.card`. On Windows it runs here only, under Git Bash: a
build is a build folder `scripts/windows/build.py` made (`build\NAME`), its `BlueWake\` or its `BlueWake.exe`, its
commit the folder's `BuilderProvenance.json`, and the card `%APPDATA%\BlueWake\GZLE01.card`. The harness and the
analysis run under `python` there (`python3` is often the Microsoft Store's stand-in; set `PYTHON` for another). The
game's window opens on screen, so run it with the screen locked if it shouldn't be seen; rendered runs work locked.
`--cpu-profile` needs Linux `perf`.

**Scenes.** By default, the benchmark tour of [`bench_tour.py`](BENCHMARK_TOUR.md): Outset, Windfall, Dragon Roost
Cavern, Forest Haven and the sea by the Fortress, one every 1800 retraces (`BLUEWAKE_TEST_WARP`), from a save state
at Outset. With `--states DIR`, each `*.bwstate` in `DIR`, in lexical order, is a scene named after its file
(`states/1-outset.bwstate` is "1-outset"), played for `--retraces` (1800).

**Runs.** One build: `--runs` (3). Two builds: `--pairs` (2) of A/B/B/A, so heat or battery drift hits both alike;
`--warmup` runs of each first aren't measured. Every measured run is unpaced, with vsync off, the perf log, guest
checkpoints every 300 retraces and Aurora's GPU profiler (the pictures runs, below, are paced). Rendered (the
default) runs the real renderer under offscreen gamescope (nothing shows on the Deck's screen; `--display desktop`
uses the running session instead). `--headless` runs without a renderer, as `benchmark_on_steamdeck` does; the
tour's start state is still made with a rendered warp, so the first `--headless` run on a target needs gamescope or
a desktop session too.

**Numbers**, taken as `bench_tour.py` takes them: per scene, skip 300 retraces, average the rest. A build's number
for a scene is the median over its runs; its overall FPS is the median of each run's geometric mean over the
scenes. Changes are A against B. GPU ms is the noisiest number: two runs of one build differed by 8.5% at a stop on
a Deck, where FPS agreed within 0.3%. Read FPS first; a GPU ms change of a few percent is not a finding.
**Checks**: every scene loaded in every run, and every run's guest checkpoints the same, for both builds. A tour
stop counts as loaded when the game's `[load]` line names its stage; a `--states` scene has no stage to check, so
it counts as loaded when the run reached it and timed it. **Pictures** (rendered, two builds): after the measured runs, one paced run of each build
(not measured: unpaced, a capture lands a moment apart from run to run) captures a frame every `--every` (300)
retraces in each scene. Identical frame hashes make a scene identical without copying anything; the frames of a
scene that differs come back and are compared with `frame_compare.py check` (`worst-<scene>.png` shows A, B and
the difference). Measured runs set `BLUEWAKE_VSYNC=0`: on X11 vsync is on by default and held both builds at 60.

The exit code is 0 when everything passed, 1 when a run failed, checkpoints differ or pictures changed, and 2 for a
setup error.

**CPU attribution.** `--cpu-profile` adds one more run of each build under Linux `perf`, after the measured runs
and not counted in the FPS table (sampling slows the game). Each scene is profiled over the same window as its FPS:
`perf` attaches when the log's `[perf]` line reaches 300 retraces into the scene and detaches 240 before its end,
so boot, the state load and the warps between tour stops are left out. The tour gets one window per stop. Counts
are taken per thread and divided by the game frames (2 retraces) the window actually covered, which is read back
from the `[perf]` lines. The report gives:

- per scene and build: cycles, instructions, IPC, branch and cache misses per game frame for the **game thread**
  (the busiest thread; the host doesn't name its threads), its busy %, and all threads' cycles per frame;
- with two builds, the game thread's change against the reference next to the FPS change. In a CPU-bound scene the
  two should be about the same size; if FPS moves much more, the scene is waiting on something else (the GX worker,
  the GPU);
- the game thread's hottest functions per scene (`perf report --tid`). `func_ADDR` is a chunk of translated code,
  not one function ([docs/PROFILING.md](PROFILING.md)).

Sampling is 999 Hz without call stacks, so `perf.data` stays small enough to copy back from a Deck. Per scene, the
run's folder in `raw/runs/` keeps `perf-SCENE.data`, `perf-SCENE-stat.csv` (counters per thread),
`perf-SCENE-window.json` (the retraces covered and the game thread's id), `perf-SCENE-report.txt` (all threads) and
`perf-SCENE-busiest.txt` (the game thread). For call stacks of one function, run `perf record -g` by hand on a
short run (docs/PROFILING.md). The target needs a working `perf` for its kernel (on a Steam Deck, the `perf` package:
`pacman -Qo /usr/bin/perf`), and `/proc/sys/kernel/perf_event_paranoid` at 2 or lower, which allows profiling your
own processes in user space; `render_bench` checks for `perf` before it starts.

### How to run it

1. **Once, on the machine you run it from:** numpy and Pillow (`pip install --user numpy pillow`; on a Mac, whose
   Python refuses that, in a virtual environment: `python3 -m venv ~/.venv-bench`, `~/.venv-bench/bin/pip install
   numpy pillow`, and that `bin` first on `PATH`), and builds, for example from `build_script`
   (`build/artifacts/ALIAS/latest`).
2. **Once, on the target** (a Deck or any Linux machine): nothing to install. It needs `python3` and `tar` (SteamOS
   has both), the game's disc set up as for playing, and a card with a Wind Waker save in any quest log (for the
   tour's start state; or pass `--card`). For SSH, keys or `~/.ssh/config` avoid the password prompt; without them
   it asks once.
3. **Run it**, from the directory where you want the results:

   ```
   # Two builds, the tour, on the Deck: A is the candidate, B the reference
   scripts/render_bench deck@10.10.20.168 build/artifacts/gather-candidate/latest build/artifacts/main-090/latest

   # The same here (no SSH): the game windows open on your desktop if there's no gamescope
   scripts/render_bench build/artifacts/gather-candidate/latest build/artifacts/main-090/latest

   # Two Linux folder builds, copied to the target for the run
   scripts/render_bench deck@10.10.20.168 build/portable-new/BlueWake build/portable-main/BlueWake

   # Two Windows builds, here (Git Bash), from your own states
   scripts/render_bench build/candidate build/main --states test_render/states --card my.card

   # One build only, five runs
   scripts/render_bench deck@10.10.20.168 build/artifacts/gather-candidate/latest --runs 5

   # Your own scenes instead of the tour (see "Making your own states" below), 1200 retraces each
   scripts/render_bench deck@10.10.20.168 new.AppImage main.AppImage --states test_render/states --retraces 1200

   # More runs for numbers you can quote: a warm-up and five A/B/B/A pairs
   scripts/render_bench deck@10.10.20.168 new.AppImage main.AppImage --warmup 1 --pairs 5

   # Headless (no renderer), as benchmark_on_steamdeck measures
   scripts/render_bench deck@10.10.20.168 new.AppImage main.AppImage --headless

   # The same rendered A/B plus separate CPU-attribution passes for every scene
   scripts/render_bench deck@10.10.20.168 new.AppImage main.AppImage \
       --states test_render/states --cpu-profile

   # One build, a setting apart: the same AppImage as A and B, at the player's render scale,
   # with the setting under test on A only (the report lists what each build ran with)
   scripts/render_bench deck@10.10.20.168 new.AppImage new.AppImage --states test_render/states \
       --set DOL_AURORA_RENDER_SCALE=3 --set-a DOL_AURORA_EFFICIENT_EFFECTS=1
   ```

   `--set KEY=VALUE` reaches every run of both builds and `--set-a` / `--set-b` one build's; each is repeatable.
   Settings matter: a run without `--set DOL_AURORA_RENDER_SCALE` renders at the default scale, and the GPU cost
   of a full-screen effect grows with the square of the scale (the Lava Room's heat haze was about 15 of 23 ms of
   GPU time at scale 3, little at scale 1). Measure at the scale the change is for.

4. **Pictures with Smooth Motion on:** a capture can land on an in-between frame in one build and on a real frame in
   the other, so the two pictures show different moments (a low PSNR from moving particles, with identical guest
   checkpoints). To judge a rendering change, compare with `--set DOL_AURORA_FRAME_INTERP=0`.
5. **Read the results** in the new `YYYY_MM_DD-HHMMSS/` folder: `report.txt` is also printed at the end,
   `report.md` is ready to paste into a pull request, `report.html` opens in a browser.

A run of two builds with the defaults is 4 measured runs and 2 pictures runs: about 15 minutes on a Deck, 8 on a
desktop PC (the tour is 9000 retraces; unpaced, a faster machine gets through it sooner). Running `render_bench
--help` lists every option.

### Output

What a run looks like: gather-candidate against `main-090`, on a desktop PC (real output, trimmed; that run
wrote to `--out`, shown here with the default folder name). While it runs:

```
A: /home/claudio/Software/bluewake/build/artifacts/gather-candidate/8d8587b37c36/BlueWake-gather-candidate-x86_64.AppImage
B (reference): /home/claudio/Software/bluewake/build/artifacts/main-090/211abfdecf13/BlueWake-main-090-x86_64.AppImage
Running here; results in /home/claudio/Software/bluewake/2026_10_29-074512
== 6 run(s) of 1 state(s) on desktop (AMD Radeon RX 9070 XT (RADV GFX1201), rendered, desktop session)
== [1/6] A measured: tour
tour: running 30 s, last: [gpu-prof] frames=240 frame_ms=0.440 max=0.456 busy_ms=0.411 ...
tour: exit 0 after 53 s, 0 frames, 0 dumped images, 87.07 game frames/s, busy 99.4%, GPU busy 0.718 ms
== [2/6] B measured: tour
tour: exit 0 after 58 s, 0 frames, 0 dumped images, 78.83 game frames/s, busy 99.8%, GPU busy 0.714 ms
== [3/6] B measured: tour
...
== [5/6] A pictures: tour
tour: exit 0 after 145 s, 29 frames, 0 dumped images, 31.0 game frames/s, busy 39.0%
== [6/6] B pictures: tour
...
Analysing here...
```

Over SSH, it adds `Connecting to deck@...`, `Staging builds and harness...` before and `Copying back logs and frame
hashes...` after (and `Copying back the frames of SCENE` for any scene whose pictures differ). The first run on a
target also makes the tour's start state:

```
== making tour.bwstate: warp sea:44:0:0, saved 180 retraces after Link arrives
== quest log 1 did not get there (empty)
== quest log 2 did not get there (empty)
== made /home/deck/bluewake-render/tour.bwstate at retrace 902, from quest log 3
```

The results folder:

```
2026_10_29-074512/
  report.md          the report in Markdown, for a pull request
  report.txt         the same as plain text (printed at the end)
  report.html        the same as one page for a browser
  summary.json       every number
  worst-SCENE.png    A, B and their difference, for a scene whose pictures differ
  raw/               collected.json (what ran), runs/ (each run's log), frames/ (differing scenes' frames)
```

`report.txt`:

```
BlueWake A/B benchmark, rendered
================================

Compared 8d8587b37c36 (A) against 211abfdecf13 (B, the reference) on desktop.

Methodology
-----------

- desktop: Linux-7.1.5-gentoo-x86_64-AMD_Ryzen_7_5800X3D_8-Core_Processor-with-glibc2.43; GPU AMD Radeon RX 9070 XT (RADV GFX1201)
- Rendered (desktop session), unpaced, vsync off, the benchmark tour's five stops
- 4 measured runs (2 of A, 2 of B), interleaved A/B/B/A
- Pictures from one paced run of each build, not measured
- 4/4 runs loaded every scene
- Guest checkpoints identical across every run of both builds (30 each)
- State tour.bwstate at retrace 865, SHA-256 080c4a088fa543a6…

Builds
------

Build          Commit           FPS  Busy %  GPU ms  FPS change
-------------  ------------  ------  ------  ------  ----------
A              8d8587b37c36  86.084  99.111   0.581     +10.18%
B (reference)  211abfdecf13  78.129  99.900   0.591

FPS is game frames a second: each run's geometric mean over the scenes, the median over runs. Busy and GPU ms are the median over the scenes.

Scenes
------

Scene                  FPS B    FPS A      FPS  Busy % B  Busy % A    Busy  GPU ms B  GPU ms A     GPU  Loaded B, A
-------------------  -------  -------  -------  --------  --------  ------  --------  --------  ------  --------
Outset                79.343   86.864   +9.48%    99.900    99.111  -0.79%     0.312     0.311  -0.32%  2/2, 2/2
Windfall              82.369   91.031  +10.52%    99.833    99.000  -0.83%     0.591     0.581  -1.69%  2/2, 2/2
Dragon Roost Cavern  111.911  125.192  +11.87%    99.000    99.000  +0.00%     0.390     0.390  +0.00%  2/2, 2/2
Forest Haven          66.277   72.227   +8.98%   100.000    99.955  -0.05%     0.744     0.803  +7.93%  2/2, 2/2
sea by the Fortress   60.056   66.123  +10.10%   100.000   100.000  +0.00%     0.686     0.679  -1.02%  2/2, 2/2

Pictures
--------

Scene                Verdict    Images  Min PSNR  Why
-------------------  ---------  ------  --------  ---
Outset               identical       5         -  -
Windfall             identical       5         -  -
Dragon Roost Cavern  identical       5         -  -
Forest Haven         identical       5         -  -
sea by the Fortress  identical       5         -  -

Tested artifacts
----------------

- A: 8d8587b37c36, SHA-256 5723637ae268060245bf17a8ee3618677354818e9e9a7c44c9b0e19ab1fba4a1
- B: 211abfdecf13, SHA-256 476a335f0cf6ec35a4070850de5a17e5c8d45441a60638603fc3e6e7e8d4be48

Conclusion
----------

8d8587b37c36 was faster overall by 10.18%. Identical guest checkpoints mean both builds ran the game the same way.
```

<details><summary>The same report as <code>report.md</code></summary>

````markdown
## BlueWake A/B benchmark, rendered

Compared `8d8587b37c36` (A) against `211abfdecf13` (B, the reference) on desktop.

### Methodology

- desktop: Linux-7.1.5-gentoo-x86_64-AMD_Ryzen_7_5800X3D_8-Core_Processor-with-glibc2.43; GPU AMD Radeon RX 9070 XT (RADV GFX1201)
- Rendered (desktop session), unpaced, vsync off, the benchmark tour's five stops
- 4 measured runs (2 of A, 2 of B), interleaved A/B/B/A
- Pictures from one paced run of each build, not measured
- 4/4 runs loaded every scene
- Guest checkpoints identical across every run of both builds (30 each)
- State `tour.bwstate` at retrace 865, SHA-256 `080c4a088fa543a6…`

### Builds

| Build | Commit | FPS | Busy % | GPU ms | FPS change |
| --- | --- | ---: | ---: | ---: | ---: |
| A | `8d8587b37c36` | 86.084 | 99.111 | 0.581 | **+10.18%** |
| B (reference) | `211abfdecf13` | 78.129 | 99.900 | 0.591 |  |

FPS is game frames a second: each run's geometric mean over the scenes, the median over runs. Busy and GPU ms are the median over the scenes.

### Scenes

| Scene | FPS B | FPS A | FPS | Busy % B | Busy % A | Busy | GPU ms B | GPU ms A | GPU | Loaded B, A |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Outset | 79.343 | 86.864 | **+9.48%** | 99.900 | 99.111 | **-0.79%** | 0.312 | 0.311 | **-0.32%** | 2/2, 2/2 |
| Windfall | 82.369 | 91.031 | **+10.52%** | 99.833 | 99.000 | **-0.83%** | 0.591 | 0.581 | **-1.69%** | 2/2, 2/2 |
| Dragon Roost Cavern | 111.911 | 125.192 | **+11.87%** | 99.000 | 99.000 | **+0.00%** | 0.390 | 0.390 | **+0.00%** | 2/2, 2/2 |
| Forest Haven | 66.277 | 72.227 | **+8.98%** | 100.000 | 99.955 | **-0.05%** | 0.744 | 0.803 | **+7.93%** | 2/2, 2/2 |
| sea by the Fortress | 60.056 | 66.123 | **+10.10%** | 100.000 | 100.000 | **+0.00%** | 0.686 | 0.679 | **-1.02%** | 2/2, 2/2 |

### Pictures

| Scene | Verdict | Images | Min PSNR | Why |
| --- | --- | ---: | ---: | --- |
| Outset | identical | 5 | - | - |
| Windfall | identical | 5 | - | - |
| Dragon Roost Cavern | identical | 5 | - | - |
| Forest Haven | identical | 5 | - | - |
| sea by the Fortress | identical | 5 | - | - |

### Tested artifacts

- A: `8d8587b37c36`, SHA-256 `5723637ae268060245bf17a8ee3618677354818e9e9a7c44c9b0e19ab1fba4a1`
- B: `211abfdecf13`, SHA-256 `476a335f0cf6ec35a4070850de5a17e5c8d45441a60638603fc3e6e7e8d4be48`

### Conclusion

`8d8587b37c36` was faster overall by **10.18%**. Identical guest checkpoints mean both builds ran the game the same way.
````

</details>

### Making your own states

For `--states`, any save state works. Name each file after its scene; a number prefix sets the order
(`1-outset.bwstate`, `2-lava-room.bwstate`, ...). Two ways to get them:

- **Play and press F5.** Play to the spot, set up what the scene should show, and press F5 ("State saved" at the
  bottom left). The state is `quick-RETRACE.bwstate` in the data folder (`~/.local/share/BlueWake/` on Linux and the
  Deck; the newest has the highest number, and the session log in `logs/` names the stage you were in). Copy it into
  your states folder under the scene's name:

  ```
  cp ~/.local/share/BlueWake/quick-136213.bwstate test_render/states/2-lava-room.bwstate
  scp deck@10.10.20.168:.local/share/BlueWake/quick-152104.bwstate test_render/states/5-shipwreck.bwstate
  ```

  Each scene plays `--retraces` (1800, 30 seconds) from the state, the first 300 not measured, and nothing presses
  buttons: save just before what you want to see, somewhere the scene moves on its own (water, weather, a busy
  room). Settings (HD look, render scale) aren't in a state.
- **Warp there.** `render_bench.py make-state` makes one anywhere the stage select can warp, here on your machine
  (a state loads in any build, on any machine):

  ```
  python3 scripts/render_bench.py make-state test_render/states/6-windfall-night.bwstate \
      --app build/artifacts/main-090/latest --warp sea:11:0 --time night
  ```

  It warps there (`--warp stage:room:point[:layer]`, names in `config/stage_select/names.tsv`) from the card's
  first quest log with a save (`--card`, default `~/.local/share/BlueWake/GZLE01.card`), and saves the state
  `--settle` (180) retraces after Link arrives. If a warp stalls at the file select, it tries again.

**The tour's start state.** Made once on the target and kept in `~/bluewake-render/tour.bwstate`: the stage select
warps to Outset (`sea:44:0:0`) from the card's first quest log with a save (1, 2 and 3 are tried; an empty one ends
its try at once), and the state is saved 180 retraces after Link arrives. Not `bench_tour.py`'s own setup, which
always continues quest log 1: on a card whose save is in another log, that starts a new game and saves the state
in the opening. The card is `--card` (copied from here) or the target's own; it's only read, and only needed until
the state exists. Delete the state to make it again.

**On the target:** `python3` and `tar` (SteamOS has both), a display for rendered runs (gamescope's offscreen
backend, which SteamOS has, or `--display desktop` in Desktop Mode), and the disc, as for playing: each run gets a
fresh data folder, and `rendered_run.py` copies the player's `disc.txt` and `game.txt` into it (on Linux the app
keeps which disc to read there). **Here:** numpy and Pillow for the pictures. None of this runs in CI (no game
there): CI runs `tests/test_render_bench.py`, which drives the same scripts with a fake app
(`tests/render_fixtures/fake_bluewake.py`).

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

### From another machine, over SSH

`render_bench` doesn't take a Windows PC as an SSH target (a Windows build runs only on the PC itself), so run it
on the PC and start it from your machine:

- **Not straight from the SSH session.** Windows runs an SSH login in session 0, which has no desktop. A rendered
  run started there either loses its Direct3D 12 swap chain (`DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`, then a lost
  device) or finishes with numbers that are wrong: main from a save state at Outset went 25 game frames a second
  with no GPU time, against 56-58 from the desktop. `Invoke-CimMethod Win32_Process -MethodName Create` from SSH
  starts its process in session 0 too. Headless runs (`--headless`) are fine there.
- **Start it as a scheduled task in the desktop session.** A task with an interactive principal runs in the
  session of the account logged on at the desktop, even with the PC locked. Put the command in a `.cmd` file (a
  file editor, not a heredoc; see the pitfalls) that writes the console to a file. Its folder must be a checkout
  that has `render_bench`:

  ```bat
  @echo off
  cd /d C:\src\bluewake
  "C:\Program Files\Git\bin\bash.exe" -lc "scripts/render_bench build/candidate build/main --states /c/bench/states --out /c/bench/out/run1" > C:\bench\out\run1-console.txt 2>&1
  ```

  Then, over SSH (PowerShell is Windows' SSH shell), run it with
  [`scripts/windows/desktop_task.ps1`](../scripts/windows/desktop_task.ps1):

  ```powershell
  powershell -ExecutionPolicy Bypass -File C:\src\bluewake\scripts\windows\desktop_task.ps1 -Command C:\bench\run1.cmd
  ```

  It registers the task, starts it, waits for it to end, returns the command's exit code and deletes the task. The
  game's window opens on the PC's screen. Log in over SSH as the account that runs BlueWake at the desktop; then
  it needs no password and no admin. It runs the task at normal priority: a task's default is below normal, which every process it starts
  inherits, and `schtasks.exe` can't change that. Below normal skews A/B results, not only the speed: one pair
  started with `schtasks` called A 2.7% faster, where at the desk it was 1.8% slower and through `desktop_task.ps1` 0.7% slower.
- **Follow it** meanwhile, if you like, from a second SSH session: `Get-Content C:\bench\out\run1-console.txt -Tail 5`, and
  each run's `raw/runs/NN-BUILD/SCENE/status.txt`. Then copy the results folder back with `scp -r`; `report.md` and
  `summary.json` are the summary.
- The PC needs Git Bash and `python` with numpy and Pillow (`python3` is often the Microsoft Store's stand-in there;
  the wrapper uses `python`, and `PYTHON=` chooses another).

Tested on Windows 11 (i5-12600KF, RTX 5070 Ti), from the Outset save state, 600 retraces:

| Started | main (a773185) | A/B: a candidate against main |
| --- | --- | --- |
| at the desk | 58.1 game frames a second | 57.0 against 58.1: A 1.8% slower |
| straight from SSH | 25.3, no GPU time | |
| a task made with `schtasks /IT` | 56.3 | 54.6 against 53.1: A 2.7% faster |
| `desktop_task.ps1` | 57.5 | 57.3 against 57.7: A 0.7% slower |

Each A/B is one A/B/B/A pair; the other main runs are one run each, started from a Mac, and the desk's main is
the B of its pair. Stopping a run partway hasn't been tried.

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
- A rendered run started straight from an SSH session on Windows has no desktop: it fails or runs at half speed.
  Start it as a scheduled task in the desktop session ([From another machine, over SSH](#from-another-machine-over-ssh)).
- A shell whose working directory is inside a folder keeps Windows from moving or deleting that folder.
- Keep the agent's own changes for a screenshot or a test out of commits: one change per commit, and say plainly
  what was measured, on what, and what wasn't checked.
