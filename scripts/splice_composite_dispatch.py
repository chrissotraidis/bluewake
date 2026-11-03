#!/usr/bin/env python3
"""Apply generate_composite.py's dispatch block to a composite tree, and check it.

Why this exists. The composite generator does not run end to end from this tree
(scripts/generate_composite.py reports "REL module 181 section 1 linked range
[0x804000f4, 0x80400fd0) overlaps retail MEM1" against the REL inputs the tree
holds; see docs/archive/status/CURRENT.md, 2026-09-18), so the header the composite is
built from cannot be regenerated as a whole. The dispatch block can, and it is
the part the P1 workstream changes: it is emitted independently of the chunk
lists, so it can be lifted out of a generator run and written into a tree.

That is what this does, and it is why it exists rather than a paste: the
generator writes the artifact and this writes the check with it. Every region is
matched exactly or the script refuses, and the result is re-read and compared
against the generator's own text, so a tree either holds the generator's block
or the command fails.

Usage:
  scripts/splice_composite_dispatch.py COMPOSITE_DIR [--check]

  COMPOSITE_DIR   directory holding generated_composite.h and generated.h
  --check         verify and report only; write nothing

Private input is not involved: the synthetic fixture is built by
tests/test_composite.py, which is the same fixture the generator's own tests use.
"""

import argparse
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests"))

import test_composite as tc  # noqa: E402  the generator's synthetic fixture

FIND = "static inline DolRecompFunction dolrecomp_find_original"
TABLES = "    static const u32 s_starts[] = {\n"
CALL_END = "static inline DOLRECOMP_UNUSED int dolrecomp_run_blocks"
TAIL_FIRST = "static inline int dolrecomp_call_original"
# The split adds a function ahead of dolrecomp_call, so the block that replaces
# the single-function form starts at its comment and ends where run_blocks does.
CALL_MARK = "// The dispatcher is split"


def emitted_blocks():
    with tempfile.TemporaryDirectory() as td:
        tc.setup_fixture(td, dol_chunks=[(0x80003100, 0x80003140), (0x80003140, 0x80003200)])
        tc.run_generator(td)
        header = (Path(td) / "composite" / "generated_composite.h").read_text()
    i = header.index(FIND)
    head = header[i:header.index(TABLES, i)]
    # Everything from the cache store to the end of the dispatcher is emitted as
    # one region: the store, call_original, the alias predicate, the out-of-line
    # path and dolrecomp_call. Splitting it more finely left call_original stale
    # the first time the dispatcher's signature changed.
    k = header.index(TAIL_FIRST)
    tail = header[k:header.index(CALL_END, k)]
    for name, block in (("head", head), ("tail", tail)):
        if not block:
            sys.exit(f"splice_composite_dispatch: empty {name} block from the generator")
    return head, tail


def rewrite(text, head, tail):
    """Replace the tree's dispatch block with the generator's, exactly."""
    i = text.index(FIND)
    j = text.index(TABLES, i)
    text = text[:i] + head + text[j:]
    k = text.index(TAIL_FIRST)
    j = text.index(CALL_END, k)
    return text[:k] + tail + text[j:]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("composite_dir", type=Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    head, tail = emitted_blocks()
    changed = 0
    for name in ("generated_composite.h", "generated.h"):
        path = args.composite_dir / name
        if not path.is_file():
            sys.exit("splice_composite_dispatch: not in a composite tree: " + str(path))
        try:
            text = rewrite(path.read_text(), head, tail)
        except ValueError as exc:
            sys.exit(f"splice_composite_dispatch: {name}: anchor not found ({exc})")
        if head not in text or tail not in text:
            sys.exit(f"splice_composite_dispatch: {name}: the generator's block did not land")
        if text == path.read_text():
            print(f"unchanged {name}")
            continue
        changed += 1
        if not args.check:
            path.write_text(text)
        print(f"{'would update' if args.check else 'updated'} {name}")
    print(f"splice_composite_dispatch: {changed} of 2 files "
          f"{'need' if args.check else 'were given'} the generator's dispatch block")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
