#!/usr/bin/env python3
"""Build the instrumented module at -O2 (not -O0) to dodge the Houdini crash."""
import subprocess, sys, time, os
from pathlib import Path

REPO = Path("/home/james/nfs/Projects/bluewake")
PREPARED = Path("/home/james/bw-build-0215")
OUT = Path("/home/james/bw-build-android")
SDK = Path("/home/james/android-sdk")
NDK = SDK / "ndk/android-ndk-r30"
TOOLCHAIN = NDK / "build/cmake/android.toolchain.cmake"

SWITCHES = {"FIXED_CPU": True, "NATIVE_J3D": True, "NATIVE_VEC": True,
            "NATIVE_GAME_MATH": True, "NATIVE_SKIN": True, "NATIVE_MATH": True,
            "NATIVE_ENTRIES": False, "DIRECT_CALLS": True, "GATHER_PIPE": True,
            "INLINE_FP": True, "FIXED_MEM1": True}
CPU = "cortex-a78"

def log(m): print(f"[{time.strftime('%H:%M:%S')}] {m}", flush=True)

def default_jobs():
    with open("/proc/meminfo") as m:
        for line in m:
            if line.startswith("MemAvailable:"):
                return max(1, min(os.cpu_count() or 8, int(int(line.split()[1])*1024 // (2.0*2**30))))
    return os.cpu_count() or 8

def run(name, cmd):
    lf = OUT / "logs" / f"{name}.log"
    lf.parent.mkdir(parents=True, exist_ok=True)
    with open(lf, "wb") as s:
        p = subprocess.Popen([str(c) for c in cmd], cwd=REPO, stdout=s, stderr=subprocess.STDOUT)
        while True:
            try:
                rc = p.wait(timeout=15); break
            except subprocess.TimeoutExpired:
                pass
    if rc != 0:
        print(lf.read_bytes()[-3000:].decode(errors="replace"), file=sys.stderr)
        raise SystemExit(f"{name} failed ({rc})")

build = OUT / "composite-instr-o2"
cflags = f"-mcpu={CPU} -fno-slp-vectorize -mllvm -large-interval-freq-threshold=10 -fprofile-instr-generate"
linkflags = "-Wl,-z,max-page-size=16384 -Wl,--gc-sections -fuse-ld=lld -fprofile-instr-generate"
cmd = ["cmake", "-S", str(REPO / "cmake/composite"), "-B", str(build),
       "-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
       "-DANDROID_ABI=arm64-v8a", "-DANDROID_PLATFORM=android-33",
       "-DANDROID_STL=c++_static", "-DCMAKE_BUILD_TYPE=Release",
       f"-DCMAKE_C_FLAGS={cflags}", f"-DCMAKE_SHARED_LINKER_FLAGS={linkflags}",
       "-DCOMPOSITE_OPTIMIZATION_LEVEL=2",
       f"-DCOMPOSITE_DIR={PREPARED / 'composite-src'}",
       f"-DGXRUNTIME_DIR={REPO / 'ref/recompcore/GXRuntime'}",
       f"-DABI_DIR={REPO / 'ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp'}",
       "-DCOMPOSITE_COLD_SOURCES_FILE=",
       f"-DCOMPOSITE_EXTRA_SOURCES={REPO / 'android/src/profile_flush.c'}"]
for n, on in SWITCHES.items():
    cmd.append(f"-DBLUEWAKE_{n}={'ON' if on else 'OFF'}")
log("configure -O2 instrumented")
run("instr-o2-configure", cmd)
jobs = default_jobs()
log(f"{jobs} jobs")
while True:
    try:
        run("instr-o2-build", ["cmake", "--build", str(build), "-j", str(jobs), "--", "-k", "0"])
        break
    except SystemExit:
        lf = OUT / "logs" / "instr-o2-build.log"
        t = lf.read_text(errors="replace") if lf.exists() else ""
        if jobs <= 1 or "out of memory" not in t:
            raise
        jobs = max(1, jobs // 2)
        log(f"OOM retry {jobs}")
mod = build / "gGZLE01_recomp.so"
log(f"done: {mod} ({mod.stat().st_size >> 20} MB)")
