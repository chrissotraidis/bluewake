#!/usr/bin/env python3
"""Timing distributions from BlueWake logs.

Usage: scripts/frame_tail.py LOG [--kind retrace|display] [--window LOW:HIGH] [--worst N]
Retrace stamps measure VI delivery intervals (nominally 60/s), not the 30 Hz game
update rate. Display stamps measure actual surface-present completion intervals
with DOL_AURORA_PRESENT_LOG=1; they do not measure photons or GPU execution time.
No acceptance gate is inferred from either stream alone.
"""

import re
import sys

STAMPS = {
    "retrace": re.compile(r"\[frame-timing\] retrace=(\d+) us=(\d+)$"),
    "display": re.compile(r"\[display-timing\] present=(\d+) us=(\d+)$"),
}


def main() -> int:
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    path = args[0]
    window = (0, 1 << 40)
    worst_n = 10
    kind = "retrace"
    i = 1
    while i < len(args):
        if args[i] == "--kind" and i + 1 < len(args):
            kind = args[i + 1]
            if kind not in STAMPS: return 2
            i += 2
        elif args[i] == "--window" and i + 1 < len(args):
            low, high = args[i + 1].split(":")
            window = (int(low), int(high))
            i += 2
        elif args[i] == "--worst" and i + 1 < len(args):
            worst_n = int(args[i + 1])
            i += 2
        else:
            print(__doc__)
            return 2

    stamps = []
    try:
        handle = open(path, errors="replace")
    except OSError as exc:
        sys.exit("frame_tail: %s" % exc)
    for line in handle:
        match = STAMPS[kind].search(line.rstrip("\n"))
        if match:
            stamps.append((int(match.group(1)), int(match.group(2))))
    handle.close()

    deltas = []
    for (first_retrace, first_us), (second_retrace, second_us) in zip(stamps, stamps[1:]):
        if second_retrace - first_retrace != 1 or second_us < first_us:
            continue
        if not window[0] <= second_retrace <= window[1]:
            continue
        deltas.append((second_us - first_us, second_retrace))
    if not deltas:
        sys.exit("frame_tail: no consecutive frame stamps in window %d:%d" % window)

    times = sorted(delta for delta, _ in deltas)
    count = len(times)

    def percentile(percent):
        return times[min(count - 1, int(round(percent / 100.0 * (count - 1))))]

    mean = sum(times) / count
    print("%s: %d %s intervals in %d:%d" % (path, count, kind, window[0], window[1]))
    print("  mean %.2f ms   p50 %.2f   p90 %.2f   p95 %.2f   p99 %.2f   max %.2f"
          % (mean / 1000.0, percentile(50) / 1000.0, percentile(90) / 1000.0,
             percentile(95) / 1000.0, percentile(99) / 1000.0, times[-1] / 1000.0))
    print("  interval rate %.2f/s; stalls >50ms=%d >100ms=%d" %
          (1e6 / mean if mean > 0 else 0, sum(t > 50000 for t in times), sum(t > 100000 for t in times)))
    print("  worst %d:" % worst_n)
    for delta, retrace in sorted(deltas, reverse=True)[:worst_n]:
        print("    %s %d  %.2f ms" % (kind, retrace, delta / 1000.0))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

