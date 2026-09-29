#!/usr/bin/env python3
"""Include the Windows builder's inline helpers in every translated chunk.

  chunk_headers.py COMPOSITE_SRC

- cmake/composite/gather_pipe.h, ahead of the generated header: guest stores
  to the GX gather pipe call the host's GX writer directly instead of going
  through the MMIO handler (the generated header's paired-single stores
  included).
- cmake/composite/inline_fp.h, after it: the common floating-point
  instructions inline instead of calls into the interpreter, with the
  interpreter's results, flags and registers (tests/inline_fp_test.c).

What each instruction does is unchanged. The change is repeatable (a
prepared chunk is left as it is) and keeps LF line ends. Run it with the other
source steps in scripts/windows/build.py, before
scripts/mods/prepare_simulation_60hz.py, whose manifest hashes the chunks as
they finally are.
"""
import sys
from pathlib import Path

MARK = "/* bluewake: inline helpers (scripts/windows/chunk_headers.py) */\n"
INCLUDE = '#include "../generated.h"\n'
BEFORE = MARK + '#include "gather_pipe.h"\n'
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
    print(f"inline helpers (gather pipe, floating point): {changed} of {len(chunks)} chunks")


if __name__ == "__main__":
    main()
