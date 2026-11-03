#!/usr/bin/env python3
"""Read the portable memory-card container the runtime writes.

The runtime's slot-A card is not a raw 4-Mbit image: it is the GXRuntime
container (see ref/recompcore/GXRuntime/src/memory_card.c), a 40-byte header
followed by one 68-byte record per file and then that file's bytes. The empty
file the card manager creates when a new game starts at file-select and the
gameplay save the pause menu writes differ only in what those bytes are, so
this is the instrument that tells them apart without trusting the guest to say
so. Both P4 milestone 9 and the P5 save/load campaign need it.

    scripts/card_container.py CARD            # one-line-per-file summary
    scripts/card_container.py A CARD          # what changed from A to CARD

The container's own hash of each file's bytes is recomputed here, so a
container whose record hash does not match its payload is reported rather than
silently summarised.
"""

import hashlib
import struct
import sys

MAGIC = b"DOLCARD1"
HEADER_SIZE = 40
RECORD_SIZE = 68


def fnv1a_32(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def load(path):
    with open(path, "rb") as handle:
        blob = handle.read()
    if len(blob) < HEADER_SIZE or blob[:8] != MAGIC:
        raise SystemExit(f"{path}: not a GXRuntime card container")
    version, size_mbits, encoding, sector_size = struct.unpack_from(
        ">IHHI", blob, 8
    )
    serial = struct.unpack_from(">Q", blob, 20)[0]
    file_count, body_size = struct.unpack_from(">II", blob, 28)
    body_hash = struct.unpack_from(">I", blob, 36)[0]
    body = blob[HEADER_SIZE : HEADER_SIZE + body_size]
    files = []
    cursor = 0
    for _ in range(file_count):
        index = struct.unpack_from(">H", body, cursor)[0]
        length, time = struct.unpack_from(">II", body, cursor + 4)
        name = body[cursor + 12 : cursor + 44].split(b"\0")[0].decode(
            "ascii", "replace"
        )
        game_code = body[cursor + 44 : cursor + 48].decode("ascii", "replace")
        company = body[cursor + 48 : cursor + 50].decode("ascii", "replace")
        stored_hash = struct.unpack_from(">I", body, cursor + 64)[0]
        data = body[cursor + RECORD_SIZE : cursor + RECORD_SIZE + length]
        files.append(
            {
                "index": index,
                "name": name,
                "game": game_code + company,
                "length": length,
                "blocks": (length + sector_size - 1) // sector_size,
                "time": time,
                "hash": stored_hash,
                "data": data,
                "data_sha256": hashlib.sha256(data).hexdigest(),
                "hash_ok": stored_hash == fnv1a_32(data),
            }
        )
        cursor += RECORD_SIZE + length
    return {
        "path": path,
        "version": version,
        "size_mbits": size_mbits,
        "encoding": encoding,
        "sector_size": sector_size,
        "serial": serial,
        "files": files,
        "body_size": body_size,
        "body_hash_ok": body_hash == fnv1a_32(body),
        "sha256": hashlib.sha256(blob).hexdigest(),
    }


def describe(card):
    print(
        f"{card['path']}: {card['size_mbits']} Mbit, {card['sector_size']}-byte "
        f"sectors, serial 0x{card['serial']:016X}, sha256 {card['sha256'][:16]}..."
    )
    if not card["body_hash_ok"]:
        print("  body hash MISMATCH: the container is damaged")
    if not card["files"]:
        print("  no files")
    for entry in card["files"]:
        print(
            f"  #{entry['index']} {entry['name']!r} {entry['game']} "
            f"{entry['length']} bytes ({entry['blocks']} blocks) "
            f"time=0x{entry['time']:08X} "
            f"data-sha256 {entry['data_sha256'][:16]}..."
            + ("" if entry["hash_ok"] else " HASH MISMATCH")
        )


def main(argv):
    if len(argv) == 2:
        describe(load(argv[1]))
        return 0
    if len(argv) == 3:
        before = load(argv[1])
        after = load(argv[2])
        describe(before)
        print("-")
        describe(after)
        before_names = {entry["name"]: entry for entry in before["files"]}
        after_names = {entry["name"]: entry for entry in after["files"]}
        for name in sorted(set(before_names) | set(after_names)):
            old = before_names.get(name)
            new = after_names.get(name)
            if old is None:
                print(f"+ {name!r} added")
            elif new is None:
                print(f"- {name!r} removed")
            elif old["data_sha256"] != new["data_sha256"]:
                print(
                    f"~ {name!r} data changed "
                    f"({old['length']} -> {new['length']} bytes)"
                )
                changed = sum(
                    1
                    for a, b in zip(old["data"], new["data"])
                    if a != b
                )
                span = min(len(old["data"]), len(new["data"]))
                print(f"  {changed} of {span} bytes differ")
            else:
                print(f"= {name!r} identical")
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
