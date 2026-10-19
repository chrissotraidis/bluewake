"""Bounded CPU pressure child used only by explicit qualification runs."""

import argparse
import hashlib
import time


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, required=True)
    parser.add_argument("--duty", type=int, choices=range(1, 101), required=True)
    args = parser.parse_args(argv)
    if args.seconds <= 0:
        parser.error("--seconds must be positive")
    deadline = time.monotonic() + args.seconds
    period = 0.05
    payload = b"bluewake qualification pressure"
    while time.monotonic() < deadline:
        active_until = min(deadline, time.monotonic() + period * args.duty / 100)
        while time.monotonic() < active_until:
            payload = hashlib.sha256(payload).digest()
        sleep_for = min(deadline - time.monotonic(), period * (100 - args.duty) / 100)
        if sleep_for > 0:
            time.sleep(sleep_for)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
