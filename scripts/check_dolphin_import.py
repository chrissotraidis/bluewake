#!/usr/bin/env python3
"""Check the in-app Dolphin save importer against ww_save.py, byte for byte.

Builds tests/dolphin_save_import_cli.c (with apple/ios/src/dolphin_save_import.c)
using clang, then for every .gci in SAVES and every BlueWake card given, puts
quest log 1 into each slot with both the C code and build/device-setup/ww_save.py
and compares the resulting cards. It also checks a raw Dolphin memory card
built here from each .gci (blocks scattered, backup directory older) gives the
same card as the .gci, and that a card with no saves yet gains a valid gczelda.

    scripts/check_dolphin_import.py [SAVES_DIR] [CARD...]

The saves come from https://github.com/cbartondock/Windwaker (clone it to /tmp).
"""
import glob, os, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
sys.path.insert(0, os.path.join(ROOT, 'build', 'device-setup'))
import card_container
import ww_save

BLOCK = 0x2000


def raw_card_from_gci(gci, path):
    """A 4 Mbit Dolphin raw card holding just this file, its blocks out of order."""
    entry, data = gci[:64], gci[64:]
    card = bytearray(b'\0' * (64 * BLOCK))
    order = [5 + 2 * i for i in range(12)][::-1]  # 27, 25, ... 5
    entry = bytearray(entry)
    struct.pack_into('>HH', entry, 0x36, order[0], 12)
    for dir_block, counter in ((1, 7), (2, 6)):  # block 2 is the older backup
        d = bytearray(b'\xff' * BLOCK)
        d[0:64] = entry if dir_block == 1 else b'\xff' * 64
        struct.pack_into('>H', d, 0x1FFA, counter)
        card[dir_block * BLOCK:(dir_block + 1) * BLOCK] = d
    for bat_block, counter in ((3, 1), (4, 2)):  # block 4 is current
        b = bytearray(BLOCK)
        struct.pack_into('>H', b, 4, counter)
        if bat_block == 4:
            for i, blk in enumerate(order):
                nxt = order[i + 1] if i + 1 < len(order) else 0xFFFF
                struct.pack_into('>H', b, 0x0A + (blk - 5) * 2, nxt)
        card[bat_block * BLOCK:(bat_block + 1) * BLOCK] = b
    for i, blk in enumerate(order):
        card[blk * BLOCK:(blk + 1) * BLOCK] = data[i * BLOCK:(i + 1) * BLOCK]
    open(path, 'wb').write(card)


def empty_card(path):
    header = b'DOLCARD1' + struct.pack('>IHHIQIII', 1, 4, 0, BLOCK, 0x1234, 0, 0, card_container.fnv1a_32(b''))
    open(path, 'wb').write(header)


def main():
    saves = sys.argv[1] if len(sys.argv) > 1 else '/tmp/ww-saves/Windwaker/Save States/USA_Saves'
    cards = sys.argv[2:] or [os.path.join(ROOT, 'build/device-setup/card-backup', n)
                             for n in ('GZLE01-with-slot2.card', 'GZLE01-20260926.card')]
    gcis = sorted(glob.glob(os.path.join(saves, '*.gci')))
    if not gcis:
        sys.exit(f'no .gci files in {saves}')
    work = tempfile.mkdtemp(prefix='bw-dolphin-import-')
    cli = os.path.join(work, 'cli')
    subprocess.run(['clang', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-o', cli, os.path.join(ROOT, 'tests/dolphin_save_import_cli.c'),
                    os.path.join(ROOT, 'apple/ios/src/dolphin_save_import.c')], check=True)

    def c_inject(card, src, s, d, out):
        subprocess.run([cli, 'inject', card, src, str(s), str(d), out], check=True, stdout=subprocess.DEVNULL)
        return open(out, 'rb').read()

    failures = checked = 0
    for card in cards:
        for gci in gcis:
            raw = os.path.join(work, 'dolphin.raw')
            raw_card_from_gci(open(gci, 'rb').read(), raw)
            for dst in (1, 2, 3):
                py_out, c_out, raw_out = (os.path.join(work, n) for n in ('py.card', 'c.card', 'raw.card'))
                with open(os.devnull, 'w') as quiet:
                    stdout, sys.stdout = sys.stdout, quiet
                    try:
                        ww_save.inject(card, gci, 1, dst, py_out)
                    finally:
                        sys.stdout = stdout
                want = open(py_out, 'rb').read()
                ok = c_inject(card, gci, 1, dst, c_out) == want and c_inject(card, raw, 1, dst, raw_out) == want
                checked += 1
                if not ok:
                    failures += 1
                    print(f'MISMATCH {os.path.basename(card)} + {os.path.basename(gci)} -> slot {dst}')
        # The quest log summary the app shows, against ww_save's reading.
        listing = subprocess.run([cli, 'show', card], check=True, capture_output=True, text=True).stdout
        data = ww_save.load_gczelda(card)
        for i in range(3):
            ok_, maxlife, rupee, _, _ = ww_save.slot_info(data, 0, i)
            line = f'checksum {"ok" if ok_ else "BAD"} hearts {maxlife / 4:.2f} rupees {rupee} '
            if line not in listing.splitlines()[i + 1]:
                failures += 1
                print(f'SHOW MISMATCH {card} slot {i + 1}: {listing.splitlines()[i + 1]!r}')

    # A card with no saves yet gains the whole Dolphin file as a valid record.
    blank = os.path.join(work, 'blank.card')
    empty_card(blank)
    out = os.path.join(work, 'fresh.card')
    c_inject(blank, gcis[0], 1, 1, out)
    fresh = card_container.load(out)
    gci = open(gcis[0], 'rb').read()
    f = fresh['files'][0] if fresh['files'] else None
    if not (fresh['body_hash_ok'] and f and f['hash_ok'] and f['name'] == 'gczelda' and f['game'] == 'GZLE01'
            and f['data'] == gci[64:] and f['time'] == struct.unpack_from('>I', gci, 0x28)[0]):
        failures += 1
        print('MISMATCH empty card import')
    checked += 1

    print(f'{checked} imports and {len(cards)} listings checked, {failures} failures ({work})')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
