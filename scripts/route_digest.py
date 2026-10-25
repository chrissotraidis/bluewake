#!/usr/bin/env python3
"""Build and compare deterministic BlueWake gameplay-route digests."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re


RECORD = re.compile(
    r"^\[(?:clock|cycle-delivery|aram-dma|dsp-lle|boot-milestone|"
    r"scene-draw|dvd-lifecycle|collision-provenance|fp|cpu-abi)\] "
    r"(?:summary|external\[|dsp\[)"
)
NORMAL_STOP = re.compile(r"^\[run\] stopped: normal after \d+ blocks at (pc=\S+)$")

# D1, docs/archive/GOAL_PROMPT_V16_2026-09-14.md. The delivery-timing aggregate is not
# a guest-state record. `[cycle-delivery] summary` shares the tag this selector
# matches on, so its `hash=` field -- an aggregate over (cycle, cause, pc,
# context) -- used to gate the route digest, which made any change to
# interrupt-delivery timing fail the gate by construction. V14 already decided
# that schedule must not gate the digest when it moved `[delivery-hash]`
# outside the record set; this finishes that decision. The counts and the DSP
# cadence stay in the record; only the timing hash is canonicalised away, and
# delivery timing is gated as a bounded quantity instead of an equality.
DELIVERY_SUMMARY_PREFIX = "[cycle-delivery] summary"
DELIVERY_TIMING_HASH = re.compile(r"\s+hash=[0-9A-F]+(?=\s|$)")


# D2, docs/archive/status/CURRENT.md "2026-09-17 the DSP slice has Dolphin's idle skip
# switched off, and it is worth 1.7x on the route". This finishes D1. D1
# canonicalised the delivery-timing *aggregate* out of the digest and said in as
# many words that "delivery timing is gated as a bounded quantity instead of an
# equality" - but the bound was never implemented, so the per-delivery records
# still gated the guest's exact pc at delivery, which is a host turn-boundary
# quantity. Any change to how long a host turn is moved that field, so the gate
# rejected performance work by construction while every guest-state record in
# the route stayed identical.
#
# D2 gates the schedule as a bound instead of an equality. Removed from the
# digest: the per-delivery `cycle`, `prefix` and `pc`, and the route clock's
# `cycles` and `ai_remainder`. Still gated, exactly: the record and delivery
# counts, the delivery ordinals, causes and contexts, every other record in the
# set, the normal-stop pc, and the card. The removed quantities are compared as
# bounds on every multi-run comparison below, so a change that actually moves
# the schedule past the bound still fails.
#
# The bounds are the measured spread, not a guess. Across the four idle-skip
# settings that engage the donor's skip (batch 16, 32, 128, 256) against the
# shipping batch of 8, on the full 14,100-retrace route: same 1,050 records,
# same 1,024 deliveries, 899 of 1,024 delivery cycles bit-identical, the largest
# delivery move exactly one cycle, and the route clock -2 cycles out of
# 114,210,000,002. All four settings produced the same values as each other.
DELIVERY_EXTERNAL_PREFIX = "[cycle-delivery] external["
DELIVERY_CYCLE = re.compile(r"\s+cycle=(\d+)")
DELIVERY_SCHEDULE = re.compile(r"\s+(?:cycle=\d+|prefix=\S+|pc=\S+)")
CLOCK_SUMMARY_PREFIX = "[clock] summary"
CLOCK_CYCLES = re.compile(r"\s+cycles=(\d+)")
CLOCK_SCHEDULE = re.compile(r"\s+(?:cycles=\d+|ai_remainder=\d+)")

# Largest permitted schedule drift between two runs of the same route.
DELIVERY_CYCLE_BOUND = 1
CLOCK_CYCLE_BOUND = 8


def canonical_record(record: str) -> str:
    """Reduce a raw record to the guest-state part the route digest gates."""
    if record.startswith(DELIVERY_SUMMARY_PREFIX):
        return DELIVERY_TIMING_HASH.sub("", record, count=1)
    if record.startswith(DELIVERY_EXTERNAL_PREFIX):
        return DELIVERY_SCHEDULE.sub("", record)
    if record.startswith(CLOCK_SUMMARY_PREFIX):
        return CLOCK_SCHEDULE.sub("", record)
    return record


def schedule(path: Path) -> tuple[list[int], int | None]:
    """Per-delivery cycles and route clock cycles, the D2-bounded quantities."""
    deliveries: list[int] = []
    clock: int | None = None
    for line in path.read_text().splitlines():
        if line.startswith(DELIVERY_EXTERNAL_PREFIX):
            match = DELIVERY_CYCLE.search(line)
            if match:
                deliveries.append(int(match.group(1)))
        elif line.startswith(CLOCK_SUMMARY_PREFIX):
            match = CLOCK_CYCLES.search(line)
            if match:
                clock = int(match.group(1))
    return deliveries, clock


def route_records(path: Path) -> list[str]:
    records: list[str] = []
    for line in path.read_text().splitlines():
        if RECORD.match(line):
            records.append(canonical_record(line))
            continue
        stop = NORMAL_STOP.match(line)
        if stop:
            records.append(f"[run] stopped: normal {stop.group(1)}")
    return records


def route_digest(log: Path, card: Path) -> tuple[str, int]:
    records = route_records(log)
    if not records:
        raise ValueError(f"no route records found in {log}")
    if not any(line.startswith("[run] stopped: normal") for line in records):
        raise ValueError(f"normal stop record missing from {log}")

    digest = hashlib.sha256()
    for record in records:
        digest.update(record.encode("utf-8"))
        digest.update(b"\n")
    digest.update(b"[card] sha256=")
    digest.update(hashlib.sha256(card.read_bytes()).hexdigest().encode("ascii"))
    digest.update(b"\n")
    return digest.hexdigest(), len(records)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pairs", nargs="+", metavar="LOG:CARD")
    args = parser.parse_args()

    results: list[tuple[Path, str, int]] = []
    try:
        for pair in args.pairs:
            log_name, separator, card_name = pair.partition(":")
            if not separator:
                raise ValueError(f"expected LOG:CARD, got {pair!r}")
            log = Path(log_name)
            digest, count = route_digest(log, Path(card_name))
            results.append((log, digest, count))
    except (OSError, ValueError) as error:
        parser.error(str(error))

    for log, digest, count in results:
        print(f"{digest}  {log} ({count} records)")

    baseline = results[0][1]
    if any(digest != baseline for _, digest, _ in results[1:]):
        print("route digests differ")
        return 1
    if len(results) == 1:
        return 0
    print(f"identical route digest across {len(results)} runs")

    # D2: the equality above excludes the schedule, so assert the schedule the
    # way D1 said it would be asserted - as a bounded quantity. A run that
    # matches on guest state and stays inside the bound on timing is the same
    # route; a run that leaves the bound is not.
    try:
        schedules = [schedule(log) for log, _, _ in results]
    except OSError as error:
        parser.error(str(error))
    base_deliveries, base_clock = schedules[0]
    if not base_deliveries:
        print("schedule bound: not applied (no per-delivery records in baseline)")
        return 0
    for (log, _, _), (deliveries, clock) in zip(results[1:], schedules[1:]):
        if len(deliveries) != len(base_deliveries):
            print(f"delivery count differs: {len(deliveries)} against "
                  f"{len(base_deliveries)} in {log}")
            return 1
        drift = max(abs(a - b) for a, b in zip(base_deliveries, deliveries))
        moved = sum(1 for a, b in zip(base_deliveries, deliveries) if a != b)
        if drift > DELIVERY_CYCLE_BOUND:
            print(f"delivery cycle drift {drift} exceeds "
                  f"{DELIVERY_CYCLE_BOUND} in {log}")
            return 1
        if base_clock is not None and clock is not None:
            clock_drift = abs(base_clock - clock)
            if clock_drift > CLOCK_CYCLE_BOUND:
                print(f"route clock drift {clock_drift} exceeds "
                      f"{CLOCK_CYCLE_BOUND} cycles in {log}")
                return 1
            print(f"schedule bound: max delivery drift {drift} cycle "
                  f"({moved}/{len(deliveries)} deliveries), route clock "
                  f"drift {clock_drift} cycles in {log}")
        else:
            print(f"schedule bound: max delivery drift {drift} cycle "
                  f"({moved}/{len(deliveries)} deliveries); route clock "
                  f"absent from one run, not compared")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
