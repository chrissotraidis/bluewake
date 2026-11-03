#!/usr/bin/env python3
"""Replay a BlueWake pad trace in Dolphin: write an input movie (.dtm).

    scripts/pad_trace_to_dtm.py OUT.dtm TRACE.log --from R0 --to R1 --at-poll P
                                [--prefix ENTRY[,ENTRY...]] [--tail POLLS]

TRACE.log is a BlueWake run's stderr with BLUEWAKE_PAD_TRACE=1: one
"[pad-trace] retrace=N button=0x.... stick=x,y c=x,y l=L r=R" line whenever
channel 0 changes. Retraces R0..R1 of that trace (for example a steered route
from the moment the player can move) are written from poll P on, two polls a
retrace: Wind Waker sets SIPOLL to 0x00F60200 (a poll every 246 lines), and
Dolphin's VideoInterface polls once at each field's start and again 246 lines
later, inside the 262.5-line field, taking one movie state per poll.
--prefix adds make_dtm.py-shaped presses before P (poll:buttons:length), such
as the A presses that load a save. Open-loop replay holds as long as the game
is in the same idle state at R0 and at P, so pick R0 at the player-ready
retrace and P after Dolphin has certainly given control.
"""
import argparse
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_dtm import apply_game_config  # noqa: E402

BITS = {
    0x1000: (0, 0), 0x0100: (0, 1), 0x0200: (0, 2), 0x0400: (0, 3),
    0x0800: (0, 4), 0x0010: (0, 5), 0x0008: (0, 6), 0x0004: (0, 7),
    0x0001: (1, 0), 0x0002: (1, 1), 0x0040: (1, 2), 0x0020: (1, 3),
}
LINE = re.compile(r'\[pad-trace\] retrace=(\d+) button=0x([0-9A-Fa-f]+) '
                  r'stick=(-?\d+),(-?\d+) c=(-?\d+),(-?\d+) l=(\d+) r=(\d+)')


def state(buttons, sx, sy, cx, cy, tl, tr):
    b = [0, 1 << 6]  # is_connected
    for bit, (byte, pos) in BITS.items():
        if buttons & bit:
            b[byte] |= 1 << pos
    if b[1] & (1 << 2):
        tl = 255
    if b[1] & (1 << 3):
        tr = 255
    clamp = lambda v: max(0, min(255, 128 + v))
    return bytes([b[0], b[1], tl, tr, clamp(sx), clamp(sy), clamp(cx), clamp(cy)])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('out')
    ap.add_argument('trace')
    ap.add_argument('--from', dest='r0', type=int, required=True)
    ap.add_argument('--to', dest='r1', type=int, required=True)
    ap.add_argument('--at-poll', dest='poll', type=int, required=True)
    ap.add_argument('--prefix', default='')
    ap.add_argument('--tail', type=int, default=600)
    a = ap.parse_args()

    changes = []
    for line in open(a.trace, errors='replace'):
        m = LINE.search(line)
        if m:
            v = [int(m.group(1)), int(m.group(2), 16)] + [int(m.group(i)) for i in range(3, 9)]
            changes.append(v)
    if not changes:
        raise SystemExit('no [pad-trace] lines in %s (run with BLUEWAKE_PAD_TRACE=1)' % a.trace)
    idle = state(0, 0, 0, 0, 0, 0, 0)
    per_retrace = []
    cur = None
    i = 0
    for r in range(a.r0, a.r1):
        while i < len(changes) and changes[i][0] <= r:
            cur = changes[i]
            i += 1
        per_retrace.append(state(*cur[1:]) if cur else idle)

    total = a.poll + 2 * len(per_retrace) + a.tail
    states = [idle] * total
    for item in filter(None, a.prefix.split(',')):
        f = [int(x, 0) for x in item.split(':')]
        for p in range(f[0], min(total, f[0] + f[2])):
            states[p] = state(f[1], 0, 0, 0, 0, 0, 0)
    for k, s in enumerate(per_retrace):
        states[a.poll + 2 * k] = s
        states[a.poll + 2 * k + 1] = s

    h = bytearray(256)
    h[0:4] = b'DTM\x1a'
    h[4:10] = b'GZLE01'
    h[11] = 0x01                                    # GC controller, port 1
    struct.pack_into('<Q', h, 13, total)            # frameCount (informational)
    struct.pack_into('<Q', h, 21, total)            # inputCount
    h[151] = 0x01                                   # memory card in slot A
    apply_game_config(h)
    struct.pack_into('<Q', h, 237, 486000000 * 900)  # tickCount: 900 s, see make_dtm.py
    open(a.out, 'wb').write(bytes(h) + b''.join(states))
    moving = sum(1 for s in per_retrace if s != idle)
    print('wrote %s: %d polls; retraces %d-%d (%d with input) from poll %d'
          % (a.out, total, a.r0, a.r1, moving, a.poll))


if __name__ == '__main__':
    main()
