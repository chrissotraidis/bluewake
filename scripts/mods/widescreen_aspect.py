#!/usr/bin/env python3
"""Derive a widescreen Gecko code for another aspect ratio from the 16:9 one.

The community 16:9 code (mods/widescreen/GZLE01.gecko, Dolphin's GZLE01.ini)
widens the game's camera and moves the HUD by changing a set of numbers: the
camera aspect constant, the 2D screen bounds, HUD positions in the meter and
map tuning structures, a few instruction immediates, and loads that pick other
float constants from the game's small-data pool. Everything else it does
(no-ops, redirected calls, the code it adds) is the same at any aspect.

Each of those numbers moves with the width the picture gains over 4:3, so for
a target aspect A it is interpolated between the game's own 4:3 value and the
16:9 value:  v(A) = v(4:3) + (v(16:9) - v(4:3)) * (A - 4/3) / (16/9 - 4/3).
The 4:3 values are the game's (read from main.dol for the constants, from the
decompiled tuning constructors for the runtime structures). A constant load can
only reach the existing pool around r2, so it takes the nearest constant there;
the choices for 16:10 are within about a pixel, and those for 21:9 (t = 2.25,
beyond the 16:9 code) within half a pixel. The Picto Box gallery's picture
viewport is computed instead (picture_viewport) and written into pool padding.

    widescreen_aspect.py 16:10 > mods/widescreen/GZLE01-16x10.gecko
    widescreen_aspect.py 21:9 > mods/widescreen/GZLE01-21x9.gecko
"""
import math
import struct
import sys

ASPECT_43 = 4.0 / 3.0
ASPECT_169 = 16.0 / 9.0

# Float constants in .sdata2 the 16:9 code rewrites: address -> 4:3 value.
FLOAT_WRITES = {
    0x803F7D68: -9.0,       # 2D screen left
    0x803F7D6C: 650.0,      # 2D screen right
    0x803F89B8: -9.0,       # 2D bounds (x, y, width, height)
    0x803F89BC: -21.0,
    0x803F89C0: 659.0,
    0x803F89C4: 524.0,
    0x803FA998: ASPECT_43,  # camera aspect (projection and view culling)
    0x803FB77C: 1.0296875,  # 2D width scale
    0x803FB78C: -9.0,       # 2D left
}
# s16 fields of the HUD tuning structures (bss, set by their constructors):
# g_meter_mapHIO (dMeter_map_HIO_c) and g_meterHIO (dMeter_HIO_c).
HALF_WRITES = {
    0x803E68E4: 35,     # map field_0x8: minimap left
    0x803E68E8: -180,   # map field_0xc
    0x803E68F0: 590,    # map field_0x14: free icon x
    0x803E6958: 7,      # meter field_0x50: HUD pane x offset
    0x803E69A4: 0,      # meter field_0x9c
}
# Values carried a different way than the interpolation, per aspect. field_0xc
# is the minimap's x when it is put away (left on the D-pad, or on the boat):
# interpolated to 21:9 (-351) it leaves the map's right side, about 130 wide
# with its arrow, on screen. It moves with the HUD shift past 16:9's instead
# (-256 - (257 - 114)), keeping 16:9's few units of clearance off the left edge.
HALF_OVERRIDES = {
    0x803E68E8: {"21:9": -399},
}
# Instruction immediates the 16:9 code sets: address -> 4:3 value.
LI_IMMEDIATES = {
    0x8004A428: 116,    # dMap_c::calcScissor width
    0x8004A444: 0,      # dMap_c::calcScissor x
    0x801F0B70: 7,      # dMeter_childPaneTransChildTrans: g_meterHIO.field_0x50
}
# The added dPlace_name_c code stores this float (lis r0, hi) at 0x800037E4.
LIS_FLOAT = (0x800037E4, -9.0)

R2 = 0x803FFD00
# Loads that the 16:9 code points at pool constants: the 16:9 constant's r2
# offset -> {aspect: r2 offset of the constant to use}. 114 is the HUD shift
# (0 at 4:3), 206 the rupee counter x (320 at 4:3), 1.3333 the boomerang
# sight's x scale (1 at 4:3).
# -130 and 130 are the x and width of the black bars the added code draws over
# the side margins (the telescope's mask): 130 covers 16:10 and 16:9, so only
# 21:9 (margins of 265.5 and 272.6) needs larger ones; the bars only overlap.
POOL_SWAPS = {
    -18772: {"21:9": -21120},  # -130.0 -> -288.0 (needs <= -265.5)
    -18768: {"21:9": -29036},  # 130.0  -> 292.5  (needs >= 288 and 272.6)
    -27188: {"16:10": -21536, "21:9": -19536},  # 114.0  -> 68.0, 257.0 (wants 68.4, 256.5)
    -28500: {"16:10": -19520, "21:9": -24296},  # 206.0  -> 250.5, 63.0 (wants 251.6, 63.5)
    -30556: {"16:10": -32212, "21:9": -32576},  # 1.3333 -> 1.2, 1.75 (wants 1.2, 1.75)
}
# The Picto Box gallery draws a photo in its own 2D projection, -9 to 650
# across (the C_MTXOrtho at 0x80227BB8), squeezed into a viewport whose x and
# width the 16:9 code loads (0x80227A18, 0x80227A20), while the photo's frame
# is drawn on the screen's 2D layout, which the widescreen code widens
# (0x803F7D68, 0x803F7D6C). The two match when the viewport spans -9 to 650 of
# the screen's layout (picture_viewport): x 82.0 and width 473.9 at 16:9, 139.3
# and 357.8 at 21:9. Nearby values (Dolphin's 79 and 480, the pool's 367 at
# 21:9) let the photos either side of the middle one drift out of their frames,
# so the 16:9 code loads the two from alignment padding in .sdata2 (a word
# between a single and the double after it, which no load in main.dol or any
# REL reads), and each variant writes its own values there.
PICTURE_SLOTS = (0x803F7D14, 0x803F7D34)  # x, width
PICTURE_ORTHO = (-9.0, 650.0)     # the gallery's projection for a picture, left and right
SCREEN_2D = ((-9.0, 650.0), (-123.0, 767.0))  # the 2D screen's left and right at 4:3 and 16:9


def picture_viewport(lerp):
    """The picture viewport's x and width that put -9 to 650 of the picture's
    projection on -9 to 650 of the screen's widened 2D layout."""
    left = lerp(SCREEN_2D[0][0], SCREEN_2D[1][0])
    right = lerp(SCREEN_2D[0][1], SCREEN_2D[1][1])
    scale = 640.0 / (right - left)
    return (PICTURE_ORTHO[0] - left) * scale, (PICTURE_ORTHO[1] - PICTURE_ORTHO[0]) * scale


ASPECTS = {"16:9": ASPECT_169, "16:10": 16.0 / 10.0, "21:9": 21.0 / 9.0}


def f32_bits(v):
    return struct.unpack(">I", struct.pack(">f", v))[0]


def bits_f32(b):
    return struct.unpack(">f", struct.pack(">I", b))[0]


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ASPECTS:
        sys.exit(f"usage: {sys.argv[0]} {'|'.join(ASPECTS)}")
    name = sys.argv[1]
    aspect = ASPECTS[name]
    t = (aspect - ASPECT_43) / (ASPECT_169 - ASPECT_43)
    lerp = lambda v43, v169: v43 + (v169 - v43) * t

    source = open(sys.path[0] + "/../../mods/widescreen/GZLE01.gecko").read().split("\n")
    out = [f"# {name} widescreen for GZLE01 (USA), derived by scripts/mods/widescreen_aspect.py from",
           "# GZLE01.gecko (the 16:9 code): aspect-dependent values interpolated, t = %.4f." % t]
    words = []  # (address, word) of the 06 blocks, patched then re-emitted
    lines = [l for l in source if l and not l.startswith("#")]
    i = 0
    while i < len(lines):
        a, b = (int(x, 16) for x in lines[i].split())
        kind, addr = a >> 24, 0x80000000 | (a & 0x01FFFFFF)
        if kind in (0x06, 0x07):  # write b bytes starting at addr
            n = b
            data = []
            for j in range(i + 1, i + 1 + (n + 7) // 8):
                data += [int(x, 16) for x in lines[j].split()]
            i += 1 + (n + 7) // 8
            data = [patch_word(addr + 4 * k, w, lerp, name) for k, w in enumerate(data[: n // 4])]
            out.append(f"{a:08X} {n:08X}")
            data += [0] * (len(data) % 2)
            out += [f"{data[k]:08X} {data[k + 1]:08X}" for k in range(0, len(data), 2)]
            continue
        if kind in (0x04, 0x05):
            b = patch_word(addr, b, lerp, name)
        elif kind in (0x02, 0x03) and addr in HALF_WRITES:
            v169 = (b & 0xFFFF) - (0x10000 if b & 0x8000 else 0)
            v = HALF_OVERRIDES.get(addr, {}).get(name, round(lerp(HALF_WRITES[addr], v169)))
            b = (b & 0xFFFF0000) | (v & 0xFFFF)
        out.append(f"{a:08X} {b:08X}")
        i += 1
    print("\n".join(out))


def patch_word(addr, w, lerp, name):
    if addr in PICTURE_SLOTS:
        return f32_bits(picture_viewport(lerp)[PICTURE_SLOTS.index(addr)])
    if addr in FLOAT_WRITES:
        if addr == 0x803FA998:
            return f32_bits(ASPECTS[name])
        return f32_bits(lerp(FLOAT_WRITES[addr], bits_f32(w)))
    if addr in LI_IMMEDIATES and w >> 16 in (0x3800, 0x3860, 0x3880):  # li r0/r3/r4
        v169 = (w & 0xFFFF) - (0x10000 if w & 0x8000 else 0)
        return (w & 0xFFFF0000) | (round(lerp(LI_IMMEDIATES[addr], v169)) & 0xFFFF)
    if addr == LIS_FLOAT[0] and w >> 16 == 0x3C00:  # lis r0, hi(float)
        v = lerp(LIS_FLOAT[1], bits_f32((w & 0xFFFF) << 16))
        return 0x3C000000 | ((f32_bits(v) + 0x8000) >> 16)
    op = w >> 26
    if op == 48 and (w >> 16) & 0x1F == 2:  # lfs fD, d(r2)
        d = w & 0xFFFF
        d = d - 0x10000 if d & 0x8000 else d
        if d in POOL_SWAPS:
            return (w & 0xFFFF0000) | (POOL_SWAPS[d].get(name, d) & 0xFFFF)
    return w


if __name__ == "__main__":
    main()
