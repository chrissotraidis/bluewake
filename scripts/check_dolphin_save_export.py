#!/usr/bin/env python3
"""Check the on-demand Dolphin export: BlueWake's card out to a .gci.

Builds tests/dolphin_save_import_cli.c (with apple/ios/src/dolphin_save_import.c)
and, for every .gci in SAVES and every BlueWake card given:

  * imports the .gci into an empty card, exports it again, and compares the
    result with the original: the 98,304 bytes of save data and every header
    field must match, except the two bytes that only record where a file sits
    on a card (0x35 copy counter, 0x36 first block), which Dolphin reassigns
    when it imports a .gci;
  * exports that card again through a second empty card and requires the two
    exports to be identical, so a card and a .gci cannot drift apart;
  * checks the card file is untouched by an export (its hash never changes);
  * exports a card the game wrote and checks the file is a whole gczelda whose
    quest logs the importer reads back, with the card's own bytes in it;
  * checks an empty card is refused ("no save to export yet") and a card whose
    stored hash does not match its data is refused rather than exported.

    scripts/check_dolphin_save_export.py [SAVES_DIR] [CARD...]

The .gci files come from https://github.com/cbartondock/Windwaker (clone it to
/tmp). Pass cards made in BlueWake (a GZLE01.card or a benchmarks card) to check
the game-written side too. Set CC to build the CLI with something other than
clang.
"""
import glob, hashlib, os, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import card_container

BLOCK = 0x2000
ENTRY = 64
HASH_OFFSET = 64


def empty_card(path):
    header = b'DOLCARD1' + struct.pack('>IHHIQIII', 1, 4, 0, BLOCK, 0x1234, 0, 0,
                                       card_container.fnv1a_32(b''))
    open(path, 'wb').write(header)


def executable_dir():
    """Some /tmp mounts are noexec, so the built CLI cannot live in the data dir."""
    for candidate in (os.environ.get('BW_TEST_BIN_DIR'), os.path.expanduser('~')):
        if candidate and os.path.isdir(candidate) and os.access(candidate, os.W_OK):
            return candidate
    return os.getcwd()


def digest(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def main():
    saves = sys.argv[1] if len(sys.argv) > 1 else '/tmp/ww-saves/Windwaker/Save States/USA_Saves'
    cards = sys.argv[2:]
    gcis = sorted(glob.glob(os.path.join(saves, '*.gci')))
    if not gcis:
        sys.exit(f'no .gci files in {saves}')
    for card in cards:
        if not os.path.isfile(card):
            sys.exit(f'no such card: {card}')

    work = tempfile.mkdtemp(prefix='bw-dolphin-export-')
    cli = os.path.join(tempfile.mkdtemp(prefix='bw-dolphin-export-bin-', dir=executable_dir()), 'cli')
    subprocess.run([os.environ.get('CC', 'clang'), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-o', cli,
                    os.path.join(ROOT, 'tests/dolphin_save_import_cli.c'),
                    os.path.join(ROOT, 'apple/ios/src/dolphin_save_import.c')], check=True)

    def run(args):
        return subprocess.run([cli] + args, capture_output=True, text=True)

    def new_card(name):
        path = os.path.join(work, name)
        empty_card(path)
        return path

    def import_into(gci, name):
        """A fresh empty card holding the .gci's whole file, as the importer adds it."""
        target = new_card(name)
        result = run(['inject', target, gci, '1', '1', target])
        if result.returncode != 0:
            raise SystemExit(f'inject {gci} into {card}: {result.stderr.strip()}')
        return target

    failures = checked = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH {what}')

    # Dolphin keeps the file and the header in one .gci, so an import followed by
    # an export has to come back to the same thing.
    for gci in gcis:
        want = open(gci, 'rb').read()
        card = import_into(gci, 'roundtrip.card')
        out = os.path.join(work, 'roundtrip.gci')
        result = run(['export', card, out])
        got = open(out, 'rb').read() if result.returncode == 0 else b''
        if result.returncode != 0:
            check(False, f'{os.path.basename(gci)} export: {result.stderr.strip()}')
            continue
        checked += 1
        check(len(got) == len(want), f'{os.path.basename(gci)} length {len(got)} != {len(want)}')
        if len(got) != len(want):
            continue
        check(got[ENTRY:] == want[ENTRY:], f'{os.path.basename(gci)} save data')
        # Only the two card-layout bytes may differ: the copy counter and the
        # first block, which Dolphin reassigns when it imports a .gci.
        allowed = set(range(0x35, 0x38))  # copy counter, first block
        differing = [i for i in range(ENTRY) if got[i] != want[i] and i not in allowed]
        check(not differing, f'{os.path.basename(gci)} header bytes {[hex(i) for i in differing]}')

        # A second trip through a card must not change anything at all.
        again = import_into(out, 'again.card')
        out2 = os.path.join(work, 'again.gci')
        result = run(['export', again, out2])
        checked += 1
        check(result.returncode == 0 and open(out2, 'rb').read() == got,
              f'{os.path.basename(gci)} export is not a fixed point')

    # A card the game wrote: the export is the whole file, its quest logs read
    # back, and exporting leaves the card exactly as it was.
    for card in cards:
        before = digest(card)
        out = os.path.join(work, os.path.basename(card) + '.gci')
        result = run(['export', card, out])
        checked += 1
        if result.returncode != 0:
            check(False, f'{card} export: {result.stderr.strip()}')
            continue
        got = open(out, 'rb').read()
        check(len(got) == ENTRY + 12 * BLOCK, f'{card} length {len(got)}')
        check(struct.unpack_from('>H', got, 0x38)[0] == 12, f'{card} block count')
        check(got[0x08:0x0F] == b'gczelda', f'{card} file name {got[0x08:0x0F]!r}')
        check(got[0x00:0x06] == b'GZLE01', f'{card} game code {got[0x00:0x06]!r}')
        check(got[0x06] == 0xFF and struct.unpack_from('>H', got, 0x3A)[0] == 0xFFFF,
              f'{card} reserved header bytes')
        loaded = card_container.load(card)
        record = loaded['files'][0] if loaded['files'] else None
        check(record is not None and record['name'] == 'gczelda' and record['hash_ok'],
              f'{card} record')
        if record is not None:
            check(got[ENTRY:] == record['data'], f'{card} save data')
            check(struct.unpack_from('>I', got, 0x28)[0] == record['time'], f'{card} time')
        shown = run(['show', out])
        check(shown.returncode == 0, f'{card} exported file does not parse: {shown.stderr.strip()}')
        check(digest(card) == before, f'{card} was changed by the export')

    # Refusals: nothing to export, and a save whose stored hash is wrong.
    blank = new_card('blank.card')
    result = run(['export', blank, os.path.join(work, 'blank.gci')])
    checked += 1
    check(result.returncode != 0 and 'no save to export' in result.stderr,
          f'empty card export said {result.stderr.strip()!r}')

    damaged = import_into(gcis[0], 'damaged.card')
    blob = bytearray(open(damaged, 'rb').read())
    blob[-1] ^= 0xFF  # the last byte of the save data, so the record's own hash no longer matches
    # Keep the container's own body hash right, so this is only the record's hash.
    struct.pack_into('>I', blob, 36, card_container.fnv1a_32(bytes(blob[40:])))
    open(damaged, 'wb').write(blob)
    result = run(['export', damaged, os.path.join(work, 'damaged.gci')])
    checked += 1
    check(result.returncode != 0 and 'damaged' in result.stderr,
          f'damaged card export said {result.stderr.strip()!r}')

    print(f'{checked} exports checked, {failures} failures ({work})')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
