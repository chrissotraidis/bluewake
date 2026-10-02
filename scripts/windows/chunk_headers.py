#!/usr/bin/env python3
"""Include independently selected floating-point helpers in translated chunks.

  chunk_headers.py COMPOSITE_SRC

Place inline_fp.h after generated.h so ordinary and fixed-CPU chunks use the
same interpreter-compatible operations. This preparation does not enable
memory wrappers or gather-pipe batching. Repeated runs leave files unchanged.
"""
import sys
from pathlib import Path

MARK = "/* bluewake: inline floating point (scripts/windows/chunk_headers.py) */\n"
INCLUDE = '#include "../generated.h"\n'
BEFORE = MARK
AFTER = '#include "inline_fp.h"\n'


def transform(text, path):
    if MARK in text:
        return text
    if INCLUDE not in text:
        raise ValueError(f"{path}: no generated.h include to place the headers around")
    return text.replace(INCLUDE, BEFORE + INCLUDE + AFTER, 1)


def main():
    root = Path(sys.argv[1])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    changed = 0
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
    print(f"inline floating point: {changed} of {len(chunks)} chunks")


if __name__ == "__main__":
    main()
