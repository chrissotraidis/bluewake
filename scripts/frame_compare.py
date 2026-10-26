#!/usr/bin/env python3
"""Find flicker and differences in rendered frames (scripts/rendered_run.py's captures and dumps).

  python3 scripts/frame_compare.py blinks DIR [--top 10] [--sheet blinks.png]
  python3 scripts/frame_compare.py ratio WITH WITHOUT [--mask DEBUG_DIR] [--frames 20-40]
  python3 scripts/frame_compare.py track DIR [--frames 20-40]
  python3 scripts/frame_compare.py diff A B
  python3 scripts/frame_compare.py sheet DIR [DIR ...] --frames 26,28,30 --out sheet.png [--crop ...]

DIR is a run folder (its frame-RETRACE.ppm captures) or its dump/ (frameNNNNNN-real.ppm and
-between.ppm, -between2.ppm, ...). On screen, frame N's in-between images come before real N: they run
from N-1 toward N. Every command takes them in that order.

blinks   images that differ from both neighbours the same way (by --threshold of 255) while the
         neighbours match each other (within --match): flicker, which motion doesn't do. Prints the
         count per image's worst, and --sheet draws the top ones with the blinks in red.
ratio    two runs from the same save state that differ in one effect (sun shadows on and off): the
         median of WITH / WITHOUT per real frame. With --mask, a debug run from the same state whose
         frames are black where the effect is full and white where it is absent
         (DOL_AURORA_SUN_SHADOW_MAP_DEBUG=1 with DOL_AURORA_AO_DEBUG=1 and BLUEWAKE_HD_AO=0), the
         ratio is taken in each part apart: one sun shadow is 1 - its strength (0.65); less means it was
         applied twice.
track    the mean brightness of each image: a pulse at the game's 30 FPS shows at a glance.
diff     two runs image by image: the mean difference and the pixels that changed (by more than 8).
sheet    the chosen frames of one or more runs side by side, labelled.

--region x0,y0,x1,y1 (fractions of the image, default the whole image) limits what is measured, to
leave the HUD out. Needs numpy and Pillow (pip install numpy pillow).
"""
import argparse
import re
import sys
from pathlib import Path

try:
    import numpy as np
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("frame_compare.py needs numpy and Pillow: pip install numpy pillow")

DUMP = re.compile(r"frame(\d+)-(real|between(\d*))\.ppm$")
CAPTURE = re.compile(r"frame-(\d+)\.ppm$")


def images(folder):
    """(frame, step, path), in the order they are on screen."""
    folder = Path(folder)
    found = []
    for path in folder.glob("*.ppm"):
        dump = DUMP.search(path.name)
        if dump:
            step = 99 if dump.group(2) == "real" else int(dump.group(3) or 1)
            found.append((int(dump.group(1)), step, path))
            continue
        capture = CAPTURE.search(path.name)
        if capture:
            found.append((int(capture.group(1)), 99, path))
    if not found and (folder / "dump").is_dir():
        return images(folder / "dump")
    return sorted(found)


def label(frame, step):
    return f"{frame}" if step == 99 else f"{frame}-between{step if step > 1 else ''}"


def frames_arg(text):
    if not text:
        return None
    out = set()
    for part in text.split(","):
        first, _, last = part.partition("-")
        out.update(range(int(first), int(last or first) + 1))
    return out


def region(shape, text):
    h, w = shape[:2]
    x0, y0, x1, y1 = [float(v) for v in (text or "0,0,1,1").split(",")]
    return slice(int(h * y0), int(h * y1)), slice(int(w * x0), int(w * x1))


def gray(path):
    return np.asarray(Image.open(path).convert("L"), dtype=np.float32)


def rgb(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)


def blinks(args):
    seq = images(args.dir)
    if len(seq) < 3:
        sys.exit(f"{args.dir}: fewer than three images")
    results = []
    window = [gray(seq[0][2]), gray(seq[1][2])]
    ys, xs = region(window[0].shape, args.region)
    for i in range(1, len(seq) - 1):
        window.append(gray(seq[i + 1][2]))
        a, b, c = window
        same = np.abs(a - c) < args.match
        up = (b - a > args.threshold) & (b - c > args.threshold)
        down = (b - a < -args.threshold) & (b - c < -args.threshold)
        count = int((same & (up | down))[ys, xs].sum())
        results.append((count, i))
        window.pop(0)
    counts = sorted(c for c, _ in results)
    print(f"{args.dir}: {len(seq)} images, blink pixels per image: median {counts[len(counts) // 2]}, "
          f"max {counts[-1]}")
    top = sorted(results, reverse=True)[:args.top]
    for count, i in top:
        print(f"  {label(*seq[i][:2])}: {count}")
    if args.sheet:
        rows = []
        for count, i in top[:4]:
            a, b, c = (rgb(seq[j][2]) for j in (i - 1, i, i + 1))
            ga, gb, gc = (x.mean(axis=2) for x in (a, b, c))
            mask = (np.abs(ga - gc) < args.match) & (((gb - ga > args.threshold) & (gb - gc > args.threshold)) |
                                                     ((gb - ga < -args.threshold) & (gb - gc < -args.threshold)))
            marked = b.copy()
            marked[mask] = (255, 0, 0)
            names = (label(*seq[i - 1][:2]), label(*seq[i][:2]), label(*seq[i + 1][:2]), f"red: {count} px")
            rows.append(list(zip((a, b, c, marked), names)))
        save_sheet(rows, args.sheet, args.width)


def ratio(args):
    with_, without = {(f, s): p for f, s, p in images(args.with_)}, {(f, s): p for f, s, p in images(args.without)}
    mask = {(f, s): p for f, s, p in images(args.mask)} if args.mask else {}
    wanted = frames_arg(args.frames)
    keys = sorted(k for k in with_ if k in without and k[1] == 99 and (wanted is None or k[0] in wanted))
    print("frame  all" + ("    full    absent" if mask else ""))
    for key in keys:
        a, b = gray(with_[key]), gray(without[key])
        ys, xs = region(a.shape, args.region)
        keep = np.zeros(a.shape, bool)
        keep[ys, xs] = True
        keep &= b > 30
        line = f"{key[0]:5}  {np.median(a[keep] / b[keep]):.3f}"
        if key in mask:
            m = gray(mask[key])
            full, absent = keep & (m < 5), keep & (m > 250)
            line += f"   {np.median(a[full] / b[full]):.3f}   {np.median(a[absent] / b[absent]):.3f}"
        print(line)


def track(args):
    wanted = frames_arg(args.frames)
    for frame, step, path in images(args.dir):
        if wanted is None or frame in wanted:
            g = gray(path)
            ys, xs = region(g.shape, args.region)
            print(f"{label(frame, step):>14}  {g[ys, xs].mean():6.1f}")


def diff(args):
    a, b = {(f, s): p for f, s, p in images(args.a)}, {(f, s): p for f, s, p in images(args.b)}
    for key in sorted(k for k in a if k in b):
        x, y = rgb(a[key]), rgb(b[key])
        if x.shape != y.shape:
            print(f"{label(*key):>14}  sizes differ")
            continue
        d = np.abs(x - y)
        print(f"{label(*key):>14}  mean {d.mean():.3f}  changed {int((d.max(axis=2) > 8).sum())}")


def sheet(args):
    wanted = sorted(frames_arg(args.frames))
    rows = []
    for folder in args.dirs:
        found = {f: p for f, s, p in images(folder) if s == 99}
        row = []
        for frame in wanted:
            if frame in found:
                image = rgb(found[frame])
                if args.crop:
                    ys, xs = region(image.shape, args.crop)
                    image = image[ys, xs]
                row.append((image, f"{Path(folder).name} {frame}"))
        rows.append(row)
    save_sheet(rows, args.out, args.width)


def save_sheet(rows, out, width):
    tiles = [[(Image.fromarray(np.clip(im, 0, 255).astype(np.uint8)), text) for im, text in row] for row in rows]
    first = tiles[0][0][0]
    w, h = width, int(width * first.height / first.width)
    cols = max(len(row) for row in tiles)
    canvas = Image.new("RGB", (cols * (w + 4), len(tiles) * (h + 20)), "black")
    draw = ImageDraw.Draw(canvas)
    for r, row in enumerate(tiles):
        for c, (im, text) in enumerate(row):
            x, y = c * (w + 4), r * (h + 20)
            canvas.paste(im.resize((w, h)), (x, y + 18))
            draw.text((x + 4, y + 3), text, fill=(255, 255, 0))
    canvas.save(out)
    print(f"wrote {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog="\n".join(__doc__.splitlines()[2:]))
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("blinks")
    p.add_argument("dir")
    p.add_argument("--threshold", type=float, default=40)
    p.add_argument("--match", type=float, default=12)
    p.add_argument("--top", type=int, default=10)
    p.add_argument("--sheet")
    p.add_argument("--width", type=int, default=480)
    p.add_argument("--region")
    p.set_defaults(run=blinks)
    p = sub.add_parser("ratio")
    p.add_argument("with_", metavar="WITH")
    p.add_argument("without", metavar="WITHOUT")
    p.add_argument("--mask")
    p.add_argument("--frames")
    p.add_argument("--region")
    p.set_defaults(run=ratio)
    p = sub.add_parser("track")
    p.add_argument("dir")
    p.add_argument("--frames")
    p.add_argument("--region")
    p.set_defaults(run=track)
    p = sub.add_parser("diff")
    p.add_argument("a")
    p.add_argument("b")
    p.set_defaults(run=diff)
    p = sub.add_parser("sheet")
    p.add_argument("dirs", nargs="+")
    p.add_argument("--frames", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--crop")
    p.add_argument("--width", type=int, default=480)
    p.set_defaults(run=sheet)
    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
