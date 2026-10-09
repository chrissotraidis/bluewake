#!/usr/bin/env python3
"""Prepaid copies of eligible translated blocks.

  fast_blocks.py COMPOSITE_SRC [--lean]

Each block the translator emits starts by asking whether it can charge all its
cycles now (dolrecomp_block_can_precharge: the next deadline is further than
the block). It almost always can, but every instruction still carries the
machinery for when it cannot: its pc stored, a test of cycle_block_prepaid
that would charge it alone, the observation suffix chosen between its prepaid
value and 0. In integer code that is as many host instructions as the guest
instruction itself.

This gives each block a second copy, entered right after the block's start has
charged it all (cycle_block_prepaid true), in which
  - the per-instruction prepaid tests are gone (each is false there),
  - each suffix is its prepaid value,
  - every original pc store remains, including those in arithmetic-only
    instructions, so host observations keep the original guest pc,
  - a block that can refund cycles or otherwise change its prepaid state
    keeps its original body without a copy,
  - the labels are the original block's (a jump from the copy goes where the
    same jump from the block goes), and the end continues where the block ends.
Everything else is the block's own text. Only blocks that remain prepaid
throughout get copies; unsupported forms retain their original bodies.

--lean is Elliott Tate's original transform (imported in 0db6d35, made
conservative in 42af7ac), the one Wind Waker Recomp's builds use: it copies
every block it can read, refund blocks included, and an instruction that calls
nothing does not store its pc (nothing can read it before the next store; the
copy stores the last one it skipped before it leaves). A deadline refund does
as before and then continues in the original block at the next instruction.
It differs from the conservative form in when interrupts land by a few cycles,
so it fails the strict boot-route comparison (docs/PERFORMANCE.md, phase 4); it
is for measured, opt-in builds (--lean-blocks).

The change is repeatable (a prepared chunk is left as it is) and keeps LF line
ends. Prepare mod variants first and run this before recording the final
generated-source digest. This generic transform accepts the ordinary pointer
state chunks; it does not require fixed-register or native replacements.
Qualification is manual until correctness and matched performance are accepted.
"""
import re
import sys
from pathlib import Path

# Keep the four SDK leaves reserved by the donor's optional native-math
# certification unchanged. The generic transform does not require importing
# native replacements or enabling them in a player build.
CERTIFIED = [(0x8030D0C8, 0x8030D0FC), (0x8030D0FC, 0x8030D1C8),
             (0x8030DA44, 0x8030DA98), (0x8030DA98, 0x8030DB24)]

# Keep all blocks inside the nine certified vector leaves unchanged when routed.
VEC_CERTIFIED = [(0x8030DCE0, 0x8030DD44), (0x8030DE0C, 0x8030DF08),
                 (0x8030E0B4, 0x8030E0DC)]

MARK = "/* bluewake: prepaid block copies (scripts/windows/fast_blocks.py) */\n"
LEAN_MARK = "/* bluewake: lean prepaid block copies (scripts/windows/fast_blocks.py --lean) */\n"
INCLUDE = '#include "../generated.h"\n'
FUNCTION = re.compile(r"^(?:static )?void \w+\(CPUState\* (?:ctx|ctx_param)\) \{$")
PRECHARGE = re.compile(r"^    cycle_block_prepaid = dolrecomp_block_can_precharge\(ctx, (\d+)u\);$")
CHARGE = re.compile(r"^    if \(!cycle_block_prepaid && !dolrecomp_charge_precise\(ctx, \d+u, 0x([0-9A-F]{8})u\)\) return;$")
PC = re.compile(r"^    ctx->pc = 0x([0-9A-F]{8})u;$")
LABEL = re.compile(r"^label_[0-9A-F]{8}:$")
SUFFIX = re.compile(r"cycle_block_prepaid \? (\d+u) : 0u")
# --lean: the refund a block makes when a deadline falls inside it, and the
# helpers an instruction may call without letting anything read ctx->pc.
REFUND = [
    "    if (cycle_block_prepaid &&",
    "        ctx->cycle_deadline_budget > 0 &&",
    "        (s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget) {",
    "        ctx->downcount += (s64)ctx->cycle_observation_suffix;",
    "        cycle_block_prepaid = false;",
    "    }",
]
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
PURE = {"if", "dolrecomp_rotl32", "dolrecomp_f32_from_bits", "dolrecomp_f64_from_bits",
        "dolrecomp_f32_to_bits", "dolrecomp_f64_to_bits", "dolrecomp_ps_from_bits",
        "dolrecomp_ps_to_bits", "sizeof"}


def entry_length(lines, i, cycles):
    """How many lines after a precharge line the block's start takes to charge
    it, in the standard form or the loop functions' form (whose first
    instruction then has its own charge test); None for any other form."""
    pc = r"ctx->pc = 0x[0-9A-F]{8}u;"
    standard = [
        re.escape("    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {"),
        "        " + pc,
        re.escape("        return;"),
        re.escape("    }"),
        re.escape(f"    ctx->downcount -= cycle_block_prepaid ? {cycles}u : 1u;"),
    ]
    loop = [
        re.escape("    if (cycle_block_prepaid) {"),
        re.escape("        if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {"),
        "            " + pc,
        re.escape("            return;"),
        re.escape("        }"),
        re.escape(f"        ctx->downcount -= {cycles};"),
        re.escape("    }"),
    ]
    for form, at in ((standard, 1), (loop, 2)):
        body = lines[i + 1:i + 1 + len(form)]
        if len(body) == len(form) and all(re.fullmatch(p, l) for p, l in zip(form, body)):
            return len(form), int(re.search(r"0x([0-9A-F]{8})u", body[at]).group(1), 16)
    return None


def marker_length(block, i):
    """The lines of the instruction marker at block[i] - [label] [pc store]
    charge test - or 0 where there is none."""
    j = i + 1 if i < len(block) and LABEL.match(block[i]) else i
    if j + 1 < len(block) and PC.match(block[j]) and CHARGE.match(block[j + 1]):
        return j + 2 - i
    if j < len(block) and CHARGE.match(block[j]):
        return j + 1 - i
    return 0


def split(block):
    """A block's lines as instructions, [(marker, code)]. The first has no
    marker: the block's start stored its pc and charged it."""
    parts, marker, code, i = [], [], [], 0
    while i < len(block):
        length = marker_length(block, i)
        if length:
            parts.append((marker, code))
            marker, code = block[i:i + length], []
            i += length
        else:
            code.append(block[i])
            i += 1
    parts.append((marker, code))
    return parts


def fast_copy(parts, n):
    """Copy only blocks whose prepaid state cannot change inside the body.

    Keep deadline-refund and other unsupported state transitions in the original
    block. This also retains every PC store, including arithmetic instructions.
    """
    out = []
    for marker, code in parts:
        for line in code:
            if "cycle_block_prepaid" in SUFFIX.sub(r"\1", line):
                return None
        for line in marker:
            if PC.match(line):
                out.append(line)
        out.extend(SUFFIX.sub(r"\1", line) for line in code if not LABEL.match(line))
    out.append(f"    goto bwend_{n};")
    return out


def quiet(code):
    """--lean: an instruction that cannot let anything read ctx->pc: it calls
    nothing but the pure helpers and neither returns nor jumps."""
    text = "\n".join(code)
    if "return" in text or "goto" in text:
        return False
    return all(name in PURE or name.startswith("__builtin_") for name in CALL.findall(text))


def lean_copy(parts, n, slow_label):
    """--lean: the prepaid copy of a block's instructions (Elliott Tate's
    original), or None where a refund is not the end of its instruction."""
    out, skipped = [], None
    for k, (marker, code) in enumerate(parts):
        pc = next((PC.match(line).group(1) for line in marker if PC.match(line)), None)
        if pc is not None:
            if quiet(code):
                skipped = pc
            else:
                out.append(f"    ctx->pc = 0x{pc}u;")
                skipped = None
        elif marker:
            skipped = None
        c = 0
        while c < len(code):
            if code[c:c + len(REFUND)] == REFUND:
                if any(line.strip() for line in code[c + len(REFUND):]):
                    return None
                if skipped is not None:
                    out.append(f"    ctx->pc = 0x{skipped}u;")
                    skipped = None
                out.extend([
                    "    if (ctx->cycle_deadline_budget > 0 &&",
                    "        (s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget) {",
                    "        ctx->downcount += (s64)ctx->cycle_observation_suffix;",
                    "        cycle_block_prepaid = false;",
                    f"        goto {slow_label(k + 1)};",
                    "    }",
                ])
                c += len(REFUND)
                continue
            line = code[c]
            if not LABEL.match(line):
                out.append(SUFFIX.sub(r"\1", line))
            c += 1
    if skipped is not None:
        out.append(f"    ctx->pc = 0x{skipped}u;")
    out.append(f"    goto bwend_{n};")
    return out


def transform_function(lines, certified=CERTIFIED, lean=False):
    """One function's lines, header to closing brace, with prepaid copies."""
    close = len(lines) - 1
    sites = []
    for i, line in enumerate(lines):
        m = PRECHARGE.match(line)
        if m:
            found = entry_length(lines, i, m.group(1))
            if found is not None:
                length, address = found
                sites.append((i, i + 1 + length, address))
    if not sites:
        return lines, 0
    inserts, appended, done = {}, [], 0
    for n, (i, e, address) in enumerate(sites):
        # The block ends where the next one's start begins (its label and pc
        # store included), at the return dispatch, or at the closing brace.
        limit = sites[n + 1][0] if n + 1 < len(sites) else close
        for r in range(e, limit):
            if lines[r].startswith("return_dispatch_"):
                limit = r
                break
        end = limit
        if n + 1 < len(sites) and limit == sites[n + 1][0]:
            while end > e and (PC.match(lines[end - 1]) or LABEL.match(lines[end - 1]) or not lines[end - 1].strip()):
                end -= 1
        if any(start <= address < stop for start, stop in certified):
            continue
        parts = split(lines[e:end])
        if lean:
            starts, offset = [], e
            for marker, code in parts:
                starts.append(offset)
                offset += len(marker) + len(code)
            labels = {}

            def slow_label(k, n=n, starts=starts, labels=labels):
                if k >= len(starts):
                    return f"bwend_{n}"
                if k not in labels:
                    labels[k] = f"bwslow_{n}_{k}"
                return labels[k]

            copy = lean_copy(parts, n, slow_label)
            if copy is None:
                continue
            for k, name in labels.items():
                inserts.setdefault(starts[k], []).append(f"{name}: ;")
        else:
            copy = fast_copy(parts, n)
            if copy is None:
                continue
        inserts.setdefault(e, []).insert(0, f"    if (cycle_block_prepaid) goto bwfast_{n};")
        inserts.setdefault(end, []).append(f"bwend_{n}: ;")
        appended.append(f"bwfast_{n}:")
        appended.extend(copy)
        done += 1
    if not done:
        return lines, 0
    out = []
    for r, line in enumerate(lines[:close]):
        out.extend(inserts.get(r, []))
        out.append(line)
    out.extend(inserts.get(close, []))
    out.append("    return;")
    out.extend(appended)
    out.append(lines[close])
    return out, done


def transform(text, lean=False):
    mark, other = (LEAN_MARK, MARK) if lean else (MARK, LEAN_MARK)
    if other in text:
        raise ValueError("prepared in the other mode; regenerate the source")
    if mark in text:
        return text, 0
    if INCLUDE not in text:
        raise ValueError("no generated.h include")
    lines = text.split("\n")
    heads = [i for i, line in enumerate(lines) if FUNCTION.match(line)]
    if not heads:
        return text, 0
    certified = CERTIFIED + (VEC_CERTIFIED if '#include "native_vec.h"' in text else [])
    out, blocks = lines[:heads[0]], 0
    for h, start in enumerate(heads):
        stop = heads[h + 1] if h + 1 < len(heads) else len(lines)
        close = stop - 1
        while close > start and lines[close] != "}":
            close -= 1
        body, count = transform_function(lines[start:close + 1], certified, lean)
        out.extend(body)
        out.extend(lines[close + 1:stop])
        blocks += count
    converted = "\n".join(out)
    if blocks:
        converted = converted.replace(INCLUDE, INCLUDE + mark, 1)
    return converted, blocks


def main():
    args = sys.argv[1:]
    lean = "--lean" in args
    args = [arg for arg in args if arg != "--lean"]
    if len(args) != 1:
        sys.exit(__doc__)
    root = Path(args[0])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    blocks = files = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted, count = transform(original, lean)
        if count:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            blocks += count
            files += 1
    print(f"{'lean ' if lean else ''}prepaid block copies: {blocks} blocks in {files} chunks")


if __name__ == "__main__":
    main()
