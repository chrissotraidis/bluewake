#!/usr/bin/env python3
"""Owner shares from a `sample` capture of the play window.

`scripts/bench.sh` runs headless and never executes the Aurora configuration
path, so renderer-side changes are invisible to it. Sampled shares help locate
work, but scheduling, pacing and workload changes can bias them. They are not
retired-instruction counts or proof of an FPS improvement. Compare matched
workloads and retain normal-stop and work-count checks.

Usage: scripts/sample_owners.py SAMPLE.txt [--depth N] [--top N]

Read the numbers carefully: **a share includes everything it calls.** The share
printed for a parent and for its child are not two separate costs, and
subtracting them is *not* a self-time measurement. It was done once - the GX
assembly walk was reported at "4.1% of the frame" from `accumulate_assembly`
minus `on_consumed_draw` - and the instrumented check that removed the walk
entirely left the shares unchanged, so the 4.1% was an artifact of that
subtraction. Use this tool to see where the frame goes and what a change moves;
do not derive a self time from it.
"""

import argparse
import re
import sys

LINE = re.compile(
    r"^\s*((?:[+!: |]|\s)*?)\s*(\d+)\s+(\S.*?)\s{2,}\(in ([^)]+)\)")
# A new thread's block begins with its own summary line, e.g.
#    20305 Thread_12847184: com.apple.NSEventThread
THREAD = re.compile(r"^\s*\d+\s+Thread_")


def parse(path):
    with open(path, errors="replace") as capture:
        text = capture.read().splitlines()
    start = next((i for i, l in enumerate(text) if "Call graph:" in l), None)
    if start is None:
        sys.exit("no call graph in %s" % path)
    end = next((i for i in range(start, len(text))
                if text[i].startswith("Total number in stack")), len(text))
    body = text[start + 1:end]
    mt = next((i for i, l in enumerate(body)
               if THREAD.match(l) and
               ("com.apple.main-thread" in l or re.search(r": Main Thread\b", l))), None)
    if mt is None:
        sys.exit("no main thread in %s" % path)
    # The main thread's tree ends where the next thread's summary begins; every
    # other thread is sampled over the same interval and would otherwise be
    # counted into the totals.
    nxt = next((i for i in range(mt + 1, len(body)) if THREAD.match(body[i])),
               len(body))
    rows = []
    for line in body[mt:nxt]:
        m = LINE.match(line)
        if not m:
            continue
        marks = m.group(1)
        rows.append((sum(1 for c in marks if c in "+!:"), int(m.group(2)),
                     m.group(3), m.group(4)))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sample")
    ap.add_argument("--depth", type=int, default=3)
    ap.add_argument("--top", type=int, default=14)
    ap.add_argument("--min-share", type=float, default=1.0)
    args = ap.parse_args()

    rows = parse(args.sample)
    if not rows:
        sys.exit("no main-thread frames in %s" % args.sample)
    total = rows[0][1]
    print("main-thread samples: %d" % total)
    print()
    print("top owners at depth <= %d (share of main thread):" % args.depth)
    seen = {}
    for depth, count, name, binary in rows:
        if depth == 0 or depth > args.depth:
            continue
        key = (name, binary)
        if key not in seen:
            seen[key] = (depth, count)
    for (name, binary), (depth, count) in sorted(
            seen.items(), key=lambda kv: -kv[1][1])[:args.top]:
        share = 100.0 * count / total
        if share < args.min_share:
            continue
        print("  %6.2f%%  d%d  %-62s [%s]" %
              (share, depth, name[:62], binary))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
