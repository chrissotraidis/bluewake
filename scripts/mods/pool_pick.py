#!/usr/bin/env python3
"""Pick the pool constants a new widescreen aspect needs, from the game's own DOL.

r2 is the game's small-data base; it is derived here from the five displacements
the 16:9 widescreen code depends on (-130.0, 130.0, 114.0, 206.0, 1.3333), then
every aspect-dependent constant is picked as the nearest float that a
`lfs fD, d(r2)` can actually reach (a signed 16-bit displacement), with the
cover bars taking the nearest one that reaches past the margin.

The rule for each value is in widescreen_aspect.py's comment: the HUD shift,
the rupee counter's x and the boomerang sight's x scale are interpolated, the
two black bars the added code draws over the side margins (the telescope's
mask) have to cover the margin rather than match it, and the minimap's x when
it is put away is written directly (HALF_OVERRIDES) instead of loaded.

    pool_pick.py <prepared main.dol>          # not one with a mod already applied

Its output is what widescreen_aspect.py's POOL_SWAPS entries were chosen from:
for each row, the `at` column is the offset to put in that aspect's entry. Two
rows are informational only: the shipped 21:9 code uses wider cover bars than
the nearest ones (a wider bar only overlaps more, and it is the value that has
been in release since 0.8.0, so it is not regenerated), and the minimap's x
when it is put away is written straight into the tuning structure by
HALF_OVERRIDES, so it takes the interpolated value and not a pool constant.
"""
import math
import struct
import sys

PATTERN = [(-18772, -130.0), (-18768, 130.0), (-27188, 114.0), (-28500, 206.0), (-30556, 1.3333)]
A43, A169 = 4 / 3, 16 / 9
ASPECTS = {
    "21:9 (existing)": 21 / 9,
    "64:27 (2560x1080)": 2560 / 1080,
    "43:18 (3440x1440)": 3440 / 1440,
}


def load_words(path):
    """Every word of the DOL's data sections, by address."""
    dol = open(path, "rb").read()
    words = {}
    for i in range(18):
        off, addr, size = struct.unpack_from(">III", dol, i * 12)
        if not size:
            continue
        size = min(size, len(dol) - off) & ~3
        for k in range(0, size, 4):
            (value,) = struct.unpack_from(">f", dol, off + k)
            words[addr + k] = value
    return words


def find_r2(words):
    """The address r2 holds: the one that puts the known constants where the
    16:9 code says they are."""
    for address, value in words.items():
        if value == -130.0 and words.get(address + 4) == 130.0:
            r2 = address + 18772
            # Guest RAM only: a data word with the same pair elsewhere in the
            # file is not the pool. (1.3333 is left out of the check: it is not
            # exactly representable, so this compares floats that were read.)
            if r2 >= 0x80000000 and all(words.get(r2 + d) == v for d, v in PATTERN[:4]):
                return r2
    raise SystemExit("r2 not found: is this the game's own main.dol?")


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} <prepared main.dol>")
    words = load_words(sys.argv[1])
    r2 = find_r2(words)
    print(f"r2 = 0x{r2:08X}; reachable window 0x{r2 - 32768:08X} .. 0x{r2 + 32767:08X}")
    reachable = [(address - r2, value) for address, value in words.items()
                 if -32768 <= address - r2 <= 32767 and math.isfinite(value)]
    print(f"{len(reachable)} reachable words\n")

    def pick(target, cover=None):
        best = None
        for off, value in reachable:
            if cover == "left" and value > target:
                continue
            if cover == "right" and value < target:
                continue
            if cover is None and (value < 0) != (target < 0):
                continue
            delta = abs(value - target)
            if best is None or delta < best[0]:
                best = (delta, off, value)
        return best

    for name, aspect in ASPECTS.items():
        t = (aspect - A43) / (A169 - A43)
        print(f"== {name}: t = {t:.5f}")
        for label, want, cover in [
            ("left cover bar", -(9 + 114 * t), "left"),
            ("right cover bar", 10 + 117 * t, "right"),
            ("hud shift", 114 * t, None),
            ("rupee counter x", 320 - 114 * t, None),
            ("boomerang x scale", 1 + t / 3, None),
            ("minimap x when away", -(142 + 114 * t), None),
        ]:
            hit = pick(want, cover)
            if hit is None:
                print(f"   {label:20s} want {want:10.4f}  NO ENTRY")
                continue
            delta, off, value = hit
            print(f"   {label:20s} want {want:10.4f} -> {value:10.4f} at {off:7d} (drift {delta:.3f})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
