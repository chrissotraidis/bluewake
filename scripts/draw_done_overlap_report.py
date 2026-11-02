#!/usr/bin/env python3
"""Compare session logs with the draw-done overlap off and on.

BLUEWAKE_DRAW_DONE_OVERLAP moves the guest thread's wait for the frame's
translation out of the draw-done: the switch should take that wait out of
`waits: gx=` and give it back to the game. This reads two sets of session logs
-- the same route, alternating, per docs/PERFORMANCE.md -- and prints what the
logs say, with the checks that decide whether the mode is safe, not just fast.

    python3 scripts/draw_done_overlap_report.py --off off-1.log off-2.log \\
                                                 --on  on-1.log  on-2.log

A file may be a session log or a directory of them (each *.log is read).
Nothing is averaged across pairs: each pair is printed on its own line, because
two runs of one build on this engine can differ by more than the effect.
"""

from __future__ import annotations

import argparse
import gzip
import re
import statistics
import sys
from pathlib import Path

DIP = re.compile(
    r"\[fps-dip\].*?game=(?P<game>\d+).*?speed=(?P<speed>\d+)%.*?busy=(?P<busy>\d+)%.*?"
    r"waits: gx=(?P<gx>\d+)ms present=(?P<present>\d+)ms gpu=(?P<gpu>\d+)ms"
)
OVERLAP = re.compile(
    r"\[gx\] draw-done overlap: deferred=(?P<deferred>\d+) early=(?P<early>\d+) "
    r"bound=(?P<bound>\d+) late=(?P<late>\d+) max_lead_us=(?P<lead>\d+) "
    r"presents=(?P<presents>\d+)"
)
ENABLED = "[gx] draw-done overlap on:"


class Run:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.enabled = False
        self.counters: dict[str, int] = {}
        self.game: list[int] = []
        self.busy: list[int] = []
        self.gx: list[int] = []
        self.present: list[int] = []
        self.gpu: list[int] = []

    @property
    def ok(self) -> bool:
        return bool(self.game)

    def pct(self, values: list[int], q: float) -> float:
        if not values:
            return float("nan")
        ordered = sorted(values)
        index = min(len(ordered) - 1, int(round(q * (len(ordered) - 1))))
        return float(ordered[index])


def read(path: Path) -> str:
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", errors="replace") as handle:
        return handle.read()


def collect(path: Path) -> Run:
    run = Run(path)
    text = read(path)
    run.enabled = ENABLED in text
    for match in DIP.finditer(text):
        run.game.append(int(match["game"]))
        run.busy.append(int(match["busy"]))
        run.gx.append(int(match["gx"]))
        run.present.append(int(match["present"]))
        run.gpu.append(int(match["gpu"]))
    matches = list(OVERLAP.finditer(text))
    if matches:
        last = matches[-1]
        run.counters = {key: int(last[key]) for key in
                        ("deferred", "early", "bound", "late", "lead", "presents")}
    return run


def expand(paths: list[str]) -> list[Path]:
    found: list[Path] = []
    for raw in paths:
        path = Path(raw)
        if path.is_dir():
            found.extend(sorted(path.glob("*.log")))
        else:
            found.append(path)
    missing = [str(p) for p in found if not p.is_file()]
    if missing:
        sys.exit("no such log: " + ", ".join(missing))
    return found


def describe_label(run: Run, expected: bool) -> str:
    seen = "on" if run.enabled else "off"
    want = "on" if expected else "off"
    flag = "" if seen == want else f"  [!! log says {seen}]"
    return f"{run.path.name} ({seen}){flag}"


def row(run: Run) -> str:
    if not run.ok:
        return f"  {run.path.name}: no [fps-dip] lines -- no measurement"
    return (
        f"  game fps      mean {statistics.mean(run.game):5.1f}   "
        f"p10 {run.pct(run.game, 0.10):5.1f}   min {min(run.game)}\n"
        f"  busy          mean {statistics.mean(run.busy):5.0f}%  "
        f"max {max(run.busy)}%\n"
        f"  waits gx      mean {statistics.mean(run.gx):5.1f}ms  "
        f"p90 {run.pct(run.gx, 0.90):5.1f}ms  max {max(run.gx)}ms\n"
        f"  waits present mean {statistics.mean(run.present):5.1f}ms  "
        f"waits gpu mean {statistics.mean(run.gpu):5.1f}ms\n"
        f"  dips {len(run.game)}"
    )


def counters_line(run: Run) -> str:
    if not run.counters:
        return "  overlap counters: none (switch off, or no summary printed)"
    c = run.counters
    return (
        f"  overlap       deferred {c['deferred']}  early {c['early']}  "
        f"bound {c['bound']}  late {c['late']}  "
        f"max_lead {c['lead'] / 1000.0:.2f}ms  presents {c['presents']}"
    )


def verdict(off: Run, on: Run) -> list[str]:
    notes: list[str] = []
    if not off.ok or not on.ok:
        return ["  no verdict: a run has no [fps-dip] lines"]
    if not on.counters:
        notes.append("  no verdict: the 'on' run printed no overlap counters, so it "
                     "was not running the switch")
        return notes
    if on.counters["late"]:
        notes.append(f"  FAIL late={on.counters['late']}: a finish crossed a retrace, "
                     "which is the 15 FPS pathology -- do not go further")
    if on.counters["deferred"] == 0:
        notes.append("  FAIL deferred=0: the switch never deferred, so nothing was measured")
    if on.counters["bound"] and on.counters["bound"] > on.counters["deferred"] // 10:
        notes.append(f"  WARN bound={on.counters['bound']} is over a tenth of the "
                     "deferred commits: the one-draw-done lead is being hit often")
    delta_fps = statistics.mean(on.game) - statistics.mean(off.game)
    delta_gx = statistics.mean(on.gx) - statistics.mean(off.gx)
    notes.append(f"  game fps {delta_fps:+.1f}, waits gx {delta_gx:+.1f}ms")
    if delta_fps <= 0 and delta_gx >= 0:
        notes.append("  the overlap did not pay here: no game frames gained and the "
                     "wait did not fall")
    return notes


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--off", nargs="+", required=True,
                        help="session logs (or directories) with the switch off")
    parser.add_argument("--on", nargs="+", required=True,
                        help="session logs (or directories) with the switch on")
    args = parser.parse_args()

    off = [collect(p) for p in expand(args.off)]
    on = [collect(p) for p in expand(args.on)]

    for group, expected in ((off, False), (on, True)):
        for run in group:
            print(describe_label(run, expected))
            print(row(run))
            print(counters_line(run))
            print()

    pairs = min(len(off), len(on))
    if pairs:
        print(f"{pairs} pair(s), off -> on")
        for index in range(pairs):
            print(f"  pair {index + 1}: {off[index].path.name} -> {on[index].path.name}")
            for note in verdict(off[index], on[index]):
                print(note)
        if len(off) != len(on):
            print(f"  note: {len(off)} off run(s) and {len(on)} on run(s); "
                  f"the extra ones are reported above, not paired")


if __name__ == "__main__":
    main()
