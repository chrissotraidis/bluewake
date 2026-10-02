#!/usr/bin/env python3
"""Retag arm64 Mach-O objects, static archives and dylibs to another Apple platform.

Apple arm64 code is ABI-compatible across several Apple platforms; the linker
and dyld refuse to mix them because of the platform recorded in
LC_BUILD_VERSION. This rewrites that one field in place (no load commands are
added or resized). It is a development bridge for platform packages whose
symbols and APIs are already compatible; it does not make an incompatible
library safe to use.

usage: retag_macho_platform.py --platform iossim|ios|tvos|macos IN OUT
"""
import argparse
import shutil
import struct
import sys

PLATFORMS = {"macos": 1, "ios": 2, "tvos": 3, "iossim": 7}
MH_MAGIC_64 = 0xFEEDFACF
LC_BUILD_VERSION = 0x32
LC_VERSION_MIN_MACOSX = 0x24
LC_VERSION_MIN_IPHONEOS = 0x25


def retag_macho(buf, off, platform, minos):
    magic, = struct.unpack_from("<I", buf, off)
    if magic != MH_MAGIC_64:
        return 0
    ncmds, = struct.unpack_from("<I", buf, off + 16)
    p = off + 32
    changed = 0
    for _ in range(ncmds):
        cmd, size = struct.unpack_from("<II", buf, p)
        if cmd == LC_BUILD_VERSION:
            struct.pack_into("<I", buf, p + 8, platform)
            if minos is not None:
                struct.pack_into("<I", buf, p + 12, minos)
            changed += 1
        elif cmd in (LC_VERSION_MIN_MACOSX, LC_VERSION_MIN_IPHONEOS):
            # Same size as a zero-tool LC_BUILD_VERSION is not guaranteed; refuse.
            raise SystemExit(f"legacy version-min command at {p:#x}; not supported")
        p += size
    return changed


def retag_archive(buf, platform, minos):
    assert buf[:8] == b"!<arch>\n"
    p = 8
    total = 0
    while p + 60 <= len(buf):
        hdr = bytes(buf[p:p + 60])
        name = hdr[:16].decode().strip()
        size = int(hdr[48:58].decode().strip())
        data = p + 60
        body = data
        if name.startswith("#1/"):
            body += int(name[3:])
        total += retag_macho(buf, body, platform, minos)
        p = data + size + (size & 1)
    return total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--platform", required=True, choices=PLATFORMS)
    ap.add_argument("--minos", help="major.minor, e.g. 17.0")
    ap.add_argument("src")
    ap.add_argument("dst")
    a = ap.parse_args()
    minos = None
    if a.minos:
        major, minor = (int(x) for x in a.minos.split("."))
        minos = (major << 16) | (minor << 8)
    if a.src != a.dst:
        shutil.copyfile(a.src, a.dst)
    with open(a.dst, "r+b") as f:
        buf = bytearray(f.read())
        if buf[:8] == b"!<arch>\n":
            n = retag_archive(buf, PLATFORMS[a.platform], minos)
        else:
            n = retag_macho(buf, 0, PLATFORMS[a.platform], minos)
        if n == 0:
            sys.exit("no LC_BUILD_VERSION found")
        f.seek(0)
        f.write(buf)
    print(f"retagged {n} Mach-O images to {a.platform}")


if __name__ == "__main__":
    main()
