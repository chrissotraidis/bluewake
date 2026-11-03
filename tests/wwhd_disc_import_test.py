#!/usr/bin/env python3
"""End-to-end synthetic pack test: python this_file.py /path/to/native/CLI.

Requires only pycryptodome, through wwhd_disc_fixture. BTI/BFRES construction
is adapted from build/wwhd-ios-simulator-check/make_fixtures.py in the primary
checkout. No Pillow, private input, game assets or external key files are used.
Everything generated is removed with its TemporaryDirectory, including on a
failed assertion. The only subprocess is the explicitly supplied native CLI.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import wwhd_disc_fixture as fixture
from wwhd.disc import WiiUDisc
from wwhd.vendor import addrlib

ROOT = Path(__file__).resolve().parents[1]
COLOR = bytes((173, 61, 29, 255))
# The GC fixture is I8: replacement R, G, B and A are the HD red channel.
# A constant opaque source stays constant at every downsampled mip, so this
# oracle needs neither the production resampler nor Pillow.
EXPECTED_PIXEL = bytes((173, 173, 173, 173))
MIP_SIZES = (32, 16, 8, 4, 2, 1)


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def make_gamecube():
    disc = bytearray(8192)
    disc[:8] = b"GZLE01\0\0"
    fixture.put(disc, 28, "I", 0xC2339F3D)
    fst = bytearray(24) + b"A.bti\0"
    fixture.put(fst, 0, "III", 0x01000000, 0, 2)
    fixture.put(fst, 12, "III", 0, 0x800, 288)
    disc[0x800] = 1  # I8, 16x16, one level, no executable content.
    fixture.put(disc, 0x802, "HH", 16, 16)
    disc[0x818] = 1
    fixture.put(disc, 0x81C, "I", 32)
    disc[0x820:0x920] = bytes([42]) * 256
    fixture.put(disc, 0x424, "II", 0x500, len(fst))
    disc[0x500:0x500 + len(fst)] = fst
    return disc


def make_bfres(size=32):
    data = bytearray(0x2000)
    data[:10] = b"FRES\x03\0\0\0\xfe\xff"
    fixture.put(data, 0x24, "i", 0x100 - 0x24)
    fixture.put(data, 0x100, "II", 40, 1)
    fixture.put(data, 0x120, "ii", 0x200 - 0x120, 0x240 - 0x124)
    data[0x200:0x202] = b"A\0"
    data[0x240:0x244] = b"FTEX"
    info = addrlib.getSurfaceInfo(0x1A, size, size, 1, 1, 1, 0, 0)
    fixture.put(data, 0x244, "16I", 1, size, size, 1, 1, 0x1A, 0, 1,
                info.surfSize, 0, 0, 0, 1, 0, info.baseAlign, info.pitch)
    data[0x2C8:0x2CC] = bytes((0, 1, 2, 3))
    fixture.put(data, 0x2F0, "i", 0x800 - 0x2F0)
    data.extend(bytes(max(0, 0x800 + info.surfSize - len(data))))
    data[0x800:0x800 + info.surfSize] = COLOR * (info.surfSize // 4)
    return bytes(data)



def make_sarc(bfres):
    names = b"A.bfres\0B.bfres\0"
    payload = 0x14 + 12 + 32 + 8 + len(names)
    data = bytearray(payload)
    struct.pack_into(">4sHHIII", data, 0, b"SARC", 0x14, 0xFEFF,
                     payload + 2 * len(bfres), payload, 0x01000000)
    struct.pack_into(">4sHHI", data, 0x14, b"SFAT", 12, 2, 0x65)
    for i in range(2):
        struct.pack_into(">IIII", data, 0x20 + i * 16, 0, 0x01000000 | (i * 2),
                         i * len(bfres), (i + 1) * len(bfres))
    struct.pack_into(">4sHH", data, 0x40, b"SFNT", 8, 0)
    data[0x48:] = names
    return bytes(data) + bfres * 2


def make_yaz0(data):
    return b"Yaz0" + struct.pack(">I", len(data)) + bytes(8) + b"".join(
        b"\xff" + data[at:at + 8] for at in range(0, len(data), 8))


def make_wiiu(bfres):
    image, _, _ = fixture.make_disc()
    offset = fixture.HASH_DATA - 6  # BFRES header and payload cross hash blocks.
    fst = fixture.file_table([(1, 1), (2, 2), (4, 2), (6, 6)], [
        (1, "content", 0, 3, 0, 0),
        (0, "A.bfres", offset // fixture.SCALE, len(bfres), 0, 3),
    ], fixture.SCALE)
    fixture.install_metadata(image, fst)
    plain = bytearray(fixture.pattern(3 * fixture.HASH_DATA, 131))
    plain[offset:offset + len(bfres)] = bfres
    for index in range(3):
        data = plain[index * fixture.HASH_DATA:(index + 1) * fixture.HASH_DATA]
        h0 = hashlib.sha1(data).digest()
        hashes = bytearray(0x400)
        hashes[index * 20:index * 20 + 20] = h0
        start = fixture.HASHED + index * fixture.BLOCK
        image[start:start + fixture.BLOCK] = (
            fixture.encrypt(hashes, fixture.TITLE)
            + fixture.encrypt(data, fixture.TITLE, h0[:16]))
    return image


def check_reference(path, bfres):
    for title, common in ((fixture.TITLE, None), (None, fixture.COMMON)):
        disc = WiiUDisc(path, fixture.DISC, title_key=title, common_key=common)
        try:
            check(len(disc.entries) == 1, "toy disc must have exactly one game file")
            check(disc.entries[0].path == "content/A.bfres", "toy disc path differs")
            check(disc.read(disc.entries[0]) == bfres, "Python reference BFRES bytes differ")
        finally:
            disc.close()


def check_png(data, size, label):
    """Independent stdlib check of every PNG chunk, CRC, scanline and pixel."""
    check(data[:8] == b"\x89PNG\r\n\x1a\n", f"{label}: PNG signature")
    at, compressed, kinds = 8, bytearray(), []
    while at < len(data):
        check(at + 12 <= len(data), f"{label}: truncated chunk")
        length, kind = struct.unpack_from(">I4s", data, at)
        end = at + 12 + length
        check(end <= len(data), f"{label}: truncated chunk body")
        body = data[at + 8:at + 8 + length]
        crc = struct.unpack_from(">I", data, at + 8 + length)[0]
        check(crc == zlib.crc32(kind + body) & 0xFFFFFFFF, f"{label}: bad {kind!r} CRC")
        kinds.append(kind)
        if kind == b"IHDR":
            check(length == 13 and kinds == [b"IHDR"], f"{label}: invalid IHDR")
            check(struct.unpack(">IIBBBBB", body) == (size, size, 8, 6, 0, 0, 0),
                  f"{label}: wrong mip dimensions or PNG format")
        elif kind == b"IDAT":
            compressed.extend(body)
        elif kind == b"IEND":
            check(not body and end == len(data), f"{label}: invalid IEND/trailing data")
        else:
            raise AssertionError(f"{label}: unexpected chunk {kind!r}")
        at = end
    check(kinds[0] == b"IHDR" and kinds[-1] == b"IEND" and b"IDAT" in kinds,
          f"{label}: incomplete PNG")
    inflater = zlib.decompressobj()
    raw = inflater.decompress(compressed) + inflater.flush()
    check(inflater.eof and not inflater.unused_data, f"{label}: invalid deflate stream")
    # The native PNG writer promises unfiltered RGBA scanlines. The expected
    # pixels are fixed above, independently of folder/disc output comparison.
    expected = (b"\0" + EXPECTED_PIXEL * size) * size
    check(raw == expected, f"{label}: unexpected pixels, scanlines or mip size")


def check_pack(output, exact=True, expected_conflicts=0):
    report = json.loads((output / "import-report.json").read_text(encoding="utf-8"))
    check(report["replacement_count"] == 1 and report["conflicts_skipped"] == expected_conflicts
          and report["unsupported"] == [] and len(report["textures"]) == 1,
          f"{output.name}: unexpected replacements, conflicts or errors")
    texture = report["textures"][0]
    check(texture["original_size"] == [16, 16] and texture["hd_size"] == [32, 32]
          and texture["mipmaps"] == len(MIP_SIZES), f"{output.name}: texture metadata")
    base = texture["file"]
    check(re.fullmatch(r"tex1_16x16_[0-9a-f]{16}_1\.png", base) is not None,
          f"{output.name}: invalid replacement filename")
    names = [base] + [base[:-4] + f"_mip{i}.png" for i in range(1, len(MIP_SIZES))]
    pngs = {path.relative_to(output).as_posix(): path.read_bytes() for path in output.rglob("*.png")}
    check(set(pngs) == set(names), f"{output.name}: missing or extra PNGs")
    for name, size in zip(names, MIP_SIZES):
        check_png(pngs[name], size, output.name + "/" + name)
    hashes = {name: hashlib.sha256(data).hexdigest() for name, data in pngs.items()}
    check(texture["files_sha256"] == (hashes if exact else {}), f"{output.name}: report PNG hashes differ")
    return pngs


def snapshot(directory):
    # Include directories so even empty staging folders cannot escape a check.
    return {p.relative_to(directory).as_posix(): p.read_bytes() if p.is_file() else None
            for p in directory.rglob("*") if p.name != ".bluewake-wwhd-import.lock"}


def run(cli, root):
    bfres = make_bfres()
    gc = root / "synthetic.iso"
    gc.write_bytes(make_gamecube())
    folder = root / "folder"
    (folder / "content").mkdir(parents=True)
    (folder / "content/A.bfres").write_bytes(bfres)
    images, keys, outputs = (root / name for name in ("images", "keys", "outputs"))
    for directory in (images, keys, outputs):
        directory.mkdir()
    key_paths = {}
    for name, key in (("disc", fixture.DISC), ("title", fixture.TITLE),
                      ("common", fixture.COMMON), ("wrong", fixture.WRONG)):
        key_paths[name] = keys / ("SYNTHETIC-" + name + ".key")
        key_paths[name].write_bytes(key)
    # Fallback names are read only beside these freshly generated toy images.
    (images / "game.key").write_bytes(fixture.DISC)
    (images / "common.key").write_bytes(fixture.COMMON)
    wud = make_wiiu(bfres)
    (images / "toy.wud").write_bytes(wud)
    (images / "toy.wux").write_bytes(fixture.mapped_wux(wud))
    for name in ("toy.wud", "toy.wux"):
        check_reference(images / name, bfres)

    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith("WWHD_TEST_") and key != "WWHD_EXACT_REPORT"}
    def invoke(source, output, arguments=(), failure=None, *, gc_input=gc, exact=True,
               cancel=None, after=1, expected_code=1, evidence=None, remove_name=None, conflicts=0):
        before = snapshot(outputs)
        command = [str(cli), str(source), str(gc_input), str(output)] + [str(a) for a in arguments]
        child_environment = dict(environment)
        if exact:
            child_environment["WWHD_EXACT_REPORT"] = "1"
        if cancel:
            child_environment.update(WWHD_TEST_CANCEL=cancel, WWHD_TEST_AFTER=str(after))
        if remove_name:
            child_environment["WWHD_TEST_REMOVE_NAME"] = remove_name
        result = subprocess.run(command, cwd=root, env=child_environment, capture_output=True,
                                text=True, timeout=30, check=False)
        detail = f"{output.name}: exit {result.returncode}\n{result.stdout}\n{result.stderr}"
        if evidence is not None:
            check(evidence in result.stderr, "missing test checkpoint evidence: " + detail)
        if failure is not None:
            check(result.returncode == expected_code, "expected a handled import failure: " + detail)
            check(failure.lower() in result.stderr.lower(), "unexpected failure reason: " + detail)
            check(snapshot(outputs) == before, "failed import changed output or left staging: " + detail)
            return None
        check(result.returncode == 0, detail)
        check(output.is_dir(), "successful import produced no output: " + detail)
        after = snapshot(outputs)
        relative = output.relative_to(outputs).as_posix()
        prefix = relative + "/"
        outside = {p: data for p, data in after.items() if p != relative and not p.startswith(prefix)}
        check(outside == before, "successful import modified another output or left staging: " + detail)
        return check_pack(output, exact, conflicts)

    baseline = outputs / "folder-pack"
    expected_pngs = invoke(folder, baseline)
    # A and B share original texture bytes but disagree in HD. C remains a
    # valid replacement, so ignoring an A/B deletion failure would publish a
    # pack containing rejected artwork rather than fail for an empty pack.
    conflict_gc = root / "conflicting-originals.iso"
    conflict_disc = make_gamecube()
    fst = bytearray(48) + b"A.bti\0B.bti\0C.bti\0"
    fixture.put(fst, 0, "III", 0x01000000, 0, 4)
    for index, offset in enumerate((0x800, 0x800, 0x1000), 1):
        fixture.put(fst, index * 12, "III", (index - 1) * 6, offset, 288)
    conflict_disc[0x1000:0x1120] = conflict_disc[0x800:0x920]
    conflict_disc[0x1020:0x1120] = bytes([43]) * 256
    fixture.put(conflict_disc, 0x424, "II", 0x500, len(fst))
    conflict_disc[0x500:0x500 + len(fst)] = fst
    conflict_gc.write_bytes(conflict_disc)
    conflict_folder = root / "conflicting-resources"
    conflict_folder.mkdir()
    for name in ("A", "B", "C"):
        texture = bytearray(bfres)
        texture[0x200] = ord(name)
        if name == "B":
            texture[0x800:] = bytes((31, 61, 29, 255)) * ((len(texture) - 0x800) // 4)
        (conflict_folder / (name + ".bfres")).write_bytes(texture)
    invoke(conflict_folder, outputs / "conflict-pruned", gc_input=conflict_gc, conflicts=1)
    rejected_name = next(name for name in expected_pngs if "_mip" not in name)
    for exact in (True, False):
        invoke(conflict_folder, outputs / f"remove-failure-{exact}", gc_input=conflict_gc,
               exact=exact, cancel="remove-conflict", remove_name=rejected_name,
               failure="cannot remove generated texture", evidence="Test blocked removal:")
    title_args = (key_paths["disc"], "", key_paths["title"])
    modes = {
        "title": title_args,
        "common": (key_paths["disc"], key_paths["common"]),
        "fallback": (),
        "title-override": (key_paths["disc"], key_paths["wrong"], key_paths["title"]),
    }
    failures = 0
    for name in ("toy.wud", "toy.wux"):
        image = images / name
        for mode, arguments in modes.items():
            pngs = invoke(image, outputs / (name + "-" + mode), arguments)
            check(pngs == expected_pngs, f"{name}/{mode}: PNG bytes differ from folder pack")
        for mode, arguments in {
            "wrong-disc": (key_paths["wrong"], "", key_paths["title"]),
            "wrong-common": (key_paths["disc"], key_paths["wrong"]),
            "wrong-title": (key_paths["disc"], key_paths["common"], key_paths["wrong"]),
        }.items():
            output = outputs / (name + "-" + mode)
            invoke(image, output, arguments, failure="key")
            check(not output.exists(), f"{mode}: failed import produced an output")
            failures += 1
        # Exercise preservation with both valid and wrong keys. The baseline
        # is an actual completed toy pack, not merely an empty destination.
        for arguments in (title_args, (key_paths["wrong"], "", key_paths["title"])):
            invoke(image, baseline, arguments, failure="output already exists")
            failures += 1

    # Exercise every host-path boundary beneath a non-ASCII parent, as with a
    # Windows profile name: folder/image, GC disc, each key and the destination.
    # CJK and a supplementary character cannot round-trip through legacy ACPs.
    unicode_name = "\u00e9_\u65e5\u672c\u8a9e_\U0001f600"
    unicode_root = root / unicode_name
    unicode_folder = unicode_root / "folder"
    (unicode_folder / "content").mkdir(parents=True)
    (unicode_folder / "content/A.bfres").write_bytes(bfres)
    unicode_gc = unicode_root / (unicode_name + ".iso")
    unicode_gc.write_bytes(make_gamecube())
    unicode_keys = {}
    for name in ("disc", "common", "title"):
        unicode_keys[name] = unicode_root / (name + "-" + unicode_name + ".key")
        unicode_keys[name].write_bytes(key_paths[name].read_bytes())
    unicode_outputs = outputs / unicode_name
    unicode_outputs.mkdir()
    pngs = invoke(unicode_folder, unicode_outputs / "folder-pack", gc_input=unicode_gc)
    check(pngs == expected_pngs, "Unicode folder paths: PNG bytes differ from ASCII baseline")
    unicode_imports = 1
    for name in ("toy.wud", "toy.wux"):
        image = unicode_root / (unicode_name + "-" + name)
        image.write_bytes((images / name).read_bytes())
        for mode, arguments in {
            "common": (unicode_keys["disc"], unicode_keys["common"]),
            "title": (unicode_keys["disc"], "", unicode_keys["title"]),
        }.items():
            pngs = invoke(image, unicode_outputs / (name + "-" + mode), arguments, gc_input=unicode_gc)
            check(pngs == expected_pngs, f"Unicode {name}/{mode}: PNG bytes differ from ASCII baseline")
            unicode_imports += 1

    # Cancellation uses named test-only boundaries in the native runner. Every
    # case must throw the distinct Cancelled exception (exit 3), preserve all
    # previous packs byte-for-byte and remove even empty staging directories.
    cancellations = 0
    def cancel_case(mode, source=folder, arguments=(), *, after=1, gc_input=gc, exact=True, evidence=None):
        nonlocal cancellations
        invoke(source, outputs / f"cancel-{cancellations}", arguments,
               failure="Import cancelled", expected_code=3, cancel=mode, after=after,
               gc_input=gc_input, exact=exact, evidence=evidence or f"Test cancellation: {mode}")
        cancellations += 1

    cancel_case("pre", root / "missing-HD", gc_input=root / "missing-GC")
    cancel_case("stage:Opening Wind Waker HD", root / "missing-HD")
    cancel_case("folder-entry")
    cancel_case("stage:Reading the GameCube disc", gc_input=root / "missing-GC")
    for mode in ("gc-entry", "gc-resource", "gc-leaf"):
        cancel_case(mode)
    # Header + FST + first resource chunk succeed; cancel before the second
    # 64 KiB resource chunk, rather than reading the entire declared resource.
    large_gc = bytearray(make_gamecube())
    large_gc.extend(bytes(0x800 + 2 * fixture.BLOCK - len(large_gc)))
    fixture.put(large_gc, 0x500 + 20, "I", 2 * fixture.BLOCK)
    large_gc_path = root / "large-synthetic.iso"
    large_gc_path.write_bytes(large_gc)
    cancel_case("read-block", gc_input=large_gc_path, after=4,
                evidence="Test cancellation: read-block (4)")

    archive_folder = root / "archive-folder"
    archive_folder.mkdir()
    (archive_folder / "A.pack").write_bytes(make_sarc(bfres))
    cancel_case("archive-member", archive_folder, after=2,
                evidence="Test cancellation: archive-member (2)")
    yaz_folder = root / "yaz-folder"
    yaz_folder.mkdir()
    (yaz_folder / "A.szs").write_bytes(make_yaz0(make_sarc(bfres + bytes(2 * fixture.BLOCK))))
    cancel_case("yaz0-block", yaz_folder, after=2,
                evidence="Test cancellation: yaz0-block (2)")
    for name in ("toy.wud", "toy.wux"):
        for mode, after in (("disc-block", 2), ("wiiu-entry", 2),
                            ("metadata-block", 2), ("content-block", 2)):
            cancel_case(mode, images / name, title_args, after=after,
                        evidence=f"Test cancellation: {mode} ({after})")
    cancel_case("stage:Finishing the pack")
    cancel_case("publish")
    cancel_case("png-row", exact=False, after=2)
    # A real worker is held while all six mip jobs are queued. Cancellation
    # happens just before the producer waits; shutdown releases and joins the
    # worker, and no queued job is started. This would deadlock with wait().
    cancel_case("async-drain", exact=False, evidence="Async cleanup: 1 started, 6 queued, joined")
    # Also exercise backpressure with an actual image over the 32 MiB budget.
    large_folder = root / "large-folder"
    large_folder.mkdir()
    (large_folder / "A.bfres").write_bytes(make_bfres(3072))
    cancel_case("async-room", large_folder, exact=False,
                evidence="Async cleanup: 1 started, 1 queued, joined")
    for source, arguments in ((folder, ()), (images / "toy.wux", title_args)):
        pngs = invoke(source, outputs / ("async-" + source.name), arguments, exact=False)
        check(pngs == expected_pngs, "async PNG bytes differ from the exact-report baseline")
    # Once rename has committed a complete pack, a late cancel is success.
    pngs = invoke(folder, outputs / "cancel-after-publication", cancel="stage:Done")
    check(pngs == expected_pngs, "cancellation after publication changed the completed pack")
    close_failures = 0
    if os.name != "nt":
        for exact in (True, False):
            invoke(folder, outputs / f"close-failure-{exact}", cancel="close-failure", exact=exact,
                   failure="cannot finish writing")
            close_failures += 1

    # Unlike wrong keys (rejected while opening), a corrupt payload fails after
    # import_pack has created staging. Require its cleanup for WUD and WUX.
    wud[fixture.HASHED + fixture.BLOCK + 0x400 + 19] ^= 1
    for name, data in (("corrupt.wud", wud), ("corrupt.wux", fixture.mapped_wux(wud))):
        path = images / name
        path.write_bytes(data)
        invoke(path, outputs / name, title_args, failure="content block checksum")
        failures += 1
    check(check_pack(baseline) == expected_pngs, "original folder pack was changed")
    print(f"PASS: folder + 2 disc formats x {len(modes)} key modes; six PNG mips each, "
          f"independent pixels/CRC; {unicode_imports} Unicode-path imports match ASCII PNGs; "
          f"{failures} failure/preservation checks; {cancellations} deterministic cancellations; "
          f"2 async PNG byte comparisons; post-publication cancel succeeds; "
          f"{close_failures} buffered-close failures (POSIX only); "
          "conflict pruning and 2 removal-failure preservation checks")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} native-import-CLI")
    cli = Path(sys.argv[1]).resolve()
    check(cli.is_file(), f"native importer CLI is missing: {cli}")
    scratch = ROOT / "build/wwhd-disc-import"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pack-integration-", dir=scratch) as temporary:
        run(cli, Path(temporary))


if __name__ == "__main__":
    main()
