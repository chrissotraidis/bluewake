#!/usr/bin/env python3
"""Build generated SAF adapter instrumentation. --serial runs on an already
booted, explicitly selected emulator; this script never creates/boots an AVD.
Only the standalone dev.bluewake.hdinputs.test package is installed/removed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = "dev.bluewake.hdinputs.test"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, default=Path(os.environ.get("ANDROID_HOME", Path.home() / "Library/Android/sdk")))
    parser.add_argument("--jdk", type=Path)
    parser.add_argument("--serial", help="Explicit emulator serial; omitted means build only")
    parser.add_argument("--repo-root", type=Path, default=ROOT, help="Base checkout for unchanged sources")
    parser.add_argument("--source-overlay", type=Path, help="Prefer staged repo-relative source files")
    parser.add_argument("--out", type=Path, help="Separate evidence/build directory")
    args = parser.parse_args()
    root = args.repo_root.resolve()
    overlay = args.source_overlay.resolve() if args.source_overlay else None
    def source(name):
        staged = overlay / name if overlay else None
        return staged if staged and staged.is_file() else root / name
    out = (args.out or root / "build/wwhd-disc-import/android-inputs").resolve()
    out.mkdir(parents=True, exist_ok=True)
    commands = []
    env = dict(os.environ)
    jdk = args.jdk or Path(subprocess.check_output(["/usr/libexec/java_home"], text=True).strip())
    env["JAVA_HOME"] = str(jdk)
    tools = args.sdk / "build-tools/36.0.0"
    jar = args.sdk / "platforms/android-36/android.jar"
    ndk = args.sdk / "ndk/29.0.14206865/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android28-clang++"

    def run(command, *, check=True, timeout=120):
        command = list(map(str, command))
        commands.append(shlex.join(command))
        result = subprocess.run(command, cwd=root, env=env, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        with (out / "build.log").open("a") as log:
            log.write(shlex.join(command) + "\n" + result.stdout)
        if check and result.returncode:
            raise RuntimeError(result.stdout)
        return result.stdout

    (out / "build.log").write_text("")
    classes = out / "classes"
    if classes.exists(): shutil.rmtree(classes)
    classes.mkdir(exist_ok=True)
    names = ["android/java/dev/bluewake/android/HDImportInputs.java",
             "android/java/dev/bluewake/android/HDImportWorkspace.java",
             "tests/wwhd_android_inputs/HDImportInputsTest.java"]
    sources = [source(name) for name in names]
    run([jdk / "bin/javac", "--release", "11", "-Xlint:all", "-classpath", jar, "-d", classes, *sources])
    run([jdk / "bin/jar", "--create", "--file", out / "classes.jar", "-C", classes, "."])
    run([tools / "d8", "--release", "--min-api", "28", "--lib", jar, "--output", out, out / "classes.jar"])
    library = out / "libwwhd_inputs_probe.so"
    run([ndk, "-std=c++17", "-shared", "-fPIC", "-static-libstdc++", "-O1", "-Wall", "-Wextra",
         "-I", root / "runtime/host/src", source("tests/wwhd_android_inputs/native_probe.cpp"),
         source("runtime/host/src/wwhd/disc_image.cpp"), "-o", library])
    unsigned = out / "unsigned.apk"
    run([tools / "aapt2", "link", "-o", unsigned, "--manifest", source("tests/wwhd_android_inputs/AndroidManifest.xml"), "-I", jar])
    with zipfile.ZipFile(unsigned, "a") as archive:
        archive.write(out / "classes.dex", "classes.dex")
        archive.write(library, "lib/arm64-v8a/libwwhd_inputs_probe.so")
    run([tools / "zipalign", "-f", "4", unsigned, out / "aligned.apk"])
    key = out / "generated-test.keystore"
    if not key.exists():
        run([jdk / "bin/keytool", "-genkeypair", "-keystore", key, "-storepass", "synthetic-only",
             "-keypass", "synthetic-only", "-alias", "test", "-keyalg", "RSA", "-validity", "30",
             "-dname", "CN=Generated adapter tests", "-noprompt"])
    apk = out / "wwhd-inputs-test.apk"
    run([tools / "apksigner", "sign", "--ks", key, "--ks-pass", "pass:synthetic-only", "--out", apk, out / "aligned.apk"])
    run([tools / "apksigner", "verify", apk])
    if args.serial:
        if not args.serial.startswith("emulator-"):
            raise ValueError("This generated fixture runner only accepts an emulator serial")
        adb = [args.sdk / "platform-tools/adb", "-s", args.serial]
        if run([*adb, "shell", "getprop", "ro.kernel.qemu"]).strip() != "1":
            raise ValueError("Selected device is not an emulator")
        if run([*adb, "shell", "pm", "path", PACKAGE], check=False).strip():
            raise ValueError("Fixture package already installed; preserve the other test session")
        run([*adb, "install", "-t", apk])
        try:
            result = run([*adb, "shell", "am", "instrument", "-w", "-r",
                          PACKAGE + "/dev.bluewake.android.HDImportInputsTest"])
            (out / "instrumentation.log").write_text(result)
            if "INSTRUMENTATION_CODE: -1" not in result or "generated Android document-adapter cases" not in result or "FAIL:" in result:
                raise RuntimeError(result)
            print(result)
            # A genuine process kill leaves three real adapter owners open.
            # A new instrumentation process must reacquire the retained lock,
            # remove only marked workspaces and preserve provider/legacy data.
            run([*adb, "shell", "am", "force-stop", PACKAGE])
            hold_command = list(map(str, [*adb, "shell", "am", "instrument", "-w", "-r", "-e", "phase", "crash-prepare",
                                          PACKAGE + "/dev.bluewake.android.HDImportInputsTest"]))
            commands.append(shlex.join(hold_command))
            hold_log = out / "process-death-prepare.log"
            with hold_log.open("w") as log:
                held = subprocess.Popen(hold_command, cwd=root, env=env, text=True, stdout=log, stderr=subprocess.STDOUT)
                try:
                    deadline = time.monotonic() + 30
                    while "RECOVERY_READY pid=" not in hold_log.read_text():
                        if held.poll() is not None or time.monotonic() >= deadline:
                            raise RuntimeError("Process-death fixture did not become ready: " + hold_log.read_text())
                        time.sleep(0.1)
                    run([*adb, "shell", "am", "force-stop", PACKAGE])
                    held.wait(timeout=15)
                    if run([*adb, "shell", "pidof", PACKAGE], check=False).strip():
                        raise RuntimeError("Killed fixture process still exists")
                finally:
                    if held.poll() is None:
                        run([*adb, "shell", "am", "force-stop", PACKAGE], check=False)
                        held.terminate()
                        try: held.wait(timeout=5)
                        except subprocess.TimeoutExpired: held.kill(); held.wait(timeout=5)
            result = run([*adb, "shell", "am", "instrument", "-w", "-r", "-e", "phase", "crash-recover",
                          PACKAGE + "/dev.bluewake.android.HDImportInputsTest"])
            (out / "process-death-recover.log").write_text(result)
            if "INSTRUMENTATION_CODE: -1" not in result or "PASS: real process-death input recovery" not in result or "FAIL:" in result:
                raise RuntimeError(result)
            print(result)
        finally:
            run([*adb, "uninstall", PACKAGE])
    (out / "commands.sh").write_text("#!/bin/sh\nset -eu\ncd " + shlex.quote(str(root)) + "\n" + "\n".join(commands) + "\n")
    files = [*sources, source("tests/wwhd_android_inputs/native_probe.cpp"), source("tests/wwhd_android_inputs/AndroidManifest.xml"), Path(__file__),
             source("runtime/host/src/wwhd/disc_image.cpp"), source("runtime/host/src/wwhd/disc_image.h")]
    (out / "proof.json").write_text(json.dumps({"serial": args.serial, "apk": str(apk),
        "repo_root": str(root), "source_overlay": str(overlay) if overlay else None,
        "process_death_executed": args.serial is not None,
        "sha256": {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}, indent=2) + "\n")
    print("Evidence: " + str(out))


if __name__ == "__main__":
    main()
