#!/usr/bin/env python3
"""Check in-between frames (DOL_AURORA_FRAME_INTERP_DUMP) against the real frames around them.

Usage: frame_interp_report.py DUMP_DIR OUT_DIR (needs Pillow; ffmpeg for the video).

For each game frame k with dumps frame{k}-real.ppm and frame{k}-between.ppm
(the in-between frame shown before real k), and real k-1:
  d_prev  = mean |between - real(k-1)|
  d_next  = mean |between - real(k)|
  d_real  = mean |real(k) - real(k-1)|
An in-between frame of a moving scene differs from both neighbours by less
than they differ from each other. Writes PNGs of a few triplets and a
side-by-side video (left: 30 FPS as shown today, right: with in-between frames).
"""
import os
import re
import subprocess
import sys

from PIL import Image, ImageChops, ImageDraw, ImageStat

dump, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
frames = sorted({int(m.group(1)) for f in os.listdir(dump) if (m := re.match(r"frame(\d+)-real\.ppm", f))})


def load(k, kind):
    path = os.path.join(dump, f"frame{k:06}-{kind}.ppm")
    return Image.open(path).convert("RGB") if os.path.exists(path) else None


def mad(a, b):
    return sum(ImageStat.Stat(ImageChops.difference(a, b)).mean) / 3.0


rows = []
for k in frames:
    real, between, prev = load(k, "real"), load(k, "between"), load(k - 1, "real")
    if real is None or between is None or prev is None:
        continue
    rows.append((k, mad(between, prev), mad(between, real), mad(real, prev)))

moving = [r for r in rows if r[3] > 0.5]
between_ok = [r for r in moving if r[1] < r[3] and r[2] < r[3]]
print(f"triplets={len(rows)} moving={len(moving)} in-between={len(between_ok)}")
for k, dp, dn, dr in rows[:12]:
    print(f"  frame {k}: |B-R(k-1)|={dp:.2f} |B-R(k)|={dn:.2f} |R(k)-R(k-1)|={dr:.2f}")
if moving:
    ratio = sum((dp + dn) / dr for _, dp, dn, dr in moving) / len(moving)
    print(f"mean (|B-R(k-1)|+|B-R(k)|)/|R(k)-R(k-1)| over moving frames: {ratio:.2f} (1.0 = exactly between)")

# Triplet images of the three most moving frames.
for k, _, _, _ in sorted(moving, key=lambda r: -r[3])[:3]:
    prev, between, real = load(k - 1, "real"), load(k, "between"), load(k, "real")
    w, h = real.size
    scale = 640 / w
    tiles = [im.resize((640, int(h * scale))) for im in (prev, between, real)]
    sheet = Image.new("RGB", (640 * 3 + 20, tiles[0].height + 30), "black")
    draw = ImageDraw.Draw(sheet)
    for i, (tile, label) in enumerate(zip(tiles, (f"real {k-1}", f"in-between", f"real {k}"))):
        sheet.paste(tile, (i * 650, 30))
        draw.text((i * 650 + 8, 8), label, fill="white")
    sheet.save(os.path.join(out, f"triplet-{k}.png"))

# Side-by-side video: 30 FPS (each real frame twice) against 60 FPS.
seq = [k for k in frames if load(k, "between") is not None and load(k - 1, "real") is not None]
if seq:
    vdir = os.path.join(out, "video")
    os.makedirs(vdir, exist_ok=True)
    n = 0
    for k in seq:
        prev = load(k - 1, "real")
        # Slot 1: both show real k-1. Slot 2: 30 FPS still shows it, the
        # interpolated display shows the in-between frame toward real k.
        for left, right in ((prev, prev), (prev, load(k, "between"))):
            w, h = left.size
            tw = 640
            th = int(h * tw / w)
            sheet = Image.new("RGB", (tw * 2 + 10, th + 28), "black")
            sheet.paste(left.resize((tw, th)), (0, 28))
            sheet.paste(right.resize((tw, th)), (tw + 10, 28))
            d = ImageDraw.Draw(sheet)
            d.text((8, 8), "30 FPS (today)", fill="white")
            d.text((tw + 18, 8), "60 FPS (in-between frames)", fill="white")
            sheet.save(os.path.join(vdir, f"{n:05}.png"))
            n += 1
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "60", "-i", os.path.join(vdir, "%05d.png"),
                    "-pix_fmt", "yuv420p", "-vf", "scale=trunc(iw/2)*2:trunc(ih/2)*2",
                    os.path.join(out, "side-by-side-60fps.mp4")], check=False)
    print(f"video: {os.path.join(out, 'side-by-side-60fps.mp4')} ({n} frames)")
