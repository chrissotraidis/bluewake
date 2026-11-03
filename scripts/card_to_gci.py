#!/usr/bin/env python3
"""Export the files of a GXRuntime card container as Dolphin .gci files.

    scripts/card_to_gci.py CARD OUTDIR

Each file becomes OUTDIR/<game><maker>-<name>.gci: the 64-byte GameCube
directory entry rebuilt from the container record, then the file's blocks.
Dolphin's GCI-folder memory card (Config: SlotA = 8, folder
User/GC/USA/Card A) loads them, which is how a BlueWake save becomes the same
save in a reference renderer.
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
import card_container  # noqa: E402

BLOCK = 8192


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    blob = open(sys.argv[1], "rb").read()
    header, records = card_container.HEADER_SIZE, card_container.RECORD_SIZE
    os.makedirs(sys.argv[2], exist_ok=True)
    pos = header
    first_block = 5
    while pos + records <= len(blob):
        rec = blob[pos:pos + records]
        length, mtime = struct.unpack(">II", rec[4:12])
        name = rec[12:44]
        game, maker = rec[44:48], rec[48:50]
        banner, perm = rec[50], rec[51]
        icon_addr, icon_fmt, icon_speed, comment = struct.unpack(">IHHI", rec[52:64])
        data = blob[pos + records:pos + records + length]
        pos += records + length
        blocks = (length + BLOCK - 1) // BLOCK
        entry = (game + maker + b"\xff" + bytes([banner]) + name +
                 struct.pack(">IIHHBBHHHI", mtime, icon_addr, icon_fmt, icon_speed,
                             perm, 0, first_block, blocks, 0xFFFF, comment))
        assert len(entry) == 64
        first_block += blocks
        label = name.split(b"\0")[0].decode("ascii", "replace")
        out = os.path.join(sys.argv[2], "%s%s-%s.gci" % (game.decode(), maker.decode(), label))
        with open(out, "wb") as handle:
            handle.write(entry + data.ljust(blocks * BLOCK, b"\0"))
        print(out, length, "bytes")


if __name__ == "__main__":
    main()
