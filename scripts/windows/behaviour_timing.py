#!/usr/bin/env python3
"""Game-behaviour timing (a prototype, off by default): every block prepaid.

  behaviour_timing.py COMPOSITE_SRC [--loop-checks]

After fast_blocks.py (and lean_memory.py, lean_deadlines.py), each block that
has a prepaid copy starts like this:

    label_X:
        ctx->pc = 0xX;
        cycle_block_prepaid = dolrecomp_block_can_precharge(ctx, N);
        if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) { ctx->pc = 0xX; return; }
        ctx->downcount -= cycle_block_prepaid ? N : 1u;
        if (cycle_block_prepaid) goto bwfast_K;

The precharge test sends a block to its precise copy (an instruction at a time)
when the next device deadline (a timer, an interrupt) falls inside it, so the
event lands on the exact instruction, as on the console. Here every block runs
its prepaid copy and is charged its N cycles at once: an event lands at the
next block's start instead, at most one block late. The turn's budget is still
checked at every block, so control comes back to the host on time; the program
counter is stored only on that way out (the prepaid copy stores it before every
call). The game behaves the same; its timing no longer matches the console
cycle for cycle, so guest checkpoints differ from a build without it.

With --loop-checks (step 2, measured separately), the turn's budget is checked
only at the start of a block that a later line jumps back to (a loop): the
others are charged and run. A call still checks it (bw_direct_call_ready), as
does the dispatcher, so control still comes back within one pass of
straight-line code.

Repeatable (a prepared chunk is left as it is); keeps LF line ends.
"""
import re
import sys
from pathlib import Path

MARK = "/* bluewake: game-behaviour timing, every block prepaid (scripts/windows/behaviour_timing.py) */\n"
LOOP_MARK = "/* bluewake: game-behaviour timing, budget checked at loops (scripts/windows/behaviour_timing.py --loop-checks) */\n"
GOTO_LABEL = re.compile(r"goto (label_[0-9A-F]{8});")
LABEL = re.compile(r"^label_([0-9A-F]{8}):$")
PC = "    ctx->pc = 0x{}u;"
PRECHARGE = re.compile(r"^    cycle_block_prepaid = dolrecomp_block_can_precharge\(ctx, (\d+)u\);$")
BUDGET = "    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {"
EXIT_PC = "        ctx->pc = 0x{}u;"
CHARGE = re.compile(r"^    ctx->downcount -= cycle_block_prepaid \? (\d+)u : 1u;$")
ENTER = re.compile(r"^    if \(cycle_block_prepaid\) goto (bwfast_\d+);$")


FAST = re.compile(r"^(bwfast_\d+):$")


def loop_heads(lines):
    """Labels a jump goes back to, by guest address: from the block at or after
    them. A prepaid copy belongs to the block whose start enters it."""
    owner = {}
    for n, line in enumerate(lines):
        m = LABEL.match(line)
        if m:
            for k in range(n + 1, min(n + 10, len(lines))):
                e = ENTER.match(lines[k])
                if e:
                    owner[e.group(1)] = int(m.group(1), 16)
                    break
    back, here = set(), None
    for line in lines:
        m = LABEL.match(line)
        if m:
            here = int(m.group(1), 16)
        f = FAST.match(line)
        if f:
            here = owner.get(f.group(1), here)
        # A chunk's dispatch switch (case 0x...u: goto label_...) checks the budget before it.
        if here is None or line.lstrip().startswith("case "):
            continue
        for target in GOTO_LABEL.findall(line):
            if int(target[6:], 16) <= here:
                back.add(target)
    return back


def transform(text, loop_checks=False):
    lines = text.split("\n")
    loops = loop_heads(lines) if loop_checks else None
    out = []
    changed = 0
    i = 0
    while i < len(lines):
        m = LABEL.match(lines[i])
        if m and i + 8 < len(lines):
            a = m.group(1)
            pre = PRECHARGE.match(lines[i + 2])
            charge = CHARGE.match(lines[i + 7])
            enter = ENTER.match(lines[i + 8])
            if (lines[i + 1] == PC.format(a) and pre and lines[i + 3] == BUDGET
                    and lines[i + 4] == EXIT_PC.format(a) and lines[i + 5] == "        return;"
                    and lines[i + 6] == "    }" and charge and enter and charge.group(1) == pre.group(1)):
                head = [lines[i]]
                if loops is None or "label_" + a in loops:
                    head += [BUDGET, EXIT_PC.format(a), "        return;", "    }"]
                out += head + ["    cycle_block_prepaid = true;",
                        "    ctx->downcount -= {}u;".format(pre.group(1)),
                        "    goto {};".format(enter.group(1))]
                changed += 1
                i += 9
                continue
        out.append(lines[i])
        i += 1
    return "\n".join(out), changed


def main():
    args = sys.argv[1:]
    loop_checks = "--loop-checks" in args
    args = [a for a in args if a != "--loop-checks"]
    if len(args) != 1:
        sys.exit(__doc__)
    root = Path(args[0])
    mark = LOOP_MARK if loop_checks else MARK
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit("behaviour_timing: no chunks under " + str(root))
    total = files = 0
    for path in chunks:
        text = path.read_text()
        if text.startswith(MARK) or text.startswith(LOOP_MARK):
            continue
        new, changed = transform(text, loop_checks)
        if changed:
            path.write_text(mark + new)
            total += changed
            files += 1
    print("behaviour_timing: {} block starts in {} chunks now always prepaid".format(total, files))


if __name__ == "__main__":
    main()
