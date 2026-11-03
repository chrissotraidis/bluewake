#!/usr/bin/env python3
"""List the chunks the optimization profile never ran, for cmake/composite to
compile at -O1 (COMPOSITE_COLD_SOURCES_FILE), as the Windows and Linux builders
do (scripts/windows/build.py cold_sources). A chunk is one function, named
func_<its file's address>.

    cold_sources.py PROFILE.profdata COMPOSITE_SRC OUT.txt
"""
import re
import subprocess
import sys
from pathlib import Path


def cold_sources(stats, src):
    counts = {name.upper(): int(count) for name, count in
              re.findall(r"(?m)^  func_([0-9A-Fa-f]+):\n(?:    .*\n)*?    Function count: (\d+)", stats)}
    return [path.relative_to(src).as_posix() for path in sorted(src.glob("chunks_*/*.c"))
            if counts.get(path.stem.rsplit("_", 1)[-1].upper()) == 0]


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    profile, src, out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    stats = subprocess.run(["xcrun", "llvm-profdata", "show", "--all-functions", str(profile)],
                           capture_output=True, text=True, check=True).stdout
    cold = cold_sources(stats, src)
    out.write_text("".join(line + "\n" for line in cold), encoding="utf-8")
    print(f"tiered: {len(cold)} chunks the training never ran at -O1")


if __name__ == "__main__":
    main()
