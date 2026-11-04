#!/usr/bin/env python3
"""BlueWake Android arm64 PGO training, mirroring PR #93's --device path.
Phase 1 'instrumented': compile the module at -O0 -fprofile-instr-generate
  (with profile_flush.c), fast.
Phase 2 'train-apk': package a training APK (instrumented module + regular app).
Phase 3 'playback': on Waydroid, run the scripted opening headless+unpaced
  (plain, then widescreen+betterww), pull the .profraw files, merge them.
Phase 4 'optimized': recompile the module -O2 -fprofile-instr-use=<profile>.
Phase 5 'final-apk': package the real APK with the optimized module.
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
DEVICE_FILES = f"/sdcard/Android/data/{APP_ID}/files"

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


def compile_composite(build, opt_level, extra_flags, extra_link_flags, name, extra_cmake=()):
    cflags = f"-mcpu={CPU} -fno-slp-vectorize -mllvm -large-interval-freq-threshold=10 " + " ".join(extra_flags)
    linkflags = "-Wl,-z,max-page-size=16384 -Wl,--gc-sections -fuse-ld=lld " + " ".join(extra_link_flags)
    cmd = ["cmake", "-S", str(REPO / "cmake/composite"), "-B", str(build), *_android_args(),
           f"-DCMAKE_C_FLAGS={cflags}", f"-DCMAKE_SHARED_LINKER_FLAGS={linkflags}",
           f"-DCOMPOSITE_OPTIMIZATION_LEVEL={opt_level}",
           f"-DCOMPOSITE_DIR={PREPARED / 'composite-src'}",
           f"-DGXRUNTIME_DIR={REPO / 'ref/recompcore/GXRuntime'}",
           f"-DABI_DIR={REPO / 'ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp'}",
           "-DCOMPOSITE_COLD_SOURCES_FILE=", *extra_cmake]
    for n, on in SWITCHES.items():
        cmd.append(f"-DBLUEWAKE_{n}={'ON' if on else 'OFF'}")
    run(f"{name}-configure", cmd)
    jobs = default_jobs()
    log(f"  {jobs} parallel compiles")
    while True:
        try:
            run(f"{name}-build", ["cmake", "--build", str(build), "-j", str(jobs), "--", "-k", "0"])
            break
        except SystemExit:
            lf = OUT / "logs" / f"{name}-build.log"
            text = lf.read_text(errors="replace") if lf.exists() else ""
            if jobs <= 1 or "out of memory" not in text:
                raise
            jobs = max(1, jobs // 2)
            log(f"  OOM; retrying with {jobs} jobs")
    mod = build / MODULE_SRC
    if not mod.exists():
        raise SystemExit(f"module not produced at {mod}")
    return mod


def adb(*a, check=True, capture=False):
    cmd = [str(ADB), *[str(x) for x in a]]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        raise SystemExit(f"adb {' '.join(map(str,a))} failed: {r.stdout}{r.stderr}")
    return r.stdout if capture else None


def build_app(build=None):
    build = build or OUT / "app"
    # app already built in OUT/app; reuse if present
    if (build / "libmain.so").exists():
        log("reusing existing libmain.so")
        return build / "libmain.so"
    raise SystemExit("libmain.so not found; run bw_android.py app first")


def package(module, apk_name, pkg_dir, provenance_name):
    pkg = OUT / pkg_dir
    shutil.rmtree(pkg, ignore_errors=True)
    (pkg / "classes").mkdir(parents=True)
    lib = OUT / "app" / "libmain.so"
    sdl_src = sorted((OUT / "app" / "_deps").glob("sdl-src/android-project/app/src/main/java"))
    if not sdl_src:
        raise SystemExit("SDL java sources not found")
    sdl_java = pkg / "sdl-java"
    shutil.copytree(sdl_src[0], sdl_java)
    activity = sdl_java / "org/libsdl/app/SDLActivity.java"
    text = activity.read_text(encoding="utf-8")
    orig = 'new Thread(new SDLMain(), "SDLThread")'
    if orig not in text:
        raise SystemExit("SDLActivity anchor missing")
    activity.write_text(text.replace(orig, 'new Thread(null, new SDLMain(), "SDLThread", 64L << 20)'), encoding="utf-8")
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
            raise SystemExit(f"SDLSurface anchor missing: {old.strip()}")
        text = text.replace(old, new)
    surface.write_text(text, encoding="utf-8")
    java = [str(p) for p in sdl_java.rglob("*.java")] + [str(p) for p in (REPO / "android/java").rglob("*.java")]
    run(f"{provenance_name}-javac", ["javac", "-nowarn", "-encoding", "UTF-8", "--release", "11",
                                     "-classpath", str(PLATFORM_JAR), "-d", str(pkg / "classes"), *java])
    classes_jar = pkg / "classes.jar"
    with zipfile.ZipFile(classes_jar, "w", zipfile.ZIP_STORED) as jar:
        for p in sorted((pkg / "classes").rglob("*.class")):
            jar.write(p, p.relative_to(pkg / "classes").as_posix())
    run(f"{provenance_name}-d8", [BT / "d8", "--release", "--min-api", str(MIN_SDK),
                                  "--lib", str(PLATFORM_JAR), "--output", str(pkg), str(classes_jar)])
    manifest = (REPO / "android/AndroidManifest.xml").read_text(encoding="utf-8")
    manifest = (manifest.replace("@PACKAGE@", APP_ID)
                .replace("@LABEL@", xml_escape("BlueWake", {'"': "&quot;", "'": "&apos;"}))
                .replace("@DEBUGGABLE@", "true"))
    (pkg / "AndroidManifest.xml").write_text(manifest, encoding="utf-8")
    res_zip = pkg / "res.zip"
    run(f"{provenance_name}-aapt2-compile", [BT / "aapt2", "compile", "--dir", str(REPO / "android/res"), "-o", str(res_zip)])
    assets = pkg / "assets"
    assets.mkdir()
    shutil.copy2(REPO / "apple/ios/resources/initial_pipeline_cache.db", assets / "initial_pipeline_cache.db")
    unsigned = pkg / "unsigned.apk"
    run(f"{provenance_name}-aapt2-link", [BT / "aapt2", "link", "-o", str(unsigned), "-A", str(assets), "-0", "db",
                                          "--manifest", str(pkg / "AndroidManifest.xml"), "-I", str(PLATFORM_JAR),
                                          "--min-sdk-version", str(MIN_SDK), "--target-sdk-version", str(TARGET_SDK),
                                          "--version-code", str(int(time.time()) // 60), "--version-name", "0.1-android",
                                          str(res_zip)])
    (pkg / "lib").mkdir()
    strip = NDK_BIN / "llvm-strip"
    stripped_lib = pkg / "lib/libmain.so"
    run(f"{provenance_name}-strip-main", [strip, "--strip-debug", "-o", str(stripped_lib), str(lib)])
    stripped_module = pkg / f"lib/{MODULE}"
    run(f"{provenance_name}-strip-module", [strip, "--strip-debug", "-o", str(stripped_module), str(module)])
    with zipfile.ZipFile(unsigned, "a") as apk:
        apk.write(pkg / "classes.dex", "classes.dex", compress_type=zipfile.ZIP_DEFLATED)
        apk.write(stripped_lib, "lib/arm64-v8a/libmain.so", compress_type=zipfile.ZIP_STORED)
        apk.write(stripped_module, f"lib/arm64-v8a/{MODULE}", compress_type=zipfile.ZIP_STORED)
    aligned = pkg / "aligned.apk"
    run(f"{provenance_name}-zipalign", [BT / "zipalign", "-f", "-P", "16", "4", str(unsigned), str(aligned)])
    keystore = OUT / "debug.keystore"
    if not keystore.exists():
        run(f"{provenance_name}-keystore", ["keytool", "-genkeypair", "-keystore", str(keystore),
                                            "-storepass", "android", "-keypass", "android", "-alias", "androiddebugkey",
                                            "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000",
                                            "-dname", "CN=Android Debug,O=Android,C=US"])
    apk_out = OUT / apk_name
    run(f"{provenance_name}-sign", [BT / "apksigner", "sign", "--ks", str(keystore),
                                    "--ks-pass", "pass:android", "--out", str(apk_out), str(aligned)])
    log(f"APK: {apk_out} ({apk_out.stat().st_size >> 20} MB)")
    return apk_out


def playback_one(name, mods):
    remote = f"{DEVICE_FILES}/pgo/{name}"
    adb("shell", "rm", "-rf", remote)
    adb("shell", "mkdir", "-p", remote)
    opening = [f"{n}:0x0100:2" for n in range(17800, 22001, 150)] + TRAINING_RUN
    env = {
        "LLVM_PROFILE_FILE": f"{remote}/%m-%p.profraw",
        "BLUEWAKE_CARD_PATH": f"{remote}/training.card",
        "BLUEWAKE_SRAM": f"{remote}/sram.bin",
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
    adb("shell", "am", "force-stop", APP_ID)
    adb("shell", "am", "start", "-n", f"{APP_ID}/{ACTIVITY}", "--ez", "showWhenLocked", "true")
    log(f"  playback {name} started ({'mods' if mods else 'plain'}, headless, unpaced)")
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
    # wait for profile flush + process exit
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
        raise SystemExit(f"playback {name} did not reach player control (see log)")
    files = [f for f in adb("shell", f"ls {remote}", capture=True).split() if f.endswith(".profraw")]
    if not files:
        raise SystemExit(f"playback {name} wrote no profile")
    raw = []
    local = OUT / "pgo" / name
    local.mkdir(parents=True, exist_ok=True)
    for f in files:
        adb("pull", f"{remote}/{f}", local / f)
        raw.append(local / f)
    log(f"  {name}: done in {int(time.monotonic()-start)//60} min, {len(raw)} profile(s)")
    return raw


def phase_instrumented():
    log("== phase 1: instrumented module (-O0 -fprofile-instr-generate) ==")
    mod = compile_composite(OUT / "composite-instr", "0",
                            ["-fprofile-instr-generate"], ["-fprofile-instr-generate"],
                            "instr",
                            extra_cmake=[f"-DCOMPOSITE_EXTRA_SOURCES={REPO / 'android/src/profile_flush.c'}"])
    log(f"instrumented module: {mod} ({mod.stat().st_size >> 20} MB)")


def phase_train_apk():
    log("== phase 2: training APK ==")
    mod = OUT / "composite-instr" / MODULE_SRC
    if not mod.exists():
        raise SystemExit("run phase 1 first")
    package(mod, "training.apk", "apk-training", "train")
    adb("install", "-r", "-g", OUT / "training.apk")


def phase_playback():
    log("== phase 3: playbacks ==")
    build_app()
    raw = playback_one("plain", None)
    raw += playback_one("mods", "widescreen,betterww")
    profile = OUT / "pgo" / "composite.profdata"
    run("pgo-merge", [NDK_BIN / "llvm-profdata", "merge", "-o", str(profile), *[str(r) for r in raw]])
    stats = subprocess.run([str(NDK_BIN / "llvm-profdata"), "show", "--all-functions", str(profile)],
                           capture_output=True, text=True).stdout
    executed = [int(c) for c in re.findall(
        r"(?m)^  func_[0-9A-Fa-f]+\S*:\n(?:    .*\n)*?    Function count: (\d+)", stats)]
    ran = sum(1 for c in executed if c > 0)
    log(f"profile: {ran} translated functions ran ({len(executed)} total)")
    if ran == 0:
        raise SystemExit("profile counted no functions")


def phase_optimized():
    log("== phase 4: optimized module (-O2 -fprofile-instr-use) ==")
    profile = OUT / "pgo" / "composite.profdata"
    if not profile.exists():
        raise SystemExit("run phase 3 first")
    flags = [f"-fprofile-instr-use={profile}", "-Wno-profile-instr-unprofiled",
             "-Wno-profile-instr-out-of-date", "-Wno-backend-plugin"]
    # cold sources: chunks the training never ran, compile at -O1
    mod = compile_composite(OUT / "composite-pgo", "2", flags, [], "pgo")
    log(f"optimized module: {mod} ({mod.stat().st_size >> 20} MB)")


def phase_final_apk():
    log("== phase 5: final APK ==")
    mod = OUT / "composite-pgo" / MODULE_SRC
    if not mod.exists():
        raise SystemExit("run phase 4 first")
    package(mod, "WindWakerRecomp.apk", "apk", "final")
    # install + keep game files (already pushed; only APK changes)
    adb("install", "-r", "-g", OUT / "WindWakerRecomp.apk")
    adb("shell", "rm", "-f", f"{DEVICE_FILES}/launch.env")
    log("PGO build installed; game files already on device")


def main():
    phase = sys.argv[1] if len(sys.argv) > 1 else "all"
    OUT.mkdir(parents=True, exist_ok=True)
    if phase in ("instrumented", "all"):
        phase_instrumented()
    if phase in ("train-apk", "all"):
        phase_train_apk()
    if phase in ("playback", "all"):
        phase_playback()
    if phase in ("optimized", "all"):
        phase_optimized()
    if phase in ("final-apk", "all"):
        phase_final_apk()


if __name__ == "__main__":
    main()
