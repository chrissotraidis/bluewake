#!/usr/bin/env python3
"""Compare same-architecture guest checkpoints with explicit route coverage.

Use alongside route_digest.py and image/gameplay checks. These hashes cover the
normalized CPUState, MEM1/MEM2 and registered REL storage, not peripheral/GPU
state or execution between checkpoints. Hash agreement is not exact-state proof.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re


CHECKPOINT = re.compile(
    r"\[guest-checkpoint\] version=1 retrace=(?P<retrace>\d+) cycle=(?P<cycle>\d+) "
    r"cpu=(?P<cpu>[0-9A-F]{32}) mem1=(?P<mem1>[0-9A-F]{32}) "
    r"mem2=(?P<mem2>[0-9A-F]{32}) aliases=(?P<aliases>[0-9A-F]{32}) "
    r"alias_spans=(?P<alias_spans>\d+) alias_bytes=(?P<alias_bytes>\d+)"
)
NORMAL_STOP = re.compile(r"\[run\] stopped: normal after \d+ blocks at pc=\S+")
RETRACES = re.compile(r"(?:^|\s)retraces=(\d+)(?:\s|$)")


def read_checkpoints(path: Path, interval: int, through: int) -> list[dict[str, str]]:
    if interval <= 0 or through < interval:
        raise ValueError("interval must be positive and through must cover at least one checkpoint")
    records = []
    normal_stops = 0
    clock_retraces = []
    previous_cycle = -1
    for line in path.read_text().splitlines():
        if line.startswith("[guest-checkpoint]"):
            match = CHECKPOINT.fullmatch(line)
            if match is None:
                raise ValueError(f"{path}: malformed, failed or unsupported checkpoint: {line}")
            record = match.groupdict()
            expected = (len(records) + 1) * interval
            if int(record["retrace"]) != expected or expected > through:
                raise ValueError(f"{path}: expected retrace {expected}, got {record['retrace']}")
            cycle = int(record["cycle"])
            if cycle <= previous_cycle:
                raise ValueError(f"{path}: checkpoint cycles are not increasing")
            previous_cycle = cycle
            records.append(record)
        elif line.startswith("[run] stopped:"):
            if NORMAL_STOP.fullmatch(line) is None:
                raise ValueError(f"{path}: run did not stop normally")
            normal_stops += 1
        elif line.startswith("[clock] summary"):
            match = RETRACES.search(line)
            if match is None:
                raise ValueError(f"{path}: clock summary lacks retrace count")
            clock_retraces.append(int(match.group(1)))
    if len(records) != through // interval:
        raise ValueError(f"{path}: incomplete coverage: {len(records)} of {through // interval} checkpoints")
    if normal_stops != 1 or clock_retraces != [through]:
        raise ValueError(f"{path}: expected one normal stop and clock summary at retrace {through}")
    return records


def compare(paths: list[Path], interval: int, through: int) -> int:
    if len(paths) < 2:
        raise ValueError("at least two logs are required")
    baseline = read_checkpoints(paths[0], interval, through)
    for path in paths[1:]:
        candidate = read_checkpoints(path, interval, through)
        for before, after in zip(baseline, candidate):
            changed = [key for key in before if before[key] != after[key]]
            if changed:
                raise ValueError(f"{path}: retrace {before['retrace']} differs in {', '.join(changed)}")
    return len(baseline)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interval", type=int, required=True)
    parser.add_argument("--through", type=int, required=True)
    parser.add_argument("logs", type=Path, nargs="+")
    args = parser.parse_args()
    try:
        count = compare(args.logs, args.interval, args.through)
    except (OSError, ValueError) as error:
        parser.exit(1, f"checkpoint comparison failed: {error}\n")
    print(f"{count} matching checkpoint hashes per run, every {args.interval} retraces "
          f"through {args.through}, across {len(args.logs)} runs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
