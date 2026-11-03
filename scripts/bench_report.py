#!/usr/bin/env python3
"""Report the five governing BlueWake benchmark numbers from one route log.

Consumes the artifact produced by scripts/bench.sh:

  * median fps and p99 frame time over the live-play retrace window, derived
    from the host frame-timing stamps;
  * wall seconds versus guest seconds (real-speed ratio);
  * peak RSS;
  * the deterministic route digest from scripts/route_digest.py.

A run whose digest diverges from the accepted baseline is reported as a
FAILED measurement, not a speedup, and exits non-zero.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from route_digest import route_digest  # noqa: E402

# Unanchored on purpose. The host writes one line per retrace, but the Aurora
# backend logs from its own thread and does not always finish a line before the
# next stamp is written, so a stamped line can arrive as
#
#     [aurora:info:aurora[frame-timing] retrace=14053 us=499523306767
#
# An anchored pattern drops exactly those retraces, and the two that collide in
# a typical play window (14053 and 14097) are what made `scripts/play.sh
# --timing` fail with a KeyError on the first rendered run it was ever asked to
# report. Searching anywhere in the line recovers the stamp; the numbers are
# host wall microseconds written by a single fprintf, so the match is still
# unambiguous.
FRAME_TIMING = re.compile(r"\[frame-timing\] retrace=(\d+) us=(\d+)")
TIME_REAL = re.compile(r"^\s*([0-9.]+) real")
TIME_USER = re.compile(r"^\s*([0-9.]+) user")
TIME_SYS = re.compile(r"^\s*([0-9.]+) sys")
TIME_RSS = re.compile(r"^\s*(\d+)\s+maximum resident set size")
STOP = re.compile(r"^\[run\] stopped: (\w+) after (\d+) blocks at (pc=\S+)$")

# The GL video interface runs at 59.94 Hz; the accepted route records whole
# retraces, so guest seconds are retraces / 60 to within the recorded rounding.
GUEST_RETRACE_HZ = 60.0


def parse_log(log: Path) -> dict:
    stamps: dict[int, int] = {}
    data = {"real": None, "user": None, "sys": None, "rss": None,
            "stop": None, "blocks": None, "pc": None}
    for line in log.read_text(errors="replace").splitlines():
        timing = FRAME_TIMING.search(line)
        if timing:
            stamps[int(timing.group(1))] = int(timing.group(2))
            continue
        stop = STOP.match(line)
        if stop:
            data["stop"] = stop.group(1)
            data["blocks"] = int(stop.group(2))
            data["pc"] = stop.group(3)
            continue
        match = TIME_REAL.match(line)
        if match:
            data["real"] = float(match.group(1))
            continue
        match = TIME_USER.match(line)
        if match:
            data["user"] = float(match.group(1))
            continue
        match = TIME_SYS.match(line)
        if match:
            data["sys"] = float(match.group(1))
            continue
        match = TIME_RSS.match(line)
        if match:
            data["rss"] = int(match.group(1))
    return {"stamps": stamps, **data}


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        raise ValueError("no samples")
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    index = fraction * (len(ordered) - 1)
    lower = math.floor(index)
    upper = math.ceil(index)
    if lower == upper:
        return ordered[int(index)]
    return ordered[lower] + (ordered[upper] - ordered[lower]) * (index - lower)


def window_metrics(stamps: dict[int, int], start: int, end: int) -> dict:
    missing = [n for n in (start, end) if n not in stamps]
    if missing:
        raise ValueError(
            f"frame-timing stamps missing for retrace(s) {missing}; "
            "the live-play window was not reached")
    # A stamp that collided with the Aurora logger is dropped rather than
    # failing the run, but only the two frames that straddle it are lost, and a
    # window that lost most of its stamps is still refused below.
    frame_us = [stamps[n] - stamps[n - 1]
                for n in range(start + 1, end + 1)
                if n in stamps and (n - 1) in stamps]
    if len(frame_us) * 2 < (end - start):
        raise ValueError(
            f"only {len(frame_us)} of {end - start} frame-timing stamps "
            "survived in the live-play window")
    if any(value <= 0 for value in frame_us):
        raise ValueError("non-monotonic frame-timing stamps")
    window_wall = (stamps[end] - stamps[start]) / 1e6
    frame_ms = [value / 1000.0 for value in frame_us]
    return {
        "frames": len(frame_us),
        "window_wall_seconds": window_wall,
        "window_mean_fps": (len(frame_us) / window_wall) if window_wall else 0.0,
        "median_frame_ms": percentile(frame_ms, 0.50),
        "median_fps": 1000.0 / percentile(frame_ms, 0.50),
        "p99_frame_ms": percentile(frame_ms, 0.99),
        "p99_fps": 1000.0 / percentile(frame_ms, 0.99),
        "min_fps": 1000.0 / max(frame_ms),
        "max_fps": 1000.0 / min(frame_ms),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", required=True)
    parser.add_argument("--card", required=True)
    parser.add_argument("--retraces", type=int, required=True)
    parser.add_argument("--window", default="13910:14100")
    parser.add_argument("--baseline-digest", default="")
    parser.add_argument("--baseline-retraces", type=int, default=14100)
    parser.add_argument("--json-out", default="")
    args = parser.parse_args()

    log = Path(args.log)
    start_text, _, end_text = args.window.partition(":")
    start, end = int(start_text), int(end_text)

    parsed = parse_log(log)
    stamps = parsed.pop("stamps")

    digest, records = route_digest(log, Path(args.card))

    report = {
        "retraces": args.retraces,
        "window": {"start": start, "end": end},
        "route_digest": digest,
        "digest_records": records,
        "stop": parsed["stop"],
        "blocks": parsed["blocks"],
        "final_pc": parsed["pc"],
        "wall_seconds": parsed["real"],
        "user_seconds": parsed["user"],
        "sys_seconds": parsed["sys"],
        "peak_rss_bytes": parsed["rss"],
        "guest_seconds": args.retraces / GUEST_RETRACE_HZ,
    }
    if parsed["real"]:
        guest = args.retraces / GUEST_RETRACE_HZ
        report["real_speed_ratio"] = guest / parsed["real"]

    try:
        report.update(window_metrics(stamps, start, end))
    except ValueError as error:
        print(f"bench_report: {error}", file=sys.stderr)
        if args.json_out:
            Path(args.json_out).write_text(json.dumps(report, indent=2) + "\n")
        return 1

    baseline = args.baseline_digest.strip()
    compare = bool(baseline) and args.retraces == args.baseline_retraces
    digest_ok = (digest == baseline) if compare else None
    report["baseline_digest"] = baseline or None
    report["digest_compared"] = compare
    report["digest_ok"] = digest_ok

    print(f"retraces          {args.retraces}")
    print(f"window            {start}..{end} ({report['frames']} frames)")
    print(f"median fps        {report['median_fps']:.2f}")
    print(f"p99 frame time    {report['p99_frame_ms']:.2f} ms "
          f"({report['p99_fps']:.2f} fps)")
    print(f"window mean fps   {report['window_mean_fps']:.2f}")
    print(f"fps range         {report['min_fps']:.2f} .. {report['max_fps']:.2f}")
    if report.get("real_speed_ratio") is not None:
        print(f"wall / guest      {report['wall_seconds']:.2f} s / "
              f"{report['guest_seconds']:.2f} s = "
              f"{report['real_speed_ratio'] * 100.0:.1f}% real speed")
    else:
        print("wall / guest      (wall time not reported by the run)")
    print(f"peak RSS          {report['peak_rss_bytes']} bytes")
    print(f"host turns        {report['blocks']}")
    print(f"final pc          {report['final_pc']}  stop={report['stop']}")
    print(f"route digest      {digest} ({records} records)")
    if compare:
        print(f"baseline digest   {baseline}")
        verdict = "UNCHANGED" if digest_ok else "DIVERGED"
        print(f"digest verdict    {verdict}")
    elif baseline:
        print("digest verdict    not compared (bounded tier; baseline is the "
              f"{args.baseline_retraces}-retrace route)")

    if args.json_out:
        Path(args.json_out).write_text(json.dumps(report, indent=2) + "\n")

    if report["stop"] != "normal":
        print("bench_report: route did not stop normally", file=sys.stderr)
        return 1
    if digest_ok is False:
        print("bench_report: route digest diverged from the accepted baseline",
              file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

