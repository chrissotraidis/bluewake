#!/usr/bin/env python3
"""BlueWake Builder for Windows: turn your own game disc into your own game, on your PC.

    python scripts/windows/build.py DISC [--out build/windows] [options]

DISC is your own The Wind Waker (GameCube, USA GZLE01 revision 0) image: an .iso or
.gcm, or a Dolphin .rvz/.wia/.gcz/.ciso/.nfs image (converted to an ISO with
nodtool, which is built from crates.io with Rust's cargo on first use).

The Windows counterpart of scripts/builder/build.sh with the bluewake profile.
It reads that profile's pins (RecompCore, DolRecomp) and verified source digest,
so both builders translate the same code; docs/WINDOWS.md explains the port.

Steps, each logged under OUT/logs:
  1 tools        Visual Studio's clang and the Windows SDK, CMake 3.25+, Ninja, git
  2 dependencies the pinned RecompCore and DolRecomp sources (ref/recompcore)
  3 disc         check the disc; convert a compressed image to an ISO
  4 extract      main.dol and the 415 RELs from the disc (the disc is verified)
  5 translate    the game's PowerPC code to C (DolRecomp)
  6 generate     the composite source, compared with the verified digest
  7 mods         widescreen 16:9 and 16:10 and Better Wind Waker's options (--no-mods skips)
  8 compile      the game module, gGZLE01_recomp.dll (the long step)
  9 app          BlueWake.exe, Aurora (Direct3D 12 through Dawn), SDL3 and the DSP
 10 package      the app folder OUT/BlueWake, ready to run

The app folder contains code translated from YOUR disc and a copy of the disc:
it is yours alone. Never share or upload it. Your saves live in
%APPDATA%\\BlueWake, outside the build, so rebuilding never touches them.
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
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
PROFILE = ROOT / "scripts/builder/profiles/bluewake.sh"
MODULE = "gGZLE01_recomp.dll"
# nodtool (https://github.com/encounter/nod) converts Dolphin's compressed
# formats to ISO and checks the result against Redump.
NODTOOL_VERSION = "2.0.0-alpha.9"
DISC_FORMATS = (".rvz", ".wia", ".gcz", ".ciso", ".nfs", ".wbfs", ".tgc")
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
        import ctypes

        class MemoryStatus(ctypes.Structure):
            _fields_ = [("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong),
                        ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                        ("ullTotalPageFile", ctypes.c_ulonglong), ("ullAvailPageFile", ctypes.c_ulonglong),
                        ("ullTotalVirtual", ctypes.c_ulonglong), ("ullAvailVirtual", ctypes.c_ulonglong),
                        ("ullAvailExtendedVirtual", ctypes.c_ulonglong)]
        status = MemoryStatus()
        status.dwLength = ctypes.sizeof(MemoryStatus)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
            available = min(status.ullAvailPhys, status.ullAvailPageFile)
            return max(1, min(cores, int(available // (2.5 * 2**30))))
    except (AttributeError, OSError):
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
        self.env = None
        self.recompcore = ROOT / "ref/recompcore"
        self.iso = None

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
        if platform.system() != "Windows":
            die("this builder is for Windows; on a Mac use scripts/builder/build.sh")
        if platform.machine().lower() not in ("amd64", "x86_64"):
            die(f"an x86-64 PC is required (this is {platform.machine()})")
        if sys.version_info < (3, 10):
            die("Python 3.10 or newer is required")
        for tool in ("git", "cmake", "ninja"):
            if shutil.which(tool) is None:
                die(f"missing {tool}: install Git, CMake 3.25+ and Ninja (pip install cmake ninja)")
        version = subprocess.check_output(["cmake", "--version"], text=True).split()[2]
        if tuple(int(x) for x in version.split(".")[:2]) < (3, 25):
            die(f"CMake 3.25 or newer is required (found {version})")
        self.env = self.visual_studio_env()
        clang = shutil.which("clang", path=self.env["PATH"])
        self.clang = clang
        if clang is None:
            die("Visual Studio's clang is missing: in the Visual Studio Installer, add "
                "'C++ Clang Compiler for Windows' (and 'MSBuild support for LLVM')")
        clang_version = subprocess.check_output([clang, "--version"], text=True, env=self.env).splitlines()[0]
        major = int(re.search(r"version (\d+)", clang_version).group(1))
        if major < 17:
            die(f"clang 17 or newer is required ({clang_version})")
        self.clang_version = clang_version
        self.check_march()
        print(f"{clang_version}; cmake {version}; ninja {subprocess.check_output(['ninja', '--version'], text=True).strip()}; "
              f"{self.args.jobs} jobs; -march={self.args.march}")

    def visual_studio_env(self):
        """The x64 developer environment of the newest Visual Studio with clang."""
        vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / \
            "Microsoft Visual Studio/Installer/vswhere.exe"
        if not vswhere.exists():
            die("Visual Studio 2022 or newer with the C++ workload is required (vswhere.exe not found)")
        found = json.loads(subprocess.check_output(
            [str(vswhere), "-all", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
             "-format", "json", "-utf8"], text=True, encoding="utf-8"))
        candidates = [Path(v["installationPath"]) for v in found
                      if (Path(v["installationPath"]) / "VC/Tools/Llvm/x64/bin/clang.exe").exists()]
        if not candidates:
            die("no Visual Studio with the C++ workload and its clang was found: in the Visual Studio "
                "Installer, add 'Desktop development with C++' and 'C++ Clang Compiler for Windows'")
        install = sorted(candidates, key=lambda p: [v for v in found if Path(v["installationPath"]) == p][0]
                         .get("installationVersion", ""))[-1]
        vcvars = install / "VC/Auxiliary/Build/vcvars64.bat"
        # vcvars fails on quoted PATH entries or ones with parentheses (some
        # SDK installers add them); keep the clean ones for the build.
        path = [p.strip('"') for p in os.environ.get("PATH", "").split(os.pathsep)]
        base = dict(os.environ, PATH=os.pathsep.join(p for p in path if p and "(" not in p and ")" not in p))
        output = subprocess.run(f'cmd /d /c ""{vcvars}" >nul 2>&1 && set"', capture_output=True, text=True,
                                env=base, shell=False)
        if output.returncode != 0 or "INCLUDE=" not in output.stdout:
            die(f"{vcvars} failed; open a 'x64 Native Tools Command Prompt' and rerun from there")
        env = {}
        for line in output.stdout.splitlines():
            key, sep, value = line.partition("=")
            if sep and key:
                env[key] = value
        env["PATH"] = str(install / "VC/Tools/Llvm/x64/bin") + os.pathsep + env.get("PATH", "")
        print(f"Visual Studio: {install}")
        return env

    def check_march(self):
        """The game module is compiled for --march; refuse a level this CPU lacks."""
        levels = {"x86-64": set(), "x86-64-v2": {"sse4.2", "popcnt"},
                  "x86-64-v3": {"sse4.2", "popcnt", "avx", "avx2", "fma", "bmi1", "bmi2", "movbe", "lzcnt"}}
        if self.args.march not in levels:
            return
        # CPUID and XGETBV directly (__builtin_cpu_supports needs compiler-rt,
        # which the MSVC target does not link); AVX also needs the OS to save
        # the YMM state.
        probe = self.out / "tools/march_probe.c"
        probe.parent.mkdir(parents=True, exist_ok=True)
        probe.write_text(r"""#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>
int main(void) {
    int r[4];
    __cpuid(r, 1);
    const int ecx1 = r[2];
    __cpuidex(r, 7, 0);
    const int ebx7 = r[1];
    __cpuid(r, 0x80000001);
    const int ecx81 = r[2];
    const int os_avx = ((ecx1 >> 27) & 1) && (_xgetbv(0) & 6) == 6;
    printf("sse4.2=%d popcnt=%d movbe=%d fma=%d avx=%d avx2=%d bmi1=%d bmi2=%d lzcnt=%d\n",
           (ecx1 >> 20) & 1, (ecx1 >> 23) & 1, (ecx1 >> 22) & 1, ((ecx1 >> 12) & 1) && os_avx,
           ((ecx1 >> 28) & 1) && os_avx, ((ebx7 >> 5) & 1) && os_avx, (ebx7 >> 3) & 1,
           (ebx7 >> 8) & 1, (ecx81 >> 5) & 1);
    return 0;
}
""")
        exe = probe.with_suffix(".exe")
        subprocess.run([self.clang, "-O1", "-mxsave", str(probe), "-o", str(exe)], env=self.env, check=True,
                       capture_output=True)
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
            # A new checkout gets the sources as committed (LF), whatever the
            # global core.autocrlf says; an existing one keeps its own setting,
            # which its clean status below depends on.
            subprocess.check_call(["git", "config", "core.autocrlf", "false"], cwd=rc)
            subprocess.check_call(["git", "config", "core.longpaths", "true"], cwd=rc)
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=rc, capture_output=True, text=True).stdout.strip()
        if head != sha:
            if self.git("status", "--porcelain", "--untracked-files=no", cwd=rc):
                die(f"{rc} has local changes and is not at {sha}: move it aside and rerun")
            print(f"fetching RecompCore {sha}")
            subprocess.run(["git", "remote", "remove", "bluewake"], cwd=rc, capture_output=True)
            subprocess.check_call(["git", "remote", "add", "bluewake", url], cwd=rc)
            self.run("recompcore-fetch", ["git", "-C", rc, "fetch", "--recurse-submodules=no", "--depth", "1", "bluewake", sha], env=os.environ)
            subprocess.check_call(["git", "checkout", "-q", "--detach", "FETCH_HEAD"], cwd=rc)
        if self.git("rev-parse", "HEAD", cwd=rc) != sha:
            die(f"{rc} is not at {sha}")
        subprocess.check_call(["git", "submodule", "sync", "-q", "--", "DolRecomp"], cwd=rc)
        sub = rc / "DolRecomp"
        current = subprocess.run(["git", "rev-parse", "HEAD"], cwd=sub, capture_output=True, text=True).stdout.strip() \
            if (sub / ".git").exists() else ""
        if current != dolrecomp_sha:
            # The submodule follows its parent's line endings.
            lf = subprocess.run(["git", "config", "--local", "core.autocrlf"], cwd=rc, capture_output=True,
                                text=True).stdout.strip() == "false"
            self.run("dolrecomp-fetch", ["git", "-C", rc, *(["-c", "core.autocrlf=false"] if lf else []),
                                         "submodule", "update", "--init", "--depth", "1", "--", "DolRecomp"],
                     env=os.environ)
            if lf:
                subprocess.check_call(["git", "config", "core.autocrlf", "false"], cwd=sub)
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
        if source.suffix.lower() in DISC_FORMATS:
            iso = self.out / "disc/GZLE01.iso"
            stamp = self.out / "disc/source.json"
            key = {"source": str(source), "size": source.stat().st_size, "mtime": source.stat().st_mtime_ns,
                   "nodtool": NODTOOL_VERSION}
            if iso.exists() and stamp.exists() and json.loads(stamp.read_text()) == key:
                print(f"reusing {iso}")
            else:
                nodtool = self.nodtool()
                iso.parent.mkdir(parents=True, exist_ok=True)
                pending = iso.with_name("GZLE01.pending.iso")  # nodtool picks the format by extension
                self.run("disc-convert", [nodtool, "--no-color", "convert", source, pending], env=os.environ)
                log = (self.logs / "disc-convert.log").read_text(encoding="utf-8", errors="replace")
                if "Redump:" in log and "\u2705" not in log.split("Redump:")[1].splitlines()[0]:
                    print("warning: nodtool did not match this image to a Redump entry")
                os.replace(pending, iso)
                stamp.write_text(json.dumps(key))
            self.iso = iso
        else:
            self.iso = source
        with open(self.iso, "rb") as disc:
            header = disc.read(0x20)
        if len(header) < 0x20 or int.from_bytes(header[0x1C:0x20], "big") != GC_MAGIC:
            die(f"{self.iso} is not a GameCube disc image")
        if header[:6] != b"GZLE01":
            die(f"this is a GameCube disc, but not The Wind Waker (USA, GZLE01): its id is "
                f"{header[:6].decode(errors='replace')}")
        if header[7] != 0:
            die(f"this is GZLE01 revision {header[7]}; BlueWake supports revision 0 only")
        print(f"disc: {self.iso} (GZLE01 revision 0)")

    def nodtool(self):
        root = ROOT / "build/tools/nodtool"
        exe = root / "bin/nodtool.exe"
        if exe.exists():
            return exe
        if shutil.which("cargo") is None:
            die("this disc image is compressed; converting it needs Rust's cargo (https://rustup.rs) "
                "to build nodtool, or convert it to .iso yourself (Dolphin: right-click the game, "
                "Convert File..., format ISO) and pass the .iso")
        print(f"building nodtool {NODTOOL_VERSION} from crates.io (once)")
        self.run("nodtool-install", ["cargo", "install", "nodtool", "--version", NODTOOL_VERSION, "--locked",
                                     "--root", root], env=os.environ)
        return exe

    # --- tools built from source ---------------------------------------
    def build_dolrecomp(self):
        build = self.out / "dolrecomp"
        # binmode.obj: the CRT opens files in binary mode by default, so the
        # translator writes the same bytes (LF) as on macOS and the generated
        # source matches the verified digest.
        self.run("dolrecomp-configure", ["cmake", "-S", self.recompcore / "DolRecomp", "-B", build, "-G", "Ninja",
                                         "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=clang-cl",
                                         "-DDOLRECOMP_WARNINGS_AS_ERRORS=OFF",
                                         "-DCMAKE_EXE_LINKER_FLAGS=binmode.obj"])
        self.run("dolrecomp-build", ["cmake", "--build", build, "--target", "dolrecomp", "-j", self.args.jobs],
                 ninja=True)
        return build / "dolrecomp.exe"

    def configure_app(self):
        self.app_build = self.out / "app"
        self.run("app-configure", [
            "cmake", "-S", ROOT / "windows", "-B", self.app_build, "-G", "Ninja",
            "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++", "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_TESTING=OFF", "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld",
            "-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld",
            f"-DBLUEWAKE_WINDOWS_CONSOLE={'ON' if self.args.console else 'OFF'}"])

    # --- 4 extract ---------------------------------------------------------
    def extract(self, iso, game, accept_sha1=None):
        self.run("disc-extract-build", ["cmake", "--build", self.app_build, "--target", "bluewake_disc_extract"],
                 ninja=True)
        env = dict(self.env)
        if accept_sha1:
            env["BLUEWAKE_ACCEPT_DOL_SHA1"] = accept_sha1
        # disc_extract checks the disc id (GZLE01) and the executable's hash
        # (revision 0) and refuses anything else.
        self.run(f"disc-extract{'-' + game.parent.name if accept_sha1 else ''}",
                 [self.app_build / "bluewake_disc_extract.exe", iso, game], env=env)
        rels = len(list((game / "rels").glob("*.rel")))
        if rels != 415:
            die(f"expected 415 RELs in {game / 'rels'}, found {rels}")

    # --- 5 translate -------------------------------------------------------
    def translate(self, dol, out, rels=None, name="translate", sites=()):
        """DolRecomp over main.dol (and the RELs); `sites` is --option-sites FILE
        for Better Wind Waker's options (scripts/mods/build_mods.sh)."""
        pending = out.with_name(out.name + ".new")
        shutil.rmtree(pending, ignore_errors=True)
        pending.mkdir(parents=True)
        self.run(f"{name}-dol", [self.dolrecomp, "--gamecube", "--backend", "c", "--cpu", "gekko",
                                 "--partition-instructions", "4096", *sites, dol, pending / "dol",
                                 "-j", self.args.jobs])
        if rels is not None:
            # With option sites, name the RELs' folder with "/" and a trailing
            # "/": DolRecomp matches a REL's sites (d_a_ship.rel+0x...) to the
            # file name after the last "/" of its path, and on Windows it joins
            # a folder and a file with "\" unless the folder already ends in a
            # separator, so the sites in the RELs would never match.
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
        # compile start over. Mods are part of the recorded inputs.
        inputs = hashlib.sha256()
        inputs.update((f"{digest}\n{int(self.mods)}\n{int(self.args.prepared_blocks)}\n"
                       f"{int(self.args.fixed_cpu)}\n{int(self.args.fixed_mem1)}\n{int(self.args.inline_fp)}\n{int(self.args.gather_pipe)}\n{int(self.args.direct_calls)}\n{int(self.args.inline_gpr)}\n{int(self.args.native_j3d)}\n{int(self.args.native_vec)}\n{int(self.args.native_math)}\n{int(self.args.native_skin)}\n").encode())
        for f in (sorted((ROOT / "scripts/mods").glob("*")) + sorted((ROOT / "mods/widescreen").glob("*.gecko"))
                  + [ROOT / "mods/betterww/options.txt", ROOT / "scripts/windows/fast_blocks.py",
                     ROOT / "scripts/windows/global_guest_cpu.py", ROOT / "scripts/windows/chunk_headers.py",
                     ROOT / "cmake/composite/inline_fp.h", ROOT / "cmake/composite/gather_pipe.h",
                     ROOT / "cmake/composite/gather_pipe.c", ROOT / "cmake/composite/gather_pipe_batch.h",
                     ROOT / "scripts/windows/direct_calls.py", ROOT / "cmake/composite/direct_calls.c",
                     ROOT / "cmake/composite/direct_calls.h", ROOT / "cmake/composite/inline_gpr.h",
                     ROOT / "cmake/composite/native_j3d.c", ROOT / "cmake/composite/native_j3d.h",
                     ROOT / "cmake/composite/native_vec.c", ROOT / "cmake/composite/native_vec.h",
                     ROOT / "scripts/windows/native_skin.py", ROOT / "cmake/composite/native_skin.c",
                     ROOT / "cmake/composite/native_skin.h", ROOT / "cmake/composite/native_math.c", ROOT / "cmake/composite/native_math.h",
                     ROOT / "cmake/composite/native_work_pool.c", ROOT / "cmake/composite/native_work_pool.h",
                     ROOT / "scripts/windows/inline_save_restore_gpr.py", Path(__file__)]):
            if f.is_file():
                inputs.update(f.read_bytes())
        if self.args.direct_calls:
            # The source-derived watch list is part of the prepared module.
            for folder in ("runtime/host/src", "windows/src"):
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
        """Widescreen 16:9 and 16:10, Better Wind Waker's options, and each
        widescreen with them, as variants compiled into the same module: the
        steps of scripts/mods/build_mods.sh (docs/MODS.md)."""
        o, m = self.out, self.out / "mods"
        m.mkdir(parents=True, exist_ok=True)
        options = ROOT / "mods/betterww/options.txt"
        # The site list DolRecomp reads, with LF line ends (Python's stdout
        # writes CRLF on Windows).
        listed = subprocess.run([sys.executable, ROOT / "scripts/mods/game_options.py", "sites", options],
                                capture_output=True, check=True).stdout
        (m / "option-sites.txt").write_bytes(listed.replace(b"\r\n", b"\n"))
        sites = ("--option-sites", m / "option-sites.txt")

        for name, gecko in (("widescreen", "GZLE01.gecko"), ("widescreen1610", "GZLE01-16x10.gecko")):
            print(name)
            (m / name).mkdir(exist_ok=True)
            self.run(f"mods-{name}-gecko", [sys.executable, ROOT / "scripts/mods/gecko_apply.py",
                                           ROOT / "mods/widescreen" / gecko, o / "game/main.dol",
                                           m / name / "main.dol", m / name / "runtime.json"])
            self.translate(m / name / "main.dol", m / name / "translated", name=f"mods-{name}-translate")
            self.composite(m / name / "translated/dol/generated", o / "translated/rels/generated/rels",
                           o / "game/rels", m / name / "main.dol", m / name / "composite-src",
                           f"mods-{name}-composite")

        # The game's own executable and modules translated with the option
        # sites; the variants are the chunks that hold a site.
        print("Better Wind Waker options")
        (m / "betterww").mkdir(exist_ok=True)
        self.translate(o / "game/main.dol", m / "betterww/translated", o / "game/rels",
                       name="mods-betterww-translate", sites=sites)
        self.composite(m / "betterww/translated/dol/generated", m / "betterww/translated/rels/generated/rels",
                       o / "game/rels", o / "game/main.dol", m / "betterww/composite-src", "mods-betterww-composite")

        for combo, widescreen in (("combo", "widescreen"), ("combo1610", "widescreen1610")):
            print(f"{widescreen} + Better Wind Waker options")
            (m / combo).mkdir(exist_ok=True)
            self.translate(m / widescreen / "main.dol", m / combo / "translated", name=f"mods-{combo}-translate",
                           sites=sites)
            self.composite(m / combo / "translated/dol/generated", m / "betterww/translated/rels/generated/rels",
                           o / "game/rels", m / widescreen / "main.dol", m / combo / "composite-src",
                           f"mods-{combo}-composite")

        print("variants into the composite source")
        base = m / "composite-src.base"
        self.composite(o / "translated/dol/generated", o / "translated/rels/generated/rels", o / "game/rels",
                       o / "game/main.dol", base, "mods-base-composite")
        # The --mod and --combo specs are colon-separated, and a Windows path
        # has a colon after its drive letter: run in the build directory and
        # name the mod trees relative to it. The mods keep build_mods.sh's
        # order, which numbers them.
        self.run("mods-variants", [
            sys.executable, ROOT / "scripts/mods/build_mod_variants.py", "--composite-src", base,
            "--base-dol", o / "game/main.dol",
            "--mod", "widescreen:mods/widescreen/composite-src:mods/widescreen/main.dol:mods/widescreen/runtime.json",
            "--mod", "betterww:mods/betterww/composite-src:game/main.dol",
            "--mod", "widescreen1610:mods/widescreen1610/composite-src:mods/widescreen1610/main.dol:"
                     "mods/widescreen1610/runtime.json",
            "--combo", "widescreen+betterww:mods/combo/composite-src",
            "--combo", "widescreen1610+betterww:mods/combo1610/composite-src",
            "--exclusive", "widescreen,widescreen1610",
            "--options", options, "--rels-bin-dir", o / "game/rels"], cwd=o)
        dst = o / "composite-src"
        if (base / "generated.h").read_bytes() != (dst / "generated.h").read_bytes():
            die(f"{dst} was generated from other inputs; rebuild it first")
        # The base tree plus the variants: only the variant chunks and the
        # files that list them change.
        sync_tree(base, dst)
        (o / "composite-final.digest").write_text(tree_digest(dst) + "\n")
        (o / "mods.done").write_text("complete\n")

    def prepare_blocks(self):
        """Explicit generic optimization, after variants and before compilation.

        generate() verifies both the input fingerprint and final tree digest.
        Interrupted/edited preparation cannot be mistaken for finished work.
        """
        o = self.out
        script = ROOT / "scripts/windows/fast_blocks.py"
        cpu_script = ROOT / "scripts/windows/global_guest_cpu.py"
        if self.args.native_j3d:
            self.run("native-j3d", [sys.executable, ROOT / "scripts/mods/prepare_native_j3d.py",
                                     o / "composite-src"])
        if self.args.native_vec:
            self.run("native-vec", [sys.executable, ROOT / "scripts/mods/prepare_native_vec.py",
                                     o / "composite-src"])
        if self.args.native_math:
            self.run("native-math", [sys.executable, ROOT / "scripts/mods/prepare_native_math.py",
                                     o / "composite-src"])
        if self.args.native_skin:
            self.run("native-skin", [sys.executable, ROOT / "scripts/windows/native_skin.py",
                                      o / "composite-src"])
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
            self.run("direct-calls", [sys.executable, ROOT / "scripts/windows/direct_calls.py",
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

    # --- 8 compile -----------------------------------------------------------
    def compile_module(self):
        build = self.out / "composite"
        rc = self.recompcore
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
        self.run("composite-configure", [
            "cmake", "-S", ROOT / "cmake/composite", "-B", build, "-G", "Ninja", "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_C_FLAGS={flags}", "-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld",
            f"-DBLUEWAKE_FIXED_CPU={'ON' if self.args.fixed_cpu else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_J3D={'ON' if self.args.native_j3d else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_VEC={'ON' if self.args.native_vec else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_SKIN={'ON' if self.args.native_skin else 'OFF'}",
            f"-DBLUEWAKE_NATIVE_MATH={'ON' if self.args.native_math else 'OFF'}",
            f"-DBLUEWAKE_DIRECT_CALLS={'ON' if self.args.direct_calls else 'OFF'}",
            f"-DBLUEWAKE_GATHER_PIPE={'ON' if self.args.gather_pipe else 'OFF'}",
            f"-DBLUEWAKE_INLINE_FP={'ON' if self.args.inline_fp else 'OFF'}",
            f"-DBLUEWAKE_FIXED_MEM1={'ON' if self.args.fixed_mem1 else 'OFF'}",
            f"-DCOMPOSITE_OPTIMIZATION_LEVEL={self.args.opt_level}", f"-DCOMPOSITE_DIR={self.out / 'composite-src'}",
            f"-DGXRUNTIME_DIR={rc / 'GXRuntime'}", f"-DABI_DIR={rc / 'Source/Core/Core/PowerPC/StaticRecomp'}"])
        # -k 0: a chunk that fails does not stop the others. The usual cause is
        # memory (clang reports "out of memory" when several of the largest
        # chunks peak together), so what failed is retried with fewer jobs.
        jobs = self.args.jobs
        while True:
            try:
                self.run("composite-build", ["cmake", "--build", build, "-j", jobs, "--", "-k", "0"], ninja=True)
                break
            except BuildError:
                log = (self.logs / "composite-build.log").read_text(errors="replace")
                if jobs <= 1 or "out of memory" not in log:
                    raise
                jobs = max(1, jobs // 2)
                print(f"  some chunks ran out of memory; compiling the rest with {jobs} jobs")
        module = build / MODULE
        if not module.exists():
            die("the game module was not produced")
        return module

    # --- 9 app ---------------------------------------------------------------
    def build_app(self):
        self.run("app-build", ["cmake", "--build", self.app_build, "--target", "BlueWake", "-j", self.args.jobs],
                 ninja=True)
        exe = self.app_build / "BlueWake.exe"
        if not exe.exists():
            die("BlueWake.exe was not produced")
        return exe

    # --- 10 package ----------------------------------------------------------
    def package(self, module):
        app = self.out / "BlueWake"
        (app / "game").mkdir(parents=True, exist_ok=True)
        (app / "dsp").mkdir(exist_ok=True)
        copied = []
        for f in sorted(self.app_build.iterdir()):
            if f.suffix.lower() in (".exe", ".dll") and f.name != "bluewake_disc_extract.exe" or \
                    f.name == "initial_pipeline_cache.db":
                shutil.copy2(f, app / f.name)
                copied.append(f.name)
        nodtool = ROOT / "build/tools/nodtool/bin/nodtool.exe"
        if nodtool.is_file():
            shutil.copy2(nodtool, app / "nodtool.exe")
            copied.append("nodtool.exe")
        shutil.copy2(module, app / MODULE)
        # The Visual C++ runtime, app-local, so the folder also runs on a PC
        # without the redistributable installed (the UCRT ships with Windows).
        redist = Path(self.env.get("VCToolsRedistDir", "")) / "x64"
        crt = sorted(redist.glob("Microsoft.VC*.CRT")) if redist.is_dir() else []
        if crt:
            for name in ("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"):
                if (crt[-1] / name).exists():
                    shutil.copy2(crt[-1] / name, app / name)
        else:
            print("note: the Visual C++ runtime DLLs were not found to copy; the app needs the "
                  "Visual C++ Redistributable on other PCs")
        shutil.copy2(self.out / "game/main.dol", app / "game/main.dol")
        rels = app / "game/rels"
        shutil.rmtree(rels, ignore_errors=True)
        shutil.copytree(self.out / "game/rels", rels)
        for name in ("dsp_rom.bin", "dsp_coef.bin"):
            shutil.copy2(self.recompcore / "Data/Sys/GC" / name, app / "dsp" / name)
        self.place(self.iso, app / "game/GZLE01.iso")
        dirty = bool(self.git("status", "--porcelain"))
        provenance = {
            "profile": "bluewake-windows",
            "containsTranslatedGameCode": True,
            "source_commit": self.git("rev-parse", "HEAD"),
            "source_modified": dirty,
            "composite_digest": (self.out / "composite-src.digest").read_text().strip(),
            "mods": bool(self.mods),
            "march": self.args.march,
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
        print(f"Building The Legend of Zelda: The Wind Waker (GameCube USA GZLE01 rev 0) for Windows from {args.disc}")
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
        print(f"  run {app / 'BlueWake.exe'}")
        print("  It contains game code translated from your disc and a copy of the disc: keep it for yourself.")
        print("  Saves: %APPDATA%\\BlueWake (outside the build).")


README = """BlueWake for Windows: The Legend of Zelda: The Wind Waker (GameCube USA),
statically recompiled from your own disc. See docs/WINDOWS.md in the source.

This folder is a personal build: gGZLE01_recomp.dll is code translated from
your disc and game\\ holds your disc image. Never share or upload it.

Run BlueWake.exe. Options (BlueWake.exe --help lists them all):
  --widescreen    16:9 (--aspect 16:10 for 16:10)
  --smooth        Smooth Motion: 60 FPS with in-between frames
  --betterww      Better Wind Waker's settings (--options to change them)
  --fullscreen    start in fullscreen

Keyboard: arrows D-pad, J A, K B, U X, I Y, W/A/S/D control stick,
H/F/T/G C-stick, E/R L/R, Q Z, Return START. Game controllers work too.
Mouse: click the game and move the mouse to turn the camera; Esc releases it.
F11 fullscreen, F10 Smooth Motion, F9 frame rate.

Saves, settings and session logs: %APPDATA%\\BlueWake
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("disc", type=Path, help="your GZLE01 revision 0 disc image (.iso, .gcm, .rvz, ...)")
    parser.add_argument("--out", type=Path, default=ROOT / "build/windows",
                        help="build directory (default build/windows; must be git-ignored inside the checkout)")
    parser.add_argument("--jobs", type=int, default=None,
                        help="parallel compile jobs (default: the cores, limited by free memory)")
    parser.add_argument("--march", default="x86-64-v3",
                        help="CPU level for the game module (default x86-64-v3: AVX2, FMA, BMI2 and MOVBE, "
                             "any Intel Haswell or AMD Zen or newer; lowered automatically on older CPUs)")
    parser.add_argument("--opt-level", choices=("1", "2"), default="2", help="game module optimization level")
    parser.add_argument("--no-mods", action="store_true", help="skip the widescreen and Better Wind Waker variants")
    parser.add_argument("--prepared-blocks", action="store_true",
                        help="opt into experimental prepaid-block optimization (off by default; Windows timing pending)")
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
    parser.add_argument("--native-skin", action="store_true",
                        help="certify and enable optional native skinning preparation (off by default)")
    parser.add_argument("--native-math", action="store_true",
                        help="prepare certified native matrix functions; off by default, compatible host opt-in required")
    parser.add_argument("--console", action="store_true", help="build BlueWake.exe as a console program")
    parser.add_argument("--accept-new-composite", action="store_true",
                        help="continue if the generated source differs from the verified one")
    parser.add_argument("--source-only", action="store_true",
                        help="stop after generating the source: checks tools, disc and translation in minutes")
    parser.add_argument("--check-only", action="store_true", help="check tools, dependencies and the disc only")
    args = parser.parse_args()
    if args.inline_gpr and not args.direct_calls:
        parser.error("--inline-gpr requires --direct-calls")
    if args.fixed_mem1 and not args.fixed_cpu:
        parser.error("--fixed-mem1 requires --fixed-cpu")
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
            parser.error("--out must not be the checkout itself; use build/windows")
        ignored = subprocess.run(["git", "check-ignore", "-q", str(args.out) + os.sep], cwd=ROOT).returncode == 0
        if not ignored:
            parser.error("--out inside this checkout must be git-ignored; use build/windows")
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
