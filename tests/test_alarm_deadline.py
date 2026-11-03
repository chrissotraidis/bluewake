import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))

from alarm_deadline import alarm_due, head_alarm_due, periodic_rearm_time


def test_deadline_ordering():
    assert not alarm_due(99, 100)
    assert alarm_due(100, 100)
    assert alarm_due(101, 100)


def test_deadline_ordering_wraps_u64():
    fire = (1 << 64) - 2
    assert not alarm_due((1 << 64) - 3, fire)
    assert alarm_due(0, fire)
    assert alarm_due(1, fire)


def test_head_controls_delivery_eligibility():
    queue = [(0xA, 200), (0xB, 100)]
    assert not head_alarm_due(150, queue)
    assert head_alarm_due(250, queue)
    assert not head_alarm_due(150, [])


def test_periodic_rearm_uses_previous_deadline():
    assert periodic_rearm_time(100, 25) == 125
    assert periodic_rearm_time((1 << 64) - 10, 25) == 15


def test_periodic_period_must_be_positive():
    try:
        periodic_rearm_time(100, 0)
    except ValueError:
        pass
    else:
        raise AssertionError("zero period must be rejected")


if __name__ == "__main__":
    for name, test in sorted(globals().items()):
        if name.startswith("test_"):
            test()
    print("Alarm deadline tests passed.")
