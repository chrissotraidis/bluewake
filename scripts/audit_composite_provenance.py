#!/usr/bin/env python3
"""Compare two ignored composite trees using public-safe file metadata.

The report contains relative paths, byte sizes, modification times, and
SHA-256 hashes only. It never reads or emits file contents.

Usage:
  python3 scripts/audit_composite_provenance.py ROOT_A ROOT_B
  python3 scripts/audit_composite_provenance.py ROOT_A ROOT_B --json-out REPORT
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path


def file_record(root, path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    stat = path.stat()
    return {
        "size": stat.st_size,
        "mtime_ns": stat.st_mtime_ns,
        "sha256": digest.hexdigest(),
    }


def index_tree(root):
    return {
        path.relative_to(root).as_posix(): file_record(root, path)
        for path in sorted(root.rglob("*"))
        if path.is_file()
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("root_a", type=Path)
    ap.add_argument("root_b", type=Path)
    ap.add_argument("--json-out", type=Path)
    args = ap.parse_args()

    roots = [root.resolve() for root in (args.root_a, args.root_b)]
    for root in roots:
        if not root.is_dir():
            print(f"FAIL: composite tree not found: {root}", file=sys.stderr)
            return 1

    indexes = [index_tree(root) for root in roots]
    names = sorted(set(indexes[0]) | set(indexes[1]))
    # Modification time is evidence about lineage, but not file identity.
    differing = [
        name
        for name in names
        if tuple(indexes[0].get(name, {}).get(key) for key in ("size", "sha256"))
        != tuple(indexes[1].get(name, {}).get(key) for key in ("size", "sha256"))
    ]
    report = {
        "roots": [str(root) for root in roots],
        "file_counts": [len(index) for index in indexes],
        "differing_files": len(differing),
        "files": {
            name: {"a": indexes[0].get(name), "b": indexes[1].get(name)}
            for name in names
            if name in differing
        },
    }
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(report, indent=2) + "\n")

    print(f"A: {report['file_counts'][0]} files")
    print(f"B: {report['file_counts'][1]} files")
    print(f"Different: {report['differing_files']} files")
    print(f"Result: {'MATCH' if not differing else 'DIFFERENT'}")
    return 0 if not differing else 2


if __name__ == "__main__":
    raise SystemExit(main())
