#!/usr/bin/env python3
"""Compare correctness invariants across BlueWake cycle-cap route logs."""

from __future__ import annotations

import argparse
from pathlib import Path
import re


INVARIANT = re.compile(
    r"^\[(?:clock|cycle-delivery|aram-dma|dsp-lle|boot-milestone|"
    r"scene-draw|dvd-lifecycle)\] (?:summary|external\[|dsp\[)"
    r"|^\[delivery-hash\] "
    r"|^\[delivery-play\] |^\[delivery-play-trace\] "
)


def invariant_lines(path: Path) -> list[str]:
    return [line for line in path.read_text().splitlines() if INVARIANT.match(line)]


def compare_logs(paths: list[Path]) -> tuple[bool, str]:
    if len(paths) < 2:
        return False, "at least two route logs are required"

    baseline = invariant_lines(paths[0])
    if not baseline:
        return False, f"no invariant records found in {paths[0]}"

    for path in paths[1:]:
        candidate = invariant_lines(path)
        if len(candidate) != len(baseline):
            return False, (
                f"{path}: invariant record count {len(candidate)} != "
                f"{len(baseline)}"
            )
        for index, (expected, actual) in enumerate(zip(baseline, candidate), 1):
            if actual != expected:
                return False, (
                    f"{path}: invariant record {index} differs\n"
                    f"expected: {expected}\nactual:   {actual}"
                )
    return True, f"identical invariants across {len(paths)} logs ({len(baseline)} records)"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", type=Path, nargs="+")
    args = parser.parse_args()
    passed, message = compare_logs(args.logs)
    print(message)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
