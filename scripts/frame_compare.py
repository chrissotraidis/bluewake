#!/usr/bin/env python3
"""Find flicker and differences in rendered frames (scripts/rendered_run.py's captures and dumps).

  python3 scripts/frame_compare.py blinks DIR [--top 10] [--sheet blinks.png]
  python3 scripts/frame_compare.py ratio WITH WITHOUT [--mask DEBUG_DIR] [--frames 20-40]
  python3 scripts/frame_compare.py track DIR [--frames 20-40]
  python3 scripts/frame_compare.py diff A B
  python3 scripts/frame_compare.py sheet DIR [DIR ...] --frames 26,28,30 --out sheet.png [--crop ...]
  python3 scripts/frame_compare.py check A B [--min-psnr 40] [--flicker-margin 50] [--json out.json] [--sheet worst.png]

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
check    a verdict on run A against reference run B (scripts/render_bench.py): identical, similar (every
         image's PSNR at least --min-psnr), changed, or failed (images missing on one side, sizes that
         differ, or A blinking more than B's worst by --flicker-margin pixels). Exits 0 identical or similar,
         1 otherwise; --json writes the numbers, --sheet A, B and the difference (x8) of the worst images.

--region x0,y0,x1,y1 (fractions of the image, default the whole image) limits what is measured, to
leave the HUD out. Needs numpy and Pillow (pip install numpy pillow).
"""
import argparse
import json
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


def blink_counts(seq, threshold, match, area):
    """(blink pixels, index) for each image with a neighbour on both sides."""
    results = []
    if len(seq) < 3:
        return results
    window = [gray(seq[0][2]), gray(seq[1][2])]
    ys, xs = region(window[0].shape, area)
    for i in range(1, len(seq) - 1):
        window.append(gray(seq[i + 1][2]))
        a, b, c = window
        same = np.abs(a - c) < match
        up = (b - a > threshold) & (b - c > threshold)
        down = (b - a < -threshold) & (b - c < -threshold)
        results.append((int((same & (up | down))[ys, xs].sum()), i))
        window.pop(0)
    return results


def blinks(args):
    seq = images(args.dir)
    if len(seq) < 3:
        sys.exit(f"{args.dir}: fewer than three images")
    results = blink_counts(seq, args.threshold, args.match, args.region)
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


def psnr(x, y):
    mse = float(((x.astype(np.float64) - y) ** 2).mean())
    return float("inf") if mse == 0 else 10 * np.log10(255 ** 2 / mse)


def check(args):
    a, b = {(f, s): p for f, s, p in images(args.a)}, {(f, s): p for f, s, p in images(args.b)}
    result = {"images": len(set(a) & set(b)), "only_a": len(set(a) - set(b)), "only_b": len(set(b) - set(a)),
              "max_diff": 0, "changed_images": 0, "min_psnr": None, "worst": None, "why": ""}
    problems = []
    if not a or not b:
        problems.append("no images in " + " and ".join(str(d) for d, x in ((args.a, a), (args.b, b)) if not x))
    if result["only_a"] or result["only_b"]:
        problems.append(f"{result['only_a']} images only in A, {result['only_b']} only in B")
    worst = None
    for key in sorted(set(a) & set(b)):
        x, y = rgb(a[key]), rgb(b[key])
        if x.shape != y.shape:
            problems.append(f"{label(*key)}: sizes differ")
            continue
        ys, xs = region(x.shape, args.region)
        x, y = x[ys, xs], y[ys, xs]
        d = int(np.abs(x - y).max())
        result["max_diff"] = max(result["max_diff"], d)
        if d:
            result["changed_images"] += 1
            p = psnr(x, y)
            if worst is None or p < worst[0]:
                worst = (p, key)
    if worst:
        result["min_psnr"], result["worst"] = round(worst[0], 2), label(*worst[1])
    flicker = {}
    for side, seq in (("a", images(args.a)), ("b", images(args.b))):
        counts = blink_counts(seq, args.threshold, args.match, args.region)
        flicker[side] = max((c for c, _ in counts), default=0)
    result["flicker_a"], result["flicker_b"] = flicker["a"], flicker["b"]
    if flicker["a"] > flicker["b"] + args.flicker_margin:
        problems.append(f"new flicker: {flicker['a']} blink pixels in A, {flicker['b']} in B")
    if problems:
        result["verdict"], result["why"] = "failed", "; ".join(problems)
    elif result["changed_images"] == 0:
        result["verdict"] = "identical"
    elif result["min_psnr"] >= args.min_psnr:
        result["verdict"] = "similar"
    else:
        result["verdict"], result["why"] = "changed", f"{result['worst']} PSNR {result['min_psnr']} < {args.min_psnr}"
    print(f"{result['verdict']}: {result['images']} images, {result['changed_images']} differ, max diff "
          f"{result['max_diff']}, min PSNR {result['min_psnr']}, flicker A {flicker['a']} B {flicker['b']}"
          + (f" ({result['why']})" if result["why"] else ""))
    if args.json:
        Path(args.json).write_text(json.dumps(result, indent=1))
    if args.sheet and worst:
        x, y = rgb(a[worst[1]]), rgb(b[worst[1]])
        name = label(*worst[1])
        save_sheet([[(x, f"A {name}"), (y, f"B {name}"), (np.abs(x - y) * 8, "difference x8")]], args.sheet,
                   args.width)
    sys.exit(0 if result["verdict"] in ("identical", "similar") else 1)


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
    p = sub.add_parser("check")
    p.add_argument("a")
    p.add_argument("b")
    p.add_argument("--min-psnr", type=float, default=40)
    p.add_argument("--flicker-margin", type=int, default=50)
    p.add_argument("--threshold", type=float, default=40)
    p.add_argument("--match", type=float, default=12)
    p.add_argument("--region")
    p.add_argument("--json")
    p.add_argument("--sheet")
    p.add_argument("--width", type=int, default=480)
    p.set_defaults(run=check)
    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
