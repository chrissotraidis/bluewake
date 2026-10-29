#!/usr/bin/env python3
"""Benchmark one build, or build A against build B, rendered: speed, GPU time and pictures, scene by scene.

  python3 scripts/render_bench.py collect OUT_DIR --app A=APP [--app B=APP] [--states DIR] [--card CARD]
                                  [--pairs 2] [--runs 3] [--warmup 0] [--headless] [--retraces 1800] [--every 300]
                                  [--display auto|headless|desktop] [--size WxH] [--cache ~/bluewake-render]
                                  [--cpu-profile]
  python3 scripts/render_bench.py make-state STATE.bwstate --app APP --warp stage:room:point [--card CARD]
                                  [--time noon] [--settle 180]
  python3 scripts/render_bench.py analyze OUT_DIR --build A=PATH [--build B=PATH] [--into DIR]
  python3 scripts/render_bench.py report OUT_DIR/summary.json

scripts/render_bench runs `collect` on a Steam Deck (or any Linux machine) over SSH, or here, then copies the
results back and runs `analyze` here; use that. `collect` renders and needs only python3; `analyze` needs numpy
and Pillow for the pictures.

Scenes. Without --states, the benchmark tour of scripts/bench_tour.py: Outset, Windfall, Dragon Roost Cavern,
Forest Haven and the sea by the Fortress, one every 1800 retraces (BLUEWAKE_TEST_WARP), from a save state at
Outset. That state is made once on the target with the stage select (from the card's first quest log with a save:
logs 1-3 are tried) and kept in --cache. With --states DIR, each *.bwstate in DIR, in lexical order, is a scene
named after its file (1-outset.bwstate is "1-outset"): a run loads it and plays --retraces retraces.

Runs. With one build, --runs runs. With two, --pairs A/B/B/A pairs, so a slow drift (heat, battery) affects both
alike; --warmup runs of each first are not measured. Every run is unpaced (as fast as it can), with the perf log,
guest checkpoints every 300 retraces and, rendered, Aurora's GPU profiler. Rendered (the default) runs under
headless gamescope when there is one; --headless runs without a renderer (BLUEWAKE_RENDERER=headless), as
scripts/bench_tour.py --mode headless does.

Numbers, as scripts/bench_tour.py takes them: per scene, the first 300 retraces are skipped and the rest averaged
(game frames a second are the [perf] rate halved). A build's number for a scene is the median over its runs; its
overall number is the median of each run's geometric mean over the scenes. Changes are A against B.

Pictures (rendered, two builds): after the measured runs, one paced run of each build (not measured; unpaced,
captures land a moment apart from run to run) captures a frame every --every retraces after each scene's first 300. Scenes whose frames' SHA-256 match are identical; the rest are compared with
scripts/frame_compare.py check (PSNR 40 or more is similar).

Checks: every scene loaded in every run (a tour stop by its [load] line), and every run's guest checkpoints the
same (the same game state at the same retraces, for both builds).
"""
import argparse
import gzip
import hashlib
import json
import os
import platform
import re
import shutil
import socket
import statistics
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import bench_tour  # noqa: E402

PERF = re.compile(r"\[perf\] retrace=(\d+)")
GPU = re.compile(r"\[gpu-prof\] frames=\d+ frame_ms=[\d.]+ max=[\d.]+ busy_ms=([\d.]+)")
COMMIT = re.compile(r"^(?:.*-)?([0-9a-f]{12,40})$")
EMPTY = "is empty; stopped pressing A"
TOUR_WARP = "sea:44:0:0"  # Outset, the tour's first stop
STOP_RETRACES = 1800
PROFILE_MARGIN = 240  # retraces before a scene's end where profiling stops: several [perf] lines, unpaced


# --- the target: collect ----------------------------------------------------------------------------------------

def state_retrace(path):
    """The retrace a save state was made at (its BWSTATE1 header), or None if it is not a save state.

    As scripts/bench_tour.py reads it: gzip; HostStateHeader (main.c) has the retrace as the little-endian u64
    at byte 32 of the HEADER chunk."""
    try:
        with gzip.open(path, "rb") as handle:
            if handle.read(8) != b"BWSTATE1":
                return None
            handle.read(4)
            if not handle.read(16).startswith(b"HEADER") or int.from_bytes(handle.read(8), "little") < 40:
                return None
            header = handle.read(40)
    except (OSError, EOFError):
        return None
    return int.from_bytes(header[32:40], "little") if len(header) == 40 else None


def display_mode(asked, headless):
    if headless:
        return "none"
    if asked != "auto":
        return asked
    if shutil.which("gamescope"):
        return "headless"
    return "desktop"


def is_deck():
    try:
        return Path("/sys/devices/virtual/dmi/id/product_name").read_text().strip() in ("Jupiter", "Galileo")
    except OSError:
        return False


def launcher(app, folder, mode, size, name):
    """APP for rendered_run.py: the app itself, or a script that starts it under headless gamescope."""
    app = Path(app).resolve()
    if mode != "headless":
        return app
    w, _, h = size.partition("x")
    folder.mkdir(parents=True, exist_ok=True)
    script = folder / f"{name}-gamescope.sh"
    script.write_text(f"#!/bin/sh\nexec gamescope --backend headless -W {w} -H {h} -w {w} -h {h} -- '{app}' \"$@\"\n")
    script.chmod(0o755)
    return script


def environment(mode):
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1")
    if mode == "desktop" and sys.platform == "linux":
        env.setdefault("DISPLAY", ":0")
        env.setdefault("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")
    return env


def gpu_name():
    try:
        if sys.platform == "darwin":
            out = subprocess.run(["system_profiler", "SPDisplaysDataType"], capture_output=True, text=True,
                                 timeout=20).stdout
            names = [l.split(":", 1)[1].strip() for l in out.splitlines() if l.strip().startswith("Chipset Model:")]
            return names[0] if names else ""
        out = subprocess.run(["vulkaninfo", "--summary"], capture_output=True, text=True, timeout=20).stdout
        names = [l.split("=", 1)[1].strip() for l in out.splitlines() if l.strip().startswith("deviceName")]
        return names[0] if names else ""
    except (OSError, subprocess.SubprocessError):
        return ""


def rendered_run(app, folder, env, *options):
    """One run with scripts/rendered_run.py; its results.json entry."""
    cmd = [sys.executable, str(HERE / "rendered_run.py"), str(app), str(folder), *map(str, options)]
    subprocess.run(cmd, env=env, stdout=sys.stdout, stderr=subprocess.STDOUT)
    results = folder / "results.json"
    found = json.loads(results.read_text()) if results.exists() else []
    return found[0] if found else {"exit": None, "timed_out": False}


def make_state(path, app, card, folder, env, warp_to=TOUR_WARP, time=None, settle=180):
    """A save state at WARP_TO (stage:room:point[:layer]): warp there with the stage select, see when Link
    arrives, warp again and save the state SETTLE retraces later. Quest logs 1-3 are tried, as a card's save is
    not always in the first. The tour's start is the one at Outset. Returns a message on failure."""
    def warp(name, quest_log, stop_after, *extra):
        rendered_run(app, folder / name, env, "--card", card, "--warp", warp_to, "--warp-file", quest_log,
                     "--stop-after", stop_after, "--max-retraces", 9000, "--timeout", 600, "--stop-on", EMPTY,
                     "--set", "BLUEWAKE_PERF_LOG=1", "--set", "BLUEWAKE_LOOSE_REL_RETRACES=1",
                     *(["--warp-time", time] if time else []), *extra)
        log = folder / name / "log.txt"
        return log.read_text(errors="replace") if log.exists() else ""

    print(f"== making {path.name}: warp {warp_to}{f' at {time}' if time else ''}, saved {settle} retraces after "
          "Link arrives", flush=True)
    for quest_log in (1, 2, 3):
        text = warp(f"find-{quest_log}", quest_log, 120)
        at = text.find("[warp] arrived")
        after = [int(m) for m in PERF.findall(text[at:])] if at >= 0 else []
        if after:
            break
        print(f"== quest log {quest_log} did not get there" + (" (empty)" if EMPTY in text else ""), flush=True)
    else:
        return f"the warp to {warp_to} never arrived from quest logs 1-3 (see {folder}/find-*/log.txt)"
    target = after[0] + settle
    made = folder / "made.bwstate"
    # A warp from boot is not frame-exact, and now and then one stalls at the file select: try again.
    for attempt in (1, 2, 3):
        made.unlink(missing_ok=True)
        text = warp(f"save-{attempt}", quest_log, settle + 240,
                    "--set", f"BLUEWAKE_SAVE_STATE={made.resolve()}@{target}")
        arrived, saved, retrace = text.find("[warp] arrived"), text.find("[state] saved"), state_retrace(made)
        # Saved after Link arrived; the save lands on the first retrace at or after the one asked for.
        if 0 <= arrived < saved and retrace is not None and target <= retrace <= target + 2:
            break
        print(f"== save {attempt}: Link had not arrived by retrace {target}", flush=True)
    else:
        return f"no state saved after Link arrived in 3 tries (see {folder}/save-*/log.txt)"
    path.parent.mkdir(parents=True, exist_ok=True)
    shutil.move(str(made), str(path))
    print(f"== made {path} at retrace {retrace}, from quest log {quest_log}", flush=True)
    return None


def scenes_and_states(args, env, maker):
    """[(scene folder name, state path)], and the tour's start retrace (None with --states); or a message."""
    if args.states:
        states = sorted(p for p in Path(args.states).iterdir() if p.suffix == ".bwstate")
        bad = [p.name for p in states if state_retrace(p) is None]
        if not states:
            return None, None, f"no *.bwstate in {args.states}"
        if bad:
            return None, None, f"not save states: {', '.join(bad)}"
        return [(p.stem, p) for p in states], None, None
    tour = Path(args.cache).expanduser() / "tour.bwstate"
    if state_retrace(tour) is None:
        if not args.card or not Path(args.card).is_file():
            return None, None, f"no tour state in {tour.parent} and no card to make it from (--card)"
        failed = make_state(tour, maker, args.card, Path(args.out) / "make-state", env)
        if failed:
            return None, None, failed
    return [("tour", tour)], state_retrace(tour) + 10, None


def plan(builds, args):
    """[(build, measured)] in the order to run: one build's runs, or warm-ups then A/B/B/A pairs."""
    if len(builds) == 1:
        return [(builds[0], False)] * args.warmup + [(builds[0], True)] * args.runs
    a, b = builds
    return [(a, False), (b, False)] * args.warmup + [(x, True) for _ in range(args.pairs) for x in (a, b, b, a)]


def collect(args):
    builds = [item.partition("=")[0] for item in args.app]
    apps = [(name, Path(item.partition("=")[2])) for name, item in zip(builds, args.app)]
    if len(apps) not in (1, 2) or len(set(builds)) != len(builds):
        print("render_bench: give one or two --app NAME=APP with different names", file=sys.stderr)
        return 2
    missing = [str(app) for _, app in apps if not app.is_file()]
    if missing:
        print(f"render_bench: no such app: {', '.join(missing)}", file=sys.stderr)
        return 2
    # The card is only read, and only to make the tour's state: a missing one matters only then.
    args.card = str(Path(args.card).expanduser()) if args.card else None
    if args.card and not Path(args.card).is_file():
        args.card = None
    mode = display_mode(args.display, args.headless)
    size = args.size or ("1280x800" if is_deck() else "1280x720")
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    env = environment(mode)
    # The tour's state is made rendered (the stage select's warp), even for a --headless benchmark.
    shown = mode if mode != "none" else display_mode(args.display, False)
    maker = launcher(apps[-1][1], out / "launch", shown, size, f"{apps[-1][0]}-make")
    scenes, tour_start, failed = scenes_and_states(args, environment(shown), maker)
    if failed:
        print(f"render_bench: {failed}", file=sys.stderr)
        return 2
    runners = {name: launcher(app, out / "launch", mode, size, name) for name, app in apps}
    stops = bench_tour.DEFAULT_STOPS
    collected = {"host": socket.gethostname(), "machine": platform.platform(), "gpu": gpu_name(),
                 "mode": "headless" if args.headless else "rendered",
                 "display": {"headless": f"headless gamescope {size}",
                             "desktop": "a window" if sys.platform == "darwin" else "desktop session",
                             "none": "no renderer"}[mode],
                 "builds": builds, "apps": {name: app.name for name, app in apps},
                 "kind": "states" if args.states else "tour", "retraces": args.retraces, "every": args.every,
                 "tour_start": tour_start, "stops": [s._asdict() for s in stops] if tour_start else None,
                 "scenes": [], "states": {}, "runs": []}
    for name, state in scenes:
        retrace = state_retrace(state)
        collected["states"][name] = {"file": state.name, "retrace": retrace,
                                     "sha256": hashlib.sha256(state.read_bytes()).hexdigest()}
        if tour_start:
            for i, stop in enumerate(stops):
                first = tour_start + i * STOP_RETRACES
                collected["scenes"].append({"name": stop.name, "log": "tour", "from": first,
                                            "to": first + STOP_RETRACES, "stage": stop.stage})
        else:
            collected["scenes"].append({"name": name, "log": name, "from": retrace,
                                        "to": retrace + args.retraces, "stage": None})
    order = [(build, "measured" if measured else "warm-up") for build, measured in plan(builds, args)]
    if args.cpu_profile:
        # One separate profile pass per build. Sampling changes timing, so these runs never feed the FPS medians.
        order += [(build, "CPU profile") for build in builds]
    if len(builds) == 2 and not args.headless and args.every > 0:
        # Pictures need a paced run: unpaced, a capture lands a retrace or two apart from run to run, a moment
        # later in the game. Paced from a state, runs are frame-exact. These are not measured.
        order += [(build, "pictures") for build in builds]
    print(f"== {len(order)} run(s) of {len(scenes)} state(s) on {collected['host']} "
          f"({collected['gpu'] or 'GPU unknown'}, {collected['mode']}, {collected['display']})", flush=True)
    for n, (build, label) in enumerate(order, 1):
        capture = label == "pictures"
        profile = label == "CPU profile"
        run = {"index": n, "build": build, "measured": label == "measured", "captured": capture,
               "profiled": profile, "logs": {}}
        for name, state in scenes:
            folder = out / "runs" / f"{n:02d}-{build}" / name
            start = state_retrace(state)
            options = ["--state", state.resolve(), "--timeout", args.timeout,
                       "--set", "BLUEWAKE_PERF_LOG=1", "--set", "BLUEWAKE_GUEST_CHECKPOINT_INTERVAL=300",
                       "--set", "BLUEWAKE_LOOSE_REL_RETRACES=1"]
            if args.card:
                options += ["--card", Path(args.card).resolve()]
            if tour_start:
                options += ["--max-retraces", bench_tour.tour_end(stops, STOP_RETRACES, tour_start),
                            "--set", f"BLUEWAKE_TEST_WARP={bench_tour.warp_string(stops, STOP_RETRACES, tour_start)}"]
            else:
                options += ["--max-retraces", start + args.retraces]
            if args.headless:
                options += ["--set", "BLUEWAKE_RENDERER=headless"]
            if not capture:
                # Unpaced and without vsync (on X11 it is on by default, and holds the game at 60 frames a second).
                options += ["--unpaced", "--set", "BLUEWAKE_VSYNC=0"] + ([] if args.headless else ["--gpu-prof"])
                if profile:
                    # Each scene in this log from 300 retraces in (as FPS is measured) to a little before its end,
                    # so perf has detached before the next warp or the game's exit.
                    options += ["--cpu-profile"]
                    for scene in collected["scenes"]:
                        if scene["log"] == name:
                            options += ["--profile-window", f"{slug(scene['name'])}:"
                                        f"{scene['from'] + bench_tour.SETTLE}:{scene['to'] - PROFILE_MARGIN}"]
            else:
                options += ["--capture", f"{(tour_start or start) + bench_tour.SETTLE}:{args.every}"]
            print(f"== [{n}/{len(order)}] {build} {label}: {name}", flush=True)
            result = rendered_run(runners[build], folder, env, *options)
            run["logs"][name] = {"folder": str(folder.relative_to(out)), "exit": result.get("exit"),
                                 "timed_out": result.get("timed_out", False),
                                 "safe_mode": result.get("safe_mode", False), "seconds": result.get("seconds")}
        collected["runs"].append(run)
    if len(builds) == 2 and not args.headless and args.every > 0:
        file_frames(collected, out)
    (out / "collected.json").write_text(json.dumps(collected, indent=1))
    print(f"== wrote {out / 'collected.json'}", flush=True)
    return 0


def file_frames(collected, out):
    """Each scene's captured frames as frames/SCENE/BUILD (hard links), with their SHA-256 in hashes.json."""
    for scene in collected["scenes"]:
        folder = out / "frames" / slug(scene["name"])
        shutil.rmtree(folder, ignore_errors=True)
        hashes = {}
        for run in collected["runs"]:
            if not run["captured"]:
                continue
            source = out / run["logs"][scene["log"]]["folder"]
            target = folder / run["build"]
            target.mkdir(parents=True)
            hashes[run["build"]] = {}
            # A capture lands on the first retrace at or after the one asked for: frames are matched by their
            # capture's slot, named frame-SLOT.ppm.
            first, every = scene["from"] + bench_tour.SETTLE, collected["every"]
            for frame in sorted(source.glob("frame-*.ppm")):
                retrace = int(frame.stem.split("-")[1])
                if first <= retrace < scene["to"]:
                    name = f"frame-{round((retrace - first) / every)}.ppm"
                    os.link(frame, target / name)
                    hashes[run["build"]][name] = hashlib.sha256(frame.read_bytes()).hexdigest()
        folder.mkdir(parents=True, exist_ok=True)
        (folder / "hashes.json").write_text(json.dumps(hashes, indent=1))


def slug(name):
    return re.sub(r"[^A-Za-z0-9_.-]+", "-", name).strip("-")


def differing(out):
    """Scene folders under frames/ whose frames differ between the two builds: the frames to copy back."""
    names = []
    for hashes in sorted(Path(out).glob("frames/*/hashes.json")):
        found = list(json.loads(hashes.read_text()).values())
        if len(found) == 2 and (found[0] != found[1] or not found[0]):
            names.append(hashes.parent.name)
    return names


# --- here: analyze and report -----------------------------------------------------------------------------------

def gpu_by_retrace(text):
    """(retrace, GPU busy ms a frame) for each [gpu-prof] report, at the last [perf] retrace before it."""
    out, retrace = [], None
    for line in text.splitlines():
        perf = PERF.match(line)
        if perf:
            retrace = int(perf.group(1))
            continue
        gpu = GPU.match(line)
        if gpu and retrace is not None:
            out.append((retrace, float(gpu.group(1))))
    return out


def scene_numbers(text, scene):
    """A scene's numbers from its run's log, as scripts/bench_tour.py takes a stop's."""
    perfs, dips, loads, _ = bench_tour.scan_log(text)
    window = bench_tour._window(perfs, dips, loads, scene["from"], scene["to"], scene["stage"], "", scene["name"])
    gpus = [ms for retrace, ms in gpu_by_retrace(text)
            if scene["from"] + bench_tour.SETTLE <= retrace < scene["to"]]
    return {"fps": window["game_frames_per_second"], "busy": window["busy_percent"],
            "gpu_ms": round(statistics.mean(gpus), 3) if gpus else None,
            "loaded": window["loaded"] if scene["stage"] else bool(perfs), "dips": window["dip_causes"]}


def mac_app(path):
    """The BlueWake.app a Mac build is: the app, its executable, or a build folder holding packaged/BlueWake.app;
    None if PATH is not one."""
    path = Path(path).resolve()
    for app in (*[p for p in path.parents if p.suffix == ".app"][:1], path, path / "BlueWake.app",
                path / "packaged" / "BlueWake.app"):
        if app.suffix == ".app" and (app / "Contents" / "MacOS" / "BlueWake").is_file():
            return app
    return None


def linux_folder(path):
    """The folder a Linux folder build is (bluewake beside its game module, as the portable builder makes it): the
    folder, its BlueWake/ subfolder, or its bluewake; None if PATH is not one."""
    path = Path(path).resolve()
    for folder in (path.parent if path.name == "bluewake" else path, path / "BlueWake"):
        if (folder / "bluewake").is_file() and os.access(folder / "bluewake", os.X_OK) and \
                not (folder / "BlueWake-x86_64.AppImage").is_file():
            return folder
    return None


def build_image(path):
    """A build's AppImage: the file itself, or BlueWake-x86_64.AppImage (else the first *.AppImage) in a folder.
    A Mac build's is its app's executable, a Linux folder build's its bluewake."""
    path = Path(path).resolve()
    app = mac_app(path)
    if app:
        return app / "Contents" / "MacOS" / "BlueWake"
    folder = linux_folder(path)
    if folder:
        return folder / "bluewake"
    if path.is_dir():
        images = sorted(path.glob("*.AppImage"))
        return path / "BlueWake-x86_64.AppImage" if (path / "BlueWake-x86_64.AppImage").is_file() else images[0]
    return path


def provenance(path):
    """BuilderProvenance.json, what the builder says a build was made from; {} without one."""
    try:
        return json.loads(Path(path).read_text())
    except (OSError, ValueError):
        return {}


def build_info(path):
    """A build's commit (from build_script's paths: build/ALIAS-COMMIT/ or build/artifacts/ALIAS/COMMIT/) and the
    SHA-256 of its AppImage."""
    image = build_image(path)
    app = mac_app(image)
    folder = None if app else linux_folder(image)
    if app or folder:
        # A Mac app or a Linux folder build says what it was built from, its game module's hash too.
        made = provenance(app / "Contents" / "Resources" / "BuilderProvenance.json" if app
                          else folder / "BuilderProvenance.json")
        commit = (made.get("source_commit") or "")[:12] or None
        if folder and not commit:
            commit = next((m.group(1) for part in reversed(folder.parts) for m in [COMMIT.match(part)] if m), None)
        return {"path": str(app or folder), "commit": commit, "modified": made.get("source_modified"),
                "sha256": hashlib.sha256(image.read_bytes()).hexdigest(), "module_sha256": made.get("module_sha256")}
    commit = next((m.group(1) for part in reversed(image.parent.parts) for m in [COMMIT.match(part)] if m), None)
    return {"path": str(image), "commit": commit, "sha256": hashlib.sha256(image.read_bytes()).hexdigest()}


def median(values):
    values = [v for v in values if v is not None]
    return round(statistics.median(values), 3) if values else None


def perf_symbols(path, limit=12):
    """Top self-cost rows of a perf report (--sort dso,symbol): [{overhead, dso, symbol}]."""
    found = []
    for line in Path(path).read_text(errors="replace").splitlines() if Path(path).is_file() else []:
        # Table rows: "  12.34%  DSO  [.] SYMBOL". Splitting avoids regex backtracking through long symbol names.
        tokens = line.split()
        if len(tokens) < 4 or not tokens[0].endswith("%") or not re.fullmatch(r"\[.\]", tokens[2]):
            continue
        try:
            overhead = float(tokens[0][:-1])
        except ValueError:
            continue
        symbol = re.sub(r"\s+-\s+-\s*$", "", " ".join(tokens[3:]))  # perf may append empty IPC columns
        found.append({"overhead": overhead, "dso": tokens[1], "symbol": symbol})
    return sorted(found, key=lambda item: item["overhead"], reverse=True)[:limit]


def cpu_profile(folder, name):
    """One profiled scene: counters per game frame actually covered, for the busiest thread (the game thread when
    the game is CPU-bound) and for all threads, and the busiest thread's symbols; None without its window."""
    window_file = folder / f"perf-{name}-window.json"
    if not window_file.is_file():
        return None
    window = json.loads(window_file.read_text())
    threads = rendered_run_module().per_thread_stat(folder / f"perf-{name}-stat.csv")
    covered = (window["to_retrace"] or 0) - (window["from_retrace"] or 0)
    if not threads or covered <= 0:
        return None
    frames = covered / 2

    def numbers(counts):
        get = lambda k: counts.get(k) or None
        ratio = lambda a, b, scale=1: round(scale * get(a) / get(b), 3) if get(a) and get(b) else None
        return {"cycles_per_frame": round(get("cycles") / frames) if get("cycles") else None,
                "instructions_per_frame": round(get("instructions") / frames) if get("instructions") else None,
                "ipc": ratio("instructions", "cycles"), "branch_miss_percent": ratio("branch-misses", "branches", 100),
                "cache_miss_percent": ratio("cache-misses", "cache-references", 100)}

    busiest = window.get("busiest_tid")
    total = {}
    for counts in threads.values():
        for event, value in counts.items():
            total[event] = total.get(event, 0) + value
    game = threads.get(busiest, {})
    return {"retraces": [window["from_retrace"], window["to_retrace"]], "seconds": window["seconds"],
            "threads": len(threads),
            "game_busy_percent": round(100 * game.get("task-clock", 0) / 1000 / window["seconds"], 1)
            if window["seconds"] else None,
            "game": numbers(game), "all": numbers(total),
            "symbols": perf_symbols(folder / f"perf-{name}-busiest.txt")}


def rendered_run_module():
    import rendered_run
    return rendered_run


def geomean(values):
    values = [v for v in values if v]
    return round(statistics.geometric_mean(values), 3) if values else None


def change(a, b):
    return round(100 * (a - b) / b, 2) if a is not None and b else None


def analyze(out, build_paths, into=None):
    """Judge OUT (collect's folder); write summary.json, report.md, report.txt, report.html and each changed
    scene's worst-SCENE.png, side by side, into INTO (default OUT)."""
    out = Path(out)
    into = Path(into or out)
    into.mkdir(parents=True, exist_ok=True)
    collected = json.loads((out / "collected.json").read_text())
    builds = collected["builds"]
    info = {name: build_info(path) for name, path in build_paths.items()}
    measured = [run for run in collected["runs"] if run["measured"]]
    profiled = [run for run in collected["runs"] if run.get("profiled")]
    per = {b: {s["name"]: [] for s in collected["scenes"]} for b in builds}
    overall = {b: [] for b in builds}
    checkpoints, problems, loaded_runs = {}, [], 0
    for run in measured:
        texts = {}
        for name, log in run["logs"].items():
            path = out / log["folder"] / "log.txt"
            texts[name] = path.read_text(errors="replace") if path.exists() else ""
            if log["timed_out"] or log["exit"] not in (0, 1) or log["safe_mode"]:
                problems.append(f"run {run['index']} ({run['build']}) {name}: "
                                + ("timed out" if log["timed_out"] else "safe mode" if log["safe_mode"]
                                   else f"exit {log['exit']}"))
            checkpoints.setdefault(name, []).append((run, bench_tour.scan_log(texts[name])[3]))
        numbers = [scene_numbers(texts[s["log"]], s) for s in collected["scenes"]]
        for scene, n in zip(collected["scenes"], numbers):
            per[run["build"]][scene["name"]].append(n)
        loaded_runs += all(n["loaded"] for n in numbers)
        overall[run["build"]].append(geomean([n["fps"] for n in numbers]))
    # The same game state at the same retraces in every run of every build.
    differ = []
    for name, found in checkpoints.items():
        first_run, reference = found[0]
        for run, points in found[1:]:
            at = bench_tour.first_differing_retrace(reference, points)
            if at is not None:
                differ.append(f"{name}: run {run['index']} ({run['build']}) differs from run "
                              f"{first_run['index']} ({first_run['build']}) at retrace {at}")
    counts = sorted({len(points) for found in checkpoints.values() for _, points in found})
    summary = {key: collected[key] for key in ("host", "machine", "gpu", "mode", "display", "kind", "retraces")}
    summary.update(builds=builds, measured=len(measured),
                   warmups=sum(not r["measured"] and not r["captured"] and not r.get("profiled")
                               for r in collected["runs"]),
                   picture_runs=sum(r["captured"] for r in collected["runs"]), profile_runs=len(profiled),
                   runs_per_build={b: sum(r["build"] == b for r in measured) for b in builds},
                   info=info, states=collected["states"], problems=problems, checkpoint_differences=differ,
                   checkpoint_counts=counts, loaded_runs=loaded_runs, scenes=[], pictures=[], cpu_profiles=[])
    for scene in collected["scenes"]:
        row = {"name": scene["name"]}
        for b in builds:
            found = per[b][scene["name"]]
            row[b] = {"fps": median(n["fps"] for n in found), "busy": median(n["busy"] for n in found),
                      "gpu_ms": median(n["gpu_ms"] for n in found),
                      "loaded": f"{sum(n['loaded'] for n in found)}/{len(found)}",
                      "dips": sorted({d for n in found for d in n["dips"]})}
        summary["scenes"].append(row)
    summary["overall"] = {b: {"fps": median(overall[b]),
                              "busy": median(row[b]["busy"] for row in summary["scenes"]),
                              "gpu_ms": median(row[b]["gpu_ms"] for row in summary["scenes"])} for b in builds}
    if len(builds) == 2:
        a, b = builds
        for row in [*summary["scenes"], summary["overall"]]:
            row["change"] = {k: change(row[a][k], row[b][k]) for k in ("fps", "busy", "gpu_ms")}
        summary["pictures"] = judge_pictures(out, collected, builds)
    for run in profiled:
        for scene in collected["scenes"]:
            found = cpu_profile(out / run["logs"][scene["log"]]["folder"], slug(scene["name"]))
            if found is None:
                problems.append(f"CPU profile of {scene['name']} ({run['build']}): perf covered no retraces")
            else:
                summary["cpu_profiles"].append({"build": run["build"], "scene": scene["name"], **found})
    for p in summary["pictures"]:
        if p.get("sheet"):
            name = f"worst-{slug(p['name'])}.png"
            shutil.copy(out / p["sheet"], into / name)
            p["sheet"] = name
    (into / "summary.json").write_text(json.dumps(summary, indent=1))
    (into / "report.md").write_text(report(summary))
    (into / "report.txt").write_text(report_text(summary))
    (into / "report.html").write_text(report_html(summary))
    return 1 if problems or differ or any(p["verdict"] in ("changed", "failed") for p in summary["pictures"]) else 0


def judge_pictures(out, collected, builds):
    """Each scene's frames, A against B: identical by their hashes, else frame_compare.py check."""
    pictures = []
    for scene in collected["scenes"]:
        folder = out / "frames" / slug(scene["name"])
        hashes = json.loads((folder / "hashes.json").read_text()) if (folder / "hashes.json").exists() else {}
        a, b = (hashes.get(x, {}) for x in builds)
        row = {"name": scene["name"], "images": len(a)}
        if not a or not b:
            row.update(verdict="failed", why="no frames")
        elif a == b:
            row.update(verdict="identical", why="")
        elif not any((folder / builds[0]).glob("*.ppm")):
            row.update(verdict="failed", why="frames differ but were not copied here")
        else:
            check = folder / "check.json"
            subprocess.run([sys.executable, str(HERE / "frame_compare.py"), "check", str(folder / builds[0]),
                            str(folder / builds[1]), "--json", str(check), "--sheet", str(folder / "worst.png"),
                            "--region", "0,0.1,1,0.9"], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
            found = json.loads(check.read_text()) if check.exists() else {"verdict": "failed", "why": "no check"}
            row.update(verdict=found["verdict"], why=found.get("why", ""), min_psnr=found.get("min_psnr"),
                       sheet=str((folder / "worst.png").relative_to(out)) if (folder / "worst.png").exists() else "")
        pictures.append(row)
    return pictures


def report_blocks(summary):
    """The report as blocks: ("h2"|"h3"|"p", text), ("ul", [items]) or ("table", header, rows, numeric columns).
    Text uses `code` and **bold**, which each format renders its own way."""
    builds = summary["builds"]
    info = summary["info"]
    name = lambda b: f"`{info[b]['commit'] or 'unknown'}`" if b in info else b
    fmt = lambda v: "-" if v is None else f"{v:.3f}" if isinstance(v, float) else f"{v}"
    pct = lambda v: "-" if v is None else f"**{v:+.2f}%**"
    two = len(builds) == 2
    blocks = [("h2", f"BlueWake {'A/B ' if two else ''}benchmark, {summary['mode']}")]
    blocks.append(("p", f"Compared {name(builds[0])} (A) against {name(builds[1])} (B, the reference) on "
                        f"{summary['host']}." if two else f"Measured {name(builds[0])} on {summary['host']}."))
    scenes = "the benchmark tour's five stops" if summary["kind"] == "tour" else \
        f"{len(summary['states'])} save state(s), {summary['retraces']} retraces each"
    method = [f"{summary['host']}: {summary['machine']}; GPU {summary['gpu'] or 'unknown'}",
              f"{summary['mode'].capitalize()} ({summary['display']}), unpaced, vsync off, {scenes}",
              f"{summary['measured']} measured runs"
              + (f" ({', '.join(f'{v} of {k}' for k, v in summary['runs_per_build'].items())}), "
                 "interleaved A/B/B/A" if two else "")
              + (f", {summary['warmups']} warm-up runs not measured" if summary["warmups"] else "")]
    if summary["picture_runs"]:
        method.append("Pictures from one paced run of each build, not measured")
    if summary.get("profile_runs"):
        method.append(f"{summary['profile_runs']} separate Linux perf runs, excluded from FPS results")
    method.append(f"{summary['loaded_runs']}/{summary['measured']} runs loaded every scene")
    if summary["checkpoint_differences"]:
        method.append(f"**Guest checkpoints differ**: {summary['checkpoint_differences'][0]}")
    else:
        method.append("Guest checkpoints identical across every run" + (" of both builds" if two else "")
                      + f" ({', '.join(map(str, summary['checkpoint_counts']))} each)")
    for found in summary["states"].values():
        method.append(f"State `{found['file']}` at retrace {found['retrace']}, SHA-256 `{found['sha256'][:16]}…`")
    blocks += [("h3", "Methodology"), ("ul", method), ("h3", "Builds")]
    rows = []
    for b in builds:
        o = summary["overall"][b]
        row = [f"{b}{' (reference)' if two and b == builds[1] else ''}", name(b), fmt(o["fps"]), fmt(o["busy"]),
               fmt(o["gpu_ms"])]
        if two:
            row.append(pct(summary["overall"]["change"]["fps"]) if b == builds[0] else "")
        rows.append(row)
    blocks.append(("table", ["Build", "Commit", "FPS", "Busy %", "GPU ms"] + (["FPS change"] if two else []), rows,
                   {2, 3, 4, 5}))
    blocks += [("p", "FPS is game frames a second: each run's geometric mean over the scenes, the median over runs. "
                     "Busy and GPU ms are the median over the scenes."), ("h3", "Scenes")]
    if two:
        a, b = builds
        rows = [[row["name"], fmt(row[b]["fps"]), fmt(row[a]["fps"]), pct(row["change"]["fps"]),
                 fmt(row[b]["busy"]), fmt(row[a]["busy"]), pct(row["change"]["busy"]),
                 fmt(row[b]["gpu_ms"]), fmt(row[a]["gpu_ms"]), pct(row["change"]["gpu_ms"]),
                 f"{row[b]['loaded']}, {row[a]['loaded']}"] for row in summary["scenes"]]
        blocks.append(("table", ["Scene", f"FPS {b}", f"FPS {a}", "FPS", f"Busy % {b}", f"Busy % {a}", "Busy",
                                 f"GPU ms {b}", f"GPU ms {a}", "GPU", f"Loaded {b}, {a}"], rows, set(range(1, 10))))
    else:
        b = builds[0]
        rows = [[row["name"], fmt(row[b]["fps"]), fmt(row[b]["busy"]), fmt(row[b]["gpu_ms"]), row[b]["loaded"],
                 ", ".join(row[b]["dips"]) or "-"] for row in summary["scenes"]]
        blocks.append(("table", ["Scene", "FPS", "Busy %", "GPU ms", "Loaded", "Slowdowns"], rows, {1, 2, 3}))
    if summary["pictures"]:
        rows = [[p["name"], p["verdict"], str(p["images"]), fmt(p.get("min_psnr")),
                 (p["why"] or "-") + (f" (`{p['sheet']}`)" if p.get("sheet") else "")] for p in summary["pictures"]]
        blocks += [("h3", "Pictures"), ("table", ["Scene", "Verdict", "Images", "Min PSNR", "Why"], rows, {2, 3})]
    if summary.get("cpu_profiles"):
        profiles = summary["cpu_profiles"]
        rows = [[p["scene"], p["build"], fmt(p["game"]["cycles_per_frame"]), fmt(p["game"]["instructions_per_frame"]),
                 fmt(p["game"]["ipc"]), fmt(p["game"]["branch_miss_percent"]), fmt(p["game"]["cache_miss_percent"]),
                 fmt(p["game_busy_percent"]), fmt(p["all"]["cycles_per_frame"])] for p in profiles]
        blocks += [("h3", "CPU profiles"),
                   ("p", "One separate Linux perf run of each build, not measured (sampling slows the game). Each "
                         "scene is profiled from 300 retraces in to shortly before its end, and counts are per game "
                         "frame (2 retraces) actually covered. The game thread is the busiest thread: it is what "
                         "holds FPS when the game is CPU-bound; the workers run beside it. Raw files are in "
                         "`raw/runs/` (`perf-SCENE-*`)."),
                   ("table", ["Scene", "Build", "Game cycles/frame", "Game instr./frame", "IPC", "Branch miss %",
                              "Cache miss %", "Game thread busy %", "All threads cycles/frame"], rows,
                    {2, 3, 4, 5, 6, 7, 8})]
        if len(builds) == 2:
            a, b = builds
            by = {(p["scene"], p["build"]): p for p in profiles}
            rows = []
            for row in summary["scenes"]:
                pa, pb = by.get((row["name"], a)), by.get((row["name"], b))
                if pa and pb:
                    rows.append([row["name"], pct(change(pa["game"]["cycles_per_frame"], pb["game"]["cycles_per_frame"])),
                                 pct(change(pa["game"]["instructions_per_frame"],
                                            pb["game"]["instructions_per_frame"])),
                                 pct(change(pa["game"]["ipc"], pb["game"]["ipc"])),
                                 pct(change(pa["all"]["cycles_per_frame"], pb["all"]["cycles_per_frame"])),
                                 pct(row["change"]["fps"])])
            blocks += [("p", f"Change, {a} against {b}. In a CPU-bound scene, game-thread cycles per frame should "
                             "fall about as much as FPS rises; if they don't, look at what else the scene waits on."),
                       ("table", ["Scene", "Game cycles/frame", "Game instr./frame", "IPC",
                                  "All threads cycles/frame", "FPS"], rows, {1, 2, 3, 4, 5})]
        blocks.append(("p", "`func_ADDR` names a chunk of translated code, not one function: look its address up in "
                            "`config/GZLE01/symbols.txt` (docs/PROFILING.md)."))
        for p in profiles:
            rows = [[fmt(sym["overhead"]), sym["dso"], sym["symbol"]] for sym in p["symbols"][:10]]
            if rows:
                blocks += [("h3", f"Game thread — {p['scene']} ({p['build']})"),
                           ("table", ["Self %", "DSO", "Symbol"], rows, {0})]
    blocks += [("h3", "Tested artifacts"),
               ("ul", [f"{b}: {name(b)}" + (" (with changes not committed)" if info[b].get("modified") else "")
                      + f", {'executable ' if info[b].get('module_sha256') else ''}SHA-256 `{info[b]['sha256']}`"
                      + (f", game module `{info[b]['module_sha256']}`" if info[b].get("module_sha256") else "")
                      for b in builds if b in info])]
    # A refresh cap holds every build at 60; one build that happens to run near 60 is not one.
    capped = [row["name"] for row in summary["scenes"]
              if all(row[b]["fps"] is not None and abs(row[b]["fps"] - 60) < 0.3 for b in builds)]
    if capped:
        blocks += [("h3", "Warning"),
                   ("p", f"FPS is at 60 in {', '.join(dict.fromkeys(capped))}: the display's refresh may be holding "
                         "it there (a build without `BLUEWAKE_VSYNC=0`), so FPS does not show a difference. Compare "
                         "busy % instead.")]
    if summary["problems"]:
        blocks += [("h3", "Problems"), ("ul", summary["problems"])]
    if two and summary["overall"]["change"]["fps"] is not None:
        verdict = summary["overall"]["change"]["fps"]
        blocks += [("h3", "Conclusion"),
                   ("p", (f"{name(builds[0])} was {'faster' if verdict > 0 else 'slower'} overall by "
                          f"**{abs(verdict):.2f}%**." if abs(verdict) >= 0.005 else
                          f"{name(builds[0])} ran at the same speed overall.")
                    + (" Identical guest checkpoints mean both builds ran the game the same way."
                       if not summary["checkpoint_differences"] else ""))]
    return blocks


def report(summary):
    """The report in Markdown, ready for a pull request."""
    out = []
    for kind, *body in report_blocks(summary):
        if kind in ("h2", "h3"):
            out += [f"{'#' * int(kind[1])} {body[0]}", ""]
        elif kind == "p":
            out += [body[0], ""]
        elif kind == "ul":
            out += [f"- {item}" for item in body[0]] + [""]
        else:
            head, rows, numeric = body
            out += ["| " + " | ".join(head) + " |",
                    "|" + "".join(" ---: |" if i in numeric else " --- |" for i in range(len(head)))]
            out += ["| " + " | ".join(row) + " |" for row in rows] + [""]
    return "\n".join(out).rstrip() + "\n"


def plain(text):
    return re.sub(r"\*\*(.+?)\*\*|`(.+?)`", lambda m: m.group(1) or m.group(2), text)


def report_text(summary):
    """The report as plain text, with the tables aligned."""
    out = []
    for kind, *body in report_blocks(summary):
        if kind in ("h2", "h3"):
            title = plain(body[0])
            out += [title, ("=" if kind == "h2" else "-") * len(title), ""]
        elif kind == "p":
            out += [plain(body[0]), ""]
        elif kind == "ul":
            out += [f"- {plain(item)}" for item in body[0]] + [""]
        else:
            head, rows, numeric = body
            table = [head] + [[plain(c) for c in row] for row in rows]
            widths = [max(len(r[i]) for r in table) for i in range(len(head))]
            line = lambda r: "  ".join(c.rjust(w) if i in numeric else c.ljust(w)
                                       for i, (c, w) in enumerate(zip(r, widths))).rstrip()
            out += [line(head), "  ".join("-" * w for w in widths)] + [line(r) for r in table[1:]] + [""]
    return "\n".join(out).rstrip() + "\n"


def report_html(summary):
    """The report as one self-contained HTML page."""
    import html
    def inline(text):
        text = html.escape(text, quote=False)
        text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
        return re.sub(r"`(.+?)`", r"<code>\1</code>", text)
    body, title = [], "BlueWake benchmark"
    for kind, *rest in report_blocks(summary):
        if kind in ("h2", "h3"):
            title = plain(rest[0]) if kind == "h2" else title
            body.append(f"<{kind}>{inline(rest[0])}</{kind}>")
        elif kind == "p":
            body.append(f"<p>{inline(rest[0])}</p>")
        elif kind == "ul":
            body.append("<ul>" + "".join(f"<li>{inline(item)}</li>" for item in rest[0]) + "</ul>")
        else:
            head, rows, numeric = rest
            cell = lambda tag, i, c: f"<{tag}{' class=num' if i in numeric else ''}>{inline(c)}</{tag}>"
            body.append("<div class=wrap><table><thead><tr>" + "".join(cell("th", i, c) for i, c in enumerate(head))
                        + "</tr></thead><tbody>" + "".join("<tr>" + "".join(cell("td", i, c) for i, c in enumerate(r))
                                                           + "</tr>" for r in rows) + "</tbody></table></div>")
    style = ("body{font:15px/1.5 system-ui,sans-serif;max-width:72rem;margin:2rem auto;padding:0 1rem;color:#1f2328;"
             "background:#fff}h2{border-bottom:1px solid #d1d9e0;padding-bottom:.3em}h3{margin-top:1.6em}"
             ".wrap{overflow-x:auto}table{border-collapse:collapse;margin:.5em 0}th,td{border:1px solid #d1d9e0;"
             "padding:.3em .7em;white-space:nowrap}th{background:#f6f8fa}td.num,th.num{text-align:right;"
             "font-variant-numeric:tabular-nums}code{font:13px ui-monospace,monospace;background:#eff1f3;"
             "padding:.1em .3em;border-radius:4px}@media(prefers-color-scheme:dark){body{background:#0d1117;"
             "color:#e6edf3}th{background:#161b22}th,td,h2{border-color:#30363d}code{background:#262c36}}")
    return (f"<!doctype html>\n<html lang=en><head><meta charset=utf-8><meta name=viewport "
            f"content=\"width=device-width,initial-scale=1\"><title>{html.escape(title)}</title><style>{style}"
            f"</style></head><body>\n" + "\n".join(body) + "\n</body></html>\n")


def make_state_command(args):
    state, card = Path(args.state), Path(args.card).expanduser()
    if not card.is_file():
        print(f"render_bench: no card at {card} (--card)", file=sys.stderr)
        return 2
    if not re.match(r"^[A-Za-z0-9_]+:-?\d+:\d+(:\d+)?$", args.warp):
        print("render_bench: --warp is stage:room:point[:layer]", file=sys.stderr)
        return 2
    mode = display_mode(args.display, False)
    size = args.size or ("1280x800" if is_deck() else "1280x720")
    work = state.parent / f".{state.stem}-making"
    shutil.rmtree(work, ignore_errors=True)
    app = launcher(build_image(args.app), work / "launch", mode, size, "app")
    failed = make_state(state, app, str(card.resolve()), work, environment(mode), args.warp, args.time, args.settle)
    if failed:
        print(f"render_bench: {failed} (kept {work})", file=sys.stderr)
        return 1
    shutil.rmtree(work, ignore_errors=True)
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog="\n".join(__doc__.splitlines()[2:]))
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("collect")
    p.add_argument("out")
    p.add_argument("--app", action="append", required=True, metavar="NAME=APP")
    p.add_argument("--states")
    p.add_argument("--card")
    p.add_argument("--cache", default="~/bluewake-render")
    p.add_argument("--pairs", type=int, default=2)
    p.add_argument("--runs", type=int, default=3)
    p.add_argument("--warmup", type=int, default=0)
    p.add_argument("--headless", action="store_true")
    p.add_argument("--cpu-profile", action="store_true",
                   help="profile measured scenes with Linux perf; do not use their FPS as benchmark results")
    p.add_argument("--retraces", type=int, default=STOP_RETRACES)
    p.add_argument("--every", type=int, default=300)
    p.add_argument("--timeout", type=int, default=1200)
    p.add_argument("--display", choices=("auto", "headless", "desktop"), default="auto")
    p.add_argument("--size")
    p = sub.add_parser("make-state")
    p.add_argument("state", help="the state file to write, e.g. states/2-windfall.bwstate")
    p.add_argument("--app", required=True)
    p.add_argument("--warp", required=True, help="stage:room:point[:layer], e.g. sea:11:1:0 (STAGE_SELECT.md)")
    p.add_argument("--card", default="~/.local/share/BlueWake/GZLE01.card")
    p.add_argument("--time", help="0-23, morning, noon, evening or night")
    p.add_argument("--settle", type=int, default=180, help="retraces after Link arrives (default 180, 3 s)")
    p.add_argument("--display", choices=("auto", "headless", "desktop"), default="auto")
    p.add_argument("--size")
    p = sub.add_parser("differing")
    p.add_argument("out")
    p = sub.add_parser("analyze")
    p.add_argument("out")
    p.add_argument("--build", action="append", default=[], metavar="NAME=PATH")
    p.add_argument("--into", help="where the reports go (default OUT)")
    p = sub.add_parser("report")
    p.add_argument("summary")
    args = parser.parse_args()
    if args.command == "collect":
        sys.exit(collect(args))
    if args.command == "make-state":
        sys.exit(make_state_command(args))
    if args.command == "differing":
        print("\n".join(differing(args.out)))
        sys.exit(0)
    if args.command == "analyze":
        sys.exit(analyze(args.out, dict(item.partition("=")[::2] for item in args.build), args.into))
    print(report(json.loads(Path(args.summary).read_text())), end="")


if __name__ == "__main__":
    main()
