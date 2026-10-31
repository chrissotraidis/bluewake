# Profiling BlueWake on the Steam Deck

This is a step-by-step guide to measuring BlueWake's frame rate on the Steam Deck and
getting a CPU profile of the game thread. It is the same procedure used to produce the
numbers behind issue #215 and the tables in `docs/PERFORMANCE.md`.

You will end up with two things:

1. A session log that proves where the slowdown comes from (CPU-bound game thread vs
   GPU vs the GX worker).
2. A 30-second `perf` profile naming the exact hot functions.

Expect about 15 minutes, almost all of it waiting on the 30-second capture. Everything
below was done on a Steam Deck (VanGogh, 4 cores / 8 threads, Zen 2) running SteamOS 3.8.

## What you need

- A Steam Deck with BlueWake installed (the AppImage).
- A save state that drops you straight into the heavy scene. The Outset Island (sea/44)
  state `quick-26502.bwstate` in `~/.local/share/BlueWake/` (the state dir is the data dir
  root, or `$BLUEWAKE_STATE_DIR` when set) is the one used below.
  `BLUEWAKE_LOAD_STATE` is the reliable way to reproduce a scene; `BLUEWAKE_TEST_WARP` is
  the fallback if you have no state.
- `perf`. It ships with the Deck and works without root, because
  `/proc/sys/kernel/perf_event_paranoid=2` allows self-profiling.
- SSH access to the Deck. You can only test a rendered (on-screen) run from the Deck's
  own desktop or game session — gamescope will not initialise over SSH.

## Quick start

If you have done this before, this is the whole thing in order:

```sh
# 1. Launch headless into the slow scene, detached so it survives SSH close:
cd /run/media/deck/GF8S5/BlueWake
BLUEWAKE_RENDERER=headless BLUEWAKE_WALL_PACE=0 \
  BLUEWAKE_LOAD_STATE=/home/deck/.local/share/BlueWake/quick-26502.bwstate \
  BLUEWAKE_FPS_WATCH=0 BLUEWAKE_MAX_RETRACES=600000 \
  setsid bash -c './BlueWake-x86_64.AppImage </dev/null >/tmp/bw_profile.log 2>&1' & disown

# 2. Confirm it is actually slow:
grep -a "fps-dip" ~/.local/share/BlueWake/logs/session-*.log | tail -3

# 3. Profile for 30 seconds:
PID=$(pgrep -f 'mount_.*bluewake')
perf record -g -F 99 -p "$PID" -o /tmp/bw.perf.data -- sleep 30
perf report -i /tmp/bw.perf.data --stdio --no-children | head -60
```

The rest of this page explains each step and how to read what you get back.

All of this is scripted in `scripts/linux/profile_deck.sh`, which launches the game,
finds its PID, runs the perf capture, and writes the flat report plus a copy of the
newest session log:

```sh
scripts/linux/profile_deck.sh --seconds 30
scripts/linux/profile_deck.sh --rendered --state /home/deck/.local/share/BlueWake/quick-26502.bwstate
```

Run it on the Deck (or any Linux host with the AppImage and a state). It defaults to
headless; `--rendered` produces the `[fps-dip]` / `[perf-summary]` numbers, which the
headless run cannot (an unpaced run is skipped by the fps watcher).

## Step 1 — Reach the slow scene without a display or controller

The game reads a set of environment variables at startup, so you can launch a profiling
run with no window, no seat and no input. These are the same flags the builder uses for
training:

```sh
cd /run/media/deck/GF8S5/BlueWake        # wherever the AppImage lives

export BLUEWAKE_RENDERER=headless        # skip Aurora/SDL entirely
export BLUEWAKE_WALL_PACE=0              # run unpaced: the game thread goes flat-out
export BLUEWAKE_LOAD_STATE=/home/deck/.local/share/BlueWake/quick-26502.bwstate
export BLUEWAKE_FPS_WATCH=0              # keep [fps-dip] out of the log so it stays readable
export BLUEWAKE_MAX_RETRACES=600000      # a large bound; see the gotchas below

# This MUST be detached with setsid, not plain nohup (nohup dies when the SSH session ends):
setsid bash -c './BlueWake-x86_64.AppImage </dev/null >/tmp/bw_profile.log 2>&1' & disown
```

Before you spend 30 seconds profiling, confirm the game is actually in the slow state:

```sh
grep -a "fps-dip" ~/.local/share/BlueWake/logs/session-*.log | tail -3
```

What you want to see is `busy=9x% cause=game-thread` — a CPU-bound game thread with the
GPU idle. That is the interesting bottleneck. If you instead see `waits: gpu=` or
`workers: gx=`, the limit is somewhere else and a game-thread profile will not explain it.

## Step 2 — Capture a 30-second profile of the game thread

Find the real process ID first. The AppImage unpacks itself into a FUSE mount whose path
changes on every run, so match that path rather than the wrapper:

```sh
pgrep -f 'mount_.*bluewake'              # -> e.g. 12345

perf record -g -F 99 -p 12345 -o /tmp/bw.perf.data -- sleep 30
```

`-g` records call graphs, `-F 99` samples 99 times per second, `-p` attaches to the game
process. When it finishes, read the report:

```sh
perf report -i /tmp/bw.perf.data --stdio --no-children | head -60
```

`--no-children` gives flat self-time per function — the number that matters for ranking
hotspots — rather than the default "with children" view that double-counts callers.

## Step 3 — Read the report

A good profile has translated game code at the top. If you see `[unknown]` or kernel
symbols instead, something is off. The game module is built with `-g`, so the translated
`func_*` chunks are named.

Two things trip people up when interpreting the output:

- **`func_<addr>` names a chunk, not a function.** Chunks start at `...6E0`-aligned
  addresses and each one contains many original functions. To find out what a chunk
  actually does, look up its start address in `config/GZLE01/symbols.txt` and pick the
  function whose `[addr, addr+size)` range contains it. Confirm the range contains the
  address — do not guess from the nearest symbol.
- **Samples are per-process, not per-thread.** To see which thread a sample came from,
  use `perf report -i /tmp/bw.perf.data --stdio --sort=tid,symbol`. To profile a single
  thread (say, the GX worker), find its tid from the per-task CPU-time deltas in
  `/proc/PID/task/*/stat` and record with `perf record -t <tid>`.

For line-level detail inside one hot function:

```sh
perf annotate --stdio -i /tmp/bw.perf.data <symbol>
```

This shows which exact instruction in the function is burning the samples, which is what
you want before deciding what to change.

## Step 4 — Profile the rendered path (GX worker and presentation)

A headless run never starts the GX worker, so it cannot answer any question about the
graphics thread. If you need that, run with the real renderer on the Deck's own desktop
session. You need the Xwayland auth cookie first:

```sh
# find the cookie from the running Xwayland process:
ps aux | grep '[X]wayland'              # -> ... -auth /run/user/1000/xauth_XXXX ...

BLUEWAKE_RENDERER=aurora DISPLAY=:0 XAUTHORITY=/run/user/1000/xauth_XXXX ./BlueWake-x86_64.AppImage
```

Then profile the same way as above. On the Deck this is how we confirmed the game thread
— not the GX worker — is the wall: the log showed `[fps-dip] ... cause=game-thread` with
`workers: gx=~55% interp=0% render=~9%` and `waits: gx=5ms`, meaning the game is not
waiting on the GX worker.

### The GX worker's own profile at Outset

A rendered run at Outset on the Deck (`sea` room 44, from a save state, the AppImage
built from `main` at `7382708` with local preparations, paced at 60 FPS with Smooth Motion
on) gives the worker something to do and names its hot paths:

```
[fps-dip] retrace=2281 shown=56.2 game=29 speed=95% interpolated=29/29 draws/frame=598
          rejected=0.2% unmatched=3.1% busy=94% waits: gx=6ms present=9ms gpu=6ms
          workers: gx=43% interp=5% render=11% stage=sea room=44
```

533 to 598 draws a frame, the game thread 94 to 97% busy, waiting 6 to 8 ms on the GX
worker and 9 ms on the present, with the worker itself taking 35 to 43% of a core and the
Dawn/Vulkan render worker 11 to 12%. `perf record -g -F 99 -p PID -- sleep 30`, then
`perf report --tid <the worker's tid>` (`7045` was the game thread and `7064` the worker
in this capture), ranks the worker's self time like this:

| Share | Symbol | What it is |
| --- | --- | --- |
| 3.2% | `gxcore::GxCoreState::build_draw_plan_into` | turning a consumed draw into a `DrawPlan`, including the cached vertex attributes |
| 2.0% | `aurora_recomp::ConsumingAuroraRenderSink::submit_packet` | handing a render packet to the Aurora sink |
| 1.7% | `gxcore::GxCoreSink::submit_packet` | the GX core's side of the same hand-off |
| 1.6% | `aurora_recomp::RetailGxFrontend::emit_new_packets` | FIFO parse into render packets |
| 1.5% | `aurora_recomp::fill_render_packet` | copying one trace event into a packet |
| 1.1% | `host_graphics_guest_resolve` | resolving a guest address the graphics path reads |
| 0.8% | `aurora_recomp::RetailGxFrontend::handle_draw` | the draw command's own handler |
| 0.75% | `gxcore::GxCoreState::apply` | applying a render-state packet |

The game thread's own graphics work is smaller and separate: `aurora_backend_gx_write`
2.3% (guest GX register writes) and `ppc_guest_alias_resolve` 2.4%. The third thread with
samples is the Dawn/Vulkan render worker (`dawn::native::vulkan::CommandBuffer::*`,
0.1 to 0.2% each, spread thin).

So on four cores the two groups worth attacking are command conversion
(`emit_new_packets`, `fill_render_packet`, both `submit_packet`s) and draw planning with
its vertex-attribute caching (`build_draw_plan_into`), with guest address resolution third.
At 60 FPS the worker is not the wall -- 35 to 43% of one core -- which is why the game
thread's own numbers move the frame rate here; these paths matter when the worker becomes
the limit, and they are the ones to measure first if it does.

## Step 5 — Extra diagnostics

A few more environment variables come in handy:

```sh
# Prove a build change did not alter game behaviour: hash guest state
# (CPU/MEM1/MEM2/aliases) every N retraces. Two runs of the SAME build must
# print identical hashes before you trust a before/after performance comparison.
export BLUEWAKE_GUEST_CHECKPOINT_INTERVAL=500

# Print the memory-resolver cost at exit (resolve_calls, pruned_by_bounds, iterations, hits).
export BLUEWAKE_TRACE_ALIAS_COST=1

# Jump to a scene at a given retrace, when you have no save state for it.
export BLUEWAKE_TEST_WARP=retrace:stage:room:point
```

## Measuring in Game Mode

Steps 1–4 measure a headless or desktop run. A change aimed at what players see on the Deck (a frame-rate loss
with Smooth Motion, a heavy room at a high render scale) has to be measured **in Game Mode, with the player's own
settings**: headless gamescope has no real scanout and does not reach the APU's power limit, and the defaults
render at scale 1. In the Lava Room, headless runs understated the real frame's GPU cost about 5× and showed
neither the power cap nor its effect on the CPU clock (bluewake [#504](https://github.com/chrissotraidis/bluewake/pull/504)).

### What decides the speed on a Deck

The APU shares one **15 W** budget between the CPU and the GPU. When the GPU is busy enough to hold the chip at
the cap, the CPU's clock falls, and the game thread with it, even though nothing on the CPU changed. Measure the
game thread's **clock** next to its IPC, and the APU's power and clocks: in the Lava Room at render scale 3 with
Smooth Motion on, the game thread ran at the same IPC but at 3.1 GHz instead of 3.45, with the APU at 14–16 W,
and the game lost about 10%. A GPU-side saving then comes back as CPU clock. Two corollaries:

- Fixing the GPU clock higher can make the game **slower** (more of the budget goes to the GPU).
- The charger does not change the cap (`power1_cap` is the APU's own limit).

### The kit: `scripts/gamemode_measure.py`

It runs here and measures the BlueWake already running on the Deck, over SSH; the results stay here. Each window
(15 s by default) records the game thread's clock, IPC and busy share (`perf stat -t` on the main thread, no root
needed), the APU's power and GPU clock and the cores' mean clock (sampled each second from sysfs), the session
log's `[interp-stats]`, `[fps-dip]` and last `[gpu-prof]` lines during the window, the settings under test from
`settings.ini`, and the process's `DOL_`/`BLUEWAKE_` variables. The settings are what the file holds: a build
that predates a setting ignores it, so check which build the shortcut runs. Game frames a second come from the
`[fps-dip]` lines (`game=`), which are written only for seconds below the game's target.

1. **Launch with the diagnostics you need.** Game Mode starts the AppImage from its Steam shortcut, so put the
   variables in the script the shortcut runs (keep a copy of the original):

   ```sh
   #!/bin/sh
   export DOL_AURORA_FRAME_INTERP_PACING=0   # never pause: every game frame gets its in-between frame
   export DOL_AURORA_FRAME_INTERP_STATS=1    # [interp-stats] each second (RecompCore #49)
   export DOL_AURORA_GPU_PROF=2              # [gpu-prof]: each pass's GPU time (=1 sums passes of a kind)
   exec /home/deck/BlueWake-x86_64.AppImage "$@"
   ```

2. **Go to the scene** (a state loaded from the menu keeps the view repeatable) and keep the camera still.
3. **A/A, then A/B, in one session.** Sample the base twice, change the setting from the menu (it applies live),
   sample, change it back, sample again:

   ```sh
   scripts/gamemode_measure.py sample deck@steamdeck off --out lava-room
   scripts/gamemode_measure.py sample deck@steamdeck off2 --out lava-room
   # Efficient soft effects on, from the menu
   scripts/gamemode_measure.py sample deck@steamdeck on --out lava-room
   # off again, then on again
   scripts/gamemode_measure.py sample deck@steamdeck off3 --out lava-room
   scripts/gamemode_measure.py sample deck@steamdeck on2 --out lava-room
   ```

4. **Report:**

   ```sh
   scripts/gamemode_measure.py report lava-room --base off,off2,off3 --test on,on2
   ```

   It prints every window and, for each metric, the base and test means, the change and the base windows'
   spread (the A/A); a change counts only beyond that spread. With never pause, the in-between frames delivered
   per second are the game's frame rate.

### Diagnostics for attribution

These come with RecompCore [#49](https://github.com/chrissotraidis/RecompCore/pull/49) unless noted; each is off
unless set.

| Variable | What it shows |
|---|---|
| `DOL_AURORA_GPU_PROF=1` (in main) / `=2` | GPU time per pass kind / per pass (`EFB 1`, `EFB 3`); the in-between replay as `IB …` |
| `DOL_AURORA_GPU_PROF_CHUNK=N` | A long pass timed in chunks of N commands (`EFB 1 @700`): which draws hold the time |
| `DOL_AURORA_DRAW_LOG=PASS:FIRST-LAST` | Those draws once, with each pipeline's state (blend, depth, TEV stages), indices and textures |
| `DOL_AURORA_SKIP_DRAWS=PASS:FIRST-LAST` | Those draws left out: what they draw (compare captures) and what they cost |
| `DOL_AURORA_FRAME_INTERP_STATS=1` | Smooth Motion each second: frames delivered, skipped, rejected; overloads by cause, with waits; per-draw jobs |
| `DOL_AURORA_FRAME_INTERP_ABLATE=match` / `upload` | Smooth Motion's matching (and upload) without drawing the in-between frame |

The path that found the Lava Room's cost: per pass (`=2`) → chunks of 50 → draw log of the hot chunks → skip
them and compare captures. Pass numbers can differ from frame to frame in some rooms; check the draw log's pass
before trusting a chunk's name.

### Pictures

- **Screenshot in Game Mode:** `XDG_RUNTIME_DIR=/run/user/1000 gamescopectl screenshot /tmp/shot.png` over SSH,
  at the size shown. The game keeps animating and the camera can move while a menu is open, so Game Mode
  screenshots show the look, not a pixel comparison.
- **Pixel comparison:** identical frames come from a save state played deterministically
  (`scripts/rendered_run.py --state … --capture FIRST:EVERY`, or `render_bench`'s pictures run), with
  **Smooth Motion off**: with it on, a capture can land on an in-between frame in one run and a real frame in the
  other.

### Example: Outset (bluewake #503)

Smooth Motion on, then off, then on again, in one session on the #501 build:

| | Smooth Motion on (6 windows) | Off (2 windows) |
|---|---|---|
| Game FPS | 23.8–23.9 | 25.1 |
| Game-thread clock | 3.445–3.451 GHz | 3.456–3.457 GHz |
| Game-thread IPC | 1.646–1.659 | 1.735–1.742 |
| APU power | 12.2–13.8 W | 11.4–12.2 W |

Outset is not power-limited (full clock, under the cap): its limit is the game thread's work, and Smooth Motion
costs it about 5% through shared cache and memory, not clock. Compare with the Lava Room, where the clock fell.

### Pitfalls met in #504

- **Settings:** read `~/.config/BlueWake/settings.ini` before comparing to headless runs (render scale, texture
  pack: `DOL_AURORA_TEXTURE_PACK` empty means none is active, whatever the folder setting says).
- **`gpu_busy_percent` near 100%** does not mean the GPU is saturated: the governor runs a light load busy at a
  low clock. Power and clock are the honest measures.
- **Profiler limits:** `gpu_prof`'s 4-deep readback ring drops frames with Smooth Motion on (per-instance costs
  hold, totals undercount); `frame_ms` is meaningless with chunked passes.
- **A run that exits at once:** a `rendered_run.py` stop retrace (`--max-retraces`) counts from the state's own
  retrace, which is usually far above zero.

## Gotchas

These each cost real time the first time around:

- **`nohup` is not enough over SSH, and `setsid … & disown` is not always either.** A
  rendered run started that way died with the session here: the log stopped mid-frame
  with no exit line. `systemd-run --user --unit=bw-profile --collect --setenv=… $APP`
  puts it in its own user unit that survives the session, and
  `journalctl --user -u bw-profile -f` reads its output. With `APPIMAGE_EXTRACT_AND_RUN=1`
  (and `TMPDIR` on the SD card, since `/home` on a Deck is often full) the AppImage also
  stops depending on a FUSE mount that the session teardown can remove.
- **`BLUEWAKE_MAX_RETRACES` is compared against the loaded state's already-counted
  retrace.** Set it to `state_retrace + the window you want`. A state at retrace 2082
  needs `MAX_RETRACES=4082` for a 2000-retrace window — otherwise the run stops after
  zero blocks.
- **Launch and record in the same script.** The process must stay alive for the whole
  capture. A 30-second record outlives the SSH foreground window's default timeout.
- **"No route to host" means the Deck is off the network**, not an SSH problem. WiFi
  dropped, it slept, or a sustained 98%-CPU load hung it. That needs a physical check on
  the Deck. `Connection refused` or `Permission denied`, by contrast, means the box is up
  and only SSH is misconfigured.
- **A rebuilt binary does not update the AppImage by itself.** Re-run
  `scripts/linux/make_appimage.sh` after any native change, and check the AppImage's
  mtime is newer than the fix commit (`ls -la --time-style=full-iso`) before profiling.
  A profile taken seconds after a fix can still be measuring the pre-fix code if the
  AppImage was never redeployed.
- **The desktop and the Deck run different glibc versions** (2.44 vs 2.41). A host
  relinked on the desktop binds `sqrtf`/`cos`/`log10f` and friends to GLIBC 2.43/2.44,
  and the Deck then refuses to run. Relink against the Deck's own `libm.so.6` — copy it
  over and use it in place of a bare `-lm`.

## What the results mean

A CPU-bound game thread shows three signatures together:

- one core saturated — `perf` reports roughly 3.45 GHz of work on a single core;
- `busy=9x%` in the `[fps-dip]` log lines;
- a flat profile dominated by `func_*` chunks spread thin, plus the host edge-service
  layer (`host_chassis_requires_full`, `host_can_skip_observation`,
  `host_direct_can_skip`) — and *not* the memory resolver (`ppc_guest_alias_resolve`,
  ~1.3%) or out-of-line FP helpers.

That pattern means the translated game itself is too much work for one Zen 2 core. It is
not a GPU problem, not a GX-worker problem, and not throttling.
