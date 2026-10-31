#!/usr/bin/env python3
"""PadMint entry point: keep the Mac builder, prepare an iOS module elsewhere."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
MODULE = "gGZLE01_recomp.dylib"
FEATURES = ("fixed_cpu", "fixed_mem1", "inline_fp", "gather_pipe", "direct_calls",
            "native_j3d", "native_vec", "native_math", "native_skin", "native_game_math")
EXTRA_FEATURES = ("native_entries", "native_gx", "native_set4")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepared_options(work):
    """Use the completed preparation's values, never guess from platform defaults."""
    receipt = json.loads((work / "prepared-blocks.json").read_text(encoding="utf-8"))
    actual = subprocess.check_output(
        [sys.executable, "-X", "utf8", str(ROOT / "scripts/ios/composite_manifest.py"), str(work / "composite-src")],
        text=True).split()[0]
    if receipt.get("final_digest") != actual or (work / "composite-final.digest").read_text().strip() != actual:
        raise ValueError("the prepared module source changed; rerun preparation before compiling")
    if any(type(receipt.get(name)) is not bool for name in FEATURES + EXTRA_FEATURES):
        raise ValueError("the preparation receipt is missing its module feature values; rerun preparation")
    # The iOS app recognizes these two configurations. Its readiness checks still
    # check the module, but an incomplete combination must not enable all defaults.
    values = [receipt[name] for name in FEATURES]
    if any(receipt[name] for name in EXTRA_FEATURES) or (any(values) and not all(values)):
        raise ValueError("the iOS app requires the complete combined-v1 preparation or the plain translation")
    flags = [f"-DBLUEWAKE_{name.upper()}={'ON' if receipt[name] else 'OFF'}"
             for name in FEATURES + EXTRA_FEATURES]
    return flags, "combined-v1" if all(values) else "none"


def write_result(work, module, mode):
    """The marker accompanies this exact module, including when clearing old state."""
    marker = work / "ModuleOptimizations"
    marker.write_bytes((mode + "\n").encode("ascii"))
    result = {
        "file": module.relative_to(work).as_posix(),
        "into": "Frameworks/" + MODULE,
        "sha256": digest(module),
        "companions": [{"file": marker.name, "into": marker.name, "sha256": digest(marker)}],
    }
    pending = work / "ios-module-result.json.tmp"
    pending.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    os.replace(pending, work / "ios-module-result.json")


def configure(command, build_dir, toolchain):
    """Refresh unknown caches; keep objects only for our exact known configuration."""
    inputs = hashlib.sha256(json.dumps(command).encode("utf-8")
                            + toolchain.read_bytes()
                            + (ROOT / "cmake/composite/CMakeLists.txt").read_bytes()).hexdigest()
    receipt = build_dir / "padmint-configure.json"
    cache = build_dir / "CMakeCache.txt"
    try:
        previous = json.loads(receipt.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        previous = {}
    known = (isinstance(previous, dict) and previous.get("inputs") == inputs
             and cache.is_file() and previous.get("cache") == digest(cache))
    # --fresh removes compiled objects too. Use it for an old draft, changed
    # toolchain/configuration or edited cache, not every ~800-chunk warm rerun.
    receipt.unlink(missing_ok=True)
    subprocess.run([command[0], *([] if known else ["--fresh"]), *command[1:]], check=True)
    pending = receipt.with_suffix(".json.tmp")
    pending.write_text(json.dumps({"inputs": inputs, "cache": digest(cache)}) + "\n", encoding="utf-8")
    os.replace(pending, receipt)


def build(args, system=None):
    system = system or platform.system()
    common = [str(args.disc), "--out", str(args.out), "--jobs", str(args.jobs)]
    if system == "Darwin":
        # Same argv and ordering as main's command/modes/options recipe. Mac
        # training, profile selection, preparation and packaging stay in build.sh.
        command = ["/bin/bash", str(ROOT / "scripts/builder/build.sh"), *common]
        if args.source_only:
            command += ["--source-only"]
        else:
            command += ["--app", str(args.app), "--train-pgo", "--ipa", str(args.ipa)]
        if args.no_mods:
            command.append("--no-mods")
        subprocess.run(command, check=True)
        return
    family = {"Windows": "windows", "Linux": "linux"}.get(system)
    if family is None:
        raise ValueError(f"unsupported build host: {system}")
    if not args.source_only and (not args.ios_toolchain or not Path(args.ios_toolchain).is_file()):
        raise ValueError("PadMint's iOS toolchain is missing; use a PadMint build with schema 2 host selection")
    work = args.out.resolve()
    work.mkdir(parents=True, exist_ok=True)
    # A failed new run cannot leave a previously successful result to consume.
    (work / "ios-module-result.json").unlink(missing_ok=True)
    command = [sys.executable, "-X", "utf8", str(ROOT / f"scripts/{family}/build.py"), *common,
               "--source-only" if args.source_only else "--module-sources"]
    if args.no_mods:
        command.append("--no-mods")
    subprocess.run(command, check=True)
    if args.source_only:
        return
    features, mode = prepared_options(work)
    build_dir = work / "composite-ios"
    runtime = ROOT / "ref/recompcore"
    # Current Apple builder's CPU and optimization level, without desktop tuning
    # flags or a claim that another host's profile replaces local Mac training.
    configure([
        "cmake", "-S", str(ROOT / "cmake/composite"), "-B", str(build_dir), "-G", "Ninja",
        f"-DCMAKE_TOOLCHAIN_FILE={Path(args.ios_toolchain).resolve()}", "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_C_FLAGS=-mcpu=apple-a13", "-DCOMPOSITE_OPTIMIZATION_LEVEL=2",
        "-DCOMPOSITE_COLD_SOURCES_FILE=", *features,
        f"-DCOMPOSITE_DIR={work / 'composite-src'}", f"-DGXRUNTIME_DIR={runtime / 'GXRuntime'}",
        f"-DABI_DIR={runtime / 'Source/Core/Core/PowerPC/StaticRecomp'}"], build_dir,
        Path(args.ios_toolchain).resolve())
    subprocess.run(["cmake", "--build", str(build_dir), "-j", str(args.jobs)], check=True)
    write_result(work, build_dir / MODULE, mode)
    print(f"iOS module and preparation settings ready: {work / 'ios-module-result.json'}")


def main():
    if sys.platform == "win32":
        for stream in (sys.stdout, sys.stderr):
            if hasattr(stream, "reconfigure"):
                stream.reconfigure(encoding="utf-8", errors="backslashreplace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disc", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--jobs", type=int, required=True)
    parser.add_argument("--ios-toolchain", default="")
    parser.add_argument("--app", type=Path)
    parser.add_argument("--ipa", type=Path)
    parser.add_argument("--train-pgo", action="store_true")  # legacy Mac mode
    parser.add_argument("--source-only", action="store_true")
    parser.add_argument("--no-mods", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if not args.source_only and (args.app is None or args.ipa is None or not args.train_pgo):
        parser.error("a full PadMint build needs --app, --train-pgo and --ipa")
    try:
        build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"builder: {error}\n")


if __name__ == "__main__":
    main()
