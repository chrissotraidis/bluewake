#!/usr/bin/env python3
"""One floating-point-available test per stretch of a prepaid block copy.

  fp_checks.py COMPOSITE_SRC

Every translated floating-point instruction starts with

    if (!ppc_fp_available_inline(ctx, 0x...u)) return;

which takes the FP-unavailable exception when MSR[FP] is clear: the Dolphin OS
switches FPU state lazily (OSSetCurrentContext clears MSR[FP] for every thread
but the FPU's owner), so the first FP instruction after a thread switch traps.
The test is the only thing in the FP path that changes MSR, and it does so
only when it fails, and then it returns: the FP operations set FPSCR bits but
never take an exception (cpu_interpreter_float.c, set_fp_exception).

A prepaid copy (fast_blocks.py) is entered only at its start, holds no label
anything can jump to, and leaves for the original block at any refund. So
between one FP test that passed and the next, MSR is what it was, unless code
in between can change it: nothing the copy does to memory, FPRs or FPSCR can.
Here the first test in a copy stays where it is, with its pc store, so a
failure behaves exactly as before; a later one is dropped while every call
since the last test is to a helper on the list below. Any other call (a direct
call, a native routine, an extracted loop, rfi) leaves the next FP instruction
with its test, as the first. The original blocks, which can be resumed in the
middle after another thread has run, keep every test.

Without its test, an FP load or store is a plain access, so lean_memory.py can
make it lean like an integer one. Run after fast_blocks.py and direct_calls.py,
before lean_memory.py. The change is repeatable (a prepared chunk is left as
it is) and keeps LF line ends.
"""
import re
import sys
from pathlib import Path

MARK = "/* bluewake: one FP-available test per prepaid stretch (scripts/windows/fp_checks.py) */\n"
FAST_MARKS = ("/* bluewake: prepaid block copies (scripts/windows/fast_blocks.py) */\n",
              "/* bluewake: lean prepaid block copies (scripts/windows/fast_blocks.py --lean) */\n")
COPY = re.compile(r"^bwfast_(\d+):$")
END = re.compile(r"^    goto bwend_(\d+);$")
CHECK = re.compile(r"^    if \(!ppc_fp_available_inline\(ctx, 0x[0-9A-F]{8}u\)\) return;$")
LABEL = re.compile(r"^\s*[A-Za-z_]\w*:\s*;?\s*$")
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
# Calls that cannot change MSR: control words, the guest memory accesses (an
# MMIO handler does not touch MSR; an exception it raises is taken only after
# the copy returns), the pure conversions, and the FP operations, which never
# take an exception.
SAFE = {"if", "while", "for", "switch", "sizeof", "return",
        "read_be16", "read_be32", "read_be64", "write_be16", "write_be32", "write_be64",
        "dolrecomp_rotl32", "dolrecomp_f32_from_bits", "dolrecomp_f64_from_bits",
        "dolrecomp_f32_to_bits", "dolrecomp_f64_to_bits", "dolrecomp_ps_from_bits",
        "dolrecomp_ps_to_bits", "ppc_fpscr_updated", "ppc_memory_fence"}
SAFE_PATTERNS = (
    re.compile(r"__builtin_\w+"),
    re.compile(r"(mem|bw)_(read|write)(8|16|32|64)(_at|_ato)?"),
    re.compile(r"ppc_(f(add|sub|mul|div)s?|fma|fcmp|fctiwz?|frsp|frsqrte|fres|fsel|fneg|fabs|fnabs|fmr)"),
    re.compile(r"ppc_ps_\w+"),
    re.compile(r"ppc_psq_(load|store)(_inline)?"),
    re.compile(r"bw_fp_\w+"),
)


def safe(name):
    return name in SAFE or any(p.fullmatch(name) for p in SAFE_PATTERNS)


def thin(body):
    """A copy's lines with every FP test after the first of its stretch removed."""
    out, tested, removed = [], False, 0
    for line in body:
        if CHECK.match(line):
            if tested:
                removed += 1
                continue
            tested = True
            out.append(line)
            continue
        stripped = line.lstrip()
        if LABEL.match(line):
            tested = False
        elif not stripped.startswith("//") and any(not safe(name) for name in CALL.findall(line)):
            tested = False
        out.append(line)
    return out, removed


def transform(text):
    if MARK in text:
        return text, 0
    fast = next((mark for mark in FAST_MARKS if mark in text), None)
    if fast is None:
        return text, 0
    lines = text.split("\n")
    out, i, total = [], 0, 0
    while i < len(lines):
        m = COPY.match(lines[i])
        if not m:
            out.append(lines[i])
            i += 1
            continue
        j = i + 1
        while j < len(lines) and not (END.match(lines[j]) and END.match(lines[j]).group(1) == m.group(1)):
            j += 1
        if j >= len(lines):
            raise ValueError(f"copy bwfast_{m.group(1)} has no end")
        body, count = thin(lines[i + 1:j])
        out.append(lines[i])
        out.extend(body)
        out.append(lines[j])
        total += count
        i = j + 1
    converted = "\n".join(out)
    if total:
        converted = converted.replace(fast, fast + MARK, 1)
    return converted, total


def main():
    root = Path(sys.argv[1])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    removed = files = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted, count = transform(original)
        if count:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            removed += count
            files += 1
    print(f"FP-available tests removed: {removed} in {files} chunks")


if __name__ == "__main__":
    main()
