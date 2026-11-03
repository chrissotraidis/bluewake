#!/usr/bin/env python3
"""Synthetic fixture tests for generate_composite.py (P3 Route A)."""
import hashlib
import re
import struct
import sys
import tempfile
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parent.parent / "scripts"
sys.path.insert(0, str(SCRIPTS_DIR))

import generate_composite as gc

DOL_TEXT_ADDR = 0x80003100
REL_BASE = 0xC0400000


def build_dol(text_data):
    hdr = bytearray(0x100)
    struct.pack_into(">I", hdr, 0x00, 0x100)
    struct.pack_into(">I", hdr, 0x48, DOL_TEXT_ADDR)
    struct.pack_into(">I", hdr, 0x90, len(text_data))
    return bytes(hdr) + text_data


def make_generated_h(chunks, entry_point=None):
    lines = ["// synthetic"]
    for start, end in chunks:
        lines.append(f"if (address >= 0x{start:08X}u && address < 0x{end:08X}u)")
        lines.append("    return true;")
    if entry_point is None:
        ep = chunks[0][0] + 0x40 if chunks else DOL_TEXT_ADDR
    else:
        ep = entry_point
    lines.append(f"#define DOLRECOMP_ENTRY_POINT 0x{ep:08X}u")
    for start, _ in chunks:
        lines.append(f"void func_{start:08X}(CPUState* ctx);")
    return "\n".join(lines) + "\n"


def write_dol_dir(d, chunks, native=False):
    d.mkdir(parents=True, exist_ok=True)
    (d / "generated.h").write_text(make_generated_h(chunks))
    (d / "generated_smc.txt").write_text("")
    cd = d / "chunks"
    cd.mkdir(exist_ok=True)
    for start, _ in chunks:
        suffix = ".o" if native else ".c"
        path = cd / f"chunk_{start:08X}{suffix}"
        if native:
            path.write_bytes(b"synthetic native object")
        else:
            path.write_text('#include "../generated.h"\n')


def write_dol_binary(root, text_bytes):
    root.mkdir(parents=True, exist_ok=True)
    (root / "main.dol").write_bytes(build_dol(text_bytes))


def build_rel(module_id, sections_data, imports=None):
    sec_count = len(sections_data)
    fixed_hdr_size = 0x1C
    header_size = fixed_hdr_size + sec_count * 8
    imp_offset = header_size
    imp_size = sum(8 + len(v) * 4 for v in (imports or {}).values())
    hdr = bytearray(fixed_hdr_size)
    struct.pack_into(">II", hdr, 0x00, module_id, 1)
    struct.pack_into(">I", hdr, 0x0C, sec_count)
    struct.pack_into(">I", hdr, 0x10, fixed_hdr_size)
    struct.pack_into(">I", hdr, 0x14, imp_size)
    sec_table = bytearray()
    body = bytearray()
    for exec_flag, _, data in sections_data:
        off = header_size + imp_size + len(body)
        body += data
        while len(body) % 4:
            body += b"\x00"
        raw = off | (1 if exec_flag else 0)
        sec_table += struct.pack(">II", raw, len(data))
    imp_table = bytearray()
    for mid in sorted(imports or {}):
        entries = imports[mid]
        imp_table += struct.pack(">II", mid, len(entries) + 1)
        imp_table += struct.pack(">I", 0)
        for e in entries:
            imp_table += e
    result = bytes(hdr[:header_size]) + bytes(sec_table)
    while len(result) < imp_offset:
        result += b"\x00"
    return result + bytes(imp_table) + bytes(body)


def write_rel_module(root, mid, chunks):
    mod_dir = root / f"d_a_test_{mid}"
    mod_dir.mkdir(parents=True)
    ep_rel = min(c[0] for c in chunks) if chunks else REL_BASE
    (mod_dir / "generated.h").write_text(make_generated_h(chunks, entry_point=ep_rel))
    cd = mod_dir / "chunks"
    cd.mkdir(exist_ok=True)
    for start, _ in chunks:
        (cd / f"chunk_{start:08X}.c").write_text('#include "../generated.h"\n')


def setup_fixture(td, dol_chunks=None, rel_specs=None):
    td = Path(td)
    if dol_chunks is None:
        dol_chunks = [(0x80003100, 0x80003140), (0x80003140, 0x80003180)]
    max_end = max(e for _, e in dol_chunks)
    text = bytes((i * 7 + 0x90) & 0xFF for i in range(max_end - DOL_TEXT_ADDR))
    write_dol_binary(td / "input", text)
    write_dol_dir(td / "dol_out", dol_chunks)
    rb = td / "rels_bin"; rb.mkdir(exist_ok=True)
    ro = td / "rels_out"; ro.mkdir(exist_ok=True)
    for spec in (rel_specs or []):
        mid = spec["module_id"]
        chunks = spec["chunks"]
        lo = min(c[0] for c in chunks); hi = max(c[1] for c in chunks)
        exec_data = bytes(((i * 13 + 0x42) & 0xFF) for i in range(hi - lo))
        (rb / f"{mid:04d}.rel").write_bytes(build_rel(mid, [(True, None, exec_data)]))
        write_rel_module(ro, mid, chunks)
    return td


def run_generator(td, expect_fail=False):
    import subprocess
    cmd = [sys.executable, str(SCRIPTS_DIR / "generate_composite.py"),
           "--dol-dir", str(Path(td)/"dol_out"), "--rels-dir", str(Path(td)/"rels_out"),
           "--rels-bin-dir", str(Path(td)/"rels_bin"), "--main-dol", str(Path(td)/"input"/"main.dol"),
           "--output-dir", str(Path(td)/"composite")]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if expect_fail:
        assert r.returncode != 0, f"expected failure:\nstdout={r.stdout}\nstderr={r.stderr}"
        return r
    assert r.returncode == 0, f"generator failed:\nstdout={r.stdout}\nstderr={r.stderr}"
    return r


def test_single_dol_no_rels():
    """Generator produces correct output from DOL-only input."""
    chunks = [(0x80003100, 0x80003140), (0x80003140, 0x80003200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, dol_chunks=chunks)
        run_generator(td)
        out = Path(td) / "composite"
        h = (out / "generated_composite.h").read_text()
        for start, _ in chunks:
            assert f"func_{start:08X}" in h
        assert "dolrecomp_block_can_precharge" in h
        assert "ctx->cycle_deadline_budget + ctx->downcount" in h
        assert "dolrecomp_refund_cycle_suffix" in h
        assert "ctx->downcount += (s64)ctx->cycle_observation_suffix" in h
        # The dispatch cache is pc-keyed and exact, and the dispatcher is split
        # so the path every block boundary takes carries no cold code. Both are
        # measured properties of the emitted text (docs/archive/status/CURRENT.md,
        # 2026-09-22); the reason they are checked here is that a cache which
        # answers for a pc it never resolved is a boot failure, not a slowdown,
        # and a dispatcher that lost its alias fallbacks is a wrong answer.
        assert "DOLRECOMP_PC_CACHE_SIZE = 4096u" in h
        assert "((address >> 2) ^ (address >> 14))" in h
        assert "if (s_cached_pc[cache_index] == address)" in h
        assert "return s_cached_pc_fn[cache_index];" in h
        assert "s_cached_pc[cache_index] = address;" in h
        assert "s_cached_pc_fn[cache_index] = s_fns[idx];" in h
        assert "s_cached_starts" not in h and "s_cached_ends" not in h
        assert "dolrecomp_call_slow" in h
        assert "return dolrecomp_call_slow(ctx, address);" in h
        assert "dolrecomp_physical_pc_alias(ctx, address, &alias)" in h
        tbl = (out / "module_tables.inc").read_text()
        for s, e in chunks:
            assert f"{{0x{s:08X}u, 0x{e:08X}u}}" in tbl
        assert "MODULE_CHUNK_RANGE_COUNT 2u" in tbl
        rm = (out / "rel_modules.inc").read_text()
        assert "MODULE_REL_MODULE_COUNT 0u" in rm
    print("PASS: test_single_dol_no_rels")


def test_native_dol_chunks_are_preserved():
    """LLVM DOL objects pass through while REL inputs remain C sources."""
    chunks = [(0x80003100, 0x80003140), (0x80003140, 0x80003200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, dol_chunks=chunks)
        dol_chunks = td / "dol_out" / "chunks"
        for source in dol_chunks.glob("*.c"):
            source.unlink()
        for start, _ in chunks:
            (dol_chunks / f"chunk_{start:08X}.o").write_bytes(
                b"synthetic native object")
        result = run_generator(td)
        out = td / "composite" / "chunks_dol"
        assert sorted(path.suffix for path in out.iterdir()) == [".o", ".o"]
        source = dol_chunks / "chunk_80003100.o"
        copied = out / "chunk_80003100.o"
        assert source.stat().st_ino == copied.stat().st_ino
        assert "DOL backend: native objects (2 chunks)" in result.stdout
    print("PASS: test_native_dol_chunks_are_preserved")


def test_mixed_dol_backends_are_rejected():
    """A stale output tree cannot silently link duplicate guest symbols."""
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td)
        chunks = td / "dol_out" / "chunks"
        (chunks / "stale.o").write_bytes(b"synthetic native object")
        result = run_generator(td, expect_fail=True)
        assert "exactly one of C or native object output" in result.stderr
    print("PASS: test_mixed_dol_backends_are_rejected")


def test_multi_section_rel():
    """REL module with exec section produces correct section table."""
    mid = 1
    b = REL_BASE + mid * 0x10000
    rc = [(b + 0x100, b + 0x200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, rel_specs=[{"module_id": mid, "chunks": rc}])
        run_generator(td)
        rm = (Path(td) / "composite" / "rel_modules.inc").read_text()
        assert f"s_rel_sec_{mid}[]" in rm
        assert "MODULE_REL_MODULE_COUNT 1u" in rm
    print("PASS: test_multi_section_rel")


def test_bss_section_gets_linked_address():
    """Zero-offset REL BSS follows its header alignment, not a fixed value."""
    rel = bytearray(0x48)
    struct.pack_into(">IIII", rel, 0, 7, 1, 0, 2)
    struct.pack_into(">I", rel, 0x10, 0x20)
    struct.pack_into(">II", rel, 0x20, 0x29, 4)  # executable, file-backed
    struct.pack_into(">II", rel, 0x28, 0, 12)  # zero-offset BSS
    struct.pack_into(">I", rel, 0x44, 8)  # BSS alignment
    rel.extend(b"ABCD")  # file size 0x4c, so BSS starts at 0x50
    sections = gc.parse_rel_sections(bytes(rel), 2, 0x20, REL_BASE)
    assert sections[0] == (0, 0xC0400028, 4)
    assert sections[1] == (1, 0xC0400050, 12)

    linked = gc.rel_linked_sections(bytes(rel), REL_BASE)
    assert linked[1]["linked_start"] == 0xC0400050
    print("PASS: test_bss_section_gets_linked_address")


def test_bss_zero_alignment_uses_four_byte_fallback():
    rel = bytearray(0x49)
    struct.pack_into(">IIII", rel, 0, 8, 1, 0, 1)
    struct.pack_into(">I", rel, 0x10, 0x20)
    struct.pack_into(">II", rel, 0x20, 0, 4)
    struct.pack_into(">I", rel, 0x44, 0)
    sections = gc.parse_rel_sections(bytes(rel), 1, 0x20, REL_BASE)
    assert sections[0] == (0, 0xC040004C, 4)
    print("PASS: test_bss_zero_alignment_uses_four_byte_fallback")


def test_rel_namespace_rejects_retail_memory():
    sections = {
        60: {
            "base": 0x81550000,
            "sections": [{
                "index": 6,
                "linked_start": 0x815581A0,
                "size": 0x214,
            }],
        }
    }
    try:
        gc.validate_rel_linked_namespace(sections)
    except ValueError as exc:
        assert "retail MEM1" in str(exc)
    else:
        raise AssertionError("retail-memory REL namespace was accepted")
    print("PASS: test_rel_namespace_rejects_retail_memory")


def test_tagged_rel_namespace_is_disjoint():
    sections = {
        60: {
            "base": 0xC1550000,
            "sections": [{
                "index": 6,
                "linked_start": 0xC15581A0,
                "size": 0x214,
            }],
        }
    }
    gc.validate_rel_linked_namespace(sections)
    print("PASS: test_tagged_rel_namespace_is_disjoint")


def test_chunk_hash_correctness():
    """FNV-1a hashes match original DOL/REL bytes for each chunk."""
    dc = [(0x80003100, 0x80003140)]
    mid = 5
    b5 = REL_BASE + mid * 0x10000
    rc = [(b5 + 0x100, b5 + 0x200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, dol_chunks=dc, rel_specs=[{"module_id": mid, "chunks": rc}])
        run_generator(td)
        gen_h_text = (Path(td) / "rels_out" / f"d_a_test_{mid}" / "generated.h").read_text()
        ep_rel = int(re.search(r"DOLRECOMP_ENTRY_POINT 0x([0-9A-F]{8})u", gen_h_text).group(1), 16)
        tbl = (Path(td) / "composite" / "module_tables.inc").read_text()
        hashes = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{16})ull,", tbl)]
        assert len(hashes) == 2, f"expected 2 hashes, got {len(hashes)}"
        # DOL chunk hash
        dol_bin = (Path(td) / "input" / "main.dol").read_bytes()
        s, e = dc[0]
        lo = 0x100 + (s - DOL_TEXT_ADDR)
        assert hashes[0] == gc.fnv1a64(dol_bin[lo : lo + (e - s)]), "DOL hash mismatch"
        # REL chunk hash
        rel_bin = (Path(td) / "rels_bin" / f"{mid:04d}.rel").read_bytes()
        nsec = struct.unpack_from(">I", rel_bin, 0xC)[0]
        sio = struct.unpack_from(">I", rel_bin, 0x10)[0]
        for i in range(nsec):
            raw_off, sz = struct.unpack_from(">II", rel_bin, sio + i * 8)
            if raw_off & 1 and sz > 0:
                off = raw_off & ~1
                lo2 = off + (rc[0][0] - ep_rel)
                data = rel_bin[lo2 : lo2 + (rc[0][1] - rc[0][0])]
                assert hashes[-1] == gc.fnv1a64(data), "REL hash mismatch"
                break
    print("PASS: test_chunk_hash_correctness")


def test_dispatch_sorted_no_overlap():
    """Chunk starts are sorted and non-overlapping across DOL and RELs."""
    dc = [(0x80003100, 0x80003140), (0x80003140, 0x80003200)]
    specs = []
    for mid in (3, 7):
        b = REL_BASE + mid * 0x10000
        specs.append({"module_id": mid,
                      "chunks": [(b + 0x100, b + 0x200), (b + 0x200, b + 0x300)]})
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, dol_chunks=dc, rel_specs=specs)
        run_generator(td)
        h = (Path(td) / "composite" / "generated_composite.h").read_text()
        # The probe is pc-keyed, so the range test it used to carry lives in the
        # miss path; what this asserts is that the emitted probe still tests the
        # entry it is about to use, and that the tables it searches are sorted and
        # disjoint - which is the invariant the chunk dispatch depends on.
        assert "s_cached_pc[cache_index] == address" in h
        assert "s_cached_pc_fn[cache_index]" in h
        starts_m = re.search(r"static const u32 s_starts\[\] = \{(.*?)\};", h, re.S)
        ends_m = re.search(r"static const u32 s_ends\[\] = \{(.*?)\};", h, re.S)
        assert starts_m and ends_m
        starts = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{8})u", starts_m.group(1))]
        ends = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{8})u", ends_m.group(1))]
        assert starts == sorted(starts)
        for i in range(len(starts) - 1):
            assert starts[i + 1] >= ends[i], f"overlap at {starts[i]:#010x}"
    print("PASS: test_dispatch_sorted_no_overlap")


def test_overlapping_rejected():
    """Two RELs claiming the same address cause generator failure."""
    shared_lo = REL_BASE + 0x10000 + 0x100
    specs_a = [{"module_id": 1, "chunks": [(shared_lo, shared_lo + 0x100)]}]
    specs_b = [{"module_id": 2, "chunks": [(shared_lo, shared_lo + 0x80)]}]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, rel_specs=specs_a + specs_b)
        r = run_generator(td, expect_fail=True)
        assert "overlap" in r.stderr.lower(), f"got: {r.stderr}"
    print("PASS: test_overlapping_rejected")


def test_missing_rel_binary_fails():
    """Missing .rel binary when a REL module dir exists causes failure."""
    mid = 99
    b = REL_BASE + mid * 0x10000
    rc = [(b + 0x100, b + 0x200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, rel_specs=[{"module_id": mid, "chunks": rc}])
        (Path(td) / "rels_bin" / f"{mid:04d}.rel").unlink()
        r = run_generator(td, expect_fail=True)
        assert ".rel" in r.stderr.lower(), f"got: {r.stderr}"
    print("PASS: test_missing_rel_binary_fails")


def test_address_reuse():
    """Two different RELs can occupy the same address slot in separate builds."""
    shared = REL_BASE + 0x10000
    for mid in (1, 2):
        with tempfile.TemporaryDirectory() as td:
            td = setup_fixture(td, rel_specs=[{"module_id": mid,
                             "chunks": [(shared + 0x100, shared + 0x200)]}])
            run_generator(td)
            tbl = (Path(td) / "composite" / "module_tables.inc").read_text()
            assert f"0x{shared + 0x100:08X}u" in tbl
    print("PASS: test_address_reuse")


def test_determinism():
    """Two runs on identical input produce byte-identical output."""
    dc = [(0x80003100, 0x80003140)]
    mid = 4; b = REL_BASE + mid * 0x10000
    rc = [(b + 0x100, b + 0x200)]
    d1, d2 = {}, {}
    for store in (d1, d2):
        with tempfile.TemporaryDirectory() as td:
            td = setup_fixture(td, dol_chunks=dc, rel_specs=[{"module_id": mid, "chunks": rc}])
            run_generator(td)
            out = Path(td) / "composite"
            for f in ["generated_composite.h", "module_tables.inc", "rel_modules.inc"]:
                store[f] = hashlib.sha256((out / f).read_bytes()).hexdigest()
    assert d1 == d2, f"non-deterministic"
    print("PASS: test_determinism")

def test_indirect_call_lookup():
    """Each aligned address within a chunk maps to at most one entry."""
    dc = [(0x80003100, 0x80003140), (0x80003140, 0x80003200)]
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, dol_chunks=dc)
        run_generator(td)
        h = (Path(td) / "composite" / "generated_composite.h").read_text()
        starts_m = re.search(r"static const u32 s_starts\[\] = \{(.*?)\};", h, re.S)
        ends_m = re.search(r"static const u32 s_ends\[\] = \{(.*?)\};", h, re.S)
        starts = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{8})u", starts_m.group(1))]
        ends = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{8})u", ends_m.group(1))]
        for probe in [0x80003100, 0x80003104, 0x8000313C, 0x80003140, 0x800031FC]:
            m = [i for i in range(len(starts)) if starts[i] <= probe < ends[i]]
            assert len(m) <= 1
            if m:
                assert (probe - starts[m[0]]) % 4 == 0
    print("PASS: test_indirect_call_lookup")


def test_lifecycle_metadata_complete():
    """All REL modules appear in rel_modules.inc with section tables."""
    specs = []
    for mid in (1, 2, 3):
        b = REL_BASE + mid * 0x10000
        specs.append({"module_id": mid,
                      "chunks": [(b + 0x100, b + 0x200), (b + 0x200, b + 0x280)]})
    with tempfile.TemporaryDirectory() as td:
        td = setup_fixture(td, rel_specs=specs)
        run_generator(td)
        rm = (Path(td) / "composite" / "rel_modules.inc").read_text()
        assert "MODULE_REL_MODULE_COUNT 3u" in rm
        for mid in (1, 2, 3):
            assert f"s_rel_sec_{mid}[]" in rm
    print("PASS: test_lifecycle_metadata_complete")


def test_import_across_modules():
    """A REL with imports still generates correctly alongside its dependency."""
    imp_mid, dep_mid = 10, 11
    bi = REL_BASE + imp_mid * 0x10000
    bd = REL_BASE + dep_mid * 0x10000
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        write_dol_binary(td / "input", b"\x90" * 0x40)
        write_dol_dir(td / "dol_out", [(0x80003100, 0x80003140)])
        rb = td / "rels_bin"; rb.mkdir(exist_ok=True)
        ro = td / "rels_out"; ro.mkdir(exist_ok=True)
        dep_exec = b"\xAB" * 0x100
        dep_chunks = [(bd + 0x100, bd + 0x200)]
        (rb / f"{dep_mid:04d}.rel").write_bytes(build_rel(dep_mid, [(True, None, dep_exec)]))
        write_rel_module(ro, dep_mid, dep_chunks)
        entries = [struct.pack(">I", (6 << 26) | (0 << 16) | 0x100),
                   struct.pack(">I", (5 << 26) | (0 << 16) | 0x104),
                   struct.pack(">I", (4 << 26) | (0 << 16) | 0x108)]
        imp_exec = b"\xCD" * 0x100
        imp_chunks = [(bi + 0x100, bi + 0x200)]
        (rb / f"{imp_mid:04d}.rel").write_bytes(
            build_rel(imp_mid, [(True, None, imp_exec)], imports={dep_mid: entries}))
        write_rel_module(ro, imp_mid, imp_chunks)
        run_generator(td)
        rm = (td / "composite" / "rel_modules.inc").read_text()
        assert "MODULE_REL_MODULE_COUNT 2u" in rm
    print("PASS: test_import_across_modules")


def test_zero_length_section():
    """A REL section with size 0 is handled without error."""
    mid = 20; b = REL_BASE + mid * 0x10000
    chunks = [(b + 0x100, b + 0x200)]
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        write_dol_binary(td / "input", b"\x90" * 0x40)
        write_dol_dir(td / "dol_out", [(0x80003100, 0x80003140)])
        rb = td / "rels_bin"; rb.mkdir(exist_ok=True)
        ro = td / "rels_out"; ro.mkdir(exist_ok=True)
        exec_data = b"\xEF" * 0x100
        (rb / f"{mid:04d}.rel").write_bytes(
            build_rel(mid, [(False, 0, b""), (True, None, exec_data)]))
        write_rel_module(ro, mid, chunks)
        run_generator(td)
        rm = (td / "composite" / "rel_modules.inc").read_text()
        assert f"s_rel_sec_{mid}[]" in rm
    print("PASS: test_zero_length_section")


def main():
    tests = [
        test_single_dol_no_rels,
        test_native_dol_chunks_are_preserved,
        test_mixed_dol_backends_are_rejected,
        test_multi_section_rel,
        test_bss_section_gets_linked_address,
        test_bss_zero_alignment_uses_four_byte_fallback,
        test_rel_namespace_rejects_retail_memory,
        test_tagged_rel_namespace_is_disjoint,
        test_chunk_hash_correctness,
        test_dispatch_sorted_no_overlap,
        test_overlapping_rejected,
        test_missing_rel_binary_fails,
        test_address_reuse,
        test_determinism,
        test_indirect_call_lookup,
        test_lifecycle_metadata_complete,
        test_import_across_modules,
        test_zero_length_section,
    ]
    failures = 0
    for t in tests:
        try:
            t()
        except Exception as e:
            import traceback; traceback.print_exc()
            print(f"FAIL: {t.__name__}: {e}")
            failures += 1
    print(f"\n{len(tests)-failures}/{len(tests)} PASS")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
