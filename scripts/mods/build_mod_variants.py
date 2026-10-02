#!/usr/bin/env python3
"""Add mods to a generated composite as variant chunks and data writes.

A mod is a patched copy of the game: a Gecko code applied to main.dol
(gecko_apply.py) or a patcher's output (Better Wind Waker). Its code cannot be
patched at runtime in a statically recompiled game, so the patched main.dol and
RELs are translated with the same translator and settings as the base and run
through generate_composite.py into a composite source tree of their own. This
script compares that tree with the base tree and adds to the base:

  * variant chunks: every chunk whose translation differs, under a new name.
    The host calls bluewake_composite_apply_mods() at boot, which points the
    chunk table at the variants of the enabled mods. Chunks never call each
    other directly (every cross-chunk edge returns to the dispatcher), so a
    variant replaces its chunk cleanly.
  * extra chunks: code ranges only the mod has (Better Wind Waker's custom code
    section), found through bluewake_mod_extra_find() when enabled.
  * one-time writes: the mod's DOL bytes that differ from the base DOL image
    (data sections and new sections) and its relocated REL data that differs,
    written into guest RAM at boot.
  * per-frame writes: a Gecko code's data writes, re-applied at every retrace
    as Dolphin does.

usage: build_mod_variants.py --composite-src BASE_TREE --base-dol BASE.dol
           --mod NAME:MOD_TREE:MOD.dol[:GECKO_RUNTIME_JSON] [--mod ...]
           [--combo NAME+NAME:COMBO_TREE ...]

Mods are numbered in the order given (bit 0, 1, ...). When two mods change the
same chunk, each keeps its own variant and a --combo tree (the game patched by
both, translated) supplies the variant used when both are enabled. A variant
applies when all the mods it requires are enabled; the most specific one wins.
"""
import argparse
import json
import re
import shutil
import sys
from pathlib import Path

FUNC_RE = re.compile(r"\bfunc_([0-9A-F]{8})\b")
CHUNK_FN_RE = re.compile(r"^void func_([0-9A-F]{8})\(CPUState\* ctx\) \{", re.M)


def dol_image(path):
    """address -> bytes of every loadable section."""
    d = Path(path).read_bytes()
    be = lambda o: int.from_bytes(d[o:o + 4], "big")
    out = {}
    for n in range(18):
        off, addr, size = be(n * 4), be(0x48 + n * 4), be(0x90 + n * 4)
        if size:
            out[addr] = d[off:off + size]
    return out


def dol_writes(base_dol, mod_dol):
    base = dol_image(base_dol)
    def base_byte(a):
        for start, data in base.items():
            if start <= a < start + len(data):
                return data[a - start]
        return None
    writes = []
    for start, data in sorted(dol_image(mod_dol).items()):
        run_start, run = None, bytearray()
        for i, b in enumerate(data):
            if base_byte(start + i) != b:
                if run_start is None:
                    run_start = start + i
                run.append(b)
            elif run_start is not None:
                writes.append((run_start, bytes(run)))
                run_start, run = None, bytearray()
        if run_start is not None:
            writes.append((run_start, bytes(run)))
    return writes


REL_DATA_RE = re.compile(r"static const u8 (s_rel_data_\d+_\d+)\[\] = \{(.*?)\};", re.S)
REL_ENTRY_RE = re.compile(r"\{(\d+)u, (\d+)u, 0x([0-9A-F]{8})u, 0x([0-9A-F]{8})u, (\w+)\},")


def rel_data(tree):
    text = (Path(tree) / "rel_data.inc").read_text()
    arrays = {name: bytes(int(v, 16) for v in re.findall(r"0x([0-9A-F]{2})u", body))
              for name, body in REL_DATA_RE.findall(text)}
    out = {}
    for mid, si, linked, size, name in REL_ENTRY_RE.findall(text):
        out[(int(mid), int(si))] = (int(linked, 16), arrays.get(name))
    return out


def rel_writes(base_tree, mod_tree):
    base, mod = rel_data(base_tree), rel_data(mod_tree)
    writes = []
    for key, (linked, data) in sorted(mod.items()):
        if data is None:
            continue
        b = base.get(key)
        if b is None or b[0] != linked or b[1] is None or len(b[1]) != len(data):
            sys.exit("REL data section %s changed shape; not supported" % (key,))
        old = b[1]
        i = 0
        while i < len(data):
            if data[i] == old[i]:
                i += 1
                continue
            j = i
            while j < len(data) and data[j] != old[j]:
                j += 1
            writes.append((linked + i, data[i:j]))
            i = j
    return writes


def chunk_ranges(tree):
    text = (Path(tree) / "generated_composite.h").read_text()
    starts = [int(v, 16) for v in re.findall(
        r"0x([0-9A-F]{8})u,", text.split("static const u32 s_dolrecomp_chunk_starts[] = {")[1].split("};")[0])]
    ends = [int(v, 16) for v in re.findall(
        r"0x([0-9A-F]{8})u,", text.split("static const u32 s_ends[] = {")[1].split("};")[0])]
    return dict(zip(starts, ends))


# Game options (game_options.py): the switches the option sites read, the host's
# native code for the hook sites, and the option table. module_export.c
# includes this file after defining RECOMP_MODULE_EXPORT and BlueWakeModWriteFn.
OPTION_GLUE = r"""
/* The switches the option sites read, set by the host before the first
 * dispatch; and the host's native code for the hook sites, which returns 0 to
 * go on or the guest address to continue at. */
volatile unsigned char dolrecomp_option_flags[256];
typedef u32 (*BlueWakeNativeHookFn)(CPUState* ctx, u32 id);
static BlueWakeNativeHookFn s_native_hook;
u32 dolrecomp_native_hook(CPUState* ctx, u32 id);
u32 dolrecomp_native_hook(CPUState* ctx, u32 id)
{
    return s_native_hook != NULL ? s_native_hook(ctx, id) : 0u;
}
typedef struct BlueWakeOption { u32 index; u32 default_on; const char* name; const char* title; } BlueWakeOption;
typedef struct BlueWakeOptionWrite { u32 option; u32 address; u32 size; const u8* bytes; } BlueWakeOptionWrite;"""

OPTION_EXPORTS = r"""
RECOMP_MODULE_EXPORT volatile unsigned char* bluewake_composite_option_flags(void);
RECOMP_MODULE_EXPORT volatile unsigned char* bluewake_composite_option_flags(void)
{
    return dolrecomp_option_flags;
}
RECOMP_MODULE_EXPORT void bluewake_composite_set_native_hook(BlueWakeNativeHookFn fn);
RECOMP_MODULE_EXPORT void bluewake_composite_set_native_hook(BlueWakeNativeHookFn fn)
{
    s_native_hook = fn;
}
RECOMP_MODULE_EXPORT u32 bluewake_composite_option_count(void);
RECOMP_MODULE_EXPORT u32 bluewake_composite_option_count(void)
{
    return MODULE_OPTION_COUNT;
}
/* The option at a position in the table: its switch, whether it is on by
 * default, its name (the setting's key) and its title (the menu's). */
RECOMP_MODULE_EXPORT const char* bluewake_composite_option(u32 position, u32* index, u32* default_on,
                                                           const char** title);
RECOMP_MODULE_EXPORT const char* bluewake_composite_option(u32 position, u32* index, u32* default_on,
                                                           const char** title)
{
    if (position >= MODULE_OPTION_COUNT)
        return NULL;
    if (index) *index = s_options[position].index;
    if (default_on) *default_on = s_options[position].default_on;
    if (title) *title = s_options[position].title;
    return s_options[position].name;
}
/* The values the switched-on options write, at boot. */
RECOMP_MODULE_EXPORT u32 bluewake_composite_option_writes(BlueWakeModWriteFn fn, void* user);
RECOMP_MODULE_EXPORT u32 bluewake_composite_option_writes(BlueWakeModWriteFn fn, void* user)
{
    u32 count = 0;
    for (u32 i = 0; i < MODULE_OPTION_WRITE_COUNT; ++i) {
        const BlueWakeOptionWrite* w = &s_option_writes[i];
        if (!dolrecomp_option_flags[w->option])
            continue;
        if (fn)
            fn(user, w->address, w->bytes, w->size);
        count++;
    }
    return count;
}"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--composite-src", required=True)
    ap.add_argument("--base-dol", required=True)
    ap.add_argument("--mod", action="append", default=[])
    ap.add_argument("--combo", action="append", default=[])
    # Mods that are never enabled together (two widescreen aspects): a chunk
    # only they share needs no combined variant. The host keeps one of them.
    ap.add_argument("--exclusive", action="append", default=[], metavar="NAME,NAME")
    # Game options (game_options.py): the option table and the values the
    # options write, resolved against the base tree's REL data.
    ap.add_argument("--options", metavar="SPEC")
    ap.add_argument("--rels-bin-dir", metavar="DIR")
    args = ap.parse_args()
    exclusive = [set(spec.split(",")) for spec in args.exclusive]
    out = Path(args.composite_src)
    base_ranges = chunk_ranges(out)
    owner = {}
    names, chunks, extras, writes, decls = [], [], [], [], []
    for bit, spec in enumerate(args.mod):
        parts = spec.split(":")
        name, tree, mod_dol = parts[0], Path(parts[1]), parts[2]
        gecko = parts[3] if len(parts) > 3 else None
        names.append(name)
        dest = out / ("chunks_mod_" + name)
        if dest.exists():
            shutil.rmtree(dest)
        dest.mkdir()
        mod_ranges = chunk_ranges(tree)
        n_var = n_extra = 0
        for chunk_dir in sorted(tree.glob("chunks_*")):
            if chunk_dir.name.startswith("chunks_mod_"):
                continue
            for variant in sorted(chunk_dir.glob("*.c")):
                original = out / chunk_dir.name / variant.name
                text = variant.read_text()
                if original.exists() and original.read_text() == text:
                    continue
                m = CHUNK_FN_RE.search(text)
                if not m:
                    sys.exit("no chunk function in " + str(variant))
                start = int(m.group(1), 16)
                owner.setdefault(start, []).append(name)
                suffix = "__mod_" + name
                target = dest / (chunk_dir.name + "__" + variant.name)
                target.write_text(FUNC_RE.sub(lambda g: g.group(0) + suffix, text), newline=chr(10))
                fn = "func_%08X%s" % (start, suffix)
                decls.append("void %s(CPUState* ctx);" % fn)
                if start in base_ranges:
                    if base_ranges[start] != mod_ranges.get(start):
                        sys.exit("chunk %08X changed its range" % start)
                    chunks.append((1 << bit, start, fn))
                    n_var += 1
                else:
                    extras.append((bit, start, mod_ranges[start], fn))
                    n_extra += 1
        once = dol_writes(args.base_dol, mod_dol) + rel_writes(out, tree)
        for address, data in once:
            writes.append((bit, 0, address, data))
        frame = []
        if gecko:
            frame = [(a, bytes.fromhex(h)) for a, h in json.load(open(gecko))["data"]]
            for address, data in frame:
                writes.append((bit, 1, address, data))
        print("%s: %d variant chunks, %d extra chunks, %d boot writes (%d bytes), %d per-frame writes" % (
            name, n_var, n_extra, len(once), sum(len(d) for _, d in once), len(frame)))
    for spec in args.combo:
        combo, tree = spec.split(":")
        members = combo.split("+")
        mask = 0
        for member in members:
            mask |= 1 << names.index(member)
        tree = Path(tree)
        dest = out / ("chunks_mod_" + "_".join(members))
        if dest.exists():
            shutil.rmtree(dest)
        dest.mkdir()
        shared = {start for start, who in owner.items() if len(set(who) & set(members)) > 1}
        found = set()
        for chunk_dir in sorted(tree.glob("chunks_*")):
            for variant in sorted(chunk_dir.glob("*.c")):
                text = variant.read_text()
                m = CHUNK_FN_RE.search(text)
                if not m or int(m.group(1), 16) not in shared:
                    continue
                start = int(m.group(1), 16)
                suffix = "__mod_" + "_".join(members)
                (dest / (chunk_dir.name + "__" + variant.name)).write_text(
                    FUNC_RE.sub(lambda g: g.group(0) + suffix, text), newline=chr(10))
                fn = "func_%08X%s" % (start, suffix)
                decls.append("void %s(CPUState* ctx);" % fn)
                chunks.append((mask, start, fn))
                found.add(start)
        if found != shared:
            sys.exit("combo %s is missing chunks %s" % (combo, sorted(shared - found)))
        print("%s: %d combined chunks" % (combo, len(found)))
    def pair_resolved(start, a, b):
        if any({a, b} <= group for group in exclusive):
            return True
        pair = (1 << names.index(a)) | (1 << names.index(b))
        return any(c[1] == start and c[0] & pair == pair for c in chunks)
    unresolved = [s_ for s_, who in owner.items() if not all(
        pair_resolved(s_, a, b) for i, a in enumerate(who) for b in who[i + 1:])]
    if unresolved:
        sys.exit("chunks changed by several mods need a --combo: %s" % ["%08X" % u for u in unresolved])
    # Most specific last: apply_mods lets a later matching variant win.
    chunks.sort(key=lambda c: bin(c[0]).count("1"))
    lines = ["// Generated by scripts/mods/build_mod_variants.py -- do not edit.",
             "#define MODULE_MOD_COUNT %du" % len(names),
             "static const char* const s_mod_names[] = {%s};" % ", ".join('"%s"' % n for n in names)]
    lines += decls
    lines.append("static const BlueWakeModChunk s_mod_chunks[] = {")
    lines += ["    {0x%Xu, 0x%08Xu, %s}," % c for c in chunks]
    lines.append("};")
    lines.append("#define MODULE_MOD_CHUNK_COUNT %du" % len(chunks))
    lines.append("static const BlueWakeModExtraChunk s_mod_extra_chunks[] = {")
    lines += ["    {%du, 0x%08Xu, 0x%08Xu, %s}," % e for e in extras]
    lines.append("};")
    lines.append("#define MODULE_MOD_EXTRA_CHUNK_COUNT %du" % len(extras))
    for i, (_, _, _, data) in enumerate(writes):
        lines.append("static const u8 s_mod_write_%d[] = {%s};" % (i, ", ".join("0x%02Xu" % b for b in data)))
    lines.append("static const BlueWakeModWrite s_mod_writes[] = {")
    lines += ["    {%du, %du, 0x%08Xu, %du, s_mod_write_%d}," % (bit, frame, a, len(d), i)
              for i, (bit, frame, a, d) in enumerate(writes)]
    lines.append("};")
    lines.append("#define MODULE_MOD_WRITE_COUNT %du" % len(writes))
    if args.options:
        import game_options
        spec = game_options.Spec(args.options)
        linked = {key: value[0] for key, value in rel_data(out).items()}
        option_writes = game_options.resolve_writes(spec, args.rels_bin_dir, linked)
        lines.append(OPTION_GLUE)
        lines.append("#define MODULE_OPTION_COUNT %du" % len(spec.options))
        lines.append("static const BlueWakeOption s_options[] = {")
        lines += ['    {%du, %du, "%s", "%s"},' % (i, on, n, t.replace('"', '\\"'))
                  for i, n, on, t in spec.options]
        lines.append("};")
        for i, (_, _, data) in enumerate(option_writes):
            lines.append("static const u8 s_option_write_%d[] = {%s};" % (
                i, ", ".join("0x%02Xu" % b for b in data)))
        lines.append("static const BlueWakeOptionWrite s_option_writes[] = {")
        lines += ["    {%du, 0x%08Xu, %du, s_option_write_%d}," % (o, a, len(d), i)
                  for i, (o, a, d) in enumerate(option_writes)]
        lines.append("};")
        lines.append("#define MODULE_OPTION_WRITE_COUNT %du" % len(option_writes))
        lines.append(OPTION_EXPORTS)
        print("options: %d, %d writes" % (len(spec.options), len(option_writes)))
    (out / "mod_variants.inc").write_text("\n".join(lines) + "\n", newline=chr(10))


if __name__ == "__main__":
    main()
