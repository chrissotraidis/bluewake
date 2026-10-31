"""Wind Waker HD's sky and light palettes (each stage's Pale and Virt tables) for the HD look's BLUEWAKE_HD_SKY.

HD keeps each GameCube entry's layout and appends fields (Pale 0x6C, Virt 0x38); its colours are linear and go back to
the GameCube's gamma (1/2.2). Kept from HD: colours, Virt index, fog start and end, and the first six of its Pale
multipliers (actor C0, actor K0, BG0 C0, BG0 K0, BG1 C0, BG1 K0: the sun's light, K0, on actors and scenery is 2.75 and
3 times at noon, linear). Kept from the GameCube: the cloud colours' alpha (what HD's means isn't known).

One stage: 'BWSK' u32 version(2) u32 pale_count u32 virt_count, then the GameCube's Pale entries (0x2C each), HD's in
the GameCube's layout, the GameCube's Virt entries (0x24 each), HD's, and HD's multipliers (24 bytes a Pale entry).
Every stage (optional/hd-sky/stages.bin): 'BWSA' u32 count, then for each its name (16 bytes, zero-padded), u32
length and the one-stage data. All big-endian, as in the game's memory; the GameCube's copies let the host know the
stage in play and put its tables back.
"""
import struct

HD_SKY_FOLDER = "optional/hd-sky"
HD_SKY_FILE = "stages.bin"


def chunks(d: bytes) -> dict:
    n, = struct.unpack('>I', d[:4])
    return {d[4 + 12 * i:8 + 12 * i].decode('latin1'): struct.unpack('>II', d[8 + 12 * i:16 + 12 * i]) for i in range(n)}


def find_dzs(bfres: bytes):
    """Where HD's Stage.bfres keeps the stage's dzs (a chunk table naming Pale), or None."""
    p = bfres.find(b'Pale')
    while p >= 0:
        for s in range(p - 4, max(0, p - 12 * 200), -4):
            if (p - s - 4) % 12:
                continue
            n, = struct.unpack('>I', bfres[s:s + 4])
            if 0 < n < 200 and s + 4 + 12 * n > p and all(
                    all(32 <= x < 127 for x in bfres[s + 4 + 12 * i:s + 8 + 12 * i]) for i in range(n)):
                ch = chunks(bfres[s:])
                if 'Pale' in ch and all(o < 0x400000 for c, o in ch.values()):
                    return s
        p = bfres.find(b'Pale', p + 1)
    return None


def gamma(b: bytes) -> bytes:
    return bytes(round(((x / 255) ** (1 / 2.2)) * 255) for x in b)


def convert(gc: bytes, hd: bytes) -> bytes:
    """One stage's file from its GameCube dzs and HD's. A stage without a sky (an interior) has no Virt table."""
    gcc, hdc = chunks(gc), chunks(hd)
    (pn, po), (vn, vo) = gcc['Pale'], gcc.get('Virt', (0, 0))
    (hpn, hpo), (hvn, hvo) = hdc['Pale'], hdc.get('Virt', (0, 0))
    if (pn, vn) != (hpn, hvn):
        raise ValueError('HD and GC tables differ in length')
    gc_pale = gc[po:po + 0x2C * pn]
    gc_virt = gc[vo:vo + 0x24 * vn]
    hd_pale = b''
    for i in range(pn):
        e = hd[hpo + 0x6C * i:hpo + 0x6C * i + 0x2C]
        hd_pale += gamma(e[:0x21]) + e[0x21:0x2C]  # 11 colours, then Virt index, pad, fog start and end
    factors = b''.join(hd[hpo + 0x6C * i + 0x30:hpo + 0x6C * i + 0x48] for i in range(pn))
    hd_virt = b''
    for i in range(vn):
        e, g = hd[hvo + 0x38 * i:hvo + 0x38 * i + 0x24], gc_virt[0x24 * i:0x24 * (i + 1)]
        hd_virt += (e[:0x10] + gamma(e[0x10:0x13]) + g[0x13:0x14] + gamma(e[0x14:0x17]) + g[0x17:0x18]
                    + gamma(e[0x18:0x21]) + e[0x21:0x24])
    return b'BWSK' + struct.pack('>III', 2, pn, vn) + gc_pale + hd_pale + gc_virt + hd_virt + factors


def pack_all(stages: dict) -> tuple[bytes, list]:
    """stages.bin from {name: (gc_dzs, hd_dzs)}; also the names left out and why."""
    entries, count, skipped = b'', 0, []
    for name in sorted(stages):
        if len(name.encode()) > 15:
            skipped.append((name, 'name too long'))
            continue
        try:
            one = convert(*stages[name])
        except (KeyError, ValueError, struct.error) as e:
            skipped.append((name, str(e)))
            continue
        entries += name.encode().ljust(16, b'\0') + struct.pack('>I', len(one)) + one
        count += 1
    return b'BWSA' + struct.pack('>I', count) + entries, skipped
