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

## Gotchas

These each cost real time the first time around:

- **`nohup` is not enough over SSH.** The process dies when the session closes, because
  the AppImage's FUSE mount tree is torn down with it. Use `setsid ... & disown`.
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
