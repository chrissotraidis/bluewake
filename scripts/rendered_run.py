#!/usr/bin/env python3
"""Run BlueWake rendered, the same way each time: its frames, its speed and its GPU time, for each variant.

  python3 scripts/rendered_run.py APP OUT_DIR --card CARD [where] [input] [pictures] [--set KEY=VALUE ...]
                                  [--variant NAME [KEY=VALUE ...]] ...

APP is a BlueWake executable (on a Mac, BlueWake.app/Contents/MacOS/BlueWake). CARD is a memory card; each
run gets a copy of it. OUT_DIR gets each run's folder, status.txt and results.json.

Where (one of):
  --state FILE                 start from a save state (made with BLUEWAKE_SAVE_STATE=FILE@RETRACE, or F5)
  --warp stage:room:point[:layer] [--warp-file N] [--warp-time noon] [--stop-after PLAY_FRAMES]
                               start quest log N on the card and go there (docs/STAGE_SELECT.md)
Input:
  --pad "retrace:buttons:length[:x:y[:cx:cy]],..."   pad presses (BLUEWAKE_PAD_SCRIPT; B is 0x0200)
Pictures:
  --capture FROM:EVERY         the game's frame every EVERY retraces from retrace FROM (frame-RETRACE.ppm)
  --dump FROM:TO               every real and in-between frame, game frames FROM to TO (needs --smooth)
  --smooth 1|3|display         Smooth Motion: 1 is 60 FPS, 3 is 120 FPS
  --scale N                    render scale (1 is the game's 640x480 picture at the window's size)
Run:
  --max-retraces N             end the run at this retrace; a state's retrace counts on from where it was saved
  --unpaced                    as fast as it can (BLUEWAKE_WALL_PACE=0): captures in seconds, no in-between frames
  --set KEY=VALUE              any other variable, for every variant (BLUEWAKE_HD_LOOK=1, DOL_AURORA_AO_DEBUG=1, ...)
Measure:
  --gpu-prof                   Aurora's GPU time per zone (DOL_AURORA_GPU_PROF=1; needs timestamp queries)
  --repeat N                   each variant N times (OUT_DIR/NAME-1, -2, ...), for the spread between runs
  --skip RETRACES              leave the first RETRACES after the run's first [perf] line out (default 300)
  --variant NAME [KEY=VALUE ...]
                               a run of its own in OUT_DIR/NAME with these variables on top; APP=PATH runs
                               another build. Without --variant there is one run, in OUT_DIR itself.

After each run its log is read: game frames a second (the [perf] rate halved, as scripts/bench_tour.py
does), busy, the worst frame, and with --gpu-prof the last [gpu-prof] report's GPU busy time and zones.
OUT_DIR/results.json holds every run; the end prints each variant's mean. Paced, game frames a second stop
at 30 and busy and GPU time are what to compare; unpaced, the rate is the speed. For the standard speed
benchmark at fixed stops, use scripts/bench_tour.py.

Each run gets its own copy of the card, its own SRAM, settings and data folder, and none of the caller's
BLUEWAKE_ or DOL_ variables, so it never reads or changes the player's own. OUT_DIR/status.txt gets a line
as each run starts and ends; each run's folder has log.txt. From a save state, runs are frame-exact (the
game's randomness is in the state): two runs that differ in one variable can be compared pixel for pixel
(scripts/frame_compare.py). From boot they are not. docs/RENDERED_TESTING.md has the rest.

Example: two swings from a state, the build against main, 120 FPS dumps at render scale 2:
  python3 scripts/rendered_run.py build/windows/BlueWake/BlueWake.exe out/trees --card my.card \\
      --state trees.bwstate --pad 1885:0x0200:6,1915:0x0200:6 --max-retraces 2400 \\
      --dump 0:260 --smooth 3 --scale 2 --set BLUEWAKE_HD_LOOK=1 \\
      --variant new --variant main APP=build/main/BlueWake/BlueWake.exe
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path


PERF = re.compile(r"\[perf\] retrace=(?P<retrace>\d+) rate=(?P<rate>\d+(?:\.\d+)?) "
                  r"worst_ms=(?P<worst>\d+(?:\.\d+)?) hitches=\d+ busy=(?P<busy>\d+(?:\.\d+)?)%")
GPU = re.compile(r"\[gpu-prof\] frames=\d+ frame_ms=[\d.]+ max=[\d.]+ busy_ms=(?P<busy>[\d.]+)")
ZONE = re.compile(r"\| (?P<name>[^|]+?) (?P<ms>\d+\.\d+) x(?P<count>\d+(?:\.\d+)?)")


def measure(log_text, skip):
    """What one run's log says about its speed and GPU time."""
    perfs = [m for m in map(PERF.search, log_text.splitlines()) if m]
    out = {}
    if "no HD look, mods or options" in log_text:  # desktop_settings.cpp apply_safe_mode
        out["safe_mode"] = True
    if perfs:
        first = int(perfs[0]["retrace"])
        kept = [m for m in perfs if int(m["retrace"]) >= first + skip] or perfs
        out.update(game_fps=round(sum(float(m["rate"]) for m in kept) / len(kept) / 2, 2),
                   busy=round(sum(float(m["busy"]) for m in kept) / len(kept), 1),
                   worst_ms=max(float(m["worst"]) for m in kept), perf_lines=len(kept))
    gpus = [line for line in log_text.splitlines() if GPU.search(line)]
    if gpus:
        last = gpus[-1]
        out["gpu_busy_ms"] = float(GPU.search(last)["busy"])
        out["gpu_zones_ms"] = {m["name"].strip(): float(m["ms"]) for m in ZONE.finditer(last)}
    return out


def variables(args):
    env = {"BLUEWAKE_NO_DIALOG": "1"}
    if args.state:
        env["BLUEWAKE_LOAD_STATE"] = str(Path(args.state).resolve())
    if args.warp:
        env.update(BLUEWAKE_STAGE_SELECT="1", BLUEWAKE_WARP=args.warp, BLUEWAKE_WARP_FILE=str(args.warp_file))
        if args.warp_time:
            env["BLUEWAKE_WARP_TIME"] = args.warp_time
        if args.stop_after:
            env["BLUEWAKE_WARP_STOP_AFTER"] = str(args.stop_after)
    if args.pad:
        env["BLUEWAKE_PAD_SCRIPT"] = args.pad
    if args.max_retraces:
        env["BLUEWAKE_MAX_RETRACES"] = str(args.max_retraces)
    if args.gpu_prof:
        env["DOL_AURORA_GPU_PROF"] = "1"
    if args.unpaced:
        env["BLUEWAKE_WALL_PACE"] = "0"
    if args.scale:
        env["DOL_AURORA_RENDER_SCALE"] = str(args.scale)
    if args.smooth:
        env.update(DOL_AURORA_FRAME_INTERP="1", DOL_AURORA_FRAME_INTERP_STEPS=args.smooth)
    else:
        env["DOL_AURORA_FRAME_INTERP"] = "0"
    for item in args.set:
        key, _, value = item.partition("=")
        env[key] = value
    return env


def run(app, card, folder, env, status, skip):
    folder.mkdir(parents=True, exist_ok=True)
    data = folder / "data"
    data.mkdir(exist_ok=True)
    # A run that ended at start-up (a crash, a wrong path) leaves the marker the
    # app takes as a failed launch: the next run in this folder would start in
    # safe mode (no HD look, no mods, 4:3).
    (data / "launch.pending").unlink(missing_ok=True)
    copy = data / "GZLE01.card"
    shutil.copy(card, copy)
    full = {k: v for k, v in os.environ.items() if not k.startswith(("BLUEWAKE_", "DOL_"))}
    full.update(HOME=str(data), BLUEWAKE_DATA_DIR=str(data), BLUEWAKE_CARD_PATH=str(copy),
                BLUEWAKE_SRAM=str(data / "sram.bin"), BLUEWAKE_SETTINGS=str(data / "settings.ini"))
    full.update(env)
    if "BLUEWAKE_CAPTURE_INTERVAL" in full:
        full["BLUEWAKE_CAPTURE_OPENING_FRAME"] = str(folder / "frame.ppm")
    if "DOL_AURORA_FRAME_INTERP_DUMP_TO" in full:
        dump = folder / "dump"
        shutil.rmtree(dump, ignore_errors=True)
        dump.mkdir()
        full["DOL_AURORA_FRAME_INTERP_DUMP"] = str(dump)
    def note(text):
        with status.open("a") as out:
            out.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')} {folder.name} {text}\n")

    note("started")
    start = time.time()
    with open(folder / "log.txt", "w") as log:
        # A GUI program on Windows: waiting on it here waits for the game to close.
        code = subprocess.run([str(app)], env=full, cwd=str(Path(app).parent), stdout=log,
                              stderr=subprocess.STDOUT).returncode
    note(f"exit {code}")
    frames = len(list(folder.glob("frame-*.ppm")))
    dumped = len(list((folder / "dump").glob("*.ppm"))) if (folder / "dump").exists() else 0
    result = {"run": folder.name, "exit": code, "seconds": round(time.time() - start, 1), "frames": frames,
              "dumped": dumped, **measure((folder / "log.txt").read_text(errors="replace"), skip)}
    speed = f", {result['game_fps']} game frames/s, busy {result['busy']}%" if "game_fps" in result else ""
    gpu = f", GPU busy {result['gpu_busy_ms']} ms" if "gpu_busy_ms" in result else ""
    print(f"{folder.name}: exit {code} after {result['seconds']:.0f} s, {frames} frames, {dumped} dumped images"
          f"{speed}{gpu}" + (" (SAFE MODE: not the settings asked for)" if result.get("safe_mode") else ""))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter, epilog="\n".join(
                                         __doc__.splitlines()[2:]))
    parser.add_argument("app", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--card", type=Path, required=True)
    parser.add_argument("--state")
    parser.add_argument("--warp")
    parser.add_argument("--warp-file", type=int, default=1)
    parser.add_argument("--warp-time")
    parser.add_argument("--stop-after", type=int)
    parser.add_argument("--pad")
    parser.add_argument("--capture", help="FROM:EVERY retraces")
    parser.add_argument("--dump", help="FROM:TO game frames")
    parser.add_argument("--smooth", choices=("1", "3", "display"))
    parser.add_argument("--scale", type=int)
    parser.add_argument("--max-retraces", type=int)
    parser.add_argument("--unpaced", action="store_true")
    parser.add_argument("--gpu-prof", action="store_true")
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--skip", type=int, default=300)
    parser.add_argument("--set", action="append", default=[], metavar="KEY=VALUE")
    parser.add_argument("--variant", action="append", nargs="+", default=[], metavar=("NAME", "KEY=VALUE"))
    args = parser.parse_args()
    if bool(args.state) == bool(args.warp):
        parser.error("give --state or --warp")
    if args.state and not Path(args.state).is_file():
        parser.error(f"no state at {args.state}")
    if args.dump and not args.smooth:
        parser.error("--dump writes Smooth Motion's frames: give --smooth")
    if args.dump and args.unpaced:
        parser.error("--dump needs a paced run: unpaced, Smooth Motion makes no in-between frames")
    env = variables(args)
    if args.capture:
        first, _, every = args.capture.partition(":")
        env.update(BLUEWAKE_CAPTURE_RETRACE=first, BLUEWAKE_CAPTURE_INTERVAL=every or "60")
    if args.dump:
        first, _, last = args.dump.partition(":")
        env.update(DOL_AURORA_FRAME_INTERP_DUMP_FROM=first, DOL_AURORA_FRAME_INTERP_DUMP_TO=last)
    args.out.mkdir(parents=True, exist_ok=True)
    status = args.out / "status.txt"
    variants = args.variant or [[""]]
    results = []
    for variant in variants:
        name, overrides = variant[0], dict(item.partition("=")[::2] for item in variant[1:])
        app = Path(overrides.pop("APP", args.app))
        if not app.exists():
            sys.exit(f"no such app: {app}")
        for repeat in range(1, args.repeat + 1):
            folder = name + (f"-{repeat}" if args.repeat > 1 else "")
            result = run(app, args.card, args.out / folder if folder else args.out, {**env, **overrides}, status,
                         args.skip)
            results.append({"variant": name, **result})
    (args.out / "results.json").write_text(json.dumps(results, indent=1))
    if len(results) > 1:
        print("variant          game frames/s   busy %   GPU busy ms")
        for name in dict.fromkeys(r["variant"] for r in results):
            mine = [r for r in results if r["variant"] == name]
            mean = lambda key: (f"{sum(r[key] for r in mine) / len(mine):.2f}"
                                if all(key in r for r in mine) else "-")
            print(f"{name or '(run)':16} {mean('game_fps'):>13}   {mean('busy'):>6}   {mean('gpu_busy_ms'):>11}")
    # A warp's own stop (BLUEWAKE_WARP_STOP_AFTER) ends with 1: that is a finished run.
    sys.exit(1 if any(r["exit"] not in (0, 1) for r in results) else 0)


if __name__ == "__main__":
    main()
