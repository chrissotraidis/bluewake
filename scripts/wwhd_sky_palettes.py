#!/usr/bin/env python3
"""HD's sky and light palettes for one stage, in the GameCube's layout, for the host to put over the stage's own.

Reads the stage's dzs from the GameCube disc and from Wind Waker HD (both the player's own), and writes OUT:
  'BWSK' u32 version(2) u32 pale_count u32 virt_count
  GC Pale entries (0x2C each), HD Pale entries in GC layout, GC Virt entries (0x24 each), HD Virt entries in GC layout,
  then for each Pale entry HD's first six multipliers (f32: actor C0, actor K0, BG0 C0, BG0 K0, BG1 C0, BG1 K0;
  at noon the sun's light, K0, on actors and scenery is 2.75 and 3 times, linear).
All big-endian, as in the game's RAM. The GC copy lets the host find the stage's tables and put them back.

HD keeps each GC entry's layout and appends fields (Pale 0x6C, Virt 0x38); those extra fields are dropped. HD's colours
are linear and go back to the GameCube's gamma (1/2.2). Kept from HD: colours, Virt index, fog start/end. Kept from GC:
the cloud colours' alpha (HD changed it; what HD's alpha means isn't known).
The native importer doesn't write this file yet; this is how it is made until it does, from the two stage
files (GC: res/Stage/sea/Stage.arc dzs/stage.dzs; HD: content/Common/Pack/first_szs_permanent.pack, sea_Stage.szs,
Stage.bfres's embedded dzs). OUT goes in the HD pack as optional/hd-sky/sea.bin (BLUEWAKE_HD_SKY).
Usage: wwhd_sky_palettes.py GC_DZS HD_DZS OUT"""
import struct, sys

gc_path, hd_path, out = sys.argv[1:4]


def chunks(d):
    n, = struct.unpack('>I', d[:4])
    return {d[4 + 12 * i:8 + 12 * i].decode('latin1'): struct.unpack('>II', d[8 + 12 * i:16 + 12 * i]) for i in range(n)}


def gamma(b):
    return bytes(round(((x / 255) ** (1 / 2.2)) * 255) for x in b)


gc, hd = open(gc_path, 'rb').read(), open(hd_path, 'rb').read()
gcc, hdc = chunks(gc), chunks(hd)
(pn, po), (vn, vo) = gcc['Pale'], gcc['Virt']
(hpn, hpo), (hvn, hvo) = hdc['Pale'], hdc['Virt']
assert (pn, vn) == (hpn, hvn), 'HD and GC tables differ in length'

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
with open(out, 'wb') as f:
    f.write(b'BWSK' + struct.pack('>III', 2, pn, vn) + gc_pale + hd_pale + gc_virt + hd_virt + factors)
print(f'{pn} Pale, {vn} Virt -> {out}')
