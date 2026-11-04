#!/usr/bin/env python3
"""BlueWake Android PGO training via module-push (no 2GB APK).
The instrumented module is pushed into the app's files/ dir (ext4, exec) and
loaded via BLUEWAKE_COMPOSITE. The host-only APK already installed stays put.
"""
import json, os, re, shutil, subprocess, sys, time, zipfile
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

REPO = Path("/home/james/nfs/Projects/bluewake")
PREPARED = Path("/home/james/bw-build-0215")
OUT = Path("/home/james/bw-build-android")
SDK = Path("/home/james/android-sdk")
NDK = SDK / "ndk/android-ndk-r30"
NDK_BIN = NDK / "toolchains/llvm/prebuilt/linux-x86_64/bin"
BT = SDK / "build-tools/android-15"
PLATFORM_JAR = SDK / "platforms/android-36/android.jar"
ADB = SDK / "platform-tools/adb"
TOOLCHAIN = NDK / "build/cmake/android.toolchain.cmake"

MODULE = "libgGZLE01_recomp.so"
MODULE_SRC = "gGZLE01_recomp.so"
APP_ID = "dev.bluewake.BlueWake"
ACTIVITY = "dev.bluewake.android.BlueWakeActivity"
MIN_SDK = 33
TARGET_SDK = 35
CPU = "cortex-a78"
DEVICE_FILES = f"/storage/emulated/0/Android/data/{APP_ID}/files"
DEVICE_MODULE = f"{DEVICE_FILES}/gGZLE01_recomp-instr.so"

TRAINING_RETRACES = 23000
TRAINING_RUN = ["20400:0:700:0:127", "21100:0:500:90:110",
                "21600:0:500:-90:110", "22100:0:900:0:127"]

SWITCHES = {"FIXED_CPU": True, "NATIVE_J3D": True, "NATIVE_VEC": True,
            "NATIVE_GAME_MATH": True, "NATIVE_SKIN": True, "NATIVE_MATH": True,
            "NATIVE_ENTRIES": False, "DIRECT_CALLS": True, "GATHER_PIPE": True,
            "INLINE_FP": True, "FIXED_MEM1": True}


def log(m): print(f"[{time.strftime('%H:%M:%S')}] {m}", flush=True)


def run(name, cmd, env=None, cwd=None):
    lf = OUT / "logs" / f"{name}.log"
    lf.parent.mkdir(parents=True, exist_ok=True)
    log(f"  {name} (log: {lf.name})")
    with open(lf, "wb") as s:
        p = subprocess.Popen([str(c) for c in cmd], cwd=cwd or REPO,
                             stdout=s, stderr=subprocess.STDOUT, env=env)
        while True:
            try:
                rc = p.wait(timeout=15); break
            except subprocess.TimeoutExpired:
                pass
    if rc != 0:
        tail = lf.read_bytes()[-4000:].decode(errors="replace")
        print(tail, file=sys.stderr)
        raise SystemExit(f"{name} failed (exit {rc}); log {lf}")


def _android_args():
    return ["-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
            "-DANDROID_ABI=arm64-v8a", f"-DANDROID_PLATFORM=android-{MIN_SDK}",
            "-DANDROID_STL=c++_static", "-DCMAKE_BUILD_TYPE=Release"]


def default_jobs():
    try:
        with open("/proc/meminfo") as m:
            for line in m:
                if line.startswith("MemAvailable:"):
                    avail = int(line.split()[1]) * 1024
                    return max(1, min(os.cpu_count() or 8, int(avail // (2.0 * 2**30))))
    except OSError:
        pass
    return os.cpu_count() or 8


def adb(*a, check=True, capture=False, timeout=None):
    cmd = [str(ADB), *[str(x) for x in a]]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    if check and r.returncode != 0:
        raise SystemExit(f"adb {' '.join(map(str,a))} failed: {r.stdout}{r.stderr}")
    return r.stdout if capture else None


def push_module():
    src = OUT / "composite-instr" / MODULE_SRC
    if not src.exists():
        raise SystemExit(f"instrumented module missing: {src}")
    log(f"pushing instrumented module ({src.stat().st_size >> 20} MB) ...")
    adb("push", src, DEVICE_MODULE, timeout=1800)
    adb("shell", "chmod", "755", DEVICE_MODULE)
    log("module pushed")


def playback_one(name, mods):
    remote = DEVICE_FILES  # flat: the app owns files/, can write directly
    opening = [f"{n}:0x0100:2" for n in range(17800, 22001, 150)] + TRAINING_RUN
    env = {
        "LLVM_PROFILE_FILE": f"{remote}/pgo-{name}-%m-%p.profraw",
        "BLUEWAKE_CARD_PATH": f"{remote}/pgo-{name}-training.card",
        "BLUEWAKE_SRAM": f"{remote}/pgo-{name}-sram.bin",
        "BLUEWAKE_SETTINGS": "none",
        "BLUEWAKE_RENDERER": "headless",
        "BLUEWAKE_MAX_RETRACES": str(TRAINING_RETRACES),
        "BLUEWAKE_WALL_PACE": "0",
        "BLUEWAKE_PLAYER_PROBE": "1",
        "BLUEWAKE_PAD_BUTTONS": "0x0100",
        "BLUEWAKE_OVERLAP_OBSERVATION": "1",
        "BLUEWAKE_PAD_PULSE_ON_TITLE_READY": "1",
        "BLUEWAKE_PAD_PULSE_LENGTH": "2",
        "BLUEWAKE_PAD_CONFIRM_EVENT": "any",
        "BLUEWAKE_PAD_SCRIPT": ",".join(opening),
        "BLUEWAKE_MODS": mods or "",
    }
    local_env = OUT / "pgo" / f"{name}.launch.env"
    local_env.parent.mkdir(parents=True, exist_ok=True)
    local_env.write_text("".join(f"{k}={v}\n" for k, v in env.items() if v), encoding="utf-8")
    adb("push", local_env, f"{DEVICE_FILES}/launch.env")
    adb("shell", "mkdir", "-p", remote)
    adb("shell", "am", "force-stop", APP_ID)
    adb("shell", "am", "start", "-n", f"{APP_ID}/{ACTIVITY}", "--ez", "showWhenLocked", "true")
    log(f"  playback {name} started ({'mods' if mods else 'plain'})")
    start = time.monotonic()
    log_name = None
    text = ""
    while True:
        time.sleep(20)
        elapsed = int(time.monotonic() - start)
        if log_name is None:
            listing = adb("shell", f"ls {DEVICE_FILES}/logs", capture=True, check=False)
            names = [x for x in listing.split() if x.startswith("session-")]
            log_name = sorted(names)[-1] if names else None
        if log_name is not None:
            text = adb("shell", f"cat {DEVICE_FILES}/logs/{log_name}", capture=True, check=False)
            if "[android] host returned" in text:
                break
            rts = re.findall(r"retrace=(\d+)", text[-30000:])
            print(f"  {name}: {elapsed//60}m{elapsed%60:02d}s retrace {rts[-1] if rts else 0}/{TRAINING_RETRACES}", flush=True)
        if elapsed > 120 and not adb("shell", "pidof", APP_ID, capture=True, check=False).strip():
            raise SystemExit(f"playback {name} stopped early")
        if elapsed > 4 * 3600:
            raise SystemExit(f"playback {name} over 4h")
    written = time.monotonic()
    while True:
        text = adb("shell", f"cat {DEVICE_FILES}/logs/{log_name}", capture=True, check=False)
        done = "optimization profile written" in text or "not instrumented" in text
        alive = adb("shell", "pidof", APP_ID, capture=True, check=False).strip()
        if done and not alive:
            break
        if time.monotonic() - written > 600:
            raise SystemExit(f"playback {name} profile write timeout")
        time.sleep(5)
    if "[player-milestone] control-admitted" not in text:
        raise SystemExit(f"playback {name} did not reach player control")
    files = [f for f in adb("shell", f"ls {remote}", capture=True).split() if f.endswith(".profraw")]
    if not files:
        raise SystemExit(f"playback {name} wrote no profile")
    raw = []
    local = OUT / "pgo" / name
    local.mkdir(parents=True, exist_ok=True)
    for f in files:
        adb("pull", f"{remote}/{f}", local / f, timeout=600)
        raw.append(local / f)
    log(f"  {name}: done in {int(time.monotonic()-start)//60} min, {len(raw)} profile(s)")
    return raw


def compile_optimized(profile):
    build = OUT / "composite-pgo"
    cflags = (f"-mcpu={CPU} -fno-slp-vectorize -mllvm -large-interval-freq-threshold=10 "
              f"-fprofile-instr-use={profile} -Wno-profile-instr-unprofiled "
              f"-Wno-profile-instr-out-of-date -Wno-backend-plugin")
    linkflags = "-Wl,-z,max-page-size=16384 -Wl,--gc-sections -fuse-ld=lld"
    cmd = ["cmake", "-S", str(REPO / "cmake/composite"), "-B", str(build), *_android_args(),
           f"-DCMAKE_C_FLAGS={cflags}", f"-DCMAKE_SHARED_LINKER_FLAGS={linkflags}",
           "-DCOMPOSITE_OPTIMIZATION_LEVEL=2",
           f"-DCOMPOSITE_DIR={PREPARED / 'composite-src'}",
           f"-DGXRUNTIME_DIR={REPO / 'ref/recompcore/GXRuntime'}",
           f"-DABI_DIR={REPO / 'ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp'}",
           "-DCOMPOSITE_COLD_SOURCES_FILE="]
    for n, on in SWITCHES.items():
        cmd.append(f"-DBLUEWAKE_{n}={'ON' if on else 'OFF'}")
    run("pgo-configure", cmd)
    jobs = default_jobs()
    while True:
        try:
            run("pgo-build", ["cmake", "--build", str(build), "-j", str(jobs), "--", "-k", "0"])
            break
        except SystemExit:
            lf = OUT / "logs" / "pgo-build.log"
            text = lf.read_text(errors="replace") if lf.exists() else ""
            if jobs <= 1 or "out of memory" not in text:
                raise
            jobs = max(1, jobs // 2)
            log(f"  OOM; retrying with {jobs} jobs")
    mod = build / MODULE_SRC
    if not mod.exists():
        raise SystemExit("optimized module not produced")
    return mod


def package_final(module):
    pkg = OUT / "apk"
    shutil.rmtree(pkg, ignore_errors=True)
    (pkg / "classes").mkdir(parents=True)
    lib = OUT / "app" / "libmain.so"
    sdl_src = sorted((OUT / "app" / "_deps").glob("sdl-src/android-project/app/src/main/java"))
    sdl_java = pkg / "sdl-java"
    shutil.copytree(sdl_src[0], sdl_java)
    activity = sdl_java / "org/libsdl/app/SDLActivity.java"
    text = activity.read_text(encoding="utf-8")
    activity.write_text(text.replace('new Thread(new SDLMain(), "SDLThread")',
                                     'new Thread(null, new SDLMain(), "SDLThread", 64L << 20)'), encoding="utf-8")
    surface = sdl_java / "org/libsdl/app/SDLSurface.java"
    text = surface.read_text(encoding="utf-8")
    edits = [
        ("        SDLActivity.onNativeSurfaceChanged();\n",
         "        SDLActivity.onNativeSurfaceChanged();\n        auroraNativeSetSurfaceReady(true);\n"),
        ("        mIsSurfaceReady = false;\n        SDLActivity.onNativeSurfaceDestroyed();\n",
         "        mIsSurfaceReady = false;\n        auroraNativeSetSurfaceReady(false);\n        SDLActivity.onNativeSurfaceDestroyed();\n"),
        ("    // Called when we have a valid drawing surface\n",
         "    static native void auroraNativeSetSurfaceReady(boolean ready);\n\n    // Called when we have a valid drawing surface\n"),
    ]
    for old, new in edits:
        if text.count(old) != 1:
            raise SystemExit(f"SDLSurface anchor missing")
        text = text.replace(old, new)
    surface.write_text(text, encoding="utf-8")
    java = [str(p) for p in sdl_java.rglob("*.java")] + [str(p) for p in (REPO / "android/java").rglob("*.java")]
    run("final-javac", ["javac", "-nowarn", "-encoding", "UTF-8", "--release", "11",
                        "-classpath", str(PLATFORM_JAR), "-d", str(pkg / "classes"), *java])
    classes_jar = pkg / "classes.jar"
    with zipfile.ZipFile(classes_jar, "w", zipfile.ZIP_STORED) as jar:
        for p in sorted((pkg / "classes").rglob("*.class")):
            jar.write(p, p.relative_to(pkg / "classes").as_posix())
    run("final-d8", [BT / "d8", "--release", "--min-api", str(MIN_SDK),
                     "--lib", str(PLATFORM_JAR), "--output", str(pkg), str(classes_jar)])
    manifest = (REPO / "android/AndroidManifest.xml").read_text(encoding="utf-8")
    manifest = (manifest.replace("@PACKAGE@", APP_ID)
                .replace("@LABEL@", xml_escape("BlueWake", {'"': "&quot;", "'": "&apos;"}))
                .replace("@DEBUGGABLE@", "false"))
    (pkg / "AndroidManifest.xml").write_text(manifest, encoding="utf-8")
    res_zip = pkg / "res.zip"
    run("final-aapt2-compile", [BT / "aapt2", "compile", "--dir", str(REPO / "android/res"), "-o", str(res_zip)])
    assets = pkg / "assets"
    assets.mkdir()
    shutil.copy2(REPO / "apple/ios/resources/initial_pipeline_cache.db", assets / "initial_pipeline_cache.db")
    unsigned = pkg / "unsigned.apk"
    run("final-aapt2-link", [BT / "aapt2", "link", "-o", str(unsigned), "-A", str(assets), "-0", "db",
                             "--manifest", str(pkg / "AndroidManifest.xml"), "-I", str(PLATFORM_JAR),
                             "--min-sdk-version", str(MIN_SDK), "--target-sdk-version", str(TARGET_SDK),
                             "--version-code", str(int(time.time()) // 60), "--version-name", "0.1-android",
                             str(res_zip)])
    (pkg / "lib").mkdir()
    strip = NDK_BIN / "llvm-strip"
    stripped_lib = pkg / "lib/libmain.so"
    run("final-strip-main", [strip, "--strip-debug", "-o", str(stripped_lib), str(lib)])
    stripped_module = pkg / f"lib/{MODULE}"
    run("final-strip-module", [strip, "--strip-debug", "-o", str(stripped_module), str(module)])
    with zipfile.ZipFile(unsigned, "a") as apk:
        apk.write(pkg / "classes.dex", "classes.dex", compress_type=zipfile.ZIP_DEFLATED)
        apk.write(stripped_lib, "lib/arm64-v8a/libmain.so", compress_type=zipfile.ZIP_STORED)
        apk.write(stripped_module, f"lib/arm64-v8a/{MODULE}", compress_type=zipfile.ZIP_STORED)
    aligned = pkg / "aligned.apk"
    run("final-zipalign", [BT / "zipalign", "-f", "-P", "16", "4", str(unsigned), str(aligned)])
    keystore = OUT / "debug.keystore"
    apk_out = OUT / "WindWakerRecomp.apk"
    run("final-sign", [BT / "apksigner", "sign", "--ks", str(keystore),
                       "--ks-pass", "pass:android", "--out", str(apk_out), str(aligned)])
    log(f"final APK: {apk_out} ({apk_out.stat().st_size >> 20} MB)")
    return apk_out


def main():
    phase = sys.argv[1] if len(sys.argv) > 1 else "all"
    OUT.mkdir(parents=True, exist_ok=True)
    if phase in ("push", "all"):
        push_module()
    if phase in ("playback", "all"):
        raw = playback_one("plain", None)
        raw += playback_one("mods", "widescreen,betterww")
        profile = OUT / "pgo" / "composite.profdata"
        run("pgo-merge", [NDK_BIN / "llvm-profdata", "merge", "-o", str(profile), *[str(r) for r in raw]])
        stats = subprocess.run([str(NDK_BIN / "llvm-profdata"), "show", "--all-functions", str(profile)],
                               capture_output=True, text=True).stdout
        executed = [int(c) for c in re.findall(
            r"(?m)^  func_[0-9A-Fa-f]+\S*:\n(?:    .*\n)*?    Function count: (\d+)", stats)]
        ran = sum(1 for c in executed if c > 0)
        log(f"profile: {ran} functions ran ({len(executed)} total)")
    if phase in ("optimized", "all"):
        profile = OUT / "pgo" / "composite.profdata"
        if not profile.exists():
            raise SystemExit("run playback first")
        mod = compile_optimized(profile)
        log(f"optimized module: {mod} ({mod.stat().st_size >> 20} MB)")
    if phase in ("package", "all"):
        mod = OUT / "composite-pgo" / MODULE_SRC
        if not mod.exists():
            raise SystemExit("run optimized first")
        apk = package_final(mod)
        adb("install", "-r", "-g", apk, timeout=600)
        adb("shell", "rm", "-f", f"{DEVICE_FILES}/launch.env")
        log("PGO APK installed")


if __name__ == "__main__":
    main()
