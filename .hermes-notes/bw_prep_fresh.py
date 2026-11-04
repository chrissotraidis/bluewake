#!/usr/bin/env python3
"""Fresh android source prep from the disc (no bw-build-0215 cache reuse).
Full extract + translate + generate + mods + prepare_blocks, outputting the
prepared composite source bw_android.py compiles (prepared-behaviour config)."""
import sys, types, subprocess
from pathlib import Path

REPO = Path("/home/james/nfs/Projects/bluewake")
sys.path.insert(0, str(REPO / "scripts/linux"))
import build as linux_build

OUT = Path("/home/james/bw-build-android/prepared-behaviour")
ISO = Path("/home/james/Downloads/loztww.iso")

args = types.SimpleNamespace(
    disc=ISO, out=OUT, jobs=16, march="x86-64-v3", opt_level="2",
    no_mods=False, no_train=True, no_pgo=True, retrain=False, tour_playbacks=None,
    prepared_blocks=True, fp_checks=True, direct_gate=True,
    fixed_cpu=True, fixed_mem1=True, inline_fp=True, gather_pipe=True,
    inline_gpr=True, direct_calls=True,
    native_j3d=True, native_vec=True, native_math=True,
    native_skin=True, native_game_math=True,
    lean_memory=True, native_entries=False, lean_blocks=True,
    lean_deadlines=True, behaviour_timing=True,
    accept_new_composite=False,
    appimage=False, ci_container=False,
    no_tiered=False, no_cold=False,
)

b = linux_build.Builder(args)
print("== tools ==", flush=True)
b.check_tools()
print("== dependencies ==", flush=True)
b.dependencies()
print("== disc ==", flush=True)
b.disc()
print("== dolrecomp ==", flush=True)
b.dolrecomp = b.build_dolrecomp()

print("== extractor (clang, direct) ==", flush=True)
extractor_dir = OUT / "extractor"
extractor_dir.mkdir(parents=True, exist_ok=True)
extractor = extractor_dir / "bluewake_disc_extract"
if not extractor.exists():
    r = subprocess.run(["clang", "-O2", "-I", str(REPO / "apple/ios/src"),
                        str(REPO / "scripts/ios/disc_extract.c"),
                        str(REPO / "apple/ios/src/disc_import.c"),
                        "-o", str(extractor)], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-4000:], file=sys.stderr)
        raise SystemExit("extractor build failed")

print("== extract ==", flush=True)
game = OUT / "game"
game.mkdir(parents=True, exist_ok=True)
r = subprocess.run([str(extractor), str(ISO), str(game)], capture_output=True, text=True)
if r.returncode != 0:
    print((r.stdout or "")[-2000:], file=sys.stderr)
    print((r.stderr or "")[-2000:], file=sys.stderr)
    raise SystemExit("extract failed")
rels = len(list((game / "rels").glob("*.rel")))
if rels != 415:
    raise SystemExit(f"expected 415 RELs, found {rels}")
print(f"extracted main.dol + {rels} RELs", flush=True)

print("== translate ==", flush=True)
b.translate(game / "main.dol", OUT / "translated", game / "rels")

print("== generate ==", flush=True)
b.mods = not args.no_mods
b.mods_pending = b.mods
b.generate()

print("== mods ==", flush=True)
b.build_mods()

print("== prepare_blocks ==", flush=True)
b.prepare_blocks()

print(f"DONE: prepared source at {OUT / 'composite-src'}", flush=True)
