#!/usr/bin/env python3
"""Point the translated chunks at the module's fixed guest CPU state.

  global_guest_cpu.py COMPOSITE_SRC

Every chunk function takes the guest CPU state as `CPUState* ctx` and reaches
each register through it. A chunk is one function of 4,096 instructions with
a label per instruction, and on x86-64 clang cannot keep that pointer in a
register across it: most field accesses reload it from the stack first (about
32,000 reloads in one 2 MB chunk). The module keeps one state object at a
fixed address (cmake/composite/guest_cpu.c) and the host runs the guest on
it, so `ctx` can name that object and every access becomes a single
RIP-relative instruction. The generated code is otherwise unchanged: the
same statements, the same control flow, the same guest behaviour.

The change is repeatable (a prepared chunk is left as it is) and keeps LF line
ends. Run it before scripts/mods/prepare_simulation_60hz.py, whose manifest
hashes the chunks as they finally are.
"""
import re
import sys
from pathlib import Path

MARK = "/* bluewake: the guest CPU at a fixed address (cmake/composite/guest_cpu.c) */\n"
DEFINE = MARK + "extern CPUState bw_guest_cpu;\n#define ctx (&bw_guest_cpu)\n"
INCLUDE = '#include "../generated.h"\n'
# Definitions and prototypes: the parameter's name only.
PARAM = re.compile(r"\bCPUState\* ctx\b(?=[,)])")


def transform(text, path):
    if MARK in text:
        return text
    if INCLUDE not in text:
        raise ValueError(f"{path}: no generated.h include to place the definition after")
    text = PARAM.sub("CPUState* ctx_param", text)
    return text.replace(INCLUDE, INCLUDE + DEFINE, 1)


def main():
    root = Path(sys.argv[1])
    changed = 0
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted = transform(original, path)
        if converted != original:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            changed += 1
    print(f"guest CPU at a fixed address: {changed} of {len(chunks)} chunks rewritten")


if __name__ == "__main__":
    main()
