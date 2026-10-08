#!/usr/bin/env python3
"""Copy a BlueWake memory card with one quest log's restart place changed.

    scripts/card_set_restart.py IN.card OUT.card STAGE ROOM POINT [SLOT]

The same change as scripts/save_set_restart.py, made directly on the card
BlueWake writes (GZLE01.card in the saves folder), so a test save can be made
from a player's own card without Dolphin or a .gci. For example

    scripts/card_set_restart.py GZLE01.card test/GZLE01.card M_NewD2 0 0

starts quest log 1 in Dragon Roost Cavern, where the dungeon map (#74) can be
opened. IN is not modified; use the copy only in a portable or scratch folder.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import card_container  # noqa: E402
from save_set_restart import BLOCK, set_restart  # noqa: E402


def main():
    if len(sys.argv) not in (6, 7):
        sys.exit(__doc__)
    src, out, stage = sys.argv[1], sys.argv[2], sys.argv[3].encode('ascii')
    room, point = int(sys.argv[4], 0), int(sys.argv[5], 0)
    slot = int(sys.argv[6]) if len(sys.argv) == 7 else 1
    if os.path.abspath(src) == os.path.abspath(out):
        sys.exit('write to a new file, not over IN')
    card = card_container.load(src)
    if not card['body_hash_ok']:
        sys.exit('%s is damaged (body hash mismatch)' % src)
    blob = bytearray(open(src, 'rb').read())
    cursor = card_container.HEADER_SIZE
    for entry in card['files']:
        start = cursor + card_container.RECORD_SIZE
        if entry['name'] == 'gczelda' and entry['game'] == 'GZLE01':
            if entry['length'] != 12 * BLOCK or not entry['hash_ok']:
                sys.exit('the Wind Waker save in %s is not a 12-block save with a good hash' % src)
            data = bytearray(entry['data'])
            set_restart(data, stage, room, point, slot, base=0)
            blob[start:start + entry['length']] = data
            struct.pack_into('>I', blob, cursor + 64, card_container.fnv1a_32(data))
            body = blob[card_container.HEADER_SIZE:card_container.HEADER_SIZE + card['body_size']]
            struct.pack_into('>I', blob, 36, card_container.fnv1a_32(body))
            os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
            open(out, 'wb').write(blob)
            print('wrote %s: quest log %d restarts at %s room %d point %d' % (out, slot, stage.decode(), room, point))
            return
        cursor = start + entry['length']
    sys.exit('%s has no Wind Waker save (gczelda)' % src)


if __name__ == '__main__':
    main()
