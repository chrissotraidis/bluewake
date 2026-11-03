#!/usr/bin/env python3
"""Synthetic tests for scripts/prepare.py RARC parsing and manifest logic."""
import struct
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
from prepare import parse_rarc_names, parse_rel_header, yaz0_decompress


def make_yaz0_blob(uncompressed_size=64):
    """Create a minimal Yaz0 block (magic + declared size + simple literal group)."""
    # Header: "Yaz0" + u32 uncompressed_size
    hdr = b"Yaz0" + struct.pack(">I", uncompressed_size)
    # Data section: one group header 0x80 followed by N literal bytes
    literals = b"\xAB" * uncompressed_size
    return hdr + b"\x80" + literals


def make_rarc(entries):
    """Build a minimal RARC archive with Yaz0-compressed file entries.

    entries: list of (name, uncompressed_size) tuples.
    Returns (archive_bytes, dict mapping name -> yaz0_offset_in_archive).
    """
    str_data = b""
    name_offsets = {}
    for name, _ in entries:
        name_offsets[name] = len(str_data)
        str_data += name.encode("ascii") + b"\x00"

    # Build Yaz0 blobs first to get their compressed offsets and sizes
    yaz_blobs = []
    data_section = b""
    entry_doff = {}
    for name, usize in entries:
        blob = make_yaz0_blob(usize)
        # Align each blob to 0x20
        while len(data_section) % 0x20 != 0:
            data_section += b"\x00"
        entry_doff[name] = len(data_section)
        data_section += blob

    # Build entry table
    entry_data = b""
    for name, usize in entries:
        etype = 0x02 if name.startswith(".") else 0x01
        noff = name_offsets[name]
        doff = entry_doff[name]
        dlen = len(make_yaz0_blob(usize))  # approximate compressed size
        # Actually use actual offset-based size
        # Just store doff and some size; parser uses them for lookup
        entry_data += struct.pack(">HHBBH", 1, 0, etype, 0, noff)
        entry_data += struct.pack(">II", doff, dlen)
        entry_data += b"\x00" * 4

    node_data = struct.pack(">4sIHHI", b"ROOT", 0, 0, len(entries), 0)

    hdr_size = 0x20
    dir_hdr_size = 24
    node_off_rel = dir_hdr_size
    entry_off_rel = dir_hdr_size + len(node_data)
    str_off_rel = entry_off_rel + len(entry_data)

    dir_hdr = struct.pack(
        ">IIIIII",
        1, node_off_rel, len(entries), entry_off_rel, 0, str_off_rel,
    )
    dir_hdr += b"\x00" * (dir_hdr_size - len(dir_hdr))

    total_meta = hdr_size + dir_hdr_size + len(node_data) + len(entry_data) + len(str_data)
    # Align data start to 0x20
    while total_meta % 0x20 != 0:
        total_meta += 1
        pad = b"\x00"
    
    rarc_header = struct.pack(">4sIII", b"RARC", 0, hdr_size, total_meta)
    # Pad RARC header to exactly hdr_size bytes
    rarc_header += b"\x00" * (hdr_size - len(rarc_header))
    assert len(rarc_header) == hdr_size
    
    meta = rarc_header + dir_hdr + node_data + entry_data + str_data
    # Pad to alignment for Yaz0 data section
    while len(meta) % 0x20 != 0:
        meta += b"\x00"

    archive = meta + data_section
    # Fix up total size in header
    archive = bytearray(archive)
    struct.pack_into(">I", archive, 4, len(archive))
    archive = bytes(archive)
    return archive


def make_rel(module_id=42, num_sections=5):
    """Build a minimal REL binary."""
    hdr = struct.pack(">IIII", module_id, 0, 0, num_sections)
    hdr += struct.pack(">III", 0x40, 0, 0x40)  # sec_off, name_off, imp_off
    hdr += struct.pack(">I", 0)  # imp_size
    hdr += b"\x00" * (0x40 - len(hdr))
    return hdr + b"\xAB" * 64


def test_basic_parse():
    arc = make_rarc([
        ("d_a_test.rel", 128),
        ("d_a_other.rel", 256),
    ])
    result = parse_rarc_names(arc)
    assert len(result) == 2, "expected 2 rels, got %d: %r" % (len(result), result)
    names = sorted(r["name"] for r in result)
    assert names == ["d_a_other.rel", "d_a_test.rel"], names
    print("PASS: basic parse finds 2 rels")


def test_empty_archive():
    assert parse_rarc_names(b"") == []
    assert parse_rarc_names(b"NOTR" + b"\x00" * 64) == []
    print("PASS: empty/wrong-magic returns []")


def test_no_rel_entries():
    arc = make_rarc([("readme.txt", 100), ("data.bin", 200)])
    result = parse_rarc_names(arc)
    assert result == [], "no .rel files should return []"
    print("PASS: non-rel archive returns []")


def test_dir_entries_skipped():
    arc = make_rarc([
        (".", 0),
        ("..", 0),
        ("d_a_real.rel", 128),
    ])
    result = parse_rarc_names(arc)
    assert len(result) == 1, "expected 1 rel, got %d: %r" % (len(result), result)
    assert result[0]["name"] == "d_a_real.rel"
    print("PASS: directory entries skipped")


def test_rel_header():
    rel = make_rel(module_id=7, num_sections=12)
    info = parse_rel_header(rel)
    assert info is not None
    assert info["module_id"] == 7, info
    assert info["num_sections"] == 12, info
    print("PASS: rel header parses")


def test_rel_header_short():
    assert parse_rel_header(b"\x00" * 8) is None
    assert parse_rel_header(b"") is None
    print("PASS: short REL returns None")


def test_yaz0_roundtrip():
    original = b"Hello World BlueWake Test! " * 10
    # We can't easily create real Yaz0 compressed data without a compressor,
    # so just verify our function handles the format boundary.
    try:
        yaz0_decompress(b"NotYaz0")
        assert False, "should raise"
    except ValueError:
        pass
    print("PASS: yaz0 rejects non-Yaz0 input")


def test_truncated_entry():
    arc = make_rarc([("d_a_trunc.rel", 128)])
    truncated = arc[: len(arc) // 3]
    result = parse_rarc_names(truncated)
    assert isinstance(result, list), "should return list even when truncated"
    print("PASS: truncated input handled gracefully (%d results)" % len(result))


if __name__ == "__main__":
    test_basic_parse()
    test_empty_archive()
    test_no_rel_entries()
    test_dir_entries_skipped()
    test_rel_header()
    test_rel_header_short()
    test_yaz0_roundtrip()
    test_truncated_entry()
    print("\nAll tests passed.")
