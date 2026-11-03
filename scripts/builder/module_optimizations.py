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


def steps(lean=False, lean_memory=False, lean_deadlines=False, behaviour_timing=False):
    """STEPS, with fast_blocks.py making Elliott Tate's lean copies when asked, as the
    Windows and Linux builders do by default (--lean-blocks), and his lean_memory.py
    after the direct calls, where the Windows builder runs it (--lean-memory), then
    lean_deadlines.py right after it (--lean-deadlines), and behaviour_timing.py last
    (--behaviour-timing, a prototype)."""
    out = []
    for step in STEPS:
        out.append(step + ("--lean",) if lean and step[0] == "scripts/windows/fast_blocks.py" else step)
        if lean_memory and step[0] == "scripts/windows/direct_calls.py":
            out.append(("scripts/windows/lean_memory.py",))
            if lean_deadlines:
                out.append(("scripts/windows/lean_deadlines.py",))
    if behaviour_timing:
        out.append(("scripts/windows/behaviour_timing.py", "--loop-checks"))
    return tuple(out)


def fingerprint(mode, lean=False, lean_memory=False, lean_deadlines=False, behaviour_timing=False):
    if mode == "none":
        return "none"
    digest = hashlib.sha256(mode.encode() + Path(__file__).read_bytes())
    if lean:
        digest.update(b"\0lean-blocks")
    if lean_memory:
        digest.update(b"\0lean-memory")
    if lean_deadlines:
        digest.update(b"\0lean-deadlines")
    if behaviour_timing:
        digest.update(b"\0behaviour-timing")
    for folder in ("scripts/windows", "scripts/mods", "cmake/composite", "runtime/host/src", "windows/src"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.is_file() and path.suffix in (".py", ".c", ".h", ".cpp", ".mm", ".m"):
                digest.update(str(path.relative_to(ROOT)).encode() + b"\0" + path.read_bytes())
    return digest.hexdigest()


def tree_digest(path):
    return subprocess.check_output([sys.executable, str(ROOT / "scripts/ios/composite_manifest.py"),
                                    str(path)], text=True).split()[0]


def prepare(out, mode, lean=False, lean_memory=False, lean_deadlines=False, behaviour_timing=False):
    if mode == "none":
        return
    source = out / "composite-src"
    receipt = out / "module-optimizations.json"
    key, before = fingerprint(mode, lean, lean_memory, lean_deadlines, behaviour_timing), tree_digest(source)
    try:
        previous = json.loads(receipt.read_text())
    except (OSError, ValueError):
        previous = {}
    if previous.get("mode") == mode and previous.get("recipe") == key and previous.get("digest") == before:
        print("module preparation: reusing verified combined source")
        return
    # A failed preparation never updates the final digest. The builder's next
    # source stage preserves the partial tree and regenerates the verified base.
    for script, *flags in steps(lean, lean_memory, lean_deadlines, behaviour_timing):
        subprocess.run([sys.executable, str(ROOT / script), str(source), *flags], check=True)
    after = tree_digest(source)
    receipt.write_text(json.dumps({"mode": mode, "recipe": key, "digest": after}, indent=2) + "\n")
    (out / "composite-final.digest").write_text(after + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("fingerprint", "flags", "prepare"))
    parser.add_argument("mode", choices=MODES)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--lean", action="store_true", help="lean block copies (fast_blocks.py --lean)")
    parser.add_argument("--lean-memory", action="store_true", help="lean_memory.py after the direct calls")
    parser.add_argument("--lean-deadlines", action="store_true", help="lean_deadlines.py after lean_memory.py")
    parser.add_argument("--behaviour-timing", action="store_true",
                        help="behaviour_timing.py --loop-checks last: every block prepaid, the budget checked at loops (a prototype)")
    args = parser.parse_args()
    if args.lean_memory and not args.lean:
        parser.error("--lean-memory needs --lean")
    if args.lean_deadlines and not args.lean_memory:
        parser.error("--lean-deadlines needs --lean-memory")
    if args.action == "fingerprint":
        print(fingerprint(args.mode, args.lean, args.lean_memory, args.lean_deadlines, args.behaviour_timing))
    elif args.action == "flags":
        print("\n".join(cmake_flags(args.mode)))
    elif args.out is None:
        parser.error("prepare requires --out")
    else:
        prepare(args.out, args.mode, args.lean, args.lean_memory, args.lean_deadlines, args.behaviour_timing)


if __name__ == "__main__":
    main()
