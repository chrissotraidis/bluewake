#!/usr/bin/env python3
"""Deadline tests in the prepaid block copies only where the deadline can move.

  lean_deadlines.py COMPOSITE_SRC

After lean_memory.py, every plain load or store in a prepaid copy still tests
the cycle deadline after it (does the rest of the block, its suffix, reach the
next device deadline?), and so do the moves to and from the link, count and
XER registers, which the translator counts with mfspr and mtspr as possible
timer reads. Both tests are always false unless something moved the deadline
since the last one:

  - a copy is entered only when dolrecomp_block_can_precharge found the
    deadline at or beyond the block's end, so the deadline budget is at least
    the block's cycles, and every suffix in the block is at most that;
  - suffixes only shrink through a block, and a test that passes leaves the
    budget at least its suffix;
  - only a call can change the budget: an access's way out of line (MMIO, the
    decrementer, the timebase all observe and re-budget there), or another
    helper an instruction calls.

So a lean access keeps its test only on its way out of line (bw_readN_ato and
bw_writeN_ato in gather_pipe.h set a flag there), and a link, count or XER
move drops its suffix store and its test, unless an instruction since the
last test called anything but the pure bit helpers and the lean accesses: then
the next test stays as it was. Nothing else changes, so the guest runs the
same instructions with the same timing; the guest checkpoint differs only in
cycle_observation_suffix, which nothing reads until an observation stores its
own (as with lean_memory.py).

Run it right after lean_memory.py. The change is repeatable (a prepared chunk
is left as it is) and keeps LF line ends.
"""
import re
import sys
from pathlib import Path

MARK = "/* bluewake: deadline tests only where the deadline can move (scripts/windows/lean_deadlines.py) */\n"
LEAN_MEMORY_MARK = "/* bluewake: lean memory accesses in prepaid copies (scripts/windows/lean_memory.py) */\n"
COPY = re.compile(r"^bwfast_(\d+):$")
END = re.compile(r"^    goto bwend_(\d+);$")
COMMENT = re.compile(r"^    // [0-9A-F]{8}: (\w+)")
SUFFIX = re.compile(r"^    ctx->cycle_observation_suffix = (\d+)u;$")
LEAN_ACCESS = re.compile(r"\bbw_(read|write)(8|16|32|64)_at\(ctx, ")
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
PURE = {"if", "dolrecomp_rotl32", "dolrecomp_f32_from_bits", "dolrecomp_f64_from_bits",
        "dolrecomp_f32_to_bits", "dolrecomp_f64_to_bits", "dolrecomp_ps_from_bits",
        "dolrecomp_ps_to_bits", "sizeof", "while", "for", "switch", "return"}
MOVES = {"mflr", "mtlr", "mfctr", "mtctr", "mfxer", "mtxer"}
# lean_memory.py's test after a lean access (the suffix a constant).
ACCESS_TEST = re.compile(r"^        \(s64\)(\d+)u > ctx->cycle_deadline_budget\) \{$")
# The copy's own test, after an instruction that stored its suffix.
STORED_TEST = [
    "    if (ctx->cycle_deadline_budget > 0 &&",
    "        (s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget) {",
    "        ctx->downcount += (s64)ctx->cycle_observation_suffix;",
    "        cycle_block_prepaid = false;",
]
GOTO = re.compile(r"^        goto (bwslow_\d+_\d+|bwend_\d+);$")


def calls(line):
    """What a line calls besides the pure bit helpers and the lean accesses."""
    return [name for name in CALL.findall(line)
            if name not in PURE and not name.startswith("__builtin_")
            and not re.fullmatch(r"bw_(read|write)(8|16|32|64)_at", name)]


def access_test_at(lines, i):
    """lean_memory.py's 8-line test at lines[i], as its suffix, or None."""
    if i + 7 < len(lines) and lines[i] == "    if (ctx->cycle_deadline_budget > 0 &&":
        m = ACCESS_TEST.match(lines[i + 1])
        if m and GOTO.match(lines[i + 6]) and lines[i + 7] == "    }":
            return int(m.group(1))
    return None


def stored_test_at(lines, i):
    return lines[i:i + 4] == STORED_TEST and i + 5 < len(lines) and GOTO.match(lines[i + 4]) \
        and lines[i + 5] == "    }"


def lean_copy(lines):
    """The copy with the tests that cannot fire removed, and the count."""
    out, changed, dirty, i = [], 0, False, 0
    while i < len(lines):
        line = lines[i]
        suffix = access_test_at(lines, i)
        if suffix is not None:
            # The instruction's text runs from its comment to here.
            start = max(k for k in range(len(out)) if COMMENT.match(out[k])) if any(
                COMMENT.match(l) for l in out) else None
            body = out[start:] if start is not None else []
            if not dirty and len(LEAN_ACCESS.findall("\n".join(body))) == 1:
                del out[start:]
                out.append("    {")
                out.append("    bool bw_ool = false;")
                out.extend(LEAN_ACCESS.sub(lambda m: f"bw_{m.group(1)}{m.group(2)}_ato(&bw_ool, ctx, ", l)
                           for l in body)
                out.append("    if (__builtin_expect(bw_ool, 0) &&")
                out.append("        ctx->cycle_deadline_budget > 0 &&")
                out.extend(lines[i + 1:i + 8])
                out.append("    }")
                changed += 1
            else:
                out.extend(lines[i:i + 8])
            dirty = False
            i += 8
            continue
        if stored_test_at(lines, i):
            out.extend(lines[i:i + 6])
            dirty = False
            i += 6
            continue
        m = SUFFIX.match(line)
        if m and i + 1 < len(lines) and COMMENT.match(lines[i + 1]) and \
                COMMENT.match(lines[i + 1]).group(1) in MOVES and not dirty:
            # suffix, the move's text (no calls), an optional pc store, the test.
            k = i + 2
            while k < len(lines) and not stored_test_at(lines, k) and not COMMENT.match(lines[k]) \
                    and not SUFFIX.match(lines[k]) and access_test_at(lines, k) is None:
                k += 1
            middle = lines[i + 1:k]
            if k < len(lines) and stored_test_at(lines, k) and not any(calls(l) for l in middle) \
                    and not any("goto" in l or "return" in l for l in middle):
                out.extend(middle)
                changed += 1
                i = k + 6
                continue
        if calls(line):
            dirty = True
        out.append(line)
        i += 1
    return out, changed


def transform(text):
    if MARK in text or LEAN_MEMORY_MARK not in text:
        return text, 0
    lines = text.split("\n")
    out, i, total = [], 0, 0
    while i < len(lines):
        m = COPY.match(lines[i])
        if not m:
            out.append(lines[i])
            i += 1
            continue
        n = m.group(1)
        j = i + 1
        while j < len(lines) and not (END.match(lines[j]) and END.match(lines[j]).group(1) == n):
            j += 1
        if j >= len(lines):
            raise ValueError(f"copy bwfast_{n} has no end")
        body, count = lean_copy(lines[i + 1:j])
        out.append(lines[i])
        out.extend(body)
        out.append(lines[j])
        total += count
        i = j + 1
    converted = "\n".join(out)
    if total:
        converted = converted.replace(LEAN_MEMORY_MARK, LEAN_MEMORY_MARK + MARK, 1)
    return converted, total


def main():
    root = Path(sys.argv[1])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    tests = files = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted, count = transform(original)
        if count:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            tests += count
            files += 1
    print(f"lean deadline tests: {tests} in {files} chunks")


if __name__ == "__main__":
    main()
