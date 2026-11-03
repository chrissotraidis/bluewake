#!/usr/bin/env python3
"""Apply a Gecko code (the 00/02/04/06 write types) to a GameCube DOL.

A statically recompiled game cannot take Gecko writes to code at runtime: the
game runs from native code translated ahead of time, so an instruction written
into guest RAM is never executed. This splits a code into the writes that land
in the DOL's text sections (applied to a copy of main.dol, which is then
translated into variant chunks, see scripts/mods/build_mod_variant.py) and the
writes to data, which the host applies to guest RAM at boot and at every
retrace, as Dolphin re-applies Gecko codes every frame.

usage: gecko_apply.py CODE.gecko IN.dol OUT.dol OUT_RUNTIME.json
"""
import json
import sys


def parse_gecko(text):
    words = []
    for line in text.splitlines():
        line = line.split("#", 1)[0].strip()
        if not line or line.startswith("$") or line.startswith("["):
            continue
        a, b = line.split()
        words.append((int(a, 16), int(b, 16)))
    writes = []  # (address, bytes)
    i = 0
    while i < len(words):
        a, b = words[i]
        kind = (a >> 24) & 0xFE
        address = 0x80000000 | (a & 0x01FFFFFF)
        if kind == 0x00:
            count = (b >> 16) + 1
            for n in range(count):
                writes.append((address + n, bytes([b & 0xFF])))
        elif kind == 0x02:
            count = (b >> 16) + 1
            for n in range(count):
                writes.append((address + 2 * n, (b & 0xFFFF).to_bytes(2, "big")))
        elif kind == 0x04:
            writes.append((address, b.to_bytes(4, "big")))
        elif kind == 0x06:
            payload = bytearray()
            j = i + 1
            while len(payload) < b:
                payload += words[j][0].to_bytes(4, "big") + words[j][1].to_bytes(4, "big")
                j += 1
            writes.append((address, bytes(payload[:b])))
            i = j - 1
        else:
            raise SystemExit("unsupported Gecko line %08X %08X" % (a, b))
        i += 1
    return writes


def dol_sections(dol):
    be = lambda o: int.from_bytes(dol[o:o + 4], "big")
    out = []
    for n in range(18):
        off, addr, size = be(n * 4), be(0x48 + n * 4), be(0x90 + n * 4)
        if size:
            out.append((n < 7, addr, size, off))
    return out


def main():
    code, dol_in, dol_out, runtime_out = sys.argv[1:5]
    writes = parse_gecko(open(code).read())
    dol = bytearray(open(dol_in, "rb").read())
    sections = dol_sections(dol)
    text_writes, data_writes = [], []
    for address, data in writes:
        placed = False
        for is_text, base, size, off in sections:
            if base <= address and address + len(data) <= base + size:
                dol[off + address - base: off + address - base + len(data)] = data
                (text_writes if is_text else data_writes).append((address, data))
                placed = True
                break
        if not placed:
            data_writes.append((address, data))  # BSS or low memory: RAM only
    open(dol_out, "wb").write(dol)
    json.dump({"data": [[a, d.hex()] for a, d in data_writes],
               "text": [[a, d.hex()] for a, d in text_writes]},
              open(runtime_out, "w"), indent=0)
    print("text writes %d (%d bytes), data writes %d" % (
        len(text_writes), sum(len(d) for _, d in text_writes), len(data_writes)))


if __name__ == "__main__":
    main()
