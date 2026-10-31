#!/usr/bin/env python3
"""Measure BlueWake as it runs in Steam Deck Game Mode, window by window, from another machine.

  python3 scripts/gamemode_measure.py sample deck@steamdeck LABEL [--windows 4] [--seconds 15] [--out DIR]
  python3 scripts/gamemode_measure.py report DIR [--base LABEL,...] [--test LABEL,...]

`sample` measures the BlueWake already running on the Deck (launched from Game Mode, with the player's own
settings) for --windows windows of --seconds each and appends them, under LABEL, to DIR/windows.jsonl here.
Each window records:

- the game thread (BlueWake's main thread): clock, IPC and busy share, from `perf stat -t` (no root needed);
- the APU: average power, GPU clock and the CPU cores' mean clock, sampled each second from sysfs;
- from the session log, the lines the build writes during the window: Smooth Motion's [interp-stats]
  (DOL_AURORA_FRAME_INTERP_STATS=1), [fps-dip] seconds and the last [gpu-prof] report (DOL_AURORA_GPU_PROF=1/2);
- the settings under test, read from ~/.config/BlueWake/settings.ini, and the process's DOL_/BLUEWAKE_ variables.
  The settings are what the file holds: a build that predates a setting ignores it, so check the build too.

Change a setting from the in-game menu between labels (it applies live) and sample again: the same session,
view and clocks, so an A/B is not confounded by a relaunch. Sample the base twice (an A/A) to see the noise.

`report` prints a Markdown table of every window and, with --base and --test, the means of each side, the change,
and whether it exceeds the spread of the base windows (docs/PROFILING.md, "Measuring in Game Mode").

The same file runs on the Deck as the sampler (`remote`), sent over SSH: only Python's standard library is used.
"""
import argparse
import json
import os
import re
import shlex
import statistics
import subprocess
import sys
import threading
import time
from pathlib import Path

DEFAULT_KEYS = ("DOL_AURORA_RENDER_SCALE", "DOL_AURORA_FRAME_INTERP", "DOL_AURORA_EFFICIENT_EFFECTS")

# --- parsing (shared by the sampler and the tests) -------------------------------------------------------------


def parse_perf(text):
    """perf stat -x';' output: task-clock (ms), cycles and instructions of the game thread."""
    values = {}
    for line in text.splitlines():
        fields = line.split(";")
        if len(fields) < 3 or not fields[0].strip() or fields[0].startswith("<"):
            continue
        event = fields[2].split(":")[0]
        if event in ("task-clock", "cycles", "instructions"):
            try:
                values[event] = float(fields[0])
            except ValueError:
                pass
    return values


def game_thread(perf, seconds):
    """Clock (GHz), IPC and busy share of the game thread over a window, from parse_perf()."""
    clock, cycles, instructions = perf.get("task-clock"), perf.get("cycles"), perf.get("instructions")
    if not clock or not cycles or not instructions:
        return {}
    return {"ghz": round(cycles / (clock * 1e6), 3), "ipc": round(instructions / cycles, 3),
            "busy": round(clock / (seconds * 1000.0), 3)}


def current_sclk(text):
    """The GPU clock (MHz) marked current in pp_dpm_sclk."""
    for line in text.splitlines():
        if line.rstrip().endswith("*"):
            match = re.search(r"(\d+)\s*Mhz", line, re.I)
            if match:
                return int(match.group(1))
    return None


def mean_cpu_mhz(cpuinfo):
    """The mean 'cpu MHz' over the cores in /proc/cpuinfo."""
    values = [float(v) for v in re.findall(r"^cpu MHz\s*:\s*([\d.]+)", cpuinfo, re.M)]
    return round(sum(values) / len(values)) if values else None


def parse_interp_stats(lines):
    """The sum of every count in the [interp-stats] lines, and of each overload's count and wait (ms)."""
    counts, overloads = {}, {}
    for line in lines:
        if "[interp-stats]" not in line:
            continue
        for key, value in re.findall(r"(\w+)=(\S+)", line.split("[interp-stats]", 1)[1]):
            if key == "budget":
                continue
            if "/" in value:
                number, wait = value.split("/", 1)
                entry = overloads.setdefault(key, [0, 0.0])
                entry[0] += int(number)
                entry[1] += float(wait.rstrip("ms") or 0)
            elif value.isdigit():
                counts[key] = counts.get(key, 0) + int(value)
    return {"counts": counts, "overloads": {k: {"count": v[0], "wait_ms": round(v[1], 1)} for k, v in overloads.items()}}


def parse_fps_dips(lines):
    """[fps-dip] seconds: how many, and the game's mean speed (%) and game frames a second in them."""
    dips = [line for line in lines if "[fps-dip]" in line]
    speeds = [int(s) for line in dips for s in re.findall(r"\bspeed=(\d+)%", line)]
    games = [int(g) for line in dips for g in re.findall(r"\bgame=(\d+)\b", line)]
    return {"seconds": len(speeds), "speed": round(sum(speeds) / len(speeds), 1) if speeds else None,
            "game_fps": round(sum(games) / len(games), 1) if games else None}


def parse_gpu_prof(line):
    """A [gpu-prof] line: frame_ms, busy_ms and each zone's ms and count per frame."""
    if not line or "[gpu-prof]" not in line:
        return None
    head, _, rest = line.partition(" | ")
    report = {key: float(value) for key, value in re.findall(r"\b(frame_ms|busy_ms|max)=([\d.]+)", head)}
    zones = []
    for part in (" | " + rest).split(" | ")[1:]:
        match = re.match(r"(.+?) ([\d.]+) x([\d.]+)\s*$", part.strip())
        if match:
            zones.append({"name": match.group(1), "ms": float(match.group(2)), "count": float(match.group(3))})
    report["zones"] = zones
    return report


def read_settings(text, keys):
    """KEY=VALUE lines of settings.ini for `keys` (absent: not in the file)."""
    found = dict(re.findall(r"^([A-Z_][A-Z0-9_]*)=(.*)$", text, re.M))
    return {key: found[key] for key in keys if key in found}


# --- the sampler, on the Deck ------------------------------------------------------------------------------------


def amdgpu_hwmon():
    for folder in sorted(Path("/sys/class/hwmon").glob("hwmon*")):
        try:
            if (folder / "name").read_text().strip() == "amdgpu":
                return folder
        except OSError:
            pass
    return None


def remote(args):
    pids = subprocess.run(["pgrep", "-f", "/usr/bin/bluewake$"], capture_output=True, text=True).stdout.split()
    if not pids:
        print(json.dumps({"error": "BlueWake is not running"}), flush=True)
        return 1
    pid = pids[0]  # the game runs on the main thread
    logs = sorted(Path.home().glob(".local/share/BlueWake/logs/session-*.log"), key=lambda p: p.stat().st_mtime)
    log = logs[-1] if logs else None
    settings_file = Path.home() / ".config/BlueWake/settings.ini"
    try:
        environ = Path(f"/proc/{pid}/environ").read_bytes().decode(errors="replace").split("\0")
    except OSError:
        environ = []
    variables = dict(item.split("=", 1) for item in environ
                     if "=" in item and item.startswith(("DOL_", "BLUEWAKE_")) and not item.startswith("BLUEWAKE_CARD"))
    hwmon = amdgpu_hwmon()
    for window in range(1, args.windows + 1):
        start = log.stat().st_size if log else 0
        samples = []
        stop = threading.Event()

        def sample():
            while not stop.wait(1.0):
                entry = {}
                if hwmon is not None:
                    try:
                        entry["watts"] = int((hwmon / "power1_average").read_text()) / 1e6
                        entry["gpu_mhz"] = current_sclk((hwmon / "device/pp_dpm_sclk").read_text())
                    except (OSError, ValueError):
                        pass
                entry["cpu_mhz"] = mean_cpu_mhz(Path("/proc/cpuinfo").read_text())
                samples.append(entry)

        thread = threading.Thread(target=sample, daemon=True)
        thread.start()
        perf = subprocess.run(["perf", "stat", "-x;", "-e", "task-clock:u,cycles:u,instructions:u", "-t", pid,
                               "--", "sleep", str(args.seconds)], capture_output=True, text=True)
        stop.set()
        thread.join()
        lines = []
        if log:
            with log.open("rb") as handle:
                handle.seek(start)
                lines = handle.read().decode(errors="replace").splitlines()
        prof = [line for line in lines if "[gpu-prof]" in line]
        mean = lambda key: (round(statistics.mean(s[key] for s in samples if s.get(key) is not None), 1)
                            if any(s.get(key) is not None for s in samples) else None)
        record = {
            "window": window, "seconds": args.seconds, "time": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "pid": int(pid), "log": log.name if log else None,
            "game_thread": game_thread(parse_perf(perf.stderr + perf.stdout), args.seconds),
            "power_w": mean("watts"), "gpu_mhz": mean("gpu_mhz"), "cpu_mhz": mean("cpu_mhz"),
            "interp": parse_interp_stats(lines), "dips": parse_fps_dips(lines),
            "gpu_prof": parse_gpu_prof(prof[-1]) if prof else None,
            "settings": read_settings(settings_file.read_text() if settings_file.exists() else "", args.key),
            "variables": variables,
        }
        print(json.dumps(record), flush=True)
    return 0


# --- the driver, here ---------------------------------------------------------------------------------------------


def sample(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    remote_args = ["remote", "--windows", str(args.windows), "--seconds", str(args.seconds)]
    for key in args.key:
        remote_args += ["--key", key]
    command = ["ssh", *(["-i", args.ssh_key] if args.ssh_key else []), args.target,
               "python3 - " + " ".join(shlex.quote(a) for a in remote_args)]
    print(f"sampling {args.windows} x {args.seconds} s on {args.target} as '{args.label}'", flush=True)
    with open(__file__, "rb") as source, (out / "windows.jsonl").open("a") as sink:
        process = subprocess.Popen(command, stdin=source, stdout=subprocess.PIPE, text=True)
        for line in process.stdout:
            record = json.loads(line)
            if "error" in record:
                print(f"error: {record['error']}", file=sys.stderr)
                continue
            record["label"] = args.label
            sink.write(json.dumps(record) + "\n")
            sink.flush()
            print(one_line(record), flush=True)
    return process.wait()


def one_line(record):
    thread = record.get("game_thread") or {}
    interp = (record.get("interp") or {}).get("counts", {})
    dips = record.get("dips") or {}
    prof = record.get("gpu_prof") or {}
    return (f"{record['label']} w{record['window']}: game {thread.get('ghz', '?')} GHz ipc {thread.get('ipc', '?')} | "
            f"{record.get('power_w')} W gpu {record.get('gpu_mhz')} MHz | delivered {interp.get('delivered', '-')} "
            f"| dips {dips.get('seconds', 0)} game {dips.get('game_fps', '-')} FPS | gpu busy {prof.get('busy_ms', '-')} ms | {record.get('settings')}")


METRICS = (  # (name, value of a window, unit, decimals)
    ("Game-thread clock", lambda r: (r.get("game_thread") or {}).get("ghz"), "GHz", 2),
    ("Game-thread IPC", lambda r: (r.get("game_thread") or {}).get("ipc"), "", 3),
    ("APU power", lambda r: r.get("power_w"), "W", 1),
    ("GPU clock", lambda r: r.get("gpu_mhz"), "MHz", 0),
    ("CPU clock (cores' mean)", lambda r: r.get("cpu_mhz"), "MHz", 0),
    ("In-between frames per second",
     lambda r: (((r.get("interp") or {}).get("counts") or {}).get("delivered") or 0) / r["seconds"]
     if (r.get("interp") or {}).get("counts") else None, "", 1),
    ("Game FPS in dip seconds", lambda r: (r.get("dips") or {}).get("game_fps"), "", 1),
    ("Seconds with a dip, per window", lambda r: (r.get("dips") or {}).get("seconds"), "", 1),
    ("GPU busy per profiled frame", lambda r: (r.get("gpu_prof") or {}).get("busy_ms"), "ms", 1),
)


def compare(records, base, test):
    """For each metric: the base and test means, the change, and the base windows' spread (max - min)."""
    rows = []
    for name, value, unit, digits in METRICS:
        a = [v for v in (value(r) for r in records if r["label"] in base) if v is not None]
        b = [v for v in (value(r) for r in records if r["label"] in test) if v is not None]
        if not a or not b:
            continue
        mean_a, mean_b = statistics.mean(a), statistics.mean(b)
        spread = max(a) - min(a)
        change = (mean_b - mean_a) / mean_a * 100 if mean_a else None
        rows.append({"metric": name, "unit": unit, "digits": digits, "base": mean_a, "test": mean_b, "change": change,
                     "spread": spread, "beyond": abs(mean_b - mean_a) > spread, "n": (len(a), len(b))})
    return rows


def report(args):
    records = [json.loads(line) for line in (Path(args.dir) / "windows.jsonl").read_text().splitlines() if line]
    print("| Label | Window | Settings | Game-thread clock | IPC | APU power | GPU clock | In-between frames / s "
          "| Dip seconds | GPU busy (ms) |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    for r in records:
        thread = r.get("game_thread") or {}
        counts = (r.get("interp") or {}).get("counts") or {}
        settings = ", ".join(f"{k}={v}" for k, v in (r.get("settings") or {}).items())
        delivered = f"{counts['delivered'] / r['seconds']:.1f}" if "delivered" in counts else "—"
        print(f"| {r['label']} | {r['window']} | {settings} | {thread.get('ghz', '—')} GHz | {thread.get('ipc', '—')} "
              f"| {r.get('power_w')} W | {r.get('gpu_mhz')} MHz | {delivered} | {(r.get('dips') or {}).get('seconds', 0)} "
              f"| {(r.get('gpu_prof') or {}).get('busy_ms', '—')} |")
    if args.base and args.test:
        base, test = args.base.split(","), args.test.split(",")
        print(f"\nBase: {', '.join(base)}. Test: {', '.join(test)}. Spread: the base windows' range (an A/A).\n")
        print("| Metric | Base | Test | Change | Base spread | Beyond the spread |")
        print("|---|---|---|---|---|---|")
        for row in compare(records, base, test):
            fmt = lambda v: f"{v:,.{row['digits']}f} {row['unit']}".strip()
            change = f"{row['change']:+.1f}%" if row["change"] is not None else "—"
            print(f"| {row['metric']} | {fmt(row['base'])} | {fmt(row['test'])} | {change} | {fmt(row['spread'])} "
                  f"| {'yes' if row['beyond'] else 'no'} |")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("sample", help="measure the running BlueWake on a Deck")
    p.add_argument("target", help="user@host of the Deck")
    p.add_argument("label", help="what this setting is, e.g. off, on, off-again")
    p.add_argument("--windows", type=int, default=4)
    p.add_argument("--seconds", type=int, default=15)
    p.add_argument("--out", default="gamemode-measure", help="the folder here (default ./gamemode-measure)")
    p.add_argument("--key", action="append", default=list(DEFAULT_KEYS), help="a settings.ini key to record")
    p.add_argument("--ssh-key", help="SSH private key")
    p = sub.add_parser("report", help="tables of the windows, and an A/B with --base and --test")
    p.add_argument("dir")
    p.add_argument("--base", help="the labels of the reference, comma-separated (e.g. off,off-again)")
    p.add_argument("--test", help="the labels of the change, comma-separated (e.g. on)")
    p = sub.add_parser("remote", help=argparse.SUPPRESS)
    p.add_argument("--windows", type=int, default=4)
    p.add_argument("--seconds", type=int, default=15)
    p.add_argument("--key", action="append", default=[])
    args = parser.parse_args()
    return {"sample": sample, "report": report, "remote": remote}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
