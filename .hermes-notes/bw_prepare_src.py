#!/usr/bin/env python3
"""Regenerate the prepared composite source using the MERGED scripts
(including the re-added return_ranges.py), stopping before the x86-64 compile.
Reuses the disc and the Linux Builder's own methods."""
import sys, types
from pathlib import Path

REPO = Path("/home/james/nfs/Projects/bluewake")
sys.path.insert(0, str(REPO / "scripts/linux"))
import build as linux_build

OUT = Path("/home/james/bw-build-android/prepared-behaviour")  # lean-memory + lean-deadlines + behaviour-timing
ISO = Path("/home/james/bw-build-0215/BlueWake/game/GZLE01.iso")

# Minimal args object matching what the Builder methods touch.
args = types.SimpleNamespace(
    disc=ISO,
    out=OUT,
    jobs=8,
    march="x86-64-v3",
    opt_level="2",
    no_mods=False,
    no_train=True,
    no_pgo=True,
    retrain=False,
    tour_playbacks=None,
    prepared_blocks=True,
    fixed_cpu=True, fixed_mem1=True, inline_fp=True, gather_pipe=True,
    inline_gpr=True, direct_calls=True,
    native_j3d=True, native_vec=True, native_math=True,
    native_skin=True, native_game_math=True,
    lean_memory=True, native_entries=False, lean_blocks=True,
    lean_deadlines=True,
    behaviour_timing=True,
    accept_new_composite=False,
    appimage=False, ci_container=False,
    no_tiered=False, no_cold=False,
)

b = linux_build.Builder(args)
print("== tools ==")
b.check_tools()
print("== dependencies ==")
b.dependencies()
print("== disc ==")
b.disc()
print("== dolrecomp ==")
b.dolrecomp = b.build_dolrecomp()
print("== extract (reuse existing game dir) ==")
game = OUT / "game"
# Reuse the already-extracted game files by copying them into the new out.
import shutil
if not (game / "main.dol").exists():
    game.mkdir(parents=True, exist_ok=True)
    shutil.copy2("/home/james/bw-build-0215/game/main.dol", game / "main.dol")
    shutil.copytree("/home/james/bw-build-0215/game/rels", game / "rels")
    stamp = OUT / "game.json"
    import json
    stamp.write_text(json.dumps({"iso": str(ISO), "size": ISO.stat().st_size,
                                 "mtime": ISO.stat().st_mtime_ns,
                                 "extractor": linux_build.sha256_file(REPO / "apple/ios/src/disc_import.c")}))
    print("copied extracted game files")
print("== translate (reuse existing translated) ==")
translated = OUT / "translated"
if not (translated / "dol/generated/chunks").exists():
    translated.mkdir(parents=True, exist_ok=True)
    shutil.copytree("/home/james/bw-build-0215/translated/dol", translated / "dol")
    shutil.copytree("/home/james/bw-build-0215/translated/rels", translated / "rels")
    print("copied translated source")
print("== generate ==")
b.mods = not args.no_mods
b.mods_pending = b.mods
b.generate()
print("== mods ==")
b.build_mods()
print("== prepare_blocks ==")
b.prepare_blocks()
print(f"DONE: prepared source at {OUT / 'composite-src'}")
