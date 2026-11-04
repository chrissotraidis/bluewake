#!/usr/bin/env python3
# App configure only: fetch SDL3 source + Dawn prebuilt via FetchContent,
# without compiling (deferred until the module finishes, to avoid OOM).
from pathlib import Path
import subprocess, sys, time

REPO = Path("/home/james/nfs/Projects/bluewake")
OUT = Path("/home/james/bw-build-android")
NDK = Path("/home/james/android-sdk/ndk/android-ndk-r30")
TOOLCHAIN = NDK / "build/cmake/android.toolchain.cmake"
build = OUT / "app"
CPU = "cortex-a78"
MIN_SDK = 33

def log(m): print(f"[{time.strftime('%H:%M:%S')}] {m}", flush=True)

cmd = ["cmake", "-S", str(REPO / "android"), "-B", str(build),
       "-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
       "-DANDROID_ABI=arm64-v8a", f"-DANDROID_PLATFORM=android-{MIN_SDK}",
       "-DANDROID_STL=c++_static", "-DCMAKE_BUILD_TYPE=Release",
       f"-DBLUEWAKE_ANDROID_CPU={CPU}",
       "-DBLUEWAKE_APP_PROFILE_GENERATE=OFF",
       "-DBLUEWAKE_APP_PROFILE_USE="]
log("app configure (fetching SDL3 + Dawn) ...")
lf = OUT / "logs/app-configure.log"
lf.parent.mkdir(parents=True, exist_ok=True)
with open(lf, "wb") as s:
    p = subprocess.Popen([str(c) for c in cmd], cwd=REPO, stdout=s, stderr=subprocess.STDOUT)
    while True:
        try:
            rc = p.wait(timeout=15); break
        except subprocess.TimeoutExpired:
            pass
if rc != 0:
    print(lf.read_bytes()[-4000:].decode(errors="replace"), file=sys.stderr)
    sys.exit(f"app configure failed (exit {rc})")
log("app configure done")
