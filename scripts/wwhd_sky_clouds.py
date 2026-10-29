#!/usr/bin/env python3
"""wwhd_sky_clouds.py SRC DST [DENSITY] [MAE_ALPHA] [NAKA]: the HD cloud layer textures for the host's one-sheet layers
(BLUEWAKE_HD_SKY). SRC holds the importer's hd-clouds (HD's ws_mae09 at mae09's name, ws_mae08 at mae08's, ws_naka06 at
naka06's); DST (optional/hd-clouds) gets: mae09 = HD ws_mae09's colour, mae08 = the alpha of HD's MAE_ALPHA (mae08 by
default: HD's sparser low layer; the GameCube's two mae layers share it), naka07 = HD ws_naka06's colour, naka06 = its
alpha; the alphas times DENSITY (default 1). Mips rebuilt by halving."""
import os, sys
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]; k = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
which = sys.argv[4] if len(sys.argv) > 4 else 'mae08'
naka_k = float(sys.argv[5]) if len(sys.argv) > 5 else 1.0  # the naka (cirrus) layer's alpha alone; HD's naka material's konst alpha is 0.5
MAE09='tex1_256x64_f1438df49f7c7b53_14'; MAE08='tex1_256x64_a2174d1e91c41074_0'
NAKA07='tex1_256x64_fb544c276a8c824b_14'; NAKA06='tex1_256x64_5587c506f01f8d4e_0'
def save(name, im):
    for f in os.listdir(dst):
        if f.startswith(name + '.') or f.startswith(name + '_mip'): os.remove(os.path.join(dst, f))
    im.save(os.path.join(dst, name + '.png')); lvl = 1; m = im
    while m.width > 1 or m.height > 1:
        m = m.resize((max(1, m.width // 2), max(1, m.height // 2)), Image.LANCZOS); m.save(os.path.join(dst, f'{name}_mip{lvl}.png')); lvl += 1
def colour(hd): r, g, b, a = hd.split(); return Image.merge('RGBA', [r, g, b, Image.new('L', hd.size, 255)])
def alpha(hd, f=1.0): a = hd.split()[3].point(lambda v: round(v * k * f)); return Image.merge('RGBA', [a, a, a, a])
mae = Image.open(os.path.join(src, MAE09 + '.png')).convert('RGBA'); naka = Image.open(os.path.join(src, NAKA06 + '.png')).convert('RGBA')
save(MAE09, colour(mae)); save(MAE08, alpha(Image.open(os.path.join(src, (MAE08 if which == 'mae08' else MAE09) + '.png')).convert('RGBA'))); save(NAKA07, colour(naka)); save(NAKA06, alpha(naka, naka_k))
print(f'density {k}, mae alpha from {which}, naka alpha x{naka_k}')
