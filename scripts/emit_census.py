#!/usr/bin/env python3
"""Attribute emitted host code to guest opcodes, per generated chunk.

The dynamic census in docs/archive/status/CURRENT.md says the play scene is
instruction-bound at IPC 4.01, so the only lever is fewer host instructions per
guest instruction - and the guest side is a long tail, which means the fix has to
be broad rather than targeted at a few functions. The question that leaves is
*which* guest instructions are expensive to emit, because that is where a leaner
emitter pays.

This reads a generated chunk's Cyrillic-comment markers (``// 8031A6E8: lwz
r3, 0(r4)``) and counts the emitted C the emitter produced for each one,
aggregated by mnemonic. Emitted lines are a size proxy, not time; the dynamic
share of each opcode is a separate measurement. But it does answer, cheaply and
for the whole chunk, where the emitted code actually goes.

Usage: scripts/emit_census.py generated_chunk.c [--top N]
"""

import argparse
import re
import sys
from collections import Counter

# A new guest instruction begins at an emitter comment of this shape.
MARK = re.compile(r"^\s*//\s*([0-9A-Fa-f]{8}):\s+(\S+)")
# A new emitted function begins here. Without this the lines of the *next*
# function's prologue - precharge, budget test, first instruction - are charged to
# whatever guest instruction happened to sit at the end of the previous function,
# and branches are exactly the instructions that end functions. That inflated `bc`
# to 38.6 lines per instruction on the first run of this script; it is a counting
# artefact, not a property of the emitter.
FUNCTION = re.compile(r"^(static )?void (func|loop)_[0-9A-F]{8}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("chunk")
    ap.add_argument("--top", type=int, default=18)
    args = ap.parse_args()

    lines = open(args.chunk, errors="replace").read().splitlines()
    per_op = Counter()      # mnemonic -> emitted C lines
    count_op = Counter()    # mnemonic -> guest instructions
    current = None
    for line in lines:
        if FUNCTION.match(line):
            current = None
            continue
        m = MARK.match(line)
        if m:
            current = m.group(2)
            count_op[current] += 1
            continue
        if current is not None:
            per_op[current] += 1

    total_lines = sum(per_op.values())
    total_insts = sum(count_op.values())
    if not total_insts:
        sys.exit("emit_census: no guest-instruction markers found in %s" % args.chunk)

    print("chunk: %s" % args.chunk)
    print("guest instructions: %d   emitted C lines: %d   %.1f lines per guest instruction"
          % (total_insts, total_lines, total_lines / total_insts))
    print()
    print("  %-10s %7s %7s %10s" % ("opcode", "count", "lines", "lines/inst"))
    for op, n in per_op.most_common(args.top):
        print("  %-10s %7d %7d %10.1f" % (op, count_op[op], n, n / count_op[op]))
    print()
    print("  costliest by lines per instruction (count >= 20):")
    rows = [(n / count_op[op], op, count_op[op], n) for op, n in per_op.items()
            if count_op[op] >= 20]
    for density, op, c, n in sorted(rows, reverse=True)[:10]:
        print("    %-10s %6.1f lines/inst   (%d instructions, %d lines)"
              % (op, density, c, n))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
