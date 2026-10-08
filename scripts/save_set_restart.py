#!/usr/bin/env python3
"""Copy a Wind Waker Dolphin save (.gci) with one quest log's restart place changed.

    scripts/save_set_restart.py IN.gci OUT.gci STAGE ROOM POINT [SLOT]

Loading that quest log then starts Link at STAGE (for example M_NewD2, Dragon
Roost Cavern), room ROOM, spawn point POINT, which reproduces scenes such as the
dungeon map (#74) without playing there. SLOT is the quest log, 1 to 3 (default
1). Both copies of the quest log are changed and their checksums recomputed, as
apple/ios/src/dolphin_save_import.c reads them; IN is not modified. Use a copy of
a save, and a scratch card (tests/dolphin_save_import_cli.c inject) to test with.
"""
import struct
import sys

BLOCK, QUEST, QUEST_DATA, HEADER = 0x2000, 0x770, 0x768, 64
RESTART = 0x30  # dSv_player_return_place_c: name[8], room, point, 2 more bytes


def quest_checksum(quest):
    total = sum(quest[:QUEST_DATA]) & 0xFFFFFFFF
    inverse = sum(~b & 0xFFFFFFFF for b in quest[:QUEST_DATA]) & 0xFFFFFFFF
    return total << 32 | inverse


def block_checksum(block):
    total = inverse = 0
    for i in range(0, BLOCK - 4, 2):
        word = block[i] << 8 | block[i + 1]
        total = (total + word) & 0xFFFF
        inverse = (inverse + (~word & 0xFFFF)) & 0xFFFF
    return total << 16 | inverse


def set_restart(data, stage, room, point, slot, base=HEADER):
    """Change quest log SLOT's restart place in DATA, the save's blocks starting at BASE."""
    if not 1 <= len(stage) <= 7 or not 1 <= slot <= 3 or not -128 <= room <= 127 or not 0 <= point <= 255:
        sys.exit('STAGE is 1-7 characters, ROOM -128..127, POINT 0..255, SLOT 1..3')
    for copy in (0, 1):
        block = base + BLOCK * (1 + copy)
        quest = block + 8 + (slot - 1) * QUEST
        if struct.unpack('>Q', data[quest + QUEST_DATA:quest + QUEST_DATA + 8])[0] != quest_checksum(data[quest:quest + QUEST]):
            sys.exit('quest log %d, copy %d, has a bad checksum' % (slot, copy + 1))
        data[quest + RESTART:quest + RESTART + 8] = stage.ljust(8, b'\0')
        data[quest + RESTART + 8] = room & 0xFF
        data[quest + RESTART + 9] = point
        data[quest + QUEST_DATA:quest + QUEST_DATA + 8] = struct.pack('>Q', quest_checksum(data[quest:quest + QUEST]))
        data[block + BLOCK - 4:block + BLOCK] = struct.pack('>I', block_checksum(data[block:block + BLOCK]))


def main():
    if len(sys.argv) not in (6, 7):
        sys.exit(__doc__)
    src, out, stage = sys.argv[1], sys.argv[2], sys.argv[3].encode('ascii')
    room, point = int(sys.argv[4], 0), int(sys.argv[5], 0)
    slot = int(sys.argv[6]) if len(sys.argv) == 7 else 1
    data = bytearray(open(src, 'rb').read())
    if len(data) != HEADER + 12 * BLOCK or data[0:4] != b'GZLE':
        sys.exit('%s is not a Wind Waker (GZLE) .gci' % src)
    set_restart(data, stage, room, point, slot)
    open(out, 'wb').write(data)
    print('wrote %s: quest log %d restarts at %s room %d point %d' % (out, slot, stage.decode(), room, point))


if __name__ == '__main__':
    main()
