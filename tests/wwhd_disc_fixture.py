#!/usr/bin/env python3
"""Generate toy WUD/WUX fixtures; requires only pycryptodome==3.23.0.

Usage: python tests/wwhd_disc_fixture.py [build/wwhd-disc-import/fixtures]
No disc dumps, private keys, downloads or game assets are used. All key values
below are deliberately public SYNTHETIC test strings. The existing Python
reader independently checks both positive images and every expected file.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from Crypto.Cipher import AES

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "scripts"))
from wwhd.disc import WiiUDisc  # noqa: E402
from wwhd.formats import FormatError  # noqa: E402

DISC = b"SYNTHETIC-DISC!!"
TITLE = b"SYNTHETIC-TITLE!"
COMMON = b"SYNTHETIC-COMMON"
WRONG = b"SYNTHETIC-WRONG!"
TITLE_ID = bytes.fromhex("0005000010143500")
SECTOR, BLOCK, HASH_DATA = 0x8000, 0x10000, 0xFC00
SI, SI_FST, SYSTEM = 0x20000, 0x28000, 0x30000
GAME, GAME_FST = 0x70000, 0x78000
RAW, CBC, HASHED = 0x80000, 0x90000, 0xA0000
TMD_AT, TICKET_AT = 0xFFF3, 0x1FFF7
SCALE = 3  # Non-unit scale, with byte offsets deliberately not AES aligned.

# path, byte offset, byte count, content index, flags. The expected bytes are
# sliced from plaintext before encryption, never from either parser's output.
FILES = [
    ("content/raw-head.bin", 0, 41, 1, 0),
    ("content/raw-cross.bin", 32763, 87, 1, 0x40),
    ("content/raw-empty.bin", 65535, 0, 1, 0),
    ("content/cbc-head.bin", 0, 49, 2, 0),
    ("content/cbc-unaligned.bin", 15, 77, 2, 0),
    ("content/cbc-cross.bin", 32763, 97, 2, 0),
    ("content/cbc-tail.bin", 65520, 15, 2, 0),
    ("content/cbc-empty.bin", 65535, 0, 2, 0),
    ("content/hashed-head.bin", 0, 19, 3, 0),
    ("content/hashed-cross.bin", HASH_DATA - 6, HASH_DATA + 23, 3, 0),
    ("content/nested/hashed-block1.bin", HASH_DATA + 21, 71, 3, 0),
    ("content/nested/hashed-tail.bin", 3 * HASH_DATA - 15, 15, 3, 0),
    ("content/nested/hashed-empty.bin", 3 * HASH_DATA, 0, 3, 0),
    ("outside.bin", 123, 15, 1, 0),
]


def put(data, at, fmt, *values):
    struct.pack_into(">" + fmt, data, at, *values)


def encrypt(data, key, iv=bytes(16)):
    data = bytes(data)
    return AES.new(key, AES.MODE_CBC, iv).encrypt(data + bytes(-len(data) % 16))


def pattern(size, seed):
    return bytes((seed + i * 29 + (i >> 8) * 7 + (i >> 16) * 13) & 255
                 for i in range(size))


def file_table(clusters, records, scale=1):
    """Encode a small explicit FST tree, not a reader or parser replica.

    records: (directory, name, offset/parent, size/end, flags, cluster).
    Offsets for files are already in on-disc scale units.
    """
    base = 32 + len(clusters) * 32
    table = bytearray(base + (len(records) + 1) * 16)
    table[:4] = b"FST\0"
    put(table, 4, "II", scale, len(clusters))
    for i, (start, sectors) in enumerate(clusters):
        put(table, 32 + i * 32, "II", start, sectors)
    put(table, base, "IIIHH", 0x01000000, 0, len(records) + 1, 0, 0)
    strings = bytearray(b"\0")
    for i, (directory, name, offset, size, flags, cluster) in enumerate(records, 1):
        put(table, base + i * 16, "IIIHH", (directory << 24) | len(strings),
            offset, size, flags, cluster)
        strings.extend(name.encode("ascii") + b"\0")
    return table + strings


def game_table():
    records = [(1, "content", 0, 17, 0, 0)]
    for path, at, size, cluster, flags in FILES:
        if path.endswith("hashed-block1.bin"):
            records.append((1, "nested", 1, 16, 0, 0))
        if path == "outside.bin":
            records.append((1, "empty", 1, 17, 0, 0))
        assert at % SCALE == 0
        records.append((0, path.rsplit("/", 1)[-1], at // SCALE, size, flags, cluster))
    assert len(records) == 17
    # A real disc counts a hashed content's allocation in payload sectors, so the
    # hash blocks stored with that payload are not in the count: three 0x10000
    # blocks are six sectors on disc but 3 x 0xFC00 payload bytes, five sectors.
    hashed_sectors = (3 * HASH_DATA) // SECTOR
    return file_table([(1, 1), (2, 2), (4, 2), (6, hashed_sectors)], records, SCALE)


def system_bytes(game_fst, bad_digest=False):
    tmd = bytearray(0xB04 + 4 * 48)
    tmd[0x18C:0x194] = TITLE_ID
    put(tmd, 0x1DE, "H", 4)
    for index, (typ, size) in enumerate(((1, len(game_fst)), (0, BLOCK),
                                       (1, BLOCK), (3, 3 * BLOCK))):
        put(tmd, 0xB04 + index * 48, "IHHQ", 0x100 + index, index, typ, size)
    digest = bytearray(hashlib.sha1(game_fst).digest())
    if bad_digest:
        digest[0] ^= 1
    tmd[0xB14:0xB28] = digest
    ticket = bytearray(0x2A4)
    ticket[0x1DC:0x1E4] = TITLE_ID
    ticket[0x1BF:0x1CF] = encrypt(TITLE, COMMON, TITLE_ID + bytes(8))
    system = bytearray(3 * BLOCK)
    system[TMD_AT:TMD_AT + len(tmd)] = tmd
    system[TICKET_AT:TICKET_AT + len(ticket)] = ticket
    si_fst = file_table([(1, 1), (2, 6)], [
        (1, "toy", 0, 4, 0, 0),
        (0, "title.tmd", TMD_AT, len(tmd), 0, 1),
        (0, "title.tik", TICKET_AT, len(ticket), 0, 1),
    ])
    return system, si_fst


def install_metadata(image, game_fst, bad_digest=False):
    system, si_fst = system_bytes(game_fst, bad_digest)
    for at, fst in ((SI, si_fst), (GAME, game_fst)):
        put(image, at, "II", 0xCC93A4F5, SECTOR)
        put(image, at + 20, "I", len(fst))
    image[SI_FST:SI_FST + len(encrypt(si_fst, DISC))] = encrypt(si_fst, DISC)
    image[GAME_FST:GAME_FST + len(encrypt(game_fst, TITLE))] = encrypt(game_fst, TITLE)
    for index in range(3):
        at = index * BLOCK
        image[SYSTEM + at:SYSTEM + at + BLOCK] = encrypt(
            system[at:at + BLOCK], DISC, bytes(8) + index.to_bytes(8, "big"))


def make_disc():
    plain = {1: pattern(BLOCK, 17), 2: pattern(BLOCK, 71),
             3: pattern(3 * HASH_DATA, 131)}
    image = bytearray(HASHED + 3 * BLOCK)
    identity = b"WUP-P-BCZE-SYNTHETIC\0"
    image[:len(identity)] = identity
    toc = bytearray(SECTOR)
    put(toc, 0, "I", 0xCCA6E67B)
    put(toc, 28, "I", 2)
    for i, (name, at) in enumerate(((b"SI", SI), (b"GM0005000010143500", GAME))):
        record = 0x800 + i * 0x80
        toc[record:record + len(name)] = name
        put(toc, record + 32, "I", at // SECTOR)
    image[0x18000:0x20000] = encrypt(toc, DISC)
    image[RAW:RAW + BLOCK] = plain[1]
    image[CBC:CBC + BLOCK] = encrypt(plain[2], TITLE, b"\0\2" + bytes(14))
    for index in range(3):
        data = plain[3][index * HASH_DATA:(index + 1) * HASH_DATA]
        digest = hashlib.sha1(data).digest()
        hashes = bytearray(pattern(0x400, 211 + index))
        hashes[index * 20:index * 20 + 20] = digest
        at = HASHED + index * BLOCK
        image[at:at + BLOCK] = encrypt(hashes, TITLE) + encrypt(data, TITLE, digest[:16])
    fst = game_table()
    install_metadata(image, fst)
    expected = {path: plain[cluster][at:at + size]
                for path, at, size, cluster, _ in FILES}
    return image, fst, expected


def mapped_wux(image):
    # Reverse physical order and deduplicate identical (mostly zero) sectors.
    # This cannot accidentally pass as an identity-mapped/raw image reader.
    sectors = [bytes(image[i:i + SECTOR]) for i in range(0, len(image), SECTOR)]
    physical = list(dict.fromkeys(reversed(sectors)))
    indices = {data: i for i, data in enumerate(physical)}
    table = [indices[data] for data in sectors]
    assert table != list(range(len(table))) and len(set(table)) < len(table)
    base = ((32 + len(table) * 4 + SECTOR - 1) // SECTOR) * SECTOR
    header = bytearray(base)
    struct.pack_into("<4sIIIQ", header, 0, b"WUX0", 0x1099D02E, SECTOR, 0, len(image))
    struct.pack_into(f"<{len(table)}I", header, 32, *table)
    return header + b"".join(physical)


def negative_images(image, fst, wux):
    # Refresh the TMD hash for structural faults, so checksum rejection cannot
    # hide a parser which accepts malformed directories, names or clusters.
    base = 32 + 4 * 32
    mutations = [
        ("fst-magic", 0, b"BAD!"),
        ("fst-count", base + 8, struct.pack(">I", 4096)),
        ("cluster", base + 2 * 16 + 14, struct.pack(">H", 4)),
        ("directory-backward", base + 16 + 8, struct.pack(">I", 1)),
        ("directory-past-end", base + 16 + 8, struct.pack(">I", 19)),
        ("directory-parent-end", base + 12 * 16 + 8, struct.pack(">I", 18)),
        ("name-offset", base + 2 * 16, struct.pack(">I", 0xFFFFFF)),
        ("path-traversal", fst.index(b"content\0"), b"../evil"),
        ("path-absolute", fst.index(b"content\0"), b"/ontent"),
        ("path-backslash", fst.index(b"content\0"), b"con\\ent"),
        ("name-unterminated", len(fst) - 1, b"X"),
        ("file-offset", base + 2 * 16 + 4, struct.pack(">I", 0xFFFFFFFF)),
        ("file-size", base + 2 * 16 + 8, struct.pack(">I", 0x10000001)),
        ("cluster-offset", 32 + 32, struct.pack(">I", 0xFFFFFFFF)),
    ]
    for name, at, value in mutations:
        changed = bytearray(fst)
        changed[at:at + len(value)] = value
        bad = bytearray(image)
        install_metadata(bad, changed)
        yield "bad-" + name + ".wud", bad
    deep = file_table([(1, 1)], [(1, "d", max(0, i - 1), 35, 0, 0)
                                       for i in range(1, 35)])
    bad = bytearray(image)
    install_metadata(bad, deep)
    yield "bad-directory-depth.wud", bad
    bad = bytearray(image)
    install_metadata(bad, fst, bad_digest=True)
    yield "bad-fst-checksum.wud", bad
    for name, at in (("hashed-data", HASHED + BLOCK + 0x400 + 33),
                     ("hashed-h0", HASHED), ("hashed-last", HASHED + 2 * BLOCK + 0x400 + 7),
                     ("identity", 10), ("toc", 0x18000), ("partition", GAME)):
        bad = bytearray(image)
        bad[at] ^= 1
        yield "bad-" + name + ".wud", bad
    for name, size in (("short-header", 12), ("short-toc", 0x18010),
                       ("short-system", SYSTEM + BLOCK + 16),
                       ("short-fst", GAME_FST + 16), ("short-content", len(image) - 1)):
        yield "bad-" + name + ".wud", image[:size]
    yield "bad-wux-short.wux", wux[:-1]
    bad = bytearray(wux)
    struct.pack_into("<I", bad, 32 + (RAW // SECTOR) * 4, 0xFFFFFFFF)
    yield "bad-wux-map.wux", bad


def check_positive(path, expected):
    for mode, title, common in (("title", TITLE, None), ("common", None, COMMON),
                                ("title-override", TITLE, WRONG)):
        disc = WiiUDisc(path, DISC, title_key=title, common_key=common)
        try:
            actual = [(e.path, e.offset, e.size, e.cluster, e.flags) for e in disc.entries]
            if actual != FILES:
                raise AssertionError(f"{path.name}/{mode}: file table differs: {actual!r}")
            for entry in reversed(disc.entries):
                if disc.read(entry) != expected[entry.path]:
                    raise AssertionError(f"{path.name}/{mode}: wrong bytes for {entry.path}")
        finally:
            disc.close()


def check_negative(path):
    # Use __new__ so even an exception during __init__ closes the reference
    # reader's stream, which otherwise stays open until garbage collection.
    disc = WiiUDisc.__new__(WiiUDisc)
    try:
        disc.__init__(path, DISC, title_key=TITLE)
        for entry in disc.entries:
            disc.read(entry)
    except FormatError as error:
        return str(error)
    finally:
        if hasattr(disc, "image"):
            disc.close()
    raise AssertionError(f"Python accepted negative fixture: {path.name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixtureDirectory", nargs="?", type=Path,
                        default=ROOT / "build/wwhd-disc-import/fixtures")
    output = parser.parse_args().fixtureDirectory.resolve()
    allowed = (ROOT / "build/wwhd-disc-import").resolve()
    if output != allowed and allowed not in output.parents:
        parser.error(f"fixtures must remain under {allowed}")
    output.mkdir(parents=True, exist_ok=True)
    marker = output / "selftest.txt"
    marker.unlink(missing_ok=True)  # A failed run must not retain a PASS marker.
    image, fst, expected = make_disc()
    wux = mapped_wux(image)
    (output / "valid.wud").write_bytes(image)
    (output / "mapped.wux").write_bytes(wux)
    for name, key in (("disc", DISC), ("title", TITLE), ("common", COMMON)):
        (output / f"SYNTHETIC-{name}.key").write_bytes(key)
    (output / "SYNTHETIC-title-hex.key").write_text(TITLE.hex() + "\n", encoding="ascii")
    (output / "expected").mkdir(exist_ok=True)
    for path, data in expected.items():
        (output / "expected" / Path(path).name).write_bytes(data)
    report = ["PASS: scripts/wwhd/disc.py", "Only generated toy data and SYNTHETIC keys.",
              "reference-sha256 " + hashlib.sha256((ROOT / "scripts/wwhd/disc.py").read_bytes()).hexdigest()]
    for name in ("valid.wud", "mapped.wux"):
        check_positive(output / name, expected)
        report.append(f"positive {name}: {len(FILES)} entries, title/common/override: PASS")
    negative_count = 0
    for name, data in negative_images(image, fst, wux):
        (output / name).write_bytes(data)
        reason = check_negative(output / name)
        report.append(f"negative {name}: {reason}")
        negative_count += 1
    marker.write_text("\n".join(report) + "\n", encoding="utf-8")
    print(f"Python reference PASS: 2 images x 3 key modes x {len(FILES)} entries; "
          f"{negative_count} negative fixtures")
    print(f"Fixtures: {output}")
    print(f"WUD {len(image)} bytes; mapped WUX {len(wux)} bytes")


if __name__ == "__main__":
    main()
