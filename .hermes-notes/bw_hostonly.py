#!/usr/bin/env python3
"""Build a HOST-ONLY debuggable APK (libmain.so, no game module) so the
instrumented module can be pushed into the app's internal dir and dlopened
via BLUEWAKE_COMPOSITE (internal data dir IS in the linker namespace, unlike
external storage)."""
import os, shutil, subprocess, sys, time, zipfile
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

REPO = Path("/home/james/nfs/Projects/bluewake")
OUT = Path("/home/james/bw-build-android")
SDK = Path("/home/james/android-sdk")
NDK_BIN = SDK / "ndk/android-ndk-r30/toolchains/llvm/prebuilt/linux-x86_64/bin"
BT = SDK / "build-tools/android-15"
PLATFORM_JAR = SDK / "platforms/android-36/android.jar"
ADB = SDK / "platform-tools/adb"
APP_ID = "dev.bluewake.BlueWake"
MIN_SDK = 33
TARGET_SDK = 35

def log(m): print(f"[{time.strftime('%H:%M:%S')}] {m}", flush=True)

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
        raise SystemExit(f"{name} failed ({rc}); {lf}")

def main():
    lib = OUT / "app" / "libmain.so"
    if not lib.exists():
        raise SystemExit("libmain.so missing; build app first")
    pkg = OUT / "apk-hostonly"
    shutil.rmtree(pkg, ignore_errors=True)
    (pkg / "classes").mkdir(parents=True)
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
            raise SystemExit("SDLSurface anchor missing")
        text = text.replace(old, new)
    surface.write_text(text, encoding="utf-8")
    java = [str(p) for p in sdl_java.rglob("*.java")] + [str(p) for p in (REPO / "android/java").rglob("*.java")]
    run("ho-javac", ["javac", "-nowarn", "-encoding", "UTF-8", "--release", "11",
                     "-classpath", str(PLATFORM_JAR), "-d", str(pkg / "classes"), *java])
    classes_jar = pkg / "classes.jar"
    with zipfile.ZipFile(classes_jar, "w", zipfile.ZIP_STORED) as jar:
        for p in sorted((pkg / "classes").rglob("*.class")):
            jar.write(p, p.relative_to(pkg / "classes").as_posix())
    run("ho-d8", [BT / "d8", "--release", "--min-api", str(MIN_SDK),
                  "--lib", str(PLATFORM_JAR), "--output", str(pkg), str(classes_jar)])
    manifest = (REPO / "android/AndroidManifest.xml").read_text(encoding="utf-8")
    manifest = (manifest.replace("@PACKAGE@", APP_ID)
                .replace("@LABEL@", xml_escape("BlueWake", {'"': "&quot;", "'": "&apos;"}))
                .replace("@DEBUGGABLE@", "true"))
    (pkg / "AndroidManifest.xml").write_text(manifest, encoding="utf-8")
    res_zip = pkg / "res.zip"
    run("ho-aapt2-compile", [BT / "aapt2", "compile", "--dir", str(REPO / "android/res"), "-o", str(res_zip)])
    assets = pkg / "assets"
    assets.mkdir()
    shutil.copy2(REPO / "apple/ios/resources/initial_pipeline_cache.db", assets / "initial_pipeline_cache.db")
    unsigned = pkg / "unsigned.apk"
    run("ho-aapt2-link", [BT / "aapt2", "link", "-o", str(unsigned), "-A", str(assets), "-0", "db",
                          "--manifest", str(pkg / "AndroidManifest.xml"), "-I", str(PLATFORM_JAR),
                          "--min-sdk-version", str(MIN_SDK), "--target-sdk-version", str(TARGET_SDK),
                          "--version-code", str(int(time.time()) // 60), "--version-name", "0.1-android",
                          str(res_zip)])
    (pkg / "lib").mkdir()
    stripped_lib = pkg / "lib/libmain.so"
    run("ho-strip", [NDK_BIN / "llvm-strip", "--strip-debug", "-o", str(stripped_lib), str(lib)])
    with zipfile.ZipFile(unsigned, "a") as apk:
        apk.write(pkg / "classes.dex", "classes.dex", compress_type=zipfile.ZIP_DEFLATED)
        apk.write(stripped_lib, "lib/arm64-v8a/libmain.so", compress_type=zipfile.ZIP_STORED)
    aligned = pkg / "aligned.apk"
    run("ho-zipalign", [BT / "zipalign", "-f", "-P", "16", "4", str(unsigned), str(aligned)])
    keystore = OUT / "debug.keystore"
    apk_out = OUT / "hostonly.apk"
    run("ho-sign", [BT / "apksigner", "sign", "--ks", str(keystore),
                    "--ks-pass", "pass:android", "--out", str(apk_out), str(aligned)])
    log(f"host-only APK: {apk_out} ({apk_out.stat().st_size >> 20} MB)")

if __name__ == "__main__":
    main()
