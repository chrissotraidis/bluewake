"""Compare two private importer outputs, including every PNG and mip level.

Usage: build/wwhd-tools/bin/python tests/wwhd_compare_packs.py OLD-PACK NEW-PACK
Accepts either the Python or native report. Prints counts only; private artwork
and report contents stay in their existing folders.
"""
import hashlib
import json
from pathlib import Path
import sys

from PIL import Image


def require(ok, message):
    if not ok:
        raise ValueError(message)


def compare(before, after):
    reports = [json.loads((p / "import-report.json").read_text()) for p in (before, after)]
    for field in ("replacement_count", "unsupported"):
        require(reports[0][field] == reports[1][field], f"different {field}")
    conflicts = [r["conflicts_skipped"] for r in reports]
    require((len(conflicts[0]) if isinstance(conflicts[0], list) else conflicts[0]) ==
            (len(conflicts[1]) if isinstance(conflicts[1], list) else conflicts[1]), "different conflict count")
    textures = [{t["file"]: t for t in r["textures"]} for r in reports]
    require(textures[0].keys() == textures[1].keys(), "different texture keys")
    for r, entries in zip(reports, textures):
        require(len(entries) == len(r["textures"]) == r["replacement_count"], "duplicate or missing texture keys")
    files = [{p.relative_to(root) for p in root.rglob("*") if p.is_file()} for root in (before, after)]
    require(files[0] == files[1], "different output filenames")
    checked = set()
    identical_bytes = 0
    generated = 0
    for name, old in textures[0].items():
        new = textures[1][name]
        for field in ("hd", "original_size", "hd_size", "mipmaps", "selection"):
            require(old.get(field) == new.get(field), f"different {field}")
        for field in ("originals", "alternatives"):
            require(sorted(old.get(field, [])) == sorted(new.get(field, [])), f"different {field}")
        generated += "#GameCube-" in new["hd"]
        sha = hashlib.sha256()
        for level in range(new["mipmaps"]):
            path = Path(name if level == 0 else f"{Path(name).stem}_mip{level}.png")
            a, b = before / path, after / path
            same = a.read_bytes() == b.read_bytes()
            identical_bytes += same
            with Image.open(b) as image:
                size = image.size
                pixels = image.convert("RGBA").tobytes()
            if not same:
                with Image.open(a) as image:
                    require(image.size == size and image.convert("RGBA").tobytes() == pixels, "different PNG pixels")
            sha.update(pixels)
            checked.add(path)
        # Fast native reports use xxh128; exact native and Python reports
        # record SHA-256. Verify each available SHA against actual PNG pixels.
        for entry in (old, new):
            digest = entry.get("image_sha256", "")
            if len(digest) == 64:
                require(sha.hexdigest() == digest, "RGBA chain differs from report SHA-256")
    require(checked == {p for p in files[1] if p.suffix == ".png"}, "unreported PNG files")
    print(f"PASS: {len(textures[1])} texture keys ({generated} generated HUD/title), "
          f"{len(files[1])} filenames, {len(checked)} PNGs with identical pixels; "
          f"{identical_bytes} byte-identical PNGs. Matching metadata, conflicts and unsupported entries.")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: wwhd_compare_packs.py OLD-PACK NEW-PACK")
    try:
        compare(Path(sys.argv[1]), Path(sys.argv[2]))
    except (OSError, ValueError, KeyError) as error:
        sys.exit(f"FAIL: {error}")
