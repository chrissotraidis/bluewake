#!/usr/bin/env python3
"""BlueWake Android (arm64) build driver for Linux, adapted from PR #93's
scripts/android/build.py. Reuses the PREPARED composite source from a prior
Linux build (same optimization set, verified digest), so extract/translate/
generate/mods/prepare are skipped; this driver compiles the game module and
the app for arm64 with the NDK and packages the APK, then (--install) pushes
it to a Waydroid instance over adb.
"""
import argparse, json, os, re, shutil, subprocess, sys, time, zipfile
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

REPO = Path("/home/james/nfs/Projects/bluewake")
PREPARED = Path("/home/james/bw-build-android/prepared-behaviour")
OUT = Path("/home/james/bw-build-android")
SDK = Path("/home/james/android-sdk")
NDK = SDK / "ndk/android-ndk-r30"
NDK_BIN = NDK / "toolchains/llvm/prebuilt/linux-x86_64/bin"
BT = SDK / "build-tools/android-15"
PLATFORM_JAR = SDK / "platforms/android-36/android.jar"
ADB = SDK / "platform-tools/adb"
TOOLCHAIN = NDK / "build/cmake/android.toolchain.cmake"

MODULE = "libgGZLE01_recomp.so"   # name inside the APK / that the host dlopens
MODULE_SRC = "gGZLE01_recomp.so"  # the built file on disk (CMake PREFIX "")
ACTIVITY = "dev.bluewake.android.BlueWakeActivity"
APP_ID = "dev.bluewake.BlueWake"
MIN_SDK = 33
TARGET_SDK = 35
CPU = "cortex-a78"

def apk_filename(variant):
    """Versioned APK filename: WindWakerRecomp-<variant>-<sha8>-<HHMMSS>.apk.
    BLUEWAKE_APK_VARIANT overrides the variant label (e.g. 'pgo')."""
    variant = os.environ.get("BLUEWAKE_APK_VARIANT", variant)
    import datetime
    try:
        sha = subprocess.check_output(["git", "-C", str(REPO), "rev-parse", "--short=8", "HEAD"],
                                      text=True).strip()
    except Exception:
        sha = "unknown"
    ts = datetime.datetime.now().strftime("%H%M%S")
    return f"WindWakerRecomp-{variant}-{sha}-{ts}.apk"

def default_jobs():
    forced = os.environ.get("BW_JOBS")
    if forced and forced.isdigit():
        return int(forced)
    try:
        with open("/proc/meminfo") as m:
            for line in m:
                if line.startswith("MemAvailable:"):
                    avail = int(line.split()[1]) * 1024
                    import os as _os
                    return max(1, min(_os.cpu_count() or 8, int(avail // (2.0 * 2**30))))
    except OSError:
        import os as _os
        return _os.cpu_count() or 8


def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)


def run(name, cmd, env=None, cwd=None):
    logs = OUT / "logs"
    logs.mkdir(parents=True, exist_ok=True)
    lf = logs / f"{name}.log"
    log(f"  {name} (log: {lf.name})")
    with open(lf, "wb") as stream:
        p = subprocess.Popen([str(c) for c in cmd], cwd=cwd or REPO,
                             stdout=stream, stderr=subprocess.STDOUT, env=env)
        while True:
            try:
                rc = p.wait(timeout=15)
                break
            except subprocess.TimeoutExpired:
                pass
    if rc != 0:
        tail = lf.read_bytes()[-4000:].decode(errors="replace")
        print(tail, file=sys.stderr)
        raise SystemExit(f"{name} failed (exit {rc}); log {lf}")


def _android_define_args():
    return ["-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
            "-DANDROID_ABI=arm64-v8a", f"-DANDROID_PLATFORM=android-{MIN_SDK}",
            "-DANDROID_STL=c++_static", "-DCMAKE_BUILD_TYPE=Release"]


def build_module():
    log("compiling the game module for arm64 (the long step)")
    build = OUT / "composite"
    switches = {
        "FIXED_CPU": True, "NATIVE_J3D": True, "NATIVE_VEC": True,
        "NATIVE_GAME_MATH": True, "NATIVE_SKIN": True, "NATIVE_MATH": True,
        "NATIVE_ENTRIES": False, "DIRECT_CALLS": True, "GATHER_PIPE": True,
        "INLINE_FP": True, "FIXED_MEM1": True,
    }
    cflags = f"-mcpu={CPU} -fno-slp-vectorize -mllvm -large-interval-freq-threshold=10"
    linkflags = "-Wl,-z,max-page-size=16384 -Wl,--gc-sections -fuse-ld=lld"
    cmd = ["cmake", "-S", str(REPO / "cmake/composite"), "-B", str(build),
           *_android_define_args(),
           f"-DCMAKE_C_FLAGS={cflags}",
           f"-DCMAKE_SHARED_LINKER_FLAGS={linkflags}",
           "-DCOMPOSITE_OPTIMIZATION_LEVEL=2",
           f"-DCOMPOSITE_DIR={PREPARED / 'composite-src'}",
           f"-DGXRUNTIME_DIR={REPO / 'ref/recompcore/GXRuntime'}",
           f"-DABI_DIR={REPO / 'ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp'}",
           "-DCOMPOSITE_COLD_SOURCES_FILE="]
    for name, on in switches.items():
        cmd.append(f"-DBLUEWAKE_{name}={'ON' if on else 'OFF'}")
    run("module-configure", cmd)
    jobs = default_jobs()
    log(f"  {jobs} parallel compiles (memory-limited)")
    while True:
        try:
            run("module-build", ["cmake", "--build", str(build), "-j", str(jobs), "--", "-k", "0"])
            break
        except SystemExit:
            lf = OUT / "logs/module-build.log"
            text = lf.read_text(errors="replace") if lf.exists() else ""
            if jobs <= 1 or "out of memory" not in text:
                raise
            jobs = max(1, jobs // 2)
            log(f"  some chunks ran out of memory; compiling the rest with {jobs} jobs")
    mod = build / MODULE_SRC
    if not mod.exists():
        raise SystemExit("game module not produced")
    log(f"module: {mod} ({mod.stat().st_size >> 20} MB)")


def build_app():
    log("building libmain.so (host + GXRuntime + Aurora/Dawn + SDL3 + DSP)")
    build = OUT / "app"
    cmd = ["cmake", "-S", str(REPO / "android"), "-B", str(build),
           *_android_define_args(),
           f"-DBLUEWAKE_ANDROID_CPU={CPU}",
           "-DBLUEWAKE_APP_PROFILE_GENERATE=OFF",
           "-DBLUEWAKE_APP_PROFILE_USE=",
           # Cross-compile fix: the desktop's HOST zstd (via pkg-config) is x86_64,
           # invisible to the NDK cross-compiler. Disable the optional cache
           # compression rather than linking host zstd. (Windows has no host zstd,
           # so PR #93 correctly fetched it from source there.)
           "-DAURORA_CACHE_USE_ZSTD=OFF"]
    run("app-configure", cmd)
    run("app-build", ["cmake", "--build", str(build), "--target", "main", "-j", str(max(4, default_jobs()))])
    lib = build / "libmain.so"
    if not lib.exists():
        raise SystemExit("libmain.so not produced")
    log(f"app: {lib} ({lib.stat().st_size >> 20} MB)")


def package():
    log("packaging the APK")
    pkg = OUT / "apk"
    shutil.rmtree(pkg, ignore_errors=True)
    (pkg / "classes").mkdir(parents=True)

    module = OUT / "composite" / MODULE_SRC
    lib = OUT / "app" / "libmain.so"

    sdl_src = sorted((OUT / "app" / "_deps").glob("sdl-src/android-project/app/src/main/java"))
    if not sdl_src:
        raise SystemExit("SDL java sources not found in app/_deps/sdl-src")
    sdl_java = pkg / "sdl-java"
    shutil.copytree(sdl_src[0], sdl_java)

    activity = sdl_java / "org/libsdl/app/SDLActivity.java"
    text = activity.read_text(encoding="utf-8")
    orig = 'new Thread(new SDLMain(), "SDLThread")'
    if orig not in text:
        raise SystemExit("SDLActivity.java thread creation anchor missing")
    activity.write_text(text.replace(orig, 'new Thread(null, new SDLMain(), "SDLThread", 64L << 20)'),
                        encoding="utf-8")

    surface = sdl_java / "org/libsdl/app/SDLSurface.java"
    text = surface.read_text(encoding="utf-8")
    edits = [
        ("        SDLActivity.onNativeSurfaceChanged();\n",
         "        SDLActivity.onNativeSurfaceChanged();\n        auroraNativeSetSurfaceReady(true);\n"),
        ("        mIsSurfaceReady = false;\n        SDLActivity.onNativeSurfaceDestroyed();\n",
         "        mIsSurfaceReady = false;\n        auroraNativeSetSurfaceReady(false);\n"
         "        SDLActivity.onNativeSurfaceDestroyed();\n"),
        ("    // Called when we have a valid drawing surface\n",
         "    static native void auroraNativeSetSurfaceReady(boolean ready);\n\n"
         "    // Called when we have a valid drawing surface\n"),
    ]
    for old, new in edits:
        if text.count(old) != 1:
            raise SystemExit(f"SDLSurface.java patch anchor missing: {old.strip()}")
        text = text.replace(old, new)
    surface.write_text(text, encoding="utf-8")

    java = [str(p) for p in sdl_java.rglob("*.java")]
    java += [str(p) for p in (REPO / "android/java").rglob("*.java")]

    run("apk-javac", ["javac", "-nowarn", "-encoding", "UTF-8", "--release", "11",
                      "-classpath", str(PLATFORM_JAR), "-d", str(pkg / "classes"), *java])

    classes_jar = pkg / "classes.jar"
    with zipfile.ZipFile(classes_jar, "w", zipfile.ZIP_STORED) as jar:
        for p in sorted((pkg / "classes").rglob("*.class")):
            jar.write(p, p.relative_to(pkg / "classes").as_posix())
    run("apk-d8", [BT / "d8", "--release", "--min-api", str(MIN_SDK),
                   "--lib", str(PLATFORM_JAR), "--output", str(pkg), str(classes_jar)])

    manifest = (REPO / "android/AndroidManifest.xml").read_text(encoding="utf-8")
    manifest = (manifest.replace("@PACKAGE@", APP_ID)
                .replace("@LABEL@", xml_escape("BlueWake", {'"': "&quot;", "'": "&apos;"}))
                .replace("@DEBUGGABLE@", "false"))
    (pkg / "AndroidManifest.xml").write_text(manifest, encoding="utf-8")

    res_zip = pkg / "res.zip"
    run("apk-aapt2-compile", [BT / "aapt2", "compile", "--dir", str(REPO / "android/res"),
                              "-o", str(res_zip)])

    assets = pkg / "assets"
    assets.mkdir()
    shutil.copy2(REPO / "apple/ios/resources/initial_pipeline_cache.db",
                 assets / "initial_pipeline_cache.db")

    unsigned = pkg / "unsigned.apk"
    run("apk-aapt2-link", [BT / "aapt2", "link", "-o", str(unsigned), "-A", str(assets), "-0", "db",
                           "--manifest", str(pkg / "AndroidManifest.xml"), "-I", str(PLATFORM_JAR),
                           "--min-sdk-version", str(MIN_SDK), "--target-sdk-version", str(TARGET_SDK),
                           "--version-code", str(int(time.time()) // 60), "--version-name", "0.1-android",
                           str(res_zip)])

    (pkg / "lib").mkdir()
    strip = NDK_BIN / "llvm-strip"
    stripped_lib = pkg / "lib/libmain.so"
    run("apk-strip-main", [strip, "--strip-debug", "-o", str(stripped_lib), str(lib)])
    stripped_module = pkg / f"lib/{MODULE}"
    run("apk-strip-module", [strip, "--strip-debug", "-o", str(stripped_module), str(module)])

    with zipfile.ZipFile(unsigned, "a") as apk:
        apk.write(pkg / "classes.dex", "classes.dex", compress_type=zipfile.ZIP_DEFLATED)
        apk.write(stripped_lib, "lib/arm64-v8a/libmain.so", compress_type=zipfile.ZIP_STORED)
        apk.write(stripped_module, f"lib/arm64-v8a/{MODULE}", compress_type=zipfile.ZIP_STORED)

    aligned = pkg / "aligned.apk"
    run("apk-zipalign", [BT / "zipalign", "-f", "-P", "16", "4", str(unsigned), str(aligned)])

    keystore = OUT / "debug.keystore"
    if not keystore.exists():
        run("apk-keystore", ["keytool", "-genkeypair", "-keystore", str(keystore),
                             "-storepass", "android", "-keypass", "android", "-alias", "androiddebugkey",
                             "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000",
                             "-dname", "CN=Android Debug,O=Android,C=US"])
    apk_out = OUT / apk_filename("untrained")
    run("apk-sign", [BT / "apksigner", "sign", "--ks", str(keystore),
                     "--ks-pass", "pass:android", "--out", str(apk_out), str(aligned)])

    prov = {
        "profile": "bluewake-android",
        "package": APP_ID,
        "containsTranslatedGameCode": True,
        "cpu": CPU,
        "iso": str(PREPARED / "BlueWake/game/GZLE01.iso"),
        "composite_digest": (PREPARED / "composite-src.digest").read_text().strip(),
        "built": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    (OUT / "BuilderProvenance.json").write_text(json.dumps(prov, indent=2) + "\n")
    log(f"APK: {apk_out} ({apk_out.stat().st_size >> 20} MB)")


def install():
    log("installing on Waydroid over adb")
    remote = f"/sdcard/Android/data/{APP_ID}/files"
    apk = sorted(OUT.glob("WindWakerRecomp-*.apk"), key=lambda p: p.stat().st_mtime)[-1]
    game = PREPARED / "game"
    iso = PREPARED / "BlueWake/game/GZLE01.iso"

    def adb(*a, check=True):
        cmd = [str(ADB), *[str(x) for x in a]]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if check and r.returncode != 0:
            raise SystemExit(f"adb {' '.join(map(str,a))} failed: {r.stdout}{r.stderr}")
        return r.stdout

    log("  adb install")
    adb("install", "-r", "-g", apk)
    adb("shell", f"mkdir -p '{remote}/game/rels'")
    log("  pushing main.dol")
    adb("push", game / "main.dol", f"{remote}/game/main.dol")
    log("  pushing 415 RELs")
    adb("push", game / "rels", f"{remote}/game/")
    log("  pushing the disc image (1.4 GB)")
    adb("push", iso, f"{remote}/game/GZLE01.iso")
    adb("shell", f"chmod 2775 '{remote}/game' '{remote}/game/rels'")
    log("done")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("step", nargs="?", default="all",
                    choices=["module", "app", "package", "install", "all"])
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if args.step in ("module", "all"):
        build_module()
    if args.step in ("app", "all"):
        build_app()
    if args.step in ("package", "all"):
        package()
    if args.step == "install":
        install()


if __name__ == "__main__":
    main()
