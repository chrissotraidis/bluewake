import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))

from alarm_deadline import GuestClock


def test_timebase_and_decrementer_share_one_elapsed_source():
    clock = GuestClock(timebase=100)
    clock.write_decrementer(10)
    clock.advance(11)
    assert clock.timebase == 100
    assert clock.decrementer == 10
    clock.advance(1)
    assert clock.timebase == 101
    assert clock.decrementer == 9
    assert not clock.pending


def test_crossing_zero_delivers_one_pending_event():
    clock = GuestClock()
    clock.write_decrementer(2)
    clock.advance(36)
    assert clock.timebase == 3
    assert clock.decrementer == 0xFFFFFFFF
    assert clock.pending
    clock.advance(4)
    assert clock.pending


def test_reprogramming_clears_expiry_for_periodic_rearm():
    clock = GuestClock()
    clock.write_decrementer(1)
    clock.advance(24)
    assert clock.pending
    clock.write_decrementer(5)
    assert not clock.pending
    clock.advance(72)
    assert clock.pending


if __name__ == "__main__":
    for name, test in sorted(globals().items()):
        if name.startswith("test_"):
            test()
    print("Guest clock tests passed.")
