#!/usr/bin/env python3
"""The measured, opt-in Mac configuration, using existing donor preparations."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
MODES = ("none", "combined-v1")
FEATURES = ("FIXED_CPU", "FIXED_MEM1", "INLINE_FP", "GATHER_PIPE", "DIRECT_CALLS",
            "NATIVE_J3D", "NATIVE_VEC", "NATIVE_MATH", "NATIVE_SKIN", "NATIVE_GAME_MATH")
HOST_DEFAULTS = {"BLUEWAKE_" + name: "1" for name in
                 ("DIRECT_CALLS", "GATHER_PIPE", "GATHER_PIPE_BATCH", "NATIVE_J3D",
                  "NATIVE_VEC", "NATIVE_MATH", "NATIVE_SKIN", "NATIVE_GAME_MATH")}
HOST_DEFAULTS["BLUEWAKE_NATIVE_WORKERS"] = "2"
# Same order as the qualified Windows preparation path. Certification precedes
# CPU/block/direct rewrites; no new optimization is introduced here.
STEPS = (
    ("scripts/windows/native_game_math.py",),
    ("scripts/mods/prepare_native_j3d.py",),
    ("scripts/mods/prepare_native_vec.py",),
    ("scripts/mods/prepare_native_math.py",),
    ("scripts/windows/native_skin.py",),
    ("scripts/windows/global_guest_cpu.py",),
    ("scripts/windows/chunk_headers.py", "--inline-fp", "--gather-pipe"),
    ("scripts/windows/inline_save_restore_gpr.py",),
    ("scripts/windows/fast_blocks.py",),
    ("scripts/windows/direct_calls.py",),
)


def cmake_flags(mode):
    if mode not in MODES:
        raise ValueError(f"unknown module optimization mode: {mode}")
    return [f"-DBLUEWAKE_{name}={'ON' if mode == 'combined-v1' else 'OFF'}" for name in FEATURES]


def fingerprint(mode):
    if mode == "none":
        return "none"
    digest = hashlib.sha256(mode.encode() + Path(__file__).read_bytes())
    for folder in ("scripts/windows", "scripts/mods", "cmake/composite", "runtime/host/src", "windows/src"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.is_file() and path.suffix in (".py", ".c", ".h", ".cpp", ".mm", ".m"):
                digest.update(str(path.relative_to(ROOT)).encode() + b"\0" + path.read_bytes())
    return digest.hexdigest()


def tree_digest(path):
    return subprocess.check_output([sys.executable, str(ROOT / "scripts/ios/composite_manifest.py"),
                                    str(path)], text=True).split()[0]


def prepare(out, mode):
    if mode == "none":
        return
    source = out / "composite-src"
    receipt = out / "module-optimizations.json"
    key, before = fingerprint(mode), tree_digest(source)
    try:
        previous = json.loads(receipt.read_text())
    except (OSError, ValueError):
        previous = {}
    if previous.get("mode") == mode and previous.get("recipe") == key and previous.get("digest") == before:
        print("module preparation: reusing verified combined source")
        return
    # A failed preparation never updates the final digest. The builder's next
    # source stage preserves the partial tree and regenerates the verified base.
    for script, *flags in STEPS:
        subprocess.run([sys.executable, str(ROOT / script), str(source), *flags], check=True)
    after = tree_digest(source)
    receipt.write_text(json.dumps({"mode": mode, "recipe": key, "digest": after}, indent=2) + "\n")
    (out / "composite-final.digest").write_text(after + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("fingerprint", "flags", "prepare"))
    parser.add_argument("mode", choices=MODES)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.action == "fingerprint":
        print(fingerprint(args.mode))
    elif args.action == "flags":
        print("\n".join(cmake_flags(args.mode)))
    elif args.out is None:
        parser.error("prepare requires --out")
    else:
        prepare(args.out, args.mode)


if __name__ == "__main__":
    main()
