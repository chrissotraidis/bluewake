#!/usr/bin/env python3
"""Print one SHA-256 over a generated composite source tree.

The digest covers every file's relative path and content hash, in sorted
order, so two trees have the same digest exactly when they are the same
generated game code. The Builder (scripts/builder/profiles/bluewake.sh) compares it with the tree
verified on 2026-09-25 (docs/status/DEVICE_BUILD.md). It reads the private
generated files but prints only hashes.

usage: scripts/ios/composite_manifest.py COMPOSITE_DIR [--list]
"""
import hashlib
import os
import sys


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    root = sys.argv[1]
    entries = []
    for directory, _, files in os.walk(root):
        for name in files:
            path = os.path.join(directory, name)
            # "/" on every platform, so Windows computes the same digest.
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            with open(path, "rb") as stream:
                entries.append((rel, hashlib.sha256(stream.read()).hexdigest()))
    entries.sort()
    total = hashlib.sha256()
    for rel, digest in entries:
        total.update(("%s %s\n" % (digest, rel)).encode())
        if "--list" in sys.argv:
            print(digest, rel)
    print("%s  %d files" % (total.hexdigest(), len(entries)))


if __name__ == "__main__":
    main()

