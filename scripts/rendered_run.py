#!/usr/bin/env python3
"""Run BlueWake rendered, the same way each time: its frames, its speed and its GPU time, for each variant.

  python3 scripts/rendered_run.py APP OUT_DIR [--card CARD] [where] [input] [pictures] [--set KEY=VALUE ...]
                                  [--variant NAME [KEY=VALUE ...]] ...

APP is a BlueWake executable (on a Mac, BlueWake.app/Contents/MacOS/BlueWake). CARD is a memory card; each
run gets a copy of it (a warp needs one with a save; from a state, a run without one gets a new, empty card). OUT_DIR gets each run's folder, status.txt and results.json.

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
  --timeout SECONDS            stop a run that is still going after this long (timed_out in results.json)
  --stop-on TEXT               stop the run as soon as its log has TEXT (e.g. "is empty; stopped pressing A")
  --unpaced                    as fast as it can (BLUEWAKE_WALL_PACE=0): captures in seconds, no in-between frames
  --set KEY=VALUE              any other variable, for every variant (BLUEWAKE_HD_LOOK=1, DOL_AURORA_AO_DEBUG=1, ...)
Measure:
  --gpu-prof                   Aurora's GPU time per zone (DOL_AURORA_GPU_PROF=1; needs timestamp queries)
  --cpu-profile                Linux perf counters per thread and sampled function cost; use only for attribution,
                               not FPS. Profiles the --profile-window ranges, or else the whole run (window "run")
  --profile-window NAME:FIRST:LAST
                               profile from retrace FIRST to LAST only (repeat for several): perf attaches when the
                               log's [perf] line reaches FIRST and detaches at LAST. In the run folder, per window:
                               perf-NAME-stat.csv (counters per thread), perf-NAME.data, perf-NAME-window.json (the
                               retraces covered), perf-NAME-report.txt (every thread) and perf-NAME-busiest.txt
                               (the busiest thread alone: the game thread when the game is CPU-bound)
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
import signal
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


def stop(process):
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except (AttributeError, OSError):
        process.kill()


def last_line(log):
    """The game's last log line so far (not gamescope's), to show a run is moving."""
    lines = [l for l in log.read_text(errors="replace").splitlines()
             if l.strip() and not l.startswith(("[gamescope]", ">", "The XKEYBOARD", "Errors from xkbcomp"))]
    return f"last: {lines[-1][:100]}" if lines else "nothing from the game yet"


def player_data():
    """The player's own BlueWake data folder (read only here)."""
    if os.environ.get("BLUEWAKE_DATA_DIR"):
        return Path(os.environ["BLUEWAKE_DATA_DIR"])
    if sys.platform == "darwin":
        return Path.home() / "Library/Application Support/BlueWake"
    if os.name == "nt":
        return Path(os.environ.get("APPDATA", Path.home())) / "BlueWake"
    return Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share")) / "BlueWake"


def child_process(root, wanted="bluewake", timeout=15):
    """Find WANTED at or below ROOT in Linux's process tree. gamescope starts the AppImage, which starts BlueWake;
    a BlueWake started directly is ROOT itself."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        children = {root}
        changed = True
        while changed:
            changed = False
            for entry in Path("/proc").iterdir():
                if not entry.name.isdigit() or int(entry.name) in children:
                    continue
                try:
                    # /proc/PID/stat's command can contain spaces and parentheses; the parent follows its last ')'.
                    fields = (entry / "stat").read_text().rsplit(")", 1)[1].split()
                    if int(fields[1]) in children:
                        children.add(int(entry.name))
                        changed = True
                except (OSError, ValueError, IndexError):
                    pass
        for pid in sorted(children):  # the app itself when it is BlueWake, else what its launcher started
            try:
                proc = Path("/proc") / str(pid)
                # Its name, or the program in its command line (a script started by #! is named after python3).
                names = [proc.joinpath("comm").read_text().strip()]
                names += [Path(arg).name for arg in proc.joinpath("cmdline").read_bytes().decode(
                    errors="replace").split("\0")[:2]]
                if wanted in (n.lower() for n in names):
                    return pid
            except OSError:
                pass
        time.sleep(0.05)
    return None


PERF_EVENTS = "task-clock,cycles,instructions,branches,branch-misses,cache-references,cache-misses"


def per_thread_stat(path):
    """perf stat --per-thread -x ';' output: {tid: {event: count}}. Each line is NAME-TID;COUNT;UNIT;EVENT;..."""
    threads = {}
    for line in Path(path).read_text(errors="replace").splitlines() if Path(path).is_file() else []:
        fields = line.split(";")
        if line.startswith("#") or len(fields) < 4 or "-" not in fields[0]:
            continue
        try:
            tid, count = int(fields[0].rsplit("-", 1)[1]), float(fields[1])
        except ValueError:
            continue  # <not counted>, <not supported>
        # With perf_event_paranoid 2 the events are user space only: cycles:u, instructions:u, ...
        threads.setdefault(tid, {})[fields[3].split(":", 1)[0]] = count
    return threads


def retrace_at(points, when):
    """The retrace at wall time WHEN, from (wall time, retrace) pairs of [perf] lines as they appeared: linear
    between the two around it, or along the last two past either end."""
    if len(points) < 2:
        return points[0][1] if points else None
    pairs = list(zip(points, points[1:]))
    (t0, r0), (t1, r1) = next(((a, b) for a, b in pairs if a[0] <= when <= b[0]),
                              pairs[0] if when < points[0][0] else pairs[-1])
    return round(r0 + (r1 - r0) * (when - t0) / (t1 - t0)) if t1 > t0 else r1


class Profiler:
    """Linux perf on BlueWake (below gamescope and the AppImage), attached only for the retrace windows asked
    for. Sampling and counting slow the game, so a profiled run is for attribution, never for FPS."""

    def __init__(self, folder, process, windows):
        if sys.platform != "linux" or not shutil.which("perf"):
            stop(process)
            raise RuntimeError("--cpu-profile needs Linux perf")
        self.folder, self.process = folder, process
        self.windows = list(windows) or [("run", 0, 1 << 62)]
        self.pid = child_process(process.pid)
        if self.pid is None:
            stop(process)
            raise RuntimeError("--cpu-profile could not find the BlueWake process below its launcher")
        self.log, self.offset, self.partial = folder / "log.txt", 0, ""
        self.points, self.active, self.done = [], None, []
        if self.windows[0][1] <= 0:
            self.attach(self.windows.pop(0))

    def poll(self):
        """Read the log's new [perf] lines, then attach or detach where the retraces say."""
        with self.log.open(errors="replace") as log:
            log.seek(self.offset)
            text = self.partial + log.read()
            self.offset = log.tell()
        lines = text.split("\n")
        self.partial = lines.pop()
        now = time.time()
        for line in lines:
            match = PERF.search(line)
            if match:
                self.points.append((now, int(match["retrace"])))
        latest = self.points[-1][1] if self.points else -1
        if self.active and latest >= self.active["last"]:
            self.detach()
        if not self.active and self.windows and latest >= self.windows[0][1]:
            self.attach(self.windows.pop(0))

    def attach(self, window):
        name, first, last = window
        out = lambda suffix: str((self.folder / f"perf-{name}{suffix}").resolve())
        self.active = {"name": name, "first": first, "last": last, "start": time.time(), "procs": [
            subprocess.Popen(["perf", "stat", "--per-thread", "-x", ";", "-o", out("-stat.csv"), "-e", PERF_EVENTS,
                              "-p", str(self.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL),
            # 999 Hz without call stacks keeps perf.data small enough to copy back from a Deck.
            subprocess.Popen(["perf", "record", "-q", "-F", "999", "-e", "cycles:u", "-o", out(".data"),
                              "-p", str(self.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)]}

    def detach(self):
        window, self.active = self.active, None
        window["stop"] = time.time()
        for proc in window["procs"]:
            proc.send_signal(signal.SIGINT)  # perf writes its results on SIGINT
        for proc in window.pop("procs"):
            try:
                proc.wait(timeout=30)
            except subprocess.TimeoutExpired:
                proc.kill()
        self.done.append(window)

    def finish(self):
        """After the game has ended: detach, then write each window's retraces and reports."""
        self.poll()
        if self.active:
            self.detach()
        for window in self.done:
            name = window["name"]
            # The retraces actually covered: perf attaches and detaches between two [perf] lines.
            covered = {"name": name, "asked": [window["first"], window["last"]],
                       "from_retrace": retrace_at(self.points, window["start"]),
                       "to_retrace": retrace_at(self.points, window["stop"]),
                       "seconds": round(window["stop"] - window["start"], 3)}
            threads = per_thread_stat(self.folder / f"perf-{name}-stat.csv")
            busiest = max(threads, key=lambda tid: threads[tid].get("task-clock", 0), default=None)
            covered["busiest_tid"] = busiest
            (self.folder / f"perf-{name}-window.json").write_text(json.dumps(covered, indent=1))
            data = self.folder / f"perf-{name}.data"
            if not data.is_file():
                continue
            report = ["perf", "report", "--stdio", "--no-children", "--percent-limit", "0.10",
                      "--sort", "dso,symbol", "-i", str(data)]
            with (self.folder / f"perf-{name}-report.txt").open("w") as out:
                subprocess.run(report, stdout=out, stderr=subprocess.STDOUT, check=False)
            if busiest is not None:
                with (self.folder / f"perf-{name}-busiest.txt").open("w") as out:
                    subprocess.run(report + ["--tid", str(busiest)], stdout=out, stderr=subprocess.STDOUT,
                                   check=False)


def run(app, card, folder, env, status, skip, timeout=None, stop_on=None, cpu_profile=False, windows=()):
    folder.mkdir(parents=True, exist_ok=True)
    data = folder / "data"
    data.mkdir(exist_ok=True)
    # A run that ended at start-up (a crash, a wrong path) leaves the marker the
    # app takes as a failed launch: the next run in this folder would start in
    # safe mode (no HD look, no mods, 4:3).
    (data / "launch.pending").unlink(missing_ok=True)
    copy = data / "GZLE01.card"
    if card:
        shutil.copy(card, copy)  # without one, a run from a state gets a new, empty card
    # Which disc to read: on Linux the app keeps it in its data folder (disc.txt, and game.txt for the
    # files it prepared from it, both absolute paths). This run's folder is new, so without them the app
    # starts with no disc and waits for one to be chosen.
    for name in ("disc.txt", "game.txt"):
        if (player_data() / name).is_file() and not (data / name).exists():
            shutil.copy(player_data() / name, data / name)
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
        # Its own process group, so a timeout also ends what it started (gamescope, the AppImage's game).
        process = subprocess.Popen([str(app)], env=full, cwd=str(Path(app).parent), stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=os.name != "nt")
        profiler = Profiler(folder, process, windows if cpu_profile else []) if cpu_profile else None
        code, timed_out, last_print = None, False, start
        while code is None:
            try:
                # Profiling follows the log closely, to attach and detach on the retraces asked for.
                code = process.wait(timeout=0.1 if profiler else min(5 if stop_on else 30, timeout or 30))
            except subprocess.TimeoutExpired:
                elapsed = time.time() - start
                if profiler:
                    profiler.poll()
                if stop_on and stop_on in (folder / "log.txt").read_text(errors="replace"):
                    stop(process)
                    code = process.wait()
                elif timeout and elapsed > timeout:
                    stop(process)
                    code, timed_out = process.wait(), True
                elif time.time() - last_print >= 29.5:
                    last_print = time.time()
                    print(f"{folder.name}: running {elapsed:.0f} s, {last_line(folder / 'log.txt')}", flush=True)
        if profiler:
            profiler.finish()
    note(f"exit {code}" + (" (timed out)" if timed_out else ""))
    frames = len(list(folder.glob("frame-*.ppm")))
    dumped = len(list((folder / "dump").glob("*.ppm"))) if (folder / "dump").exists() else 0
    result = {"run": folder.name, "exit": code, "seconds": round(time.time() - start, 1), "frames": frames,
              "dumped": dumped, "timed_out": timed_out, **measure((folder / "log.txt").read_text(errors="replace"), skip)}
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
    parser.add_argument("--card", type=Path)
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
    parser.add_argument("--timeout", type=float)
    parser.add_argument("--stop-on")
    parser.add_argument("--gpu-prof", action="store_true")
    parser.add_argument("--cpu-profile", action="store_true")
    parser.add_argument("--profile-window", action="append", default=[], metavar="NAME:FIRST:LAST")
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
                         args.skip, args.timeout, args.stop_on, args.cpu_profile,
                         [(n, int(f), int(l)) for n, f, l in (w.split(":") for w in args.profile_window)])
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
    sys.exit(1 if any(r["exit"] not in (0, 1) or r["timed_out"] for r in results) else 0)


if __name__ == "__main__":
    main()
