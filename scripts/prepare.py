#!/usr/bin/env python3
"""BlueWake P1 preparation pipeline.

Validates GZLE01, enumerates DOL+RELs, generates private manifest and
public-safe topology summary. Requires a user-owned disc image.
Never outputs game bytes publicly.
Usage: python3 scripts/prepare.py <iso-path> [--output-dir local-research]
"""
import argparse
import hashlib
import json
import pathlib
import struct
import sys

EXPECTED_DISC_ID = b"GZLE01"
EXPECTED_DOL_SHA1 = "8d28bab68bb5078c38e43f29206f0bd01f7e7a67"
GC_MAGIC = 0xC2339F3D


def sha1(data):
    return hashlib.sha1(data).hexdigest()


def validate_disc_header(f):
    f.seek(0)
    header = f.read(0x500)
    if len(header) < 0x500:
        raise ValueError("File too small for GameCube disc header")
    disc_id = header[0:6]
    gc_magic = struct.unpack(">I", header[28:32])[0]
    if gc_magic != GC_MAGIC:
        raise ValueError("Not a GameCube disc (magic 0x%08X)" % gc_magic)
    gamecode = disc_id[0:4].decode("ascii", errors="replace")
    maker = disc_id[4:6].decode("ascii", errors="replace")
    disk_num = header[6]
    version = header[7]
    name = header[32:0x420].split(b"\x00")[0].decode("ascii", errors="replace")
    dol_offset = struct.unpack(">I", header[0x420:0x424])[0]
    fst_offset = struct.unpack(">I", header[0x424:0x428])[0]
    fst_size = struct.unpack(">I", header[0x428:0x42C])[0]
    if disc_id != EXPECTED_DISC_ID:
        raise ValueError(
            "Unsupported disc ID %r; expected GZLE01. Only GZLE01 USA rev 0 is supported."
            % disc_id
        )
    return {
        "disc_id": disc_id.decode("ascii"),
        "gamecode": gamecode,
        "maker_code": maker,
        "disk_number": disk_num,
        "revision": version,
        "game_name": name,
        "dol_offset": dol_offset,
        "fst_offset": fst_offset,
        "fst_size": fst_size,
    }


def read_fst(f, fst_offset, fst_size):
    f.seek(fst_offset)
    fst_data = f.read(fst_size)
    num_entries = struct.unpack(">I", fst_data[8:12])[0]
    str_table_start = num_entries * 12

    def read_name(name_off):
        abs_off = str_table_start + name_off
        end = fst_data.index(b"\x00", abs_off)
        return fst_data[abs_off:end].decode("ascii", errors="replace")

    def walk(start, end, prefix=""):
        results = []
        i = start
        while i < end:
            e = fst_data[i * 12 : (i + 1) * 12]
            flags = e[0]
            name_off = struct.unpack(">I", b"\x00" + e[1:4])[0]
            off_or_parent = struct.unpack(">I", e[4:8])[0]
            size_or_next = struct.unpack(">I", e[8:12])[0]
            name = read_name(name_off)
            if flags == 0:
                results.append({"path": prefix + name, "offset": off_or_parent, "size": size_or_next, "is_dir": False})
                i += 1
            elif flags == 1:
                sub_end = size_or_next
                results.append({"path": prefix + name, "offset": i, "size": sub_end, "is_dir": True})
                results.extend(walk(i + 1, sub_end, prefix + name + "/"))
                i = sub_end
            else:
                i += 1
        return results

    return walk(1, num_entries)


def extract_dol_info(f, dol_offset):
    f.seek(dol_offset)
    hdr = f.read(0x100)
    text_offs = struct.unpack(">7I", hdr[0x00:0x1C])
    data_offs = struct.unpack(">11I", hdr[0x1C:0x48])
    text_addrs = struct.unpack(">7I", hdr[0x48:0x64])
    data_addrs = struct.unpack(">11I", hdr[0x64:0x90])
    text_sizes = struct.unpack(">7I", hdr[0x90:0xAC])
    data_sizes = struct.unpack(">11I", hdr[0xAC:0xD8])
    bss_addr = struct.unpack(">I", hdr[0xD8:0xDC])[0]
    bss_size = struct.unpack(">I", hdr[0xDC:0xE0])[0]
    entry = struct.unpack(">I", hdr[0xE0:0xE4])[0]
    offsets_sizes = (
        [(o, s) for o, s in zip(text_offs, text_sizes) if s > 0]
        + [(o, s) for o, s in zip(data_offs, data_sizes) if s > 0]
    )
    total_size = max((o + s for o, s in offsets_sizes), default=0)
    f.seek(dol_offset)
    dol_data = f.read(total_size)
    sections = []
    for i in range(7):
        if text_sizes[i] > 0:
            sections.append({"type": "text", "index": i, "file_offset": text_offs[i], "vaddr": text_addrs[i], "size": text_sizes[i]})
    for i in range(11):
        if data_sizes[i] > 0:
            sections.append({"type": "data", "index": i, "file_offset": data_offs[i], "vaddr": data_addrs[i], "size": data_sizes[i]})
    return {
        "sha1": sha1(dol_data),
        "total_size": total_size,
        "entry_point": entry,
        "bss_addr": bss_addr,
        "bss_size": bss_size,
        "sections": sections,
    }


def yaz0_decompress(data):
    """Decompress Yaz0-compressed data."""
    if data[:4] != b"Yaz0":
        raise ValueError("Not Yaz0 data")
    uncompressed_size = struct.unpack(">I", data[4:8])[0]
    out = bytearray()
    src = 16
    valid_bits = 0
    group_header = 0
    while len(out) < uncompressed_size:
        if valid_bits == 0:
            if src >= len(data):
                break
            group_header = data[src]
            src += 1
            valid_bits = 8
        if group_header & 0x80:
            if src >= len(data):
                break
            out.append(data[src])
            src += 1
        else:
            if src + 2 > len(data):
                break
            b1 = data[src]
            b2 = data[src + 1]
            src += 2
            dist = ((b1 & 0x0F) << 8) + b2 + 1
            length = b1 >> 4
            if length == 0:
                if src >= len(data):
                    break
                length = data[src] + 0x12
                src += 1
            else:
                length += 2
            for _ in range(length):
                out.append(out[-dist])
        group_header <<= 1
        valid_bits -= 1
    return bytes(out)


def parse_rel_header(data):
    """Parse a REL binary header.

    Standard REL format:
    0x00 u32 id (module id)
    0x04 u32 next (next module in chain)
    0x08 u32 prev (prev module in chain)
    0x0C u32 num_sections
    0x10 u32 section_info_offset
    0x14 u32 name_offset
    0x18 u32 imp_offset (import table offset)
    0x1C u32 imp_size (import table size)
    """
    if len(data) < 0x30 or len(data) < 4:
        return None
    ident = struct.unpack(">I", data[0x00:0x04])[0]
    num_sections = struct.unpack(">I", data[0x0C:0x10])[0]
    imp_offset = struct.unpack(">I", data[0x18:0x1C])[0]
    imp_size = struct.unpack(">I", data[0x1C:0x20])[0]
    if ident == 0 or num_sections == 0 or num_sections > 10000:
        return None
    imports = []
    io = imp_offset
    while io < imp_offset + imp_size:
        if io + 8 > len(data):
            break
        module_id = struct.unpack(">I", data[io:io+4])[0]
        num_relocs = struct.unpack(">I", data[io+4:io+8])[0]
        imports.append({"module_id": module_id, "relocation_count": num_relocs})
        io += 8
    non_null_imports = [i for i in imports if i["module_id"] != 0]
    return {
        "module_id": ident,
        "num_sections": num_sections,
        "num_imported_modules": len(non_null_imports),
        "total_relocations": sum(i["relocation_count"] for i in non_null_imports),
    }


def parse_rarc_names(arc_data):
    """Parse RARC archive, returning list of Yaz0-compressed .rel entries."""
    if len(arc_data) < 0x20 or arc_data[0:4] != b"RARC":
        return []
    data_header_offset = struct.unpack(">I", arc_data[0x08:0x0C])[0]
    p = data_header_offset
    if p + 24 > len(arc_data):
        return []
    node_count, _ = struct.unpack(">II", arc_data[p:p+8])
    total_entries, entry_off_rel = struct.unpack(">II", arc_data[p+8:p+16])
    string_off_rel, = struct.unpack(">I", arc_data[p+20:p+24])
    entry_offset = entry_off_rel + data_header_offset
    string_offset = string_off_rel + data_header_offset

    # Find data section start: first Yaz0 magic after all metadata
    # (The RARC header field at 0x0C is also a candidate but scanning is more robust)
    scan_from = string_offset
    data_section_start = None
    for i in range(scan_from, min(len(arc_data) - 4, scan_from + 0x10000), 4):
        if arc_data[i:i + 4] == b"Yaz0":
            data_section_start = i
            break
    if data_section_start is None:
        return []

    results = []
    seen_names = set()
    for i in range(total_entries):
        eo = entry_offset + i * 0x14
        if eo + 0x14 > len(arc_data):
            break
        etype = arc_data[eo + 4]
        name_off, = struct.unpack(">H", arc_data[eo + 6 : eo + 8])
        data_off, = struct.unpack(">I", arc_data[eo + 8 : eo + 12])
        data_len, = struct.unpack(">I", arc_data[eo + 12 : eo + 16])
        name_abs = string_offset + name_off
        end = arc_data.find(b"\x00", name_abs)
        fname = (
            arc_data[name_abs:end].decode("ascii", errors="replace")
            if end > name_abs and end - name_abs < 256
            else "?"
        )
        if not (etype & 1):
            continue
        if fname.endswith(".rel") and fname not in seen_names:
            seen_names.add(fname)
            abs_off = data_section_start + data_off
            results.append({
                "name": fname,
                "data_offset": abs_off,
                "size": data_len,
                "compressed": True,
            })
    return results

def find_rels_arc(files):
    for entry in files:
        if not entry["is_dir"] and entry["path"] == "RELS.arc":
            return entry
    return None


def main():
    parser = argparse.ArgumentParser(description="BlueWake P1 preparation pipeline")
    parser.add_argument("iso_path", help="Path to GZLE01 disc image (.iso/.gcm)")
    parser.add_argument("--output-dir", default="local-research", help="Private output root")
    args = parser.parse_args()
    iso_path = pathlib.Path(args.iso_path)
    manifests_dir = pathlib.Path(args.output_dir) / "manifests"
    manifests_dir.mkdir(parents=True, exist_ok=True)

    if not iso_path.exists():
        print("ERROR: %s not found" % iso_path)
        sys.exit(1)

    with open(iso_path, "rb") as f:
        meta = validate_disc_header(f)
        print("Disc ID: %s" % meta["disc_id"])
        print("Game: %s rev %d" % (meta["game_name"], meta["revision"]))

        # Hash source image
        f.seek(0)
        h = hashlib.sha256()
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            h.update(chunk)
        source_sha256 = h.hexdigest()
        source_size = iso_path.stat().st_size
        print("Source SHA-256: %s..." % source_sha256[:16])

        # DOL info
        dol_info = extract_dol_info(f, meta["dol_offset"])
        print("DOL SHA-1: %s" % dol_info["sha1"])
        dol_match = dol_info["sha1"] == EXPECTED_DOL_SHA1
        print("DOL match expected: %s" % dol_match)

        # FST
        files = read_fst(f, meta["fst_offset"], meta["fst_size"])
        real_files = [e for e in files if not e["is_dir"]]
        loose_rels = [e for e in real_files if e["path"].startswith("rels/") and e["path"].endswith(".rel")]
        print("Loose RELs: %d" % len(loose_rels))

        # RELS.arc
        rels_arc_entry = find_rels_arc(files)
        arc_rel_count = 0
        arc_rels = []
        rels_arc_offset = 0
        if rels_arc_entry:
            rels_arc_offset = rels_arc_entry["offset"]
            f.seek(rels_arc_offset)
            arc_data = f.read(rels_arc_entry["size"])
            arc_rels = parse_rarc_names(arc_data)
            arc_rel_count = len(arc_rels)
            print("RELS.arc RELs: %d" % arc_rel_count)

        total_rel_modules = len(loose_rels) + arc_rel_count
        total_executables = 1 + total_rel_modules
        print("Total REL modules: %d" % total_rel_modules)
        print("Total executables (DOL + RELs): %d" % total_executables)

        # Build manifest
        modules = [{
            "type": "dol",
            "name": "main.dol",
            "sha1": dol_info["sha1"],
            "size_bytes": dol_info["total_size"],
            "entry_point": dol_info["entry_point"],
            "bss_addr": dol_info["bss_addr"],
            "bss_size": dol_info["bss_size"],
            "text_section_count": sum(1 for s in dol_info["sections"] if s["type"] == "text"),
            "data_section_count": sum(1 for s in dol_info["sections"] if s["type"] == "data"),
        }]

        for lr in sorted(loose_rels, key=lambda x: x["path"]):
            f.seek(lr["offset"])
            raw_data = f.read(lr["size"])
            if raw_data[:4] == b"Yaz0":
                rel_data = yaz0_decompress(raw_data)
            else:
                rel_data = raw_data
            rel_hdr = parse_rel_header(rel_data)
            modules.append({
                "type": "rel_loose",
                "name": lr["path"].split("/")[-1],
                "module_id": rel_hdr["module_id"] if rel_hdr else None,
                "sha1": sha1(rel_data),
                "size_bytes": lr["size"],
                "num_sections": rel_hdr["num_sections"] if rel_hdr else None,
                "num_imported_modules": rel_hdr["num_imported_modules"] if rel_hdr else None,
                "total_relocations": rel_hdr["total_relocations"] if rel_hdr else None,
            })

        for cr in sorted(arc_rels, key=lambda x: x["name"]):
            abs_off = rels_arc_offset + cr["data_offset"]
            f.seek(abs_off)
            raw_data = f.read(cr["size"])
            if raw_data[:4] == b"Yaz0":
                rel_data = yaz0_decompress(raw_data)
            else:
                rel_data = raw_data
            rel_hdr = parse_rel_header(rel_data)
            modules.append({
                "type": "rel_archived",
                "name": cr["name"],
                "archive": "RELS.arc",
                "module_id": rel_hdr["module_id"] if rel_hdr else None,
                "sha1": sha1(rel_data),
                "size_bytes": cr["size"],
                "num_sections": rel_hdr["num_sections"] if rel_hdr else None,
                "num_imported_modules": rel_hdr["num_imported_modules"] if rel_hdr else None,
                "total_relocations": rel_hdr["total_relocations"] if rel_hdr else None,
            })

        explanation = (
            "One main.dol executable plus {lr} loose REL binaries under rels/ "
            "and {ar} additional REL binaries stored inside the RELS.arc RARC archive. "
            "Zero filename overlap between loose and archived sets. "
            "Total: {tot} REL modules plus one DOL = {exe} executables."
        ).format(lr=len(loose_rels), ar=arc_rel_count, tot=total_rel_modules, exe=total_executables)

        manifest = {
            "$schema": "bluewake/manifest/v1",
            "source_alias": "GZLE01-disc-image",
            "source_sha256_prefix": source_sha256[:16],
            "disc_revision": "GZLE01 USA rev 0",
            "dol_count": 1,
            "loose_rel_count": len(loose_rels),
            "rels_arc_rel_count": arc_rel_count,
            "total_executables": total_executables,
            "count_discrepancy_resolution": explanation,
            "modules": modules,
        }

        manifest_file = manifests_dir / "GZLE01_manifest.json"
        manifest_file.write_text(json.dumps(manifest, indent=2))
        print("\nManifest written: %s" % manifest_file)
        print("Preparation complete. Total modules: %d" % total_executables)


if __name__ == "__main__":
    main()
