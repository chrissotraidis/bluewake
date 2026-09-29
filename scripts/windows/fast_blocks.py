#!/usr/bin/env python3
"""A prepaid copy of every translated block (the chunks' block machinery taken out).

  fast_blocks.py COMPOSITE_SRC

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
  - an instruction that calls nothing (no function but the pure bit helpers,
    no return, no goto) does not store its pc: nothing can read it before the
    next store, and the copy stores the last one it skipped before it leaves;
    every other instruction keeps its store,
  - a deadline refund (the only thing that clears cycle_block_prepaid inside a
    block) does as before and then continues in the original block at the next
    instruction, which charges itself as it would have,
  - the labels are the original block's (a jump from the copy goes where the
    same jump from the block goes), and the end continues where the block ends.
Everything else is the block's own text, so the copy does what the block does
whenever the block is prepaid. A block this cannot read with certainty keeps
no copy.

The change is repeatable (a prepared chunk is left as it is) and keeps LF line
ends. Run it last among the chunk rewrites (after direct_calls.py and
native_skin.py, which match the original text) and before
scripts/mods/prepare_simulation_60hz.py and prepare_native_math.py, which hash
the chunks as they end up.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/mods"))
from prepare_native_math import LEAVES  # noqa: E402

# The SDK leaves scripts/mods/prepare_native_math.py certifies by their
# translated bodies' hashes keep their blocks as they are (the dispatcher and
# the direct calls run them natively in any case).
CERTIFIED = [(start, end) for start, end, _, _ in LEAVES]

MARK = "/* bluewake: prepaid block copies (scripts/windows/fast_blocks.py) */\n"
INCLUDE = '#include "../generated.h"\n'
FUNCTION = re.compile(r"^(?:static )?void \w+\(CPUState\* ctx_param\) \{$")
PRECHARGE = re.compile(r"^    cycle_block_prepaid = dolrecomp_block_can_precharge\(ctx, (\d+)u\);$")
CHARGE = re.compile(r"^    if \(!cycle_block_prepaid && !dolrecomp_charge_precise\(ctx, \d+u, 0x([0-9A-F]{8})u\)\) return;$")
PC = re.compile(r"^    ctx->pc = 0x([0-9A-F]{8})u;$")
LABEL = re.compile(r"^label_[0-9A-F]{8}:$")
SUFFIX = re.compile(r"cycle_block_prepaid \? (\d+u) : 0u")
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


def quiet(code):
    """An instruction that cannot let anything read ctx->pc: it calls nothing
    but the pure helpers and neither returns nor jumps."""
    text = "\n".join(code)
    if "return" in text or "goto" in text:
        return False
    return all(name in PURE or name.startswith("__builtin_") for name in CALL.findall(text))


def fast_copy(parts, n, slow_label):
    """The prepaid copy of a block's instructions, or None where a refund is not
    the end of its instruction (the copy would skip what follows it)."""
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


def transform_function(lines):
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
        if any(start <= address < stop for start, stop in CERTIFIED):
            continue
        parts = split(lines[e:end])
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

        copy = fast_copy(parts, n, slow_label)
        if copy is None:
            continue
        for k, name in labels.items():
            inserts.setdefault(starts[k], []).append(f"{name}: ;")
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


def transform(text):
    if MARK in text:
        return text, 0
    if INCLUDE not in text:
        raise ValueError("no generated.h include")
    lines = text.split("\n")
    heads = [i for i, line in enumerate(lines) if FUNCTION.match(line)]
    if not heads:
        return text, 0
    out, blocks = lines[:heads[0]], 0
    for h, start in enumerate(heads):
        stop = heads[h + 1] if h + 1 < len(heads) else len(lines)
        close = stop - 1
        while close > start and lines[close] != "}":
            close -= 1
        body, count = transform_function(lines[start:close + 1])
        out.extend(body)
        out.extend(lines[close + 1:stop])
        blocks += count
    converted = "\n".join(out)
    if blocks:
        converted = converted.replace(INCLUDE, INCLUDE + MARK, 1)
    return converted, blocks


def main():
    root = Path(sys.argv[1])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    blocks = files = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted, count = transform(original)
        if count:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            blocks += count
            files += 1
    print(f"prepaid block copies: {blocks} blocks in {files} chunks")


if __name__ == "__main__":
    main()
