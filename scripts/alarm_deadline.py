"""Small, public-safe model of the GameCube alarm deadline contract."""

from typing import List, Tuple

TIMEBASE_MASK = (1 << 64) - 1
TIMEBASE_HALF_RANGE = 1 << 63
GUEST_CYCLES_PER_TIMEBASE_TICK = 12


class GuestClock:
    """Public model of the promoted default-on runtime clock service."""

    def __init__(self, timebase: int = 0) -> None:
        self.timebase = timebase & TIMEBASE_MASK
        self.cycle_remainder = 0
        self.decrementer = 0
        self.programmed = False
        self.expired = False
        self.pending = False

    def write_decrementer(self, value: int) -> None:
        self.decrementer = value & 0xFFFFFFFF
        self.programmed = True
        self.expired = False
        self.pending = False

    def advance(self, elapsed_cycles: int) -> None:
        if elapsed_cycles < 0:
            raise ValueError("elapsed cycles must be non-negative")
        cycle_total = elapsed_cycles + self.cycle_remainder
        elapsed_timebase, self.cycle_remainder = divmod(
            cycle_total, GUEST_CYCLES_PER_TIMEBASE_TICK
        )
        self.timebase = (self.timebase + elapsed_timebase) & TIMEBASE_MASK
        if not self.programmed or self.expired:
            return
        before = self.decrementer
        self.decrementer = (before - min(elapsed_timebase, 0xFFFFFFFF)) & 0xFFFFFFFF
        if not (before & 0x80000000) and (self.decrementer & 0x80000000):
            self.expired = True
            self.pending = True


def alarm_due(timebase: int, fire_time: int) -> bool:
    """Return whether fire_time has been reached in wrapping u64 time."""
    elapsed = (timebase - fire_time) & TIMEBASE_MASK
    return elapsed < TIMEBASE_HALF_RANGE


def periodic_rearm_time(fire_time: int, period: int) -> int:
    """Return the next deadline using retail periodic-alarm semantics."""
    if period <= 0:
        raise ValueError("period must be positive")
    return (fire_time + period) & TIMEBASE_MASK


def head_alarm_due(timebase: int, queue: List[Tuple[int, int]]) -> bool:
    """Only the queue head can make a decrementer event eligible."""
    return bool(queue) and alarm_due(timebase, queue[0][1])
