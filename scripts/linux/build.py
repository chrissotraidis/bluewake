#!/usr/bin/env python3
"""BlueWake Builder for Linux: turn your own game disc into your own game, on your PC.

    python scripts/linux/build.py DISC [--out build/linux] [options]

DISC is your own The Wind Waker (GameCube, USA GZLE01 revision 0) image, an
uncompressed .iso or .gcm. (Compressed Dolphin images are not converted here;
convert them to ISO in Dolphin first.)

The Linux counterpart of scripts/windows/build.py. It reads the bluewake
profile's pins (RecompCore, DolRecomp) and verified source digest, so every
builder translates the same code; docs/LINUX.md explains the port.

Steps, each logged under OUT/logs:
  1 tools        clang + lld + llvm-profdata, CMake 3.25+, Ninja, git, Python 3.10+
  2 dependencies the pinned RecompCore and DolRecomp sources (ref/recompcore)
  3 disc         check the disc id and revision (the disc is verified on extract)
  4 extract      main.dol and the 415 RELs from the disc
  5 translate    the game's PowerPC code to C (DolRecomp)
  6 generate     the composite source, compared with the verified digest
  7 mods         widescreen 16:9 and 16:10 and Better Wind Waker's options (--no-mods skips)
  8 prepare      the certified native accelerators, fixed CPU, direct calls and
                 gather pipe (--conservative skips them)
  9 train        local optimization profile from headless playbacks (--no-train skips)
 10 compile      the game module, gGZLE01_recomp.so (the long step)
 11 app          bluewake, Aurora (Vulkan/OpenGL through Dawn), SDL3 and the DSP
 12 package      the app folder OUT/BlueWake, ready to run

The app folder contains code translated from YOUR disc and a copy of the disc:
it is yours alone. Never share or upload it. Your saves live in
~/.local/share/BlueWake, outside the build, so rebuilding never touches them.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
PROFILE = ROOT / "scripts/builder/profiles/bluewake.sh"
MODULE = "gGZLE01_recomp.so"
GC_MAGIC = 0xC2339F3D


class BuildError(Exception):
    pass


def die(message):
    raise BuildError(message)


def step(title):
    print(f"\n==> {title}", flush=True)


def default_jobs():
    """All cores, but no more parallel compiles than memory allows: the large
    translated chunks take 1 to 3 GB each in clang, and running out of commit
    kills the compiler ("LLVM ERROR: out of memory"; compile_module retries)."""
    cores = os.cpu_count() or 8
    try:
        with open("/proc/meminfo") as mem:
            for line in mem:
                if line.startswith("MemAvailable:"):
                    available = int(line.split()[1]) * 1024
                    return max(1, min(cores, int(available // (2.5 * 2**30))))
    except OSError:
        pass
    return cores


def profile_value(name):
    """A NAME=value pin from the bluewake profile, the builders' one source."""
    match = re.search(rf"^{name}=(\S+)$", PROFILE.read_text(), re.M)
    if not match:
        die(f"{PROFILE} has no {name}")
    return match.group(1)


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(1 << 22), b""):
            digest.update(block)
    return digest.hexdigest()


def sync_tree(new, current):
    """Make `current` the same tree as `new`, replacing only files that differ,
    so the unchanged ones keep their timestamps and Ninja does not recompile
    them. `new` is removed."""
    current.mkdir(parents=True, exist_ok=True)
    wanted = set()
    for source in new.rglob("*"):
        rel = source.relative_to(new)
        target = current / rel
        wanted.add(rel)
        if source.is_dir():
            target.mkdir(exist_ok=True)
            continue
        if (target.is_file() and target.stat().st_size == source.stat().st_size
                and target.read_bytes() == source.read_bytes()):
            continue
        if target.is_dir():
            shutil.rmtree(target)
        target.parent.mkdir(parents=True, exist_ok=True)
        os.replace(source, target)
    for target in sorted(current.rglob("*"), reverse=True):
        if target.relative_to(current) not in wanted:
            if target.is_dir():
                shutil.rmtree(target)
            else:
                target.unlink()
    shutil.rmtree(new)


def tree_digest(root):
    """scripts/ios/composite_manifest.py's digest of a generated tree."""
    out = subprocess.check_output([sys.executable, str(ROOT / "scripts/ios/composite_manifest.py"), str(root)],
                                  text=True)
    return out.split()[0]


class Builder:
    def __init__(self, args):
        self.args = args
        self.out = args.out
        self.logs = self.out / "logs"
        self.env = os.environ
        self.recompcore = ROOT / "ref/recompcore"
        self.iso = None
        self.profile = None
        self.clang = None
        self.clang_version = None
        self.llvm_profdata = None

    # --- helpers -------------------------------------------------------
    def run(self, name, command, *, env=None, cwd=None, ninja=False):
        """Run a command with a complete log and progress every 15 seconds."""
        self.logs.mkdir(parents=True, exist_ok=True)
        log = self.logs / f"{name}.log"
        environment = dict(env or self.env or os.environ)
        if ninja:
            environment["NINJA_STATUS"] = "[%f/%t] "
        start = time.monotonic()
        print(f"  {name} (log: {log})", flush=True)
        with open(log, "wb") as stream:
            process = subprocess.Popen([str(c) for c in command], cwd=cwd or ROOT, stdout=stream,
                                       stderr=subprocess.STDOUT, env=environment)
            last = start
            while True:
                try:
                    status = process.wait(timeout=5)
                    break
                except subprocess.TimeoutExpired:
                    pass
                except KeyboardInterrupt:
                    process.terminate()
                    raise
                now = time.monotonic()
                if now - last >= 15:
                    last = now
                    detail = ""
                    try:
                        with open(log, "rb") as recent:
                            recent.seek(max(0, log.stat().st_size - 16384))
                            units = re.findall(rb"\[(\d+/\d+)\]", recent.read())
                        if units:
                            detail = f", {units[-1].decode()}"
                    except OSError:
                        pass
                    elapsed = int(now - start)
                    print(f"  {name}: {elapsed // 60}m {elapsed % 60:02d}s{detail}", flush=True)
        if status != 0:
            tail = log.read_bytes()[-4000:].decode(errors="replace")
            print(tail, file=sys.stderr)
            die(f"{name} failed (exit {status}); full log {log}")
        elapsed = int(time.monotonic() - start)
        if elapsed >= 60:
            print(f"  {name}: done in {elapsed // 60}m {elapsed % 60:02d}s", flush=True)
        return log

    def git(self, *args, cwd=None):
        return subprocess.check_output(["git", *args], cwd=cwd or ROOT, text=True,
                                       stderr=subprocess.DEVNULL).strip()

    # --- 1 tools -----------------------------------------------------
    def check_tools(self):
        if platform.system() != "Linux":
            die("this builder is for Linux; on Windows use scripts/windows/build.py")
        if platform.machine().lower() not in ("x86_64", "amd64"):
            die(f"an x86-64 Linux PC is required (this is {platform.machine()})")
        if sys.version_info < (3, 10):
            die("Python 3.10 or newer is required")
        for tool in ("git", "cmake", "ninja"):
            if shutil.which(tool) is None:
                die(f"missing {tool}: install Git, CMake 3.25+ and Ninja "
                    "(sudo apt install git cmake ninja-build)")
        version = subprocess.check_output(["cmake", "--version"], text=True).split()[2]
        if tuple(int(x) for x in version.split(".")[:2]) < (3, 25):
            die(f"CMake 3.25 or newer is required (found {version})")
        clang = shutil.which("clang")
        self.clang = clang
        if clang is None:
            die("missing clang: the host and the game module need it (sudo apt install clang)")
        clang_version = subprocess.check_output([clang, "--version"], text=True).splitlines()[0]
        major = int(re.search(r"version (\d+)", clang_version).group(1))
        if major < 17:
            die(f"clang 17 or newer is required ({clang_version})")
        self.clang_version = clang_version
        # Prefer llvm-profdata beside clang (they travel together in a normal
        # LLVM install), but fall back to PATH so distros that package it
        # separately (NixOS, Homebrew, split distro packages) work unchanged.
        self.llvm_profdata = shutil.which("llvm-profdata")
        beside = str(Path(clang).with_name("llvm-profdata"))
        if self.llvm_profdata is None and Path(beside).is_file():
            self.llvm_profdata = beside
        if self.llvm_profdata is None:
            die("llvm-profdata is missing; install it (sudo apt install llvm) "
                "or explicitly use --no-train for an untrained build")
        self.check_march()
        print(f"{clang_version}; cmake {version}; ninja "
              f"{subprocess.check_output(['ninja', '--version'], text=True).strip()}; "
              f"{self.args.jobs} jobs; -march={self.args.march}")

    def check_march(self):
        """The game module is compiled for --march; refuse a level this CPU lacks."""
        levels = {"x86-64": set(), "x86-64-v2": {"sse4.2", "popcnt"},
                  "x86-64-v3": {"sse4.2", "popcnt", "avx", "avx2", "fma", "bmi", "bmi2", "movbe", "lzcnt"}}
        if self.args.march not in levels:
            return
        probe = self.out / "tools/march_probe.c"
        probe.parent.mkdir(parents=True, exist_ok=True)
        probe.write_text(r"""#include <stdio.h>
int main(void) {
    printf("sse4.2=%d popcnt=%d movbe=%d fma=%d avx=%d avx2=%d bmi=%d bmi2=%d lzcnt=%d\n",
           __builtin_cpu_supports("sse4.2"), __builtin_cpu_supports("popcnt"),
           __builtin_cpu_supports("movbe"), __builtin_cpu_supports("fma"),
           __builtin_cpu_supports("avx"), __builtin_cpu_supports("avx2"),
           __builtin_cpu_supports("bmi"), __builtin_cpu_supports("bmi2"),
           __builtin_cpu_supports("lzcnt"));
    return 0;
}
""")
        exe = probe.with_suffix("")
        subprocess.run([self.clang, "-O1", str(probe), "-o", str(exe)], check=True, capture_output=True)
        report = subprocess.check_output([str(exe)], text=True).split()
        have = {name for name, bit in (item.split("=") for item in report) if bit == "1"}
        if not levels[self.args.march] <= have:
            fallback = "x86-64-v2" if levels["x86-64-v2"] <= have else "x86-64"
            print(f"this CPU lacks {self.args.march} ({', '.join(sorted(levels[self.args.march] - have))}); "
                  f"using -march={fallback}")
            self.args.march = fallback

    # --- 2 dependencies ------------------------------------------------
    def dependencies(self):
        sha = profile_value("RECOMPCORE_SHA")
        dolrecomp_sha = profile_value("DOLRECOMP_SHA")
        url = profile_value("RECOMPCORE_URL")
        rc = self.recompcore
        if not (rc / ".git").exists():
            if rc.exists() and any(rc.iterdir()):
                die(f"{rc} exists but is not a git checkout: move it aside and rerun")
            rc.mkdir(parents=True, exist_ok=True)
            subprocess.check_call(["git", "init", "-q"], cwd=rc)
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=rc, capture_output=True, text=True).stdout.strip()
        if head != sha:
            if self.git("status", "--porcelain", "--untracked-files=no", cwd=rc):
                die(f"{rc} has local changes and is not at {sha}: move it aside and rerun")
            print(f"fetching RecompCore {sha}")
            subprocess.run(["git", "remote", "remove", "bluewake"], cwd=rc, capture_output=True)
            subprocess.check_call(["git", "remote", "add", "bluewake", url], cwd=rc)
            self.run("recompcore-fetch", ["git", "-C", rc, "fetch", "--recurse-submodules=no", "--depth", "1",
                                          "bluewake", sha])
            subprocess.check_call(["git", "checkout", "-q", "--detach", "FETCH_HEAD"], cwd=rc)
        if self.git("rev-parse", "HEAD", cwd=rc) != sha:
            die(f"{rc} is not at {sha}")
        subprocess.check_call(["git", "submodule", "sync", "-q", "--", "DolRecomp"], cwd=rc)
        sub = rc / "DolRecomp"
        current = subprocess.run(["git", "rev-parse", "HEAD"], cwd=sub, capture_output=True, text=True).stdout.strip() \
            if (sub / ".git").exists() else ""
        if current != dolrecomp_sha:
            self.run("dolrecomp-fetch", ["git", "-C", rc, "submodule", "update", "--init", "--depth", "1", "--",
                                         "DolRecomp"])
        if self.git("rev-parse", "HEAD", cwd=sub) != dolrecomp_sha:
            die(f"{sub} is not at {dolrecomp_sha}")
        if self.git("status", "--porcelain", "--untracked-files=no", cwd=rc) or \
                self.git("status", "--porcelain", "--untracked-files=no", cwd=sub):
            die(f"{rc} has local changes; the build must use the pinned source exactly")
        print(f"RecompCore {sha}, DolRecomp {dolrecomp_sha}")

    # --- 3 disc ----------------------------------------------------------
    def disc(self):
        source = self.args.disc.resolve()
        if not source.is_file():
            die(f"disc image not found: {source}")
        if source.suffix.lower() not in (".iso", ".gcm"):
            die(f"this builder reads an uncompressed .iso or .gcm; convert {source.name} "
                "to ISO in Dolphin first (right-click the game, Convert File -> ISO)")
        with open(source, "rb") as disc:
            header = disc.read(0x20)
        if len(header) < 0x20 or int.from_bytes(header[0x1C:0x20], "big") != GC_MAGIC:
            die(f"{source} is not a GameCube disc image")
        if header[:6] != b"GZLE01":
            die(f"this is a GameCube disc, but not The Wind Waker (USA, GZLE01): its id is "
                f"{header[:6].decode(errors='replace')}")
        if header[7] != 0:
            die(f"this is GZLE01 revision {header[7]}; BlueWake supports revision 0 only")
        self.iso = source
        print(f"disc: {self.iso} (GZLE01 revision 0)")

    # --- tools built from source ---------------------------------------
    def build_dolrecomp(self):
        build = self.out / "dolrecomp"
        self.run("dolrecomp-configure", ["cmake", "-S", self.recompcore / "DolRecomp", "-B", build, "-G", "Ninja",
                                         "-DCMAKE_BUILD_TYPE=Release", "-DDOLRECOMP_WARNINGS_AS_ERRORS=OFF"])
        self.run("dolrecomp-build", ["cmake", "--build", build, "--target", "dolrecomp", "-j", self.args.jobs],
                 ninja=True)
        return build / "dolrecomp"

    def configure_app(self, build=None):
        """The app (host) build: Aurora on Dawn (Vulkan/OpenGL) with vendored SDL3,
        compiled with clang at the same CPU level as the game module (the FIFO
        worker's matrix work for Smooth Motion needs AVX2 and FMA to keep up)."""
        self.app_build = build or self.out / "app"
        self.run("app-configure", [
            "cmake", "-S", ROOT / "linux", "-B", self.app_build, "-G", "Ninja",
            "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++",
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=OFF",
            f"-DCMAKE_C_FLAGS=-march={self.args.march} -g",
            f"-DCMAKE_CXX_FLAGS=-march={self.args.march} -g",
            "-DAURORA_DAWN_PROVIDER=package", "-DAURORA_DAWN_LINKAGE=static",
            "-DAURORA_SDL3_PROVIDER=vendor", "-DAURORA_SDL3_LINKAGE=static"])

    # --- 4 extract -------------------------------------------------------
    def extract(self, iso, game):
        self.run("disc-extract-build", ["cmake", "--build", self.app_build, "--target", "bluewake_disc_extract"],
                 ninja=True)
        self.run("disc-extract", [self.app_build / "bluewake_disc_extract", iso, game])
        rels = len(list((game / "rels").glob("*.rel")))
        if rels != 415:
            die(f"expected 415 RELs in {game / 'rels'}, found {rels}")

    # --- 5 translate -------------------------------------------------------
    def translate(self, dol, out, rels=None, name="translate", sites=()):
        pending = out.with_name(out.name + ".new")
        shutil.rmtree(pending, ignore_errors=True)
        pending.mkdir(parents=True)
        self.run(f"{name}-dol", [self.dolrecomp, "--gamecube", "--backend", "c", "--cpu", "gekko",
                                 "--partition-instructions", "4096", *sites, dol, pending / "dol", "-j", self.args.jobs])
        if rels is not None:
            rels_arg = rels.as_posix().rstrip("/") + "/" if sites else rels
            self.run(f"{name}-rels", [self.dolrecomp, "--gamecube", "--backend", "c", "--cpu", "gekko",
                                      "--rel-base", "0xC0400000", *sites, rels_arg, pending / "rels",
                                      "-j", self.args.jobs])
        shutil.rmtree(out, ignore_errors=True)
        os.replace(pending, out)

    def composite(self, dol_dir, rels_dir, rels_bin, main_dol, out, name):
        shutil.rmtree(out, ignore_errors=True)
        self.run(name, [sys.executable, ROOT / "scripts/generate_composite.py", "--dol-dir", dol_dir,
                        "--rels-dir", rels_dir, "--rels-bin-dir", rels_bin, "--main-dol", main_dol,
                        "--output-dir", out])

    # --- 6 generate ------------------------------------------------------
    def generate(self):
        o = self.out
        new = o / "composite-src.new"
        self.composite(o / "translated/dol/generated", o / "translated/rels/generated/rels", o / "game/rels",
                       o / "game/main.dol", new, "composite-generate")
        expected = profile_value("COMPOSITE_DIGEST")
        digest = tree_digest(new)
        if digest == expected:
            print(f"composite source digest {digest}: the verified tree")
        elif self.args.accept_new_composite:
            print(f"composite source digest {digest} differs from the verified {expected} (accepted)")
        else:
            die(f"composite source digest {digest} differs from the verified {expected} (wrong disc revision "
                f"or translator?); --accept-new-composite overrides")
        # Keep an identical tree in place: rewriting 750 files would make the
        # compile start over. Mods and the prepared optimizations are part of
        # the recorded inputs.
        inputs = hashlib.sha256()
        inputs.update((f"{digest}\n{int(self.mods)}\n{int(self.args.prepared_blocks)}\n"
                       f"{int(self.args.fixed_cpu)}\n{int(self.args.fixed_mem1)}\n{int(self.args.inline_fp)}\n{int(self.args.gather_pipe)}\n{int(self.args.direct_calls)}\n{int(self.args.inline_gpr)}\n{int(self.args.native_j3d)}\n{int(self.args.native_vec)}\n{int(self.args.native_math)}\n{int(self.args.native_skin)}\n{int(self.args.native_game_math)}\n"
                       f"{int(self.args.lean_memory)}\n{int(self.args.native_entries)}\n").encode())
        for f in (sorted((ROOT / "scripts/mods").glob("*")) + sorted((ROOT / "mods/widescreen").glob("*.gecko"))
                  + [ROOT / "mods/betterww/options.txt", ROOT / "scripts/windows/fast_blocks.py",
                     ROOT / "scripts/windows/global_guest_cpu.py", ROOT / "scripts/windows/chunk_headers.py",
                     ROOT / "cmake/composite/inline_fp.h", ROOT / "cmake/composite/gather_pipe.h",
                     ROOT / "cmake/composite/gather_pipe.c", ROOT / "cmake/composite/gather_pipe_batch.h",
                     ROOT / "scripts/windows/direct_calls.py", ROOT / "cmake/composite/direct_calls.c",
                     ROOT / "cmake/composite/direct_calls.h", ROOT / "cmake/composite/inline_gpr.h",
                     ROOT / "cmake/composite/native_j3d.c", ROOT / "cmake/composite/native_j3d.h",
                     ROOT / "cmake/composite/native_vec.c", ROOT / "cmake/composite/native_vec.h",
                     ROOT / "scripts/windows/native_game_math.py", ROOT / "cmake/composite/native_game_math.c",
                     ROOT / "cmake/composite/native_game_math.h", ROOT / "scripts/windows/native_skin.py",
                     ROOT / "cmake/composite/native_skin.c", ROOT / "cmake/composite/native_skin.h",
                     ROOT / "cmake/composite/native_math.c", ROOT / "cmake/composite/native_math.h",
                     ROOT / "cmake/composite/native_work_pool.c", ROOT / "cmake/composite/native_work_pool.h",
                     ROOT / "scripts/windows/inline_save_restore_gpr.py",
                     ROOT / "cmake/composite/native_fifo.c", ROOT / "cmake/composite/native_fifo.h",
                     ROOT / "cmake/composite/native_bg.c", ROOT / "cmake/composite/native_bg.h",
                     ROOT / "cmake/composite/native_mtxcalc.c", ROOT / "cmake/composite/native_mtxcalc.h",
                     ROOT / "cmake/composite/native_search.c", ROOT / "cmake/composite/native_search.h",
                     ROOT / "scripts/windows/native_entries.py",
                     ROOT / "scripts/windows/lean_memory.py", Path(__file__)]):
            if f.is_file():
                inputs.update(f.read_bytes())
        if self.args.direct_calls or self.args.native_game_math:
            # The source-derived watch list is part of the prepared module.
            for folder in ("runtime/host/src", "linux/src"):
                for path in sorted((ROOT / folder).rglob("*")):
                    if path.suffix in (".c", ".h", ".cpp", ".mm", ".m"):
                        inputs.update(str(path.relative_to(ROOT)).encode())
                        inputs.update(path.read_bytes())
        inputs = inputs.hexdigest()
        current = o / "composite-src"
        saved = (o / "composite-final.digest").read_text().strip() if (o / "composite-final.digest").exists() else ""
        same_inputs = (o / "composite-inputs.digest").exists() and \
            (o / "composite-inputs.digest").read_text().strip() == inputs
        if current.exists() and same_inputs and saved and tree_digest(current) == saved:
            shutil.rmtree(new)
            print("the existing composite source is current")
            self.mods_pending = (o / "mods.done").read_text().strip() != "complete" \
                if (o / "mods.done").exists() else self.mods
        else:
            sync_tree(new, current)
            (o / "composite-src.digest").write_text(digest + "\n")
            (o / "composite-inputs.digest").write_text(inputs + "\n")
            (o / "composite-final.digest").write_text(digest + "\n")
            (o / "mods.done").write_text("pending\n")
            self.mods_pending = self.mods

    # --- 7 mods --------------------------------------------------------------
    def build_mods(self):
        self.run("mods", ["bash", ROOT / "scripts/mods/build_mods.sh", self.out, self.iso])
        o = self.out
        (o / "composite-final.digest").write_text(tree_digest(o / "composite-src") + "\n")
        (o / "mods.done").write_text("complete\n")

    # --- 8 prepare the accelerators ---------------------------------------
    def prepare_blocks(self):
        """Explicit generic optimization, after variants and before compilation.

        generate() verifies both the input fingerprint and final tree digest.
        Interrupted/edited preparation cannot be mistaken for finished work.
        Each script is portable Python and changes only what it can prove, so
        the unmodified translation remains wherever a native guard declines.
        """
        o = self.out
        script = ROOT / "scripts/windows/fast_blocks.py"
        cpu_script = ROOT / "scripts/windows/global_guest_cpu.py"
        # The accelerator scripts re-verify their certified fragments against
        # the source before rewriting it, so they are NOT safe to re-run on an
        # already-prepared tree: fast_blocks.py changes the bodies
        # native_game_math.py certifies, and a second pass fails the hash. The
        # receipt's final_digest is the tree after every enabled accelerator,
        # so if it matches the current tree under the same option set the phase
        # is already done -- skip it.
        receipt_path = o / "prepared-blocks.json"
        if receipt_path.exists():
            try:
                receipt = json.loads(receipt_path.read_text())
            except ValueError:
                receipt = {}
            option_names = ("enabled", "fixed_cpu", "fixed_mem1", "inline_fp", "gather_pipe",
                            "direct_calls", "inline_gpr", "native_j3d", "native_vec", "native_math",
                            "native_skin", "native_game_math", "lean_memory", "native_entries")
            arg_names = ("prepared_blocks", "fixed_cpu", "fixed_mem1", "inline_fp", "gather_pipe",
                         "direct_calls", "inline_gpr", "native_j3d", "native_vec", "native_math",
                         "native_skin", "native_game_math", "lean_memory", "native_entries")
            options_match = all(receipt.get(name) == bool(getattr(self.args, arg))
                                for name, arg in zip(option_names, arg_names))
            if options_match and receipt.get("final_digest") == tree_digest(o / "composite-src"):
                print("the existing composite source is already prepared")
                return
        if self.args.native_game_math:
            self.run("native-game-math", [sys.executable, ROOT / "scripts/windows/native_game_math.py",
                                          o / "composite-src"])
        if self.args.native_j3d:
            self.run("native-j3d", [sys.executable, ROOT / "scripts/mods/prepare_native_j3d.py", o / "composite-src"])
        if self.args.native_vec:
            self.run("native-vec", [sys.executable, ROOT / "scripts/mods/prepare_native_vec.py", o / "composite-src"])
        if self.args.native_math:
            self.run("native-math", [sys.executable, ROOT / "scripts/mods/prepare_native_math.py", o / "composite-src"])
        if self.args.native_skin:
            self.run("native-skin", [sys.executable, ROOT / "scripts/windows/native_skin.py", o / "composite-src"])
        if self.args.fixed_cpu:
            self.run("fixed-cpu", [sys.executable, cpu_script, o / "composite-src"])
        if self.args.inline_fp or self.args.gather_pipe:
            helpers = [sys.executable, ROOT / "scripts/windows/chunk_headers.py", o / "composite-src"]
            if self.args.inline_fp:
                helpers.append("--inline-fp")
            if self.args.gather_pipe:
                helpers.append("--gather-pipe")
            self.run("inline-helpers", helpers)
        if self.args.inline_gpr:
            self.run("inline-gpr", [sys.executable, ROOT / "scripts/windows/inline_save_restore_gpr.py",
                                     o / "composite-src"])
        if self.args.prepared_blocks:
            self.run("prepared-blocks", [sys.executable, script, o / "composite-src"])
        if self.args.direct_calls:
            self.run("direct-calls", [sys.executable, ROOT / "scripts/windows/direct_calls.py", o / "composite-src"])
        # Elliott Tate's Windows steps, off by default. Each changes only what it
        # can prove: lean_memory.py needs the prepaid copies' deadline test, and
        # native_entries.py hooks a native only where the translation hashes to the
        # one its comparison test was run on (it reports the rest as not hooked).
        if self.args.lean_memory:
            self.run("lean-memory", [sys.executable, ROOT / "scripts/windows/lean_memory.py", o / "composite-src"])
        if self.args.native_entries:
            self.run("native-entries", [sys.executable, ROOT / "scripts/windows/native_entries.py",
                                         o / "composite-src"])
        digest = tree_digest(o / "composite-src")
        receipt = {"enabled": self.args.prepared_blocks,
                   "fixed_cpu": self.args.fixed_cpu,
                   "fixed_mem1": self.args.fixed_mem1,
                   "inline_fp": self.args.inline_fp,
                   "gather_pipe": self.args.gather_pipe,
                   "direct_calls": self.args.direct_calls,
                   "inline_gpr": self.args.inline_gpr,
                   "native_j3d": self.args.native_j3d,
                   "native_vec": self.args.native_vec,
                   "native_math": self.args.native_math,
                   "native_skin": self.args.native_skin,
                   "native_game_math": self.args.native_game_math,
                   "lean_memory": self.args.lean_memory,
                   "native_entries": self.args.native_entries,
                   "gather_sha256": {name: sha256_file(ROOT / "cmake/composite" / name)
                                     for name in ("gather_pipe.h", "gather_pipe.c", "gather_pipe_batch.h")},
                   "inline_fp_script_sha256": sha256_file(ROOT / "scripts/windows/chunk_headers.py"),
                   "inline_fp_header_sha256": sha256_file(ROOT / "cmake/composite/inline_fp.h"),
                   "fixed_cpu_script_sha256": sha256_file(cpu_script),
                   "script_sha256": sha256_file(script),
                   "base_digest": (o / "composite-src.digest").read_text().strip(),
                   "final_digest": digest}
        pending = o / "prepared-blocks.json.tmp"
        pending.write_text(json.dumps(receipt, indent=2) + "\n")
        os.replace(pending, o / "prepared-blocks.json")
        pending = o / "composite-final.digest.tmp"
        pending.write_text(digest + "\n")
        os.replace(pending, o / "composite-final.digest")

    # --- 9 compile -----------------------------------------------------------
    def compile_module(self):
        flags = []
        if self.profile is not None:
            # The profile is a compiler input but not a header dependency: its
            # hash in the file name makes Ninja recompile when the counts change.
            # Code the training never ran is optimized as cold; that saves size
            # and costs nothing in the scenes that matter (docs/BUILDER.md).
            flags = [f"-fprofile-instr-use={self.profile.as_posix()}", "-Wno-profile-instr-unprofiled",
                     "-Wno-profile-instr-out-of-date", "-Wno-backend-plugin"]
            print(f"with the optimization profile {self.profile.name}")
            if getattr(self.args, "no_cold", False):
                # Code the training never ran (cutscenes, combat, bosses) is
                # optimized for speed like the rest, not for size: profile-guided
                # size optimization off, and every chunk at -O2 (docs/PERFORMANCE.md).
                flags += ["-mllvm", "-pgso=false"]
                print("--no-cold: code the training never ran is compiled for speed too")
        tiered = (self.profile is not None and not getattr(self.args, "no_tiered", False)
                  and not getattr(self.args, "no_cold", False))
        cold = self.cold_sources() if tiered else None
        return self.compile_composite(self.out / "composite", self.args.opt_level, flags, [], "composite", cold)

    def cold_sources(self):
        """The chunks whose function the training never ran, listed for
        cmake/composite to compile at -O1 without GVN's memory dependence
        analysis (the module's longest passes on its largest functions). A chunk
        is one function, named func_<its file's address>."""
        stats = subprocess.run([self.llvm_profdata, "show", "--all-functions", self.profile], capture_output=True,
                               text=True).stdout
        counts = {name.upper(): int(count) for name, count in
                  re.findall(r"(?m)^  func_([0-9A-Fa-f]+):\n(?:    .*\n)*?    Function count: (\d+)", stats)}
        src = self.out / "composite-src"
        cold = []
        for path in sorted(src.glob("chunks_*/*.c")):
            address = path.stem.rsplit("_", 1)[-1].upper()
            if counts.get(address) == 0:
                cold.append(path.relative_to(src).as_posix())
        listing = self.out / "composite-cold-sources.txt"
        listing.write_text("\n".join(cold) + "\n", encoding="utf-8")
        print(f"tiered: {len(cold)} chunks the training never ran at -O1")
        return listing

    def compile_composite(self, build, opt_level, extra_flags, extra_link_flags, name, cold=None):
        rc = self.recompcore
        # The build directory may hold a stale configure from an earlier
        # builder (the old Linux builder compiled the module with gcc). CMake
        # cannot switch compilers in place: it reconfigures against the stale
        # cache and the Threads probe (and friends) fail with a confusing
        # "Could NOT find Threads". Wipe a build dir whose cached compiler is
        # not clang, or whose cache is gone but its ninja file remains.
        cache = build / "CMakeCache.txt"
        stale = False
        if cache.exists():
            cached = re.search(r"^CMAKE_C_COMPILER:FILEPATH=(.*)$", cache.read_text(), re.M)
            if cached and "clang" not in cached.group(1):
                print(f"  {build} was configured with {cached.group(1)}; removing it for clang")
                stale = True
        elif (build / "build.ninja").exists():
            print(f"  {build} has no CMake cache; removing the stale tree for a clean configure")
            stale = True
        if stale:
            shutil.rmtree(build)
        # Each chunk is one very large function, and two LLVM passes are
        # superlinear on it (clang 22, x86-64, measured with -ftime-report):
        # - the SLP vectorizer took 92 percent of a typical large chunk's time;
        #   -fno-slp-vectorize took the 1.8 MB chunks from over 30 minutes each
        #   to about 2. There is little to vectorize across register moves.
        # - the register coalescer took 95 percent of d_a_movie_player's 44
        #   minutes, joining copies into the context pointer's function-long
        #   live interval over and over. Capping that per large interval took
        #   it to about 2 minutes, at the cost of a few register copies.
        flags = (f"-march={self.args.march} -fno-slp-vectorize "
                 "-mllvm -large-interval-freq-threshold=10")
        flags += " " + subprocess.list2cmdline(extra_flags)
        link_flags = subprocess.list2cmdline(["-fuse-ld=lld", *extra_link_flags])
        self.run(f"{name}-configure", [
            "cmake", "-S", ROOT / "cmake/composite", "-B", build, "-G", "Ninja", "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_C_FLAGS={flags}", f"-DCMAKE_SHARED_LINKER_FLAGS={link_flags}",
            f"-DBLUEWAKE_FIXED_CPU={'ON' if self.args.fixed_cpu else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_J3D={'ON' if self.args.native_j3d else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_VEC={'ON' if self.args.native_vec else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_GAME_MATH={'ON' if self.args.native_game_math else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_SKIN={'ON' if self.args.native_skin else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_MATH={'ON' if self.args.native_math else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_ENTRIES={'ON' if self.args.native_entries else 'OFF'}",
            f"-DBLUEWAKE_DIRECT_CALLS={'ON' if self.args.direct_calls else 'OFF'}",
            f"-DBLUEWAKE_GATHER_PIPE={'ON' if self.args.gather_pipe else 'OFF'}",
            f"-DBLUEWAKE_INLINE_FP={'ON' if self.args.inline_fp else 'OFF'}",
            f"-DBLUEWAKE_FIXED_MEM1={'ON' if self.args.fixed_mem1 else 'OFF'}",
            f"-DCOMPOSITE_OPTIMIZATION_LEVEL={opt_level}", f"-DCOMPOSITE_DIR={self.out / 'composite-src'}",
            f"-DGXRUNTIME_DIR={rc / 'GXRuntime'}", f"-DABI_DIR={rc / 'Source/Core/Core/PowerPC/StaticRecomp'}",
            f"-DCOMPOSITE_COLD_SOURCES_FILE={cold if cold is not None else ''}"])
        # -k 0: a chunk that fails does not stop the others. The usual cause is
        # memory (clang reports "out of memory" when several of the largest
        # chunks peak together), so what failed is retried with fewer jobs.
        jobs = self.args.jobs
        while True:
            try:
                self.run(f"{name}-build", ["cmake", "--build", build, "-j", jobs, "--", "-k", "0"], ninja=True)
                break
            except BuildError:
                log = (self.logs / f"{name}-build.log").read_text(errors="replace")
                if jobs <= 1 or "out of memory" not in log:
                    raise
                jobs = max(1, jobs // 2)
                print(f"  some chunks ran out of memory; compiling the rest with {jobs} jobs")
        module = build / MODULE
        if not module.exists():
            die("the game module was not produced")
        return module

    # --- local optimization training ---------------------------------------
    # The counterpart of scripts/windows/build.py's training: the game module is
    # compiled with LLVM's instrumentation, the normal app plays the opening to
    # player control headless with it, and the counts it records guide the
    # optimized compile. The profile is made from the game, so it is private and
    # stays in the build directory. Adapted from Elliott Tate's Windows builder.
    TRAINING_VERSION = "bluewake-2"  # the tour of the game (Elliott Tate's TRAINING_VERSION 4)
    TRAINING_RETRACES = 23000
    TRAINING_RUN = ["20400:0:700:0:127", "21100:0:500:90:110", "21600:0:500:-90:110", "22100:0:900:0:127"]
    TRAINING_TOUR = ["sea:11:1",        # Windfall Island
                     "sea:13:0",        # Dragon Roost Island
                     "M_NewD2:0:0",     # Dragon Roost Cavern
                     "sea:41:0",        # Forest Haven
                     "kindan:0:0",      # the Forbidden Woods
                     "Siren:0:0",       # the Tower of the Gods
                     "majroom:0:0",     # the Forsaken Fortress
                     "sea:1:100",       # the sea by the Fortress, on the boat
                     "Hyrule:0:0",      # Hyrule Castle
                     "sea:44:0"]        # back to Outset
    TOUR_START = 23200
    TOUR_STOP = 1500  # retraces at each stop

    def training_tour(self):
        """The tour's warps, its runs at each stop, and the retraces it ends at."""
        warps, moves = [], []
        for i, place in enumerate(self.TRAINING_TOUR):
            at = self.TOUR_START + i * self.TOUR_STOP
            warps.append(f"{at}:{place}")
            moves += [f"{at + 450}:0:300:0:127", f"{at + 780}:0:300:110:60", f"{at + 1110}:0:300:-110:60"]
        return warps, moves, self.TOUR_START + len(self.TRAINING_TOUR) * self.TOUR_STOP + 300

    def training_fingerprint(self):
        """Bind local counts to actual prepared source, compiler and playback code."""
        key = hashlib.sha256()
        key.update(json.dumps({"recipe": self.TRAINING_VERSION,
                               "compiler": self.clang_version, "march": self.args.march,
                               "mods": self.mods,
                               "options": {name: getattr(self.args, name) for name in
                                           ("prepared_blocks", "fixed_cpu", "fixed_mem1", "inline_fp",
                                            "gather_pipe", "direct_calls", "inline_gpr", "native_j3d",
                                            "native_vec", "native_math", "native_skin", "native_game_math",
                                            "lean_memory", "native_entries")},
                               "runtime": self.git("-C", str(self.recompcore), "rev-parse", "HEAD"),
                               "source": tree_digest(self.out / "composite-src")},
                              sort_keys=True).encode())
        for folder in ("cmake/composite", "runtime/host/src", "linux/src"):
            for path in sorted((ROOT / folder).rglob("*")):
                if path.is_file():
                    key.update(path.relative_to(ROOT).as_posix().encode())
                    key.update(path.read_bytes())
        key.update(Path(__file__).read_bytes())
        return key.hexdigest()

    def train(self):
        work = self.out / "pgo-local"
        work.mkdir(parents=True, exist_ok=True)
        profile = work / "composite.profdata"
        receipt = work / "training.json"
        key = self.training_fingerprint()
        if profile.exists() and receipt.exists() and not self.args.retrain:
            try:
                previous = json.loads(receipt.read_text())
            except ValueError:
                previous = {}
            if previous.get("fingerprint") == key and previous.get("profile") == sha256_file(profile):
                print("reusing the optimization profile trained for these inputs")
                return self.hashed_profile(profile)

        exe = self.build_app()
        start = time.monotonic()
        module = self.compile_composite(work / "composite", "0", ["-fprofile-instr-generate"],
                                        ["-fprofile-instr-generate"], "training-composite")
        print(f"instrumented game module built ({int(time.monotonic() - start) // 60} min)")
        attempt = Path(tempfile.mkdtemp(prefix="attempt-", dir=work))
        raw = []
        # The opening as a new player plays it, then again with widescreen and
        # Better Wind Waker's options, so the chunks those mods replace are
        # optimized for play too rather than as code that never ran.
        runs = [("plain", None)]
        if self.mods:
            runs.append(("mods", "widescreen,betterww"))
        for name, mods in runs:
            raw += self.training_run(exe, module, attempt / f"run-{name}", mods, tour=name == "plain")
        candidate = attempt / "composite.profdata"
        self.run("training-merge", [self.llvm_profdata, "merge", "-o", candidate, *raw])
        shown = subprocess.run([self.llvm_profdata, "show", "--all-functions", candidate], capture_output=True,
                               text=True, env=self.env)
        if shown.returncode:
            die(f"llvm-profdata could not read the new profile; previous profile retained (see {candidate})")
        stats = shown.stdout
        executed = [int(c) for c in re.findall(r"(?m)^  func_[0-9A-Fa-f]+\S*:\n(?:    .*\n)*?    Function count: (\d+)",
                                               stats)]
        ran = sum(1 for count in executed if count > 0)
        if ran == 0:
            die("the optimization profile counted no translated game functions")
        print(f"optimization profile: {ran} translated functions ran ({len(executed)} in the module)")
        os.replace(candidate, profile)
        pending = receipt.with_suffix(".tmp")
        pending.write_text(json.dumps({"fingerprint": key, "profile": sha256_file(profile),
                                       "trained": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                                       "executed_functions": ran}, indent=2))
        os.replace(pending, receipt)
        return self.hashed_profile(profile)

    def training_run(self, exe, module, run, mods, tour=False, headless=True):
        """One headless playback of the opening: boot, A at the title, the
        opening cutscene's text confirmed, player control on Outset, and with
        `tour` the tour of the game after it. A new card in its own folder; the
        player's saves are never touched."""
        warps, tour_moves, tour_end = self.training_tour() if tour else ([], [], self.TRAINING_RETRACES)
        run.mkdir(parents=True)
        env = {k: v for k, v in (self.env or os.environ).items() if not k.startswith(("BLUEWAKE_", "DOL_", "LLVM_PROFILE_"))}
        env.update({
            "LLVM_PROFILE_FILE": str(run / "%m-%p.profraw"),
            "BLUEWAKE_DATA_DIR": str(run), "BLUEWAKE_NO_DIALOG": "1",
            "BLUEWAKE_DOL": str(self.out / "game/main.dol"), "BLUEWAKE_RELS_DIR": str(self.out / "game/rels"),
            "BLUEWAKE_DISC": str(self.iso),
            "BLUEWAKE_DSP_IROM": str(self.recompcore / "Data/Sys/GC/dsp_rom.bin"),
            "BLUEWAKE_DSP_COEF": str(self.recompcore / "Data/Sys/GC/dsp_coef.bin"),
            "BLUEWAKE_MAX_RETRACES": str(tour_end), "BLUEWAKE_WALL_PACE": "0",
            "BLUEWAKE_PLAYER_PROBE": "1", "BLUEWAKE_PAD_BUTTONS": "0x0100",
            # The player-control milestone below waits on the overlap phase
            # this observation latches; the app turns it off for play.
            "BLUEWAKE_OVERLAP_OBSERVATION": "1",
            "BLUEWAKE_PAD_PULSE_ON_TITLE_READY": "1", "BLUEWAKE_PAD_PULSE_LENGTH": "2",
            "BLUEWAKE_PAD_CONFIRM_EVENT": "any",
            "BLUEWAKE_PAD_SCRIPT": ",".join([f"{n}:0x0100:2" for n in range(17800, 22001, 150)] + self.TRAINING_RUN +
                                            tour_moves),
        })
        env["DOL_AURORA_FRAME_INTERP"] = "0"
        for option, name in (("direct_calls", "BLUEWAKE_DIRECT_CALLS"),
                             ("gather_pipe", "BLUEWAKE_GATHER_PIPE"),
                             ("native_j3d", "BLUEWAKE_NATIVE_J3D"),
                             ("native_vec", "BLUEWAKE_NATIVE_VEC"),
                             ("native_math", "BLUEWAKE_NATIVE_MATH"),
                             ("native_skin", "BLUEWAKE_NATIVE_SKIN"),
                             ("native_game_math", "BLUEWAKE_NATIVE_GAME_MATH")):
            env[name] = "1" if getattr(self.args, option) else "0"
        if warps:
            env["BLUEWAKE_TEST_WARP"] = ",".join(warps)
        if headless:
            env["BLUEWAKE_RENDERER"] = "headless"
        if mods:
            env["BLUEWAKE_MODS"] = mods
        log = self.run(f"training-playback-{run.name[4:]}", [exe, "--module", module], env=env)
        text = log.read_text(errors="replace")
        if "[player-milestone] control-admitted" not in text:
            die(f"the training playback did not reach player control; profile rejected (see {log})")
        raw = sorted(run.glob("*.profraw"))
        if not raw:
            die(f"the training playback wrote no profile (see {log})")
        return raw

    def hashed_profile(self, profile):
        folder = self.out / "profiles"
        folder.mkdir(exist_ok=True)
        target = folder / f"composite-{sha256_file(profile)[:16]}.profdata"
        if not target.exists() or sha256_file(target) != sha256_file(profile):
            shutil.copy2(profile, target)
        return target

    # --- 10 app ---------------------------------------------------------------
    def build_app(self):
        self.run("app-build", ["cmake", "--build", self.app_build, "--target", "bluewake", "-j", self.args.jobs],
                 ninja=True)
        exe = self.app_build / "bluewake"
        if not exe.exists():
            die("bluewake was not produced")
        return exe

    # --- 11 package ----------------------------------------------------------
    def package(self, module):
        app = self.out / "BlueWake"
        (app / "game").mkdir(parents=True, exist_ok=True)
        (app / "dsp").mkdir(exist_ok=True)
        for f in sorted(self.app_build.iterdir()):
            if f.is_file() and f.name in ("bluewake", "bluewake_disc_extract"):
                shutil.copy2(f, app / f.name)
            elif f.is_file() and f.name == "initial_pipeline_cache.db":
                shutil.copy2(f, app / f.name)
            elif f.is_file() and (f.name.startswith("libSDL3") or f.name.startswith("libwebgpu_dawn")):
                shutil.copy2(f, app / f.name)
        shutil.copy2(module, app / MODULE)
        shutil.copy2(self.out / "game/main.dol", app / "game/main.dol")
        rels = app / "game/rels"
        shutil.rmtree(rels, ignore_errors=True)
        shutil.copytree(self.out / "game/rels", rels)
        for name in ("dsp_rom.bin", "dsp_coef.bin"):
            shutil.copy2(self.recompcore / "Data/Sys/GC" / name, app / "dsp" / name)
        self.place(self.iso, app / "game/GZLE01.iso")
        dirty = bool(self.git("status", "--porcelain"))
        provenance = {
            "profile": "bluewake-linux",
            "containsTranslatedGameCode": True,
            "source_commit": self.git("rev-parse", "HEAD"),
            "source_modified": dirty,
            "composite_digest": (self.out / "composite-src.digest").read_text().strip(),
            "mods": bool(self.mods),
            "march": self.args.march,
            "no_cold": getattr(self.args, "no_cold", False),
            "prepared_blocks": self.args.prepared_blocks,
            "fixed_cpu": self.args.fixed_cpu,
            "fixed_mem1": self.args.fixed_mem1,
            "inline_fp": self.args.inline_fp,
            "gather_pipe": self.args.gather_pipe,
            "direct_calls": self.args.direct_calls,
            "inline_gpr": self.args.inline_gpr,
            "native_j3d": self.args.native_j3d,
            "native_vec": self.args.native_vec,
            "native_math": self.args.native_math,
            "native_skin": self.args.native_skin,
            "native_game_math": self.args.native_game_math,
            "local_training": self.profile is not None,
            "composite_profile_sha256": sha256_file(self.profile) if self.profile else "",
            "compiler": self.clang_version,
            "module_sha256": sha256_file(app / MODULE),
            "built": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }
        (app / "BuilderProvenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
        (app / "README.txt").write_text(README)
        return app

    @staticmethod
    def place(source, target):
        """The disc goes beside the game: a hard link when it can, else a copy."""
        if target.exists():
            if os.path.samefile(source, target) or \
                    (target.stat().st_size == source.stat().st_size and
                     target.stat().st_mtime_ns >= source.stat().st_mtime_ns):
                return
            target.unlink()
        try:
            os.link(source, target)
        except OSError:
            print(f"copying {source.name} into the app folder ({source.stat().st_size >> 20} MB)")
            shutil.copy2(source, target)

    # --- the pipeline ------------------------------------------------------------
    def build(self):
        args = self.args
        self.mods = not args.no_mods
        self.mods_pending = self.mods
        print(f"Building The Legend of Zelda: The Wind Waker (GameCube USA GZLE01 rev 0) "
              f"for Linux from {args.disc}")
        step("1/10 tools")
        self.check_tools()
        step("2/10 dependencies")
        self.dependencies()
        step("3/10 disc")
        self.disc()
        if args.check_only:
            print("\ntools, dependencies and disc are ready.")
            return
        step("4/10 extract the game from the disc")
        self.dolrecomp = self.build_dolrecomp()
        self.configure_app()
        game = self.out / "game"
        stamp = self.out / "game.json"
        key = {"iso": str(self.iso), "size": self.iso.stat().st_size, "mtime": self.iso.stat().st_mtime_ns,
               "extractor": sha256_file(ROOT / "apple/ios/src/disc_import.c")}
        if (game / "main.dol").exists() and stamp.exists() and json.loads(stamp.read_text()) == key:
            print(f"reusing main.dol and the RELs in {game}")
        else:
            self.extract(self.iso, game)
            stamp.write_text(json.dumps(key))
            print(f"main.dol and 415 RELs in {game}")
        step("5/10 translate")
        self.translate(game / "main.dol", self.out / "translated", game / "rels")
        chunks = len(list((self.out / "translated/dol/generated/chunks").glob("*.c")))
        print(f"translated: {chunks} DOL chunks and 415 RELs")
        step("6/10 generate the composite source")
        self.generate()
        if args.source_only:
            print(f"\nsource check passed: {self.out / 'composite-src'}. Rerun without --source-only to "
                  f"compile and build the app.")
            return
        step("7/10 mods")
        if not self.mods:
            print("skipped")
        elif not self.mods_pending:
            print("mods already in the composite source")
        else:
            self.build_mods()
        self.prepare_blocks()
        if not (args.no_train or args.no_pgo):
            step("local optimization training (instrumented module and private opening playbacks)")
            self.profile = self.train()
        else:
            print("local training skipped: compiling without an optimization profile")
        step(f"8/10 compile the game module (-O{args.opt_level}, -march={args.march}; this is the long step)")
        start = time.monotonic()
        module = self.compile_module()
        print(f"game module: {module} ({int(time.monotonic() - start) // 60} min)")
        step("9/10 build the app")
        exe = self.build_app()
        print(f"app: {exe}")
        step("10/10 package")
        app = self.package(module)
        print(f"\nBlueWake: {app}")
        print(f"  run {app / 'bluewake'}")
        print("  It contains game code translated from your disc and a copy of the disc: keep it for yourself.")
        print("  Saves: ~/.local/share/BlueWake (outside the build).")


README = """BlueWake for Linux: The Legend of Zelda: The Wind Waker (GameCube USA),
statically recompiled from your own disc. See docs/LINUX.md in the source.

This folder is a personal build: gGZLE01_recomp.so is code translated from
your disc and game/ holds your disc image. Never share or upload it.

Run ./bluewake. Options (./bluewake --help lists them all):
  --widescreen    16:9 (--aspect 16:10 for 16:10)
  --smooth        Smooth Motion: 60 FPS with in-between frames
  --betterww      Better Wind Waker's settings (--options to change them)
  --fullscreen    start in fullscreen

Keyboard: arrows D-pad, J A, K B, U X, I Y, W/A/S/D control stick,
H/F/T/G C-stick, E/R L/R, Q Z, Return START. Game controllers work too.
Mouse: click the game and move the mouse to turn the camera; Esc releases it.
F11 fullscreen, F10 Smooth Motion, F9 frame rate.

Saves, settings and session logs: ~/.local/share/BlueWake
"""


# The optimizations Wind Waker Recomp's Windows builder always prepares (fixed
# CPU and RAM storage, inline floating point and gather-pipe writes, inlined
# register saves, prepaid blocks, direct calls and the certified natives). The
# Linux builder matches that by default so a Linux build behaves the same as a
# Windows one; --conservative builds the plain translation. The app enables each
# one only where the module it loads was prepared with it.
LINUX_DEFAULT_OPTIMIZATIONS = ("fixed_cpu", "fixed_mem1", "inline_fp", "gather_pipe", "inline_gpr",
                               "prepared_blocks", "direct_calls", "native_j3d", "native_vec", "native_math",
                               "native_skin", "native_game_math")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("disc", type=Path, help="your GZLE01 revision 0 disc image (.iso or .gcm)")
    parser.add_argument("--out", type=Path, default=ROOT / "build/linux",
                        help="build directory (default build/linux; must be git-ignored inside the checkout)")
    parser.add_argument("--jobs", type=int, default=None,
                        help="parallel compile jobs (default: the cores, limited by free memory)")
    parser.add_argument("--march", default="x86-64-v3",
                        help="CPU level for the game module (default x86-64-v3: AVX2, FMA, BMI2 and MOVBE, "
                             "any Intel Haswell or AMD Zen or newer; lowered automatically on older CPUs)")
    parser.add_argument("--opt-level", choices=("1", "2"), default="2", help="game module optimization level")
    parser.add_argument("--no-tiered", action="store_true",
                        help="compile every chunk at -O2, not only those the optimization training ran "
                             "(a build about 25 minutes longer)")
    parser.add_argument("--no-cold", action="store_true",
                        help="compile code the optimization training never ran for speed, like the code it ran: "
                             "implies --no-tiered and turns off profile-guided size optimization (a longer build; "
                             "an experiment for scenes the training skips, docs/PERFORMANCE.md)")
    parser.add_argument("--no-mods", action="store_true", help="skip the widescreen and Better Wind Waker variants")
    parser.add_argument("--no-train", action="store_true",
                        help="skip local optimization training; compile without a profile")
    parser.add_argument("--no-pgo", action="store_true", help="alias for --no-train")
    parser.add_argument("--retrain", action="store_true", help="record a new local profile instead of reusing one")
    parser.add_argument("--prepared-blocks", action="store_true",
                        help="opt into experimental prepaid-block optimization (off by default; timing pending)")
    parser.add_argument("--fixed-cpu", action="store_true",
                        help="opt into experimental fixed-address CPU state; requires a supporting app")
    parser.add_argument("--fixed-mem1", action="store_true",
                        help="opt into module-owned RAM; requires --fixed-cpu and a supporting app")
    parser.add_argument("--inline-fp", action="store_true",
                        help="opt into experimental inline floating-point helpers (off by default)")
    parser.add_argument("--gather-pipe", action="store_true",
                        help="opt into experimental gather/inline-memory wrappers (off by default; host writer setup is separate)")
    parser.add_argument("--inline-gpr", action="store_true",
                        help="inline certified register saves/restores; requires --direct-calls")
    parser.add_argument("--direct-calls", action="store_true",
                        help="opt into direct-call preparation (off by default; compatible host selection required)")
    parser.add_argument("--native-j3d", action="store_true",
                        help="prepare certified native J3D transforms; off by default, compatible host opt-in required")
    parser.add_argument("--native-vec", action="store_true",
                        help="prepare certified native vector functions; off by default, compatible host opt-in required")
    parser.add_argument("--native-game-math", action="store_true",
                        help="certify optional native game-math entry hooks (off by default)")
    parser.add_argument("--native-skin", action="store_true",
                        help="certify and enable optional native skinning preparation (off by default)")
    parser.add_argument("--native-math", action="store_true",
                        help="prepare certified native matrix functions; off by default, compatible host opt-in required")
    parser.add_argument("--lean-memory", action="store_true",
                        help="Wind Waker Recomp's lean loads and stores in prepaid copies (off by default; "
                             "needs --prepared-blocks)")
    parser.add_argument("--native-entries", action="store_true",
                        help="Wind Waker Recomp's certified native entries, second and third sets (off by default; "
                             "needs --direct-calls, --gather-pipe and --native-vec)")
    parser.add_argument("--conservative", action="store_true",
                        help="build the plain translation, without the optimizations prepared by default "
                             "(the individual --... options then add them one at a time)")
    parser.add_argument("--accept-new-composite", action="store_true",
                        help="continue if the generated source differs from the verified one")
    parser.add_argument("--source-only", action="store_true",
                        help="stop after generating the source: checks tools, disc and translation in minutes")
    parser.add_argument("--check-only", action="store_true", help="check tools, dependencies and the disc only")
    args = parser.parse_args()
    # Wind Waker Recomp's Windows builds prepare all of these every time; BlueWake
    # matches that by default. --conservative builds the plain translation.
    if not args.conservative:
        for name in LINUX_DEFAULT_OPTIMIZATIONS:
            setattr(args, name, True)
    if args.inline_gpr and not args.direct_calls:
        parser.error("--inline-gpr requires --direct-calls")
    if args.fixed_mem1 and not args.fixed_cpu:
        parser.error("--fixed-mem1 requires --fixed-cpu")
    if args.lean_memory and not args.prepared_blocks:
        parser.error("--lean-memory requires --prepared-blocks")
    if args.native_entries and not (args.direct_calls and args.gather_pipe and args.native_vec):
        parser.error("--native-entries requires --direct-calls, --gather-pipe and --native-vec")
    if args.jobs is None:
        args.jobs = default_jobs()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    args.out = args.out.resolve()
    try:
        rel = args.out.relative_to(ROOT)
    except ValueError:
        rel = None
    if rel is not None:
        if str(rel) == ".":
            parser.error("--out must not be the checkout itself; use build/linux")
        ignored = subprocess.run(["git", "check-ignore", "-q", str(args.out) + os.sep], cwd=ROOT).returncode == 0
        if not ignored:
            parser.error("--out inside this checkout must be git-ignored; use build/linux")
    args.out.mkdir(parents=True, exist_ok=True)
    try:
        Builder(args).build()
    except BuildError as error:
        print(f"\nbuilder: {error}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nbuilder: interrupted; rerun the same command to continue", file=sys.stderr)
        sys.exit(130)


if __name__ == "__main__":
    main()
