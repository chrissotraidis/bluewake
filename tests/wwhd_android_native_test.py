#!/usr/bin/env python3
"""Generated JNI import/lifetime checks, with no app, disc or UI dependencies.

Requires a JDK, CMake/Ninja, pycryptodome and local RecompCore/xxHash sources.
Always runs the real bridge on the host JVM with -Xcheck:jni. --ndk also builds
the same sources for arm64 Android API 33; --sdk prepares the app_process DEX.
--serial emulator-N runs it on an ALREADY COORDINATED emulator, never boots one,
never targets a physical device, and removes only its own remote scratch tree.
Builds/logs/generated inputs stay in --build-dir, not the production app tree.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
import wwhd_disc_import_test as fixture

ROOT = Path(__file__).resolve().parents[1]
MAIN = "dev.bluewake.android.WWHDAndroidNativeTest"


def execute(argv, log, env=None):
    with log.open("w", encoding="utf-8") as stream:
        process = subprocess.run([str(x) for x in argv], env=env, stdout=stream,
                                 stderr=subprocess.STDOUT, timeout=300)
    if process.returncode:
        raise RuntimeError(f"Command failed ({process.returncode}); see {log}")
    return log.read_text(encoding="utf-8", errors="replace")


def fixtures(directory):
    inputs = directory / "inputs"
    (inputs / "folder/content").mkdir(parents=True)
    (directory / "outputs").mkdir()
    bfres = fixture.make_bfres()
    (inputs / "folder/content/A.bfres").write_bytes(bfres)
    (inputs / "GC.iso").write_bytes(fixture.make_gamecube())
    image = fixture.make_wiiu(bfres)
    (inputs / "toy.wud").write_bytes(image)
    (inputs / "toy.wux").write_bytes(fixture.fixture.mapped_wux(image))
    for name, value in (("disc", fixture.fixture.DISC), ("title", fixture.fixture.TITLE),
                        ("common", fixture.fixture.COMMON), ("wrong", fixture.fixture.WRONG)):
        (inputs / (name + ".key")).write_bytes(value)


def verify(outputs, baseline=None):
    reports = sorted(outputs.rglob("import-report.json"))
    assert len(reports) == 8, (outputs, len(reports))
    for path in outputs.rglob("*"):
        if not path.name.startswith("."):
            continue
        # Each JNI target is an output parent. Only its persistent empty lock
        # is expected; keep rejecting wrappers, links and locks inside packs.
        assert (path.name == ".bluewake-wwhd-import.lock" and path.parent.parent == outputs
                and not path.is_symlink() and path.is_file() and path.stat().st_size == 0), "staging leaked: " + str(path)
    for report in reports:
        pngs = fixture.check_pack(report.parent, exact=False)
        if baseline is None:
            baseline = pngs
        assert pngs == baseline, "PNG bytes differ between JNI inputs/key modes"
    assert len(list(outputs.rglob("*.png"))) == 48
    return baseline


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--runtime", required=True, type=Path, help="Pinned RecompCore root")
    parser.add_argument("--xxhash", required=True, type=Path, help="Folder containing xxhash.h and xxhash.c")
    parser.add_argument("--java-home", required=True, type=Path)
    parser.add_argument("--ndk", type=Path)
    parser.add_argument("--sdk", type=Path)
    parser.add_argument("--serial", help="Coordinated, already booted emulator-N only")
    args = parser.parse_args()
    if args.serial and (not re.fullmatch(r"emulator-\d+", args.serial) or not args.sdk or not args.ndk):
        parser.error("--serial requires an emulator-N serial, --ndk and --sdk")
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    evidence = build / "runs" / stamp
    evidence.mkdir(parents=True)
    # Reuse the production source list, so the focused build cannot silently
    # omit a newly added importer source. Crypto uses the production CMake too.
    source_names = re.findall(r"\$\{BLUEWAKE_HOST_SRC\}/wwhd/(\w+\.cpp)",
                             (ROOT / "android/CMakeLists.txt").read_text())
    assert len(source_names) == len(set(source_names)) and len(source_names) >= 10
    sources = [ROOT / "runtime/host/src/wwhd" / name for name in source_names]
    bridge = ROOT / "android/src/hd_import.cpp"
    java = ROOT / "android/java/dev/bluewake/android/HDImportNative.java"
    mbed = args.runtime / "Externals/mbedtls"
    tracked = [bridge, java, ROOT / "android/CMakeLists.txt", ROOT / "cmake/wwhd_disc_crypto.cmake",
               *sorted((ROOT / "runtime/host/src/wwhd").glob("*.[ch]*")),
               *[mbed / "library" / name for name in ("aes.c", "sha1.c", "platform_util.c")],
               args.xxhash / "xxhash.c", args.xxhash / "xxhash.h", Path(__file__),
               ROOT / "tests/wwhd_android_native_test.java"]
    def hashes():
        return {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in tracked}
    before = hashes()
    record = {"revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "runtime_revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=args.runtime,
                                                          text=True).strip(),
              "sources_before": before, "importer_sources": source_names, "results": []}
    (evidence / "source-snapshot.json").write_text(json.dumps(record, indent=2) + "\n")
    project = build / "project"
    project.mkdir(exist_ok=True)
    native_sources = "\n".join('    "' + str(p) + '"' for p in [bridge, *sources])
    cmake = f'''cmake_minimum_required(VERSION 3.25)
project(WWHDAndroidNativeTest LANGUAGES C CXX)
set(GXRUNTIME_DIR "{args.runtime.resolve()}/GXRuntime")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(Threads REQUIRED)
add_library(xxhash STATIC "{args.xxhash.resolve()}/xxhash.c")
target_include_directories(xxhash PUBLIC "{args.xxhash.resolve()}")
add_library(wwhd_android_native SHARED
{native_sources})
target_include_directories(wwhd_android_native PRIVATE "{ROOT}/runtime/host/src")
target_compile_features(wwhd_android_native PRIVATE cxx_std_17)
set_source_files_properties("{bridge}" PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror")
include("{ROOT}/cmake/wwhd_disc_crypto.cmake")
target_link_libraries(wwhd_android_native PRIVATE bluewake_wwhd_disc_crypto xxhash z Threads::Threads)
if(ANDROID)
  target_link_options(wwhd_android_native PRIVATE "LINKER:--no-undefined" "LINKER:-z,max-page-size=16384")
else()
  target_include_directories(wwhd_android_native PRIVATE "{args.java_home}/include"
      "{args.java_home}/include/{'darwin' if sys.platform == 'darwin' else 'linux'}")
endif()
'''
    (project / "CMakeLists.txt").write_text(cmake)
    host = build / "host"
    execute(["cmake", "-S", project, "-B", host, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug"],
            evidence / "host-configure.log")
    execute(["cmake", "--build", host, "-j", "2"], evidence / "host-build.log")
    classes = build / "classes"
    classes.mkdir(exist_ok=True)
    execute([args.java_home / "bin/javac", "--release", "11", "-encoding", "UTF-8", "-d", classes,
             java, ROOT / "tests/wwhd_android_native_test.java"], evidence / "javac.log")
    host_lib = host / ("libwwhd_android_native.dylib" if sys.platform == "darwin" else "libwwhd_android_native.so")
    baseline = None
    for label in ("ascii", "unicode-é-船-🌊"):
        directory = evidence / label
        fixtures(directory)
        env = dict(os.environ, BLUEWAKE_DISC=str(directory / "inputs/GC.iso"))
        cmd = [args.java_home / "bin/java", "-Xcheck:jni", "-cp", classes, MAIN, host_lib, directory]
        output = execute(cmd, evidence / (label + "-host.log"), env)
        assert "JNI PASS:" in output and "WARNING in native method" not in output, output
        print(output.strip())
        baseline = verify(directory / "outputs", baseline)
        env.pop("BLUEWAKE_DISC")
        output = execute([*cmd, "missing-gc"], evidence / (label + "-missing-gc.log"), env)
        assert "JNI PASS:" in output, output
        record["results"].append({"platform": "host-JVM", "paths": label, "imports": 8, "png_checks": 48})
    if args.ndk:
        android = build / "android"
        execute(["cmake", "-S", project, "-B", android, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                 "-DCMAKE_TOOLCHAIN_FILE=" + str(args.ndk / "build/cmake/android.toolchain.cmake"),
                 "-DANDROID_ABI=arm64-v8a", "-DANDROID_PLATFORM=android-33", "-DANDROID_STL=c++_static"],
                evidence / "android-configure.log")
        execute(["cmake", "--build", android, "-j", "2"], evidence / "android-build.log")
        record["android_build"] = {"ndk": str(args.ndk), "api": 33, "abi": "arm64-v8a", "stl": "c++_static"}
        print("NDK PASS: arm64-v8a / API 33 / static libc++ / all importer and crypto sources")
        if args.sdk:
            version = lambda p: tuple(int(x) for x in re.findall(r"\d+", p.name))
            tools = max((args.sdk / "build-tools").iterdir(), key=version)
            platform = max((args.sdk / "platforms").glob("android-*"), key=version)
            dex = build / "dex"
            dex.mkdir(exist_ok=True)
            execute([tools / "d8", "--lib", platform / "android.jar", "--min-api", "33", "--output", dex,
                     *sorted(classes.rglob("*.class"))], evidence / "d8.log",
                    dict(os.environ, JAVA_HOME=str(args.java_home)))
            if args.serial:
                adb = [args.sdk / "platform-tools/adb", "-s", args.serial]
                qemu = subprocess.check_output([str(x) for x in [*adb, "shell", "getprop", "ro.kernel.qemu"]], text=True)
                assert qemu.strip() == "1", "refusing non-emulator target"
                remote = "/data/local/tmp/bluewake-wwhd-jni-" + stamp
                def device(command, label):
                    return execute([*adb, "shell", command], evidence / (label + ".log"))
                device("mkdir " + shlex.quote(remote), "device-mkdir")
                try:
                    execute([*adb, "push", android / "libwwhd_android_native.so", dex / "classes.dex", remote],
                            evidence / "device-push-code.log")
                    for index, label in enumerate(("ascii", "unicode-é-船-🌊")):
                        fresh = evidence / ("device-inputs-" + str(index))
                        fixtures(fresh)
                        target = remote + "/" + label
                        execute([*adb, "push", fresh, target], evidence / (f"device-push-{index}.log"))
                        # adb push skips empty directories; failure-message cases
                        # create their sentinel here before the importer runs.
                        device("mkdir -p " + shlex.quote(target + "/outputs"), f"device-outputs-{index}")
                        argv = ["env", "CLASSPATH=" + remote + "/classes.dex",
                                "BLUEWAKE_DISC=" + target + "/inputs/GC.iso", "/system/bin/app_process",
                                "-Xcheck:jni", "/", MAIN, remote + "/libwwhd_android_native.so", target]
                        output = device(shlex.join(argv), f"device-run-{index}")
                        assert "JNI PASS:" in output and "JNI DETECTED ERROR" not in output, output
                        print(output.strip())
                        missing_gc = ["env", "-u", "BLUEWAKE_DISC",
                                      *[arg for arg in argv[1:] if not arg.startswith("BLUEWAKE_DISC=")],
                                      "missing-gc"]
                        output = device(shlex.join(missing_gc), f"device-missing-gc-{index}")
                        assert "JNI PASS:" in output and "JNI DETECTED ERROR" not in output, output
                        pulled = evidence / ("device-results-" + str(index))
                        execute([*adb, "pull", target + "/outputs", pulled], evidence / (f"device-pull-{index}.log"))
                        verify(pulled, baseline)
                        record["results"].append({"platform": "Android-emulator-JNI", "paths": label,
                                                  "imports": 8, "png_checks": 48})
                finally:
                    device("rm -rf " + shlex.quote(remote), "device-cleanup")
    record["sources_after"] = hashes()
    assert record["sources_after"] == before, "Sources changed during the test; rerun from a stable snapshot"
    (evidence / "summary.json").write_text(json.dumps(record, indent=2) + "\n")
    print("Evidence:", evidence)


if __name__ == "__main__":
    main()
