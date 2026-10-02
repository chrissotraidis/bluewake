#!/usr/bin/env python3
"""Deterministic composite module generator for BlueWake (P3 Route A).

Merges DOL + all REL DolRecomp translation outputs into a single composite
dispatcher header plus ABI tables suitable for building one native module.
Deterministic: identical inputs produce byte-identical outputs.
"""
import argparse
import hashlib
import os
import re
import struct
import sys
from pathlib import Path

FNV64_OFFSET = 0xCBF29CE484222325
FNV64_PRIME = 0x100000001B3
RETAIL_MEM1_START = 0x80000000
RETAIL_MEM1_END = 0x81800000
RAW_REL_STAGING_START = 0x81F80000
RAW_REL_STAGING_END = 0x82000000
MASK64 = (1 << 64) - 1


def fnv1a64(data):
    h = FNV64_OFFSET
    for b in data:
        h = ((h ^ b) * FNV64_PRIME) & MASK64
    return h


def merge_ranges(ranges):
    merged = []
    for start, end in sorted(ranges):
        if start >= end:
            continue
        if not merged or merged[-1][1] < start:
            merged.append([start, end])
        elif end > merged[-1][1]:
            merged[-1][1] = end
    return [(a, b) for a, b in merged]


def load_dol_text(dol_path):
    dol = Path(dol_path).read_bytes()

    def be32(off):
        return int.from_bytes(dol[off : off + 4], "big")

    sections = []
    for i in range(18):  # 7 text + 11 data share the layout
        file_off = be32(0x00 + i * 4)
        address = be32(0x48 + i * 4)
        size = be32(0x90 + i * 4)
        if file_off and address and size:
            sections.append((address, size, file_off))

    def read_range(start, end):
        for address, size, file_off in sections:
            if address <= start and end <= address + size:
                lo = file_off + (start - address)
                return dol[lo : lo + (end - start)]
        raise ValueError(f"range [{start:#010x},{end:#010x}) not inside DOL section")

    return read_range


def parse_generated_h(header_text):
    """Parse one generated.h into (code_ranges, sorted func addresses)."""
    code_ranges = set()
    for base_s, limit_s in re.findall(
        r"address >= (0x[0-9A-Fa-f]+)u && address < (0x[0-9A-Fa-f]+)u", header_text
    ):
        code_ranges.add((int(base_s, 16), int(limit_s, 16)))

    # Offset-table runs: "address - BASEu" followed by "offset < SPANu"
    for m in re.finditer(r"u32\s+offset\s*=\s*address\s*-\s*(0x[0-9A-Fa-f]+)u", header_text):
        base = int(m.group(1), 16)
        tail = header_text[m.end() : m.end() + 300]
        span_m = re.search(r"offset < (0x[0-9A-Fa-f]+)u", tail)
        if span_m:
            span = int(span_m.group(1), 16)
            code_ranges.add((base, base + span))

    func_addrs = sorted(
        int(a, 16)
        for a in re.findall(r"void func_([0-9A-Fa-f]{8})\(CPUState\* ctx\);", header_text)
    )
    return merge_ranges(code_ranges), func_addrs


def chunk_ranges_from(funcs, ranges):
    """Each func covers up to the next func within the same containing range."""
    out = []
    for i, addr in enumerate(funcs):
        containing = None
        for cr in ranges:
            if cr[0] <= addr < cr[1]:
                containing = cr
                break
        if containing is None:
            print(f"ERROR: func_{addr:08X} outside all code ranges", file=sys.stderr)
            sys.exit(1)
        end = containing[1]
        if i + 1 < len(funcs) and containing[0] <= funcs[i + 1] < containing[1]:
            end = funcs[i + 1]
        out.append((addr, end))
    return out


def parse_rel_header(data):
    module_id, version = struct.unpack_from(">II", data, 0)
    section_count = struct.unpack_from(">I", data, 0xC)[0]
    sio = struct.unpack_from(">I", data, 0x10)[0]
    return {
        "module_id": module_id,
        "version": version,
        "section_count": section_count,
        "section_info_offset": sio,
        "file_size": len(data),
    }


def rel_bss_alignment(data):
    """Return the REL header's BSS alignment using DolRecomp's fallback."""
    alignment = struct.unpack_from(">I", data, 0x44)[0] if len(data) >= 0x48 else 4
    return alignment or 4


def align_up(value, alignment):
    return (value + alignment - 1) // alignment * alignment


def parse_rel_sections(data, section_count, sio_offset, linked_base):
    sections = []
    bss_alignment = rel_bss_alignment(data)
    bss_offset = align_up(len(data), bss_alignment)
    for i in range(section_count):
        raw_off, size = struct.unpack_from(">II", data, sio_offset + i * 8)
        exec_flag = raw_off & 1
        offset = raw_off & ~1
        if size == 0:
            linked_start = 0
        elif offset:
            linked_start = linked_base + offset
        else:
            # REL BSS sections have no file offset. Match DolRecomp's
            # header-defined placement because generated code references
            # these linked addresses directly.
            linked_start = linked_base + bss_offset
            bss_offset = align_up(bss_offset + size, bss_alignment)
        sections.append((i, linked_start, size))
    return sections


def find_rel_binary(rels_bin_dir, module_id):
    """Find .rel binary matching module_id by reading its header."""
    for bp in sorted(Path(rels_bin_dir).iterdir()):
        if bp.suffix != ".rel":
            continue
        with open(bp, "rb") as f:
            mid = struct.unpack(">I", f.read(4))[0]
        if mid == module_id:
            return bp
    return None


def read_rel_exec_range(rel_data, start, end, linked_base):
    nsec = struct.unpack_from(">I", rel_data, 0xC)[0]
    sio = struct.unpack_from(">I", rel_data, 0x10)[0]
    for i in range(nsec):
        raw_off, size = struct.unpack_from(">II", rel_data, sio + i * 8)
        exec_flag = raw_off & 1
        offset = raw_off & ~1
        if exec_flag and size > 0 and linked_base <= start and end <= linked_base + size:
            lo = offset + (start - linked_base)
            return rel_data[lo : lo + (end - start)]
    raise ValueError(f"REL exec range [{start:#010x},{end:#010x}) not found")


def rel_linked_sections(rel_data, base):
    """Return section metadata and mutable file-backed section images."""
    nsec = struct.unpack_from(">I", rel_data, 0xC)[0]
    sio = struct.unpack_from(">I", rel_data, 0x10)[0]
    sections = []
    bss_alignment = rel_bss_alignment(rel_data)
    bss_offset = align_up(len(rel_data), bss_alignment)
    for i in range(nsec):
        raw_off, size = struct.unpack_from(">II", rel_data, sio + i * 8)
        offset = raw_off & ~1
        if size == 0:
            linked_start = 0
        elif offset:
            linked_start = base + offset
        else:
            linked_start = base + bss_offset
            bss_offset = align_up(bss_offset + size, bss_alignment)
        sections.append({
            "index": i,
            "offset": offset,
            "size": size,
            "executable": bool(raw_off & 1),
            "linked_start": linked_start,
            "bytes": bytearray(rel_data[offset:offset + size]) if size and offset else None,
        })
    return sections


def ranges_overlap(start, size, reserved_start, reserved_end):
    return size > 0 and start < reserved_end and start + size > reserved_start


def validate_rel_linked_namespace(section_maps):
    """Reject build-time REL storage that aliases authentic guest memory."""
    reserved = (
        ("retail MEM1", RETAIL_MEM1_START, RETAIL_MEM1_END),
        ("raw REL staging", RAW_REL_STAGING_START, RAW_REL_STAGING_END),
    )
    for mid, module in section_maps.items():
        for section in module["sections"]:
            start = section["linked_start"]
            size = section["size"]
            for label, reserved_start, reserved_end in reserved:
                if ranges_overlap(start, size, reserved_start, reserved_end):
                    raise ValueError(
                        f"REL module {mid} section {section['index']} "
                        f"linked range [{start:#010x}, {start + size:#010x}) "
                        f"overlaps {label} [{reserved_start:#010x}, "
                        f"{reserved_end:#010x})"
                    )


def apply_rel_data_relocations(rel_bins, section_maps):
    """Apply authentic REL relocations to file-backed non-executable sections.

    DolRecomp emits executable sections as C, so this image pass deliberately
    leaves code patches to the code-generation/runtime path while making data
    imports deterministic and inspectable.
    """
    for mid, rel_data in rel_bins.items():
        if rel_data is None or mid not in section_maps:
            continue
        base = section_maps[mid]["base"]
        import_offset, import_size = struct.unpack_from(">II", rel_data, 0x28)
        # Small synthetic REL fixtures used by the public composite tests may
        # omit a real import stream; their zero-filled header fields are not a
        # request to interpret arbitrary bytes as relocations.
        if (import_size == 0 or import_size % 8 or
                import_offset > len(rel_data) or
                import_size > len(rel_data) - import_offset):
            continue
        for pos in range(import_offset, import_offset + import_size, 8):
            import_mid, cursor = struct.unpack_from(">II", rel_data, pos)
            if import_mid != 0 and import_mid not in section_maps:
                continue
            if cursor < 0 or cursor > len(rel_data) - 8:
                continue
            current_section = 0
            current_offset = 0
            while True:
                if cursor > len(rel_data) - 8:
                    break
                delta, rtype, target_section, symbol = struct.unpack_from(">HBBI", rel_data, cursor)
                cursor += 8
                if rtype == 202:  # R_DOLPHIN_SECTION
                    current_section = target_section
                    current_offset = 0
                    continue
                if rtype == 203:  # R_DOLPHIN_END
                    break
                current_offset += delta
                if rtype == 201:  # R_DOLPHIN_NOP
                    continue
                patch = section_maps[mid]["sections"][current_section]
                if patch["bytes"] is None or current_offset + 4 > patch["size"]:
                    continue
                if import_mid == 0:
                    target = symbol
                else:
                    target_sec = section_maps[import_mid]["sections"][target_section]
                    target = target_sec["linked_start"] + symbol
                raw = struct.unpack_from(">I", patch["bytes"], current_offset)[0]
                if rtype == 1:
                    value = target
                    struct.pack_into(">I", patch["bytes"], current_offset, value & 0xFFFFFFFF)
                elif rtype in (3, 4, 5, 6):
                    value = target
                    if rtype == 5:
                        value >>= 16
                    elif rtype == 6:
                        value = (value + 0x8000) >> 16
                    struct.pack_into(">H", patch["bytes"], current_offset, value & 0xFFFF)
                elif rtype == 10:
                    patch_address = patch["linked_start"] + current_offset
                    delta_value = (target - patch_address) & 0x03FFFFFC
                    struct.pack_into(">I", patch["bytes"], current_offset,
                                     (raw & 0xFC000003) | delta_value)
                elif rtype == 11:
                    patch_address = patch["linked_start"] + current_offset
                    delta_value = (target - patch_address) & 0x0000FFFC
                    struct.pack_into(">I", patch["bytes"], current_offset,
                                     (raw & 0xFFFF0003) | delta_value)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dol-dir", required=True, help="DolRecomp DOL output dir")
    ap.add_argument("--rels-dir", required=True, help="DolRecomp REL output root")
    ap.add_argument("--rels-bin-dir", required=True, help="Extracted .rel binaries dir")
    ap.add_argument("--main-dol", required=True, help="Original main.dol path")
    ap.add_argument("--game-id", default="GZLE01")
    ap.add_argument("--output-dir", required=True)
    args = ap.parse_args()

    dol_dir = Path(args.dol_dir)
    rels_dir = Path(args.rels_dir)
    rels_bin_dir = Path(args.rels_bin_dir)
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # --- Parse DOL ---
    dol_h = (dol_dir / "generated.h").read_text()
    dol_ranges, dol_funcs = parse_generated_h(dol_h)
    dol_chunks = chunk_ranges_from(dol_funcs, dol_ranges)

    ep_m = re.search(r"#define DOLRECOMP_ENTRY_POINT (0x[0-9A-Fa-f]+)", dol_h)
    entry_point = int(ep_m.group(1), 16)

    smc_txt = (dol_dir / "generated_smc.txt").read_text()
    smc_ranges = []
    for line in smc_txt.splitlines():
        m = re.match(r"(0x[0-9A-Fa-f]+)-(0x[0-9A-Fa-f]+)", line.strip())
        if m:
            smc_ranges.append((int(m.group(1), 16), int(m.group(2), 16) + 4))
    smc_ranges.sort()

    print(f"DOL: {len(dol_funcs)} chunks, {len(dol_ranges)} ranges")

    # --- Parse RELs ---
    rel_entries = []
    for mod_dir in sorted(rels_dir.iterdir()):
        gen_h = mod_dir / "generated.h"
        if not gen_h.exists():
            continue
        h2 = gen_h.read_text()
        rel_rngs, rel_funcs = parse_generated_h(h2)
        ep2 = re.search(r"#define DOLRECOMP_ENTRY_POINT (0x[0-9A-Fa-f]+)", h2)
        rel_entries.append({
            "name": mod_dir.name,
            "dir": mod_dir,
            "ranges": rel_rngs,
            "funcs": rel_funcs,
            "chunks": chunk_ranges_from(rel_funcs, rel_rngs),
            "entry_point": int(ep2.group(1), 16),
        })

    rel_entries.sort(key=lambda e: e["entry_point"])
    print(f"RELs: {len(rel_entries)} modules")

    # --- Merge ---
    all_chunks = sorted(dol_chunks + [c for e in rel_entries for c in e["chunks"]])
    prev_end = 0
    for start, end in all_chunks:
        if start < prev_end:
            print(f"ERROR: overlap at {start:#010x} prev_end={prev_end:#010x}", file=sys.stderr)
            sys.exit(1)
        prev_end = max(prev_end, end)

    all_code_ranges = merge_ranges(
        list(dol_ranges) + [r for e in rel_entries for r in e["ranges"]]
    )
    all_func_addrs = sorted(dol_funcs + [f for e in rel_entries for f in e["funcs"]])
    print(f"Composite: {len(all_chunks)} chunks, {len(all_code_ranges)} code ranges")

    # --- Hash original guest bytes ---
    read_dol = load_dol_text(args.main_dol)

    # Build a lookup: entry_point -> rel binary data (cached)
    rel_bin_cache = {}
    for e in rel_entries:
        mid = int(e["name"].rsplit("_", 1)[-1])
        if mid not in rel_bin_cache:
            bp = find_rel_binary(rels_bin_dir, mid)
            if bp:
                rel_bin_cache[mid] = bp.read_bytes()
            else:
                rel_bin_cache[mid] = None

    # Build linked section images before emitting the ABI tables. The entry
    # point is inside the executable section, so recover each REL's load base
    # from that section's file offset.
    rel_section_maps = {}
    for e in rel_entries:
        mid = int(e["name"].rsplit("_", 1)[-1])
        rel_data = rel_bin_cache.get(mid)
        if rel_data is None:
            continue
        nsec = struct.unpack_from(">I", rel_data, 0xC)[0]
        sio = struct.unpack_from(">I", rel_data, 0x10)[0]
        exec_offsets = [
            struct.unpack_from(">II", rel_data, sio + i * 8)[0] & ~1
            for i in range(nsec)
            if struct.unpack_from(">II", rel_data, sio + i * 8)[0] & 1
        ]
        if not exec_offsets:
            continue
        base = e["entry_point"] - min(exec_offsets)
        rel_section_maps[mid] = {"base": base, "sections": rel_linked_sections(rel_data, base)}
    try:
        validate_rel_linked_namespace(rel_section_maps)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
    apply_rel_data_relocations(rel_bin_cache, rel_section_maps)

    chunk_hashes = []
    for start, end in all_chunks:
        if start < 0x80400000:
            data = read_dol(start, end)
        else:
            owner = None
            for e in rel_entries:
                for r in e["ranges"]:
                    if r[0] <= start < r[1]:
                        owner = e
                        break
                if owner:
                    break
            if owner is None:
                print(f"ERROR: no REL owns {start:#010x}", file=sys.stderr)
                sys.exit(1)
            mid = int(owner["name"].rsplit("_", 1)[-1])
            rel_data = rel_bin_cache.get(mid)
            if rel_data is None:
                print(f"ERROR: no .rel binary for mid={mid}", file=sys.stderr)
                sys.exit(1)
            data = read_rel_exec_range(rel_data, start, end, owner["entry_point"])
        chunk_hashes.append(fnv1a64(data))

    # --- Emit generated_composite.h ---
    lines = []
    lines.append("// Generated by generate_composite.py -- do not edit.")
    lines.append("#ifndef RECOMP_COMPOSITE_H")
    lines.append("#define RECOMP_COMPOSITE_H")
    lines.append("")
    lines.append("#define DOLRECOMP_CPU_GEKKO 1")
    lines.append('#define DOLRECOMP_CPU_NAME "gekko"')
    lines.append("#include <string.h>")
    lines.append("#include <math.h>")
    lines.append("#ifndef DOLRECOMP_CPU_HEADER")
    lines.append('#define DOLRECOMP_CPU_HEADER "cpu/cpu.h"')
    lines.append("#endif")
    lines.append("#include DOLRECOMP_CPU_HEADER")
    lines.append("#ifndef DOLRECOMP_C_LOOP_CYCLE_BUDGET")
    # The host guarantees ctx->cycle_budget >= 1: bluewake_cycle_domain_begin_turn
    # sets 1, and bounded_budget() clamps every other assignment to 1 as well.
    # The defensive fallback therefore never fires, and it is not free - it puts a
    # compare and a select on every budget test, which the emitter emits at every
    # block leader and at every non-leader charge site. Returning the field
    # directly lets the compiler fold "downcount <= -(s64)budget" into a single
    # add-and-compare. Measured on chunk_0144 with scripts/bench_chunk.sh
    # --ablate cheap-budget: 28.86 -> 27.90 instructions per guest cycle, -3.3%
    # over two interleaved rounds with no overlap.
    lines.append("static inline s64 dolrecomp_loop_cycle_budget(const CPUState* ctx) {")
    lines.append("    return ctx->cycle_budget;")
    lines.append("}")
    lines.append("#define DOLRECOMP_C_LOOP_CYCLE_BUDGET dolrecomp_loop_cycle_budget(ctx)")
    lines.append("#endif")
    lines.append("static inline bool dolrecomp_block_can_precharge(")
    lines.append("    const CPUState* ctx, u32 block_cycles) {")
    lines.append("    if (ctx->cycle_deadline_budget <= 0) return true;")
    lines.append("    const s64 remaining = ctx->cycle_deadline_budget + ctx->downcount;")
    lines.append("    return remaining >= 0 && (u64)remaining >= (u64)block_cycles;")
    lines.append("}")
    # The precise per-instruction charge is dead on the hot path - it runs only
    # when a block could not be precharged, a window of a few cycles before a
    # device deadline - but it used to be emitted inline at every instruction.
    # Out of line it costs a cold call and saves the body: measured 2.1 percent
    # fewer host instructions per guest cycle on chunk_0144 with identical guest
    # cycles charged, and 17 percent less generated C. See
    # emit_precise_instruction_charge in the DolRecomp emitter.
    lines.append("static inline")
    lines.append("#if defined(__GNUC__) || defined(__clang__)")
    lines.append("__attribute__((noinline))")
    lines.append("#endif")
    lines.append("bool dolrecomp_charge_precise(CPUState* ctx, u32 cycles, u32 resume) {")
    lines.append("    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {")
    lines.append("        ctx->pc = resume;")
    lines.append("        return false;")
    lines.append("    }")
    lines.append("    ctx->downcount -= (s64)cycles;")
    lines.append("    return true;")
    lines.append("}")
    lines.append("static inline void dolrecomp_refund_cycle_suffix(CPUState* ctx) {")
    lines.append("    ctx->downcount += (s64)ctx->cycle_observation_suffix;")
    lines.append("    ctx->cycle_observation_suffix = 0u;")
    lines.append("}")
    lines.append("#ifndef DOLRECOMP_C_MAX_CALL_DEPTH")
    lines.append("#define DOLRECOMP_C_MAX_CALL_DEPTH 24")
    lines.append("#endif")
    lines.append("extern unsigned dolrecomp_call_depth;")
    lines.append("static inline int dolrecomp_call_enter(void) {")
    lines.append("    if (dolrecomp_call_depth >= (unsigned)DOLRECOMP_C_MAX_CALL_DEPTH)")
    lines.append("        return 0;")
    lines.append("    dolrecomp_call_depth++;")
    lines.append("    return 1;")
    lines.append("}")
    lines.append("static inline void dolrecomp_call_leave(void) {")
    lines.append("    if (dolrecomp_call_depth)")
    lines.append("        dolrecomp_call_depth--;")
    lines.append("}")
    lines.append('static inline bool ppc_fp_available_inline(CPUState* cpu, u32 cia) {')
    lines.append('    /* MSR[FP] set: available, exactly as ppc_fp_available answers. */')
    lines.append('    if (cpu->msr & PPC_MSR_FP)')
    lines.append('        return true;')
    lines.append('    return ppc_fp_available(cpu, cia);')
    lines.append('}')
    lines.append('')
    lines.append('/* Unquantised (GQR type 0) paired-single load/store inline, using the same')
    lines.append('   conversions, access order and enable check as ppc_psq_load/ppc_psq_store;')
    lines.append('   every other type goes to the runtime. */')
    lines.append('static inline bool ppc_psq_load_inline(CPUState* cpu, u8 frD, u32 ea, bool w,')
    lines.append('                                       u8 gqr, bool indexed, u32 cia) {')
    lines.append('    const u32 g = cpu->gqr[gqr & 7u];')
    lines.append('    if (((g >> 16) & 7u) != 0u || (!indexed && (cpu->hid2 & PPC_HID2_LSQE) == 0u))')
    lines.append('        return ppc_psq_load(cpu, frD, ea, w, gqr, indexed, cia);')
    lines.append('    cpu->fpr[frD] = f64_value(convert_to_double(mem_read32(cpu, ea)));')
    lines.append('    cpu->ps1[frD] = w ? 1.0 : f64_value(convert_to_double(mem_read32(cpu, ea + 4u)));')
    lines.append('    return true;')
    lines.append('}')
    lines.append('')
    lines.append('static inline bool ppc_psq_store_inline(CPUState* cpu, u8 frS, u32 ea, bool w,')
    lines.append('                                        u8 gqr, bool indexed, u32 cia) {')
    lines.append('    const u32 g = cpu->gqr[gqr & 7u];')
    lines.append('    if ((g & 7u) != 0u || (!indexed && (cpu->hid2 & PPC_HID2_LSQE) == 0u))')
    lines.append('        return ppc_psq_store(cpu, frS, ea, w, gqr, indexed, cia);')
    lines.append('    mem_write32(cpu, ea, convert_to_single_ftz(f64_bits(cpu->fpr[frS])));')
    lines.append('    if (!w)')
    lines.append('        mem_write32(cpu, ea + 4u, convert_to_single_ftz(f64_bits(cpu->ps1[frS])));')
    lines.append('    return true;')
    lines.append('}')
    lines.append("")

    # Copy helper functions verbatim from DOL header (rotl32 through ps_to_bits)
    helpers_start = dol_h.find("static inline u32 dolrecomp_rotl32")
    helpers_marker = "#define DOLRECOMP_ENTRY_POINT"
    helpers_end = dol_h.find(helpers_marker)
    if helpers_start >= 0 and helpers_end > helpers_start:
        lines.append(dol_h[helpers_start:helpers_end].rstrip())

    lines.append("")
    lines.append("// Function entry points")
    for addr in all_func_addrs:
        lines.append(f"void func_{addr:08X}(CPUState* ctx);")
    lines.append(f"#define DOLRECOMP_ENTRY_POINT 0x{entry_point:08X}u")
    lines.append("typedef void (*DolRecompFunction)(CPUState* ctx);")
    lines.append("#if defined(__GNUC__) || defined(__clang__)")
    lines.append("#define DOLRECOMP_UNUSED __attribute__((unused))")
    lines.append("#else")
    lines.append("#define DOLRECOMP_UNUSED")
    lines.append("#endif")
    lines.append("""static inline int dolrecomp_dispatch_replacement(CPUState* ctx, u32 address) {
    (void)ctx;
    (void)address;
    return 0;
}""")

    # Composite dispatcher: binary search over sorted chunks
    n = len(all_chunks)
    lines.append("static inline DolRecompFunction dolrecomp_find_original(u32 address) {")
    lines.append("    // A pc's translation is a pure function of that pc - the chunk table")
    lines.append("    // is built at compile time and nothing at runtime rewrites it - so a")
    lines.append("    // cache keyed by the pc is exact: a key match is a hit, with no range")
    lines.append("    // test, no alignment test and no second table to consult, and a")
    lines.append("    // collision is caught by the comparison rather than by a guard around")
    lines.append("    // it. The index mixes the 0x4000-window number into the low pc bits,")
    lines.append("    // so one chunk's instructions occupy different slots instead of the")
    lines.append("    // single slot that (address >> 2) alone would give all of them.")
    lines.append("    enum { DOLRECOMP_PC_CACHE_SIZE = 4096u };")
    lines.append("    static u32 s_cached_pc[DOLRECOMP_PC_CACHE_SIZE];")
    lines.append("    static DolRecompFunction s_cached_pc_fn[DOLRECOMP_PC_CACHE_SIZE];")
    lines.append("    const unsigned cache_index =")
    lines.append("        (unsigned)(((address >> 2) ^ (address >> 14)) &")
    lines.append("                   (DOLRECOMP_PC_CACHE_SIZE - 1u));")
    lines.append("    if (s_cached_pc[cache_index] == address)")
    lines.append("        return s_cached_pc_fn[cache_index];")
    lines.append("    static const u32 s_starts[] = {")
    for start, _ in all_chunks:
        lines.append(f"        0x{start:08X}u,")
    lines.append("    };")
    lines.append("    static const DolRecompFunction s_fns[] = {")
    for start, _ in all_chunks:
        lines.append(f"        func_{start:08X},")
    lines.append("    };")
    lines.append("    static const u32 s_ends[] = {")
    for _, end in all_chunks:
        lines.append(f"        0x{end:08X}u,")
    lines.append("    };")
    lines.append(f"    unsigned lo = 0, hi = {n}u;")
    lines.append("    while (lo < hi) {")
    lines.append("        unsigned mid = lo + (hi - lo) / 2u;")
    lines.append("        if (s_starts[mid] < address)")
    lines.append("            lo = mid + 1u;")
    lines.append("        else")
    lines.append("            hi = mid;")
    lines.append("    }")
    lines.append(f"    unsigned idx = lo;")
    lines.append(f"    if (lo >= {n}u || s_starts[lo] != address)")
    lines.append(f"        idx = lo - 1u;")
    lines.append(f"    if (idx < {n}u && s_starts[idx] <= address && address < s_ends[idx]")
    lines.append(f"        && ((address - s_starts[idx]) & 3u) == 0u) {{")
    lines.append("        s_cached_pc[cache_index] = address;")
    lines.append("        s_cached_pc_fn[cache_index] = s_fns[idx];")
    lines.append("        return s_fns[idx];")
    lines.append("    }")
    lines.append("    return NULL;")
    lines.append("}")

    lines.append("""static inline int dolrecomp_call_original(CPUState* ctx, u32 address) {
    DolRecompFunction fn = dolrecomp_find_original(address);
    if (!fn) return 0;
    ctx->pc = address;
    fn(ctx);
    return 1;
}

static inline bool dolrecomp_physical_pc_alias(CPUState* ctx, u32 address, u32* alias_out) {
    if (address < ctx->ram_size) {
        *alias_out = address | GC_RAM_BASE;
        return *alias_out != address;
    }
    return false;
}

// The dispatcher is split so that the path every block boundary takes - probe
// the cache, call the block - is a function with no cold code in it. The alias
// fallbacks are reached once per pc rather than once per block. Emitted
// together, the hot path paid for a six-register frame, two argument copies and
// a second pc store at every dispatch, because the compiler must preserve
// whatever the cold paths use.
static inline
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
int dolrecomp_call_slow(CPUState* ctx, u32 address) {
    u32 alias;
    if (dolrecomp_physical_pc_alias(ctx, address, &alias)) {
        ctx->pc = alias;
        if (dolrecomp_dispatch_replacement(ctx, alias)) return 1;
        if (ctx->host_call && ppc_host_call(ctx, alias)) return 1;
        if (dolrecomp_call_original(ctx, alias)) return 1;
    }
    if (address >= 0xC0000000u && address < 0xC0400000u) {
        alias = address & ~0x40000000u;
        ctx->pc = alias;
        if (dolrecomp_dispatch_replacement(ctx, alias)) return 1;
        if (ctx->host_call && ppc_host_call(ctx, alias)) return 1;
        if (dolrecomp_call_original(ctx, alias)) return 1;
    }
    return 0;
}

static inline int dolrecomp_call(CPUState* ctx, u32 address) {
    ctx->pc = address;
    if (dolrecomp_dispatch_replacement(ctx, address)) return 1;
    if (ctx->host_call && ppc_host_call(ctx, address)) return 1;
    if (dolrecomp_call_original(ctx, address)) return 1;
    return dolrecomp_call_slow(ctx, address);
}

static inline DOLRECOMP_UNUSED int dolrecomp_run_blocks(CPUState* ctx, u32 max_blocks) {
    u32 blocks = 0;
    while (max_blocks == 0u || blocks < max_blocks) {
        if (!dolrecomp_call(ctx, ctx->pc)) return 0;
        if (ctx->exception) return 0;
        blocks++;
    }
    return 1;
}""")
    lines.append("#undef DOLRECOMP_UNUSED")
    lines.append("#endif /* RECOMP_COMPOSITE_H */")
    lines.append("")

    base_text = chr(10).join(lines) + chr(10)
    # The composite's own copy makes the chunk table writable, so the module
    # can swap in a mod's variant of a chunk at boot (mod_variants.inc, from
    # scripts/mods/build_mod_variants.py). Only module_export.c includes this
    # file; the chunks include generated.h, which keeps the base rendering so
    # that adding mods never recompiles the 748 base chunks.
    fns_open = "    static const DolRecompFunction s_fns[] = {" + chr(10)
    fns_start = base_text.index(fns_open)
    fns_end = base_text.index("    };" + chr(10), fns_start) + len("    };" + chr(10))
    fn_table = base_text[fns_start + len(fns_open):fns_end - len("    };" + chr(10))]
    starts_table = chr(10).join(f"    0x{start:08X}u," for start, _ in all_chunks)
    file_scope = ("static DolRecompFunction s_dolrecomp_chunk_fns[] = {" + chr(10) + fn_table +
                  "};" + chr(10) + "static const u32 s_dolrecomp_chunk_starts[] = {" + chr(10) +
                  starts_table + chr(10) + "};" + chr(10) +
                  f"#define DOLRECOMP_CHUNK_COUNT {n}u" + chr(10))
    composite_text = (base_text[:fns_start] +
                      "    DolRecompFunction* const s_fns = s_dolrecomp_chunk_fns;" + chr(10) +
                      base_text[fns_end:])
    mod_types = chr(10).join([
        "typedef struct BlueWakeModChunk { u32 mask; u32 start; DolRecompFunction fn; } BlueWakeModChunk;",
        "typedef struct BlueWakeModExtraChunk { u32 mod; u32 start; u32 end; DolRecompFunction fn; } BlueWakeModExtraChunk;",
        "typedef struct BlueWakeModWrite { u32 mod; u32 every_frame; u32 address; u32 size; const u8* bytes; } BlueWakeModWrite;",
        "static DolRecompFunction bluewake_mod_extra_find(u32 address);", ""])
    anchor = "static inline DolRecompFunction dolrecomp_find_original(u32 address) {"
    fo_start = composite_text.index(anchor)
    miss = "    return NULL;" + chr(10) + "}" + chr(10)
    miss_at = composite_text.index(miss, fo_start)
    extra_lookup = chr(10).join([
        "    {",
        "        // A code range only an enabled mod has (mod_variants.inc).",
        "        DolRecompFunction extra = bluewake_mod_extra_find(address);",
        "        if (extra) {",
        "            s_cached_pc[cache_index] = address;",
        "            s_cached_pc_fn[cache_index] = extra;",
        "        }",
        "        return extra;",
        "    }", "}", ""])
    composite_text = composite_text[:miss_at] + extra_lookup + composite_text[miss_at + len(miss):]
    composite_text = composite_text.replace(anchor, file_scope + mod_types + anchor, 1)
    # newline=chr(10) everywhere: the output is byte-identical on Windows, whose
    # text mode would otherwise write CRLF and change the verified digest.
    (out_dir / "generated_composite.h").write_text(composite_text, newline=chr(10))
    if not (out_dir / "mod_variants.inc").exists():
        (out_dir / "mod_variants.inc").write_text(
            "// No mods: see scripts/mods/build_mod_variants.py." + chr(10) +
            "#define MODULE_MOD_COUNT 0u" + chr(10), newline=chr(10))

    # Copy chunk .c files into out_dir so that their relative include of
    # ../generated.h resolves to our composite header (NOT the original).
    import shutil
    gen_h_path = out_dir / "generated.h"
    if gen_h_path.exists() or gen_h_path.is_symlink():
        gen_h_path.unlink()
    gen_h_path.write_text(base_text, newline=chr(10))

    def copy_chunks(src_dir, dest_name, patterns=("*.c",)):
        dest = out_dir / dest_name
        if dest.exists():
            shutil.rmtree(dest)
        dest.mkdir()
        inputs = []
        for pattern in patterns:
            inputs.extend(Path(src_dir).glob(pattern))
        for cf in sorted(inputs):
            output = dest / cf.name
            if cf.suffix == ".o":
                try:
                    os.link(cf, output)
                    continue
                except OSError:
                    pass
            shutil.copy2(cf, output)
        return len(inputs)

    dol_chunk_dir = dol_dir / "chunks"
    dol_c_chunks = list(dol_chunk_dir.glob("*.c"))
    dol_object_chunks = list(dol_chunk_dir.glob("*.o"))
    if bool(dol_c_chunks) == bool(dol_object_chunks):
        print("ERROR: DOL chunks must contain exactly one of C or native object output",
              file=sys.stderr)
        sys.exit(1)
    dol_pattern = "*.o" if dol_object_chunks else "*.c"
    copied_dol = copy_chunks(dol_chunk_dir, "chunks_dol", (dol_pattern,))
    print(f"DOL backend: {'native objects' if dol_object_chunks else 'C'} "
          f"({copied_dol} chunks)")
    copied = 1
    for e in rel_entries:
        src_c = e["dir"] / "chunks"
        if src_c.exists():
            copy_chunks(src_c, "chunks_" + e["name"])
            copied += 1
    print(f"copied chunks from {copied} module(s)")

    # --- Emit module_tables.inc ---
    tbl = []
    tbl.append("// Generated by generate_composite.py -- do not edit.")
    tbl.append("static const StaticRecompRange s_code_ranges[] = {")
    for a, b in all_code_ranges:
        tbl.append(f"    {{0x{a:08X}u, 0x{b:08X}u}},")
    tbl.append("};")
    tbl.append(f"#define MODULE_CODE_RANGE_COUNT {len(all_code_ranges)}u")
    tbl.append("static const StaticRecompRange s_smc_ranges[] = {")
    for a, b in smc_ranges:
        tbl.append(f"    {{0x{a:08X}u, 0x{b:08X}u}},")
    if not smc_ranges:
        tbl.append("    {0u, 0u}, /* zero-sized */")
    tbl.append("};")
    tbl.append(f"#define MODULE_SMC_RANGE_COUNT {len(smc_ranges)}u")
    tbl.append("static const StaticRecompRange s_chunk_ranges[] = {")
    for a, b in all_chunks:
        tbl.append(f"    {{0x{a:08X}u, 0x{b:08X}u}},")
    tbl.append("};")
    tbl.append(f"#define MODULE_CHUNK_RANGE_COUNT {n}u")
    tbl.append("static const u64 s_chunk_hashes[] = {")
    for h_val in chunk_hashes:
        tbl.append(f"    0x{h_val:016X}ull,")
    tbl.append("};")
    (out_dir / "module_tables.inc").write_text(chr(10).join(tbl) + chr(10), newline=chr(10))

    # --- Emit rel_modules.inc ---
    rm_lines = []
    rm_lines.append("// Generated by generate_composite.py -- do not edit.")
    rel_module_info = []
    for e in rel_entries:
        mid = int(e["name"].rsplit("_", 1)[-1])
        bp = find_rel_binary(rels_bin_dir, mid)
        if bp is None:
            print(f"WARN: no .rel for mid={mid}", file=sys.stderr)
            continue
        rel_data = bp.read_bytes()
        info = parse_rel_header(rel_data)
        exec_offsets = [
            struct.unpack_from(">II", rel_data, info["section_info_offset"] + i * 8)[0] & ~1
            for i in range(info["section_count"])
            if (struct.unpack_from(">II", rel_data, info["section_info_offset"] + i * 8)[0] & 1)
        ]
        if not exec_offsets:
            continue
        linked_base = e["entry_point"] - min(exec_offsets)
        secs = parse_rel_sections(
            rel_data, info["section_count"], info["section_info_offset"], linked_base
        )
        info["sections"] = secs
        info["name"] = e["name"]
        rel_module_info.append(info)

    rel_module_info.sort(key=lambda m: m["module_id"])
    for m in rel_module_info:
        rm_lines.append(f"static const StaticRecompRelSection s_rel_sec_{m['module_id']}[] = {{")
        for si, ls, sz in m["sections"]:
            rm_lines.append(f"    {{{m['module_id']}u, {si}u, 0x{ls:08X}u, 0x{sz:08X}u}},")
        rm_lines.append("};")
    rm_lines.append("")
    rm_lines.append("static const StaticRecompRelModule s_rel_modules[] = {")
    for m in rel_module_info:
        rm_lines.append(
            f"    {{{m['module_id']}u, {m['version']}u, {m['section_count']}u,"
            f" 0x{m['section_info_offset']:04X}u, {m['file_size']}u,"
            f" s_rel_sec_{m['module_id']}, {len(m['sections'])}u}},"
        )
    rm_lines.append("};")
    rm_lines.append(f"#define MODULE_REL_MODULE_COUNT {len(rel_module_info)}u")
    (out_dir / "rel_modules.inc").write_text(chr(10).join(rm_lines) + chr(10), newline=chr(10))

    # Emit relocated file-backed data separately from the existing code-only
    # section metadata. Keeping this additive preserves the v3 module ABI.
    data_lines = ["// Generated by generate_composite.py -- do not edit."]
    data_entries = []
    for mid in sorted(rel_section_maps):
        for section in rel_section_maps[mid]["sections"]:
            if section["executable"] or section["size"] == 0 or section["linked_start"] == 0:
                continue
            name = "NULL"
            if section["bytes"] is not None:
                name = f"s_rel_data_{mid}_{section['index']}"
                data_lines.append(f"static const u8 {name}[] = {{")
                values = list(section["bytes"])
                for start in range(0, len(values), 16):
                    data_lines.append("    " + ", ".join(f"0x{v:02X}u" for v in values[start:start + 16]) + ",")
                data_lines.append("};")
            data_entries.append((mid, section["index"], section["linked_start"], section["size"], name))
    data_lines.append("static const BlueWakeRelData s_rel_data[] = {")
    for mid, si, linked, size, name in data_entries:
        data_lines.append(f"    {{{mid}u, {si}u, 0x{linked:08X}u, 0x{size:08X}u, {name}}},")
    data_lines.append("};")
    data_lines.append(f"#define MODULE_REL_DATA_COUNT {len(data_entries)}u")
    data_lines.append("static const BlueWakeRelLifecycle s_rel_lifecycle[] = {")
    lifecycle_count = 0
    for mid in sorted(rel_section_maps):
        rel_data = rel_bin_cache[mid]
        prolog_section = rel_data[0x30]
        prolog_offset = struct.unpack_from(">I", rel_data, 0x34)[0]
        if prolog_section >= len(rel_section_maps[mid]["sections"]):
            continue
        section = rel_section_maps[mid]["sections"][prolog_section]
        if not section["linked_start"]:
            continue
        prolog = section["linked_start"] + prolog_offset
        data_lines.append(f"    {{{mid}u, 0x{prolog:08X}u}},")
        lifecycle_count += 1
    data_lines.append("};")
    data_lines.append(f"#define MODULE_REL_LIFECYCLE_COUNT {lifecycle_count}u")
    (out_dir / "rel_data.inc").write_text(chr(10).join(data_lines) + chr(10), newline=chr(10))

    # --- Determinism audit ---
    output_files = ["generated_composite.h", "module_tables.inc", "rel_modules.inc", "rel_data.inc"]
    hashes = {}
    for fname in output_files:
        fp = out_dir / fname
        if fp.exists():
            hashes[fname] = hashlib.sha256(fp.read_bytes()).hexdigest()
    print("Output SHA-256:")
    for fname, h_val in sorted(hashes.items()):
        print(f"  {fname}: {h_val[:16]}...")

    print(f"PASS: {n} chunks, {len(rel_module_info)} rel modules, "
          f"{len(all_code_ranges)} code ranges")

    return 0


if __name__ == "__main__":
    sys.exit(main())
