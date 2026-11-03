#!/usr/bin/env python3
"""Include independently selected helpers in translated chunks.

  chunk_headers.py COMPOSITE_SRC [--inline-fp] [--gather-pipe]

Place inline_fp.h after generated.h, and gather_pipe.h before it so generated
paired-single helpers use the memory wrappers too. With no flags, retain the
original floating-point-only command. Repeated runs leave files unchanged.
"""
import argparse
from pathlib import Path

MARK = "/* bluewake: inline floating point (scripts/windows/chunk_headers.py) */\n"
INCLUDE = '#include "../generated.h"\n'
BEFORE = MARK
AFTER = '#include "inline_fp.h"\n'
GATHER_MARK = "/* bluewake: gather memory (scripts/windows/chunk_headers.py) */\n"


def transform(text, path, *, inline_fp=True, gather_pipe=False):
    if INCLUDE not in text:
        raise ValueError(f"{path}: no generated.h include to place the headers around")
    if gather_pipe and GATHER_MARK not in text:
        text = text.replace(INCLUDE, GATHER_MARK + '#include "gather_pipe.h"\n' + INCLUDE, 1)
    if inline_fp and MARK not in text:
        text = text.replace(INCLUDE, BEFORE + INCLUDE + AFTER, 1)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--inline-fp", action="store_true")
    parser.add_argument("--gather-pipe", action="store_true")
    args = parser.parse_args()
    root = args.root
    inline_fp = args.inline_fp or not args.gather_pipe
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        parser.error(f"no chunks under {root}")
    changed = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted = transform(original, path, inline_fp=inline_fp, gather_pipe=args.gather_pipe)
        if converted != original:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            changed += 1
    print(f"inline helpers (fp={inline_fp}, gather={args.gather_pipe}): {changed} of {len(chunks)} chunks")


if __name__ == "__main__":
    main()
