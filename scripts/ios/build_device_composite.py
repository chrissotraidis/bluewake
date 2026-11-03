#!/usr/bin/env python3
"""Build the translated composite for iOS devices.

Superseded on 2026-09-25 by scripts/ios/build_device.sh, which compiles the composite for iOS
directly from the generated source in a fresh checkout. This script is kept for the development
Mac: it retargets an existing macOS composite build tree (its compile_commands.json) and picks up
the local PGO profiles when they exist.

The simulator runs the macOS composite retagged for iossim, which carries
Apple M1 code generation. A device build has to target the oldest supported
iPad CPU instead (A13, iOS 17), so this recompiles every file of the composite
build tree for arm64-apple-ios17.0 with -mcpu=apple-a13 against the iPhoneOS
SDK, applies the same profile-guided optimization as the macOS composite (the
hot chunks and the runtime/dispatch files each with their own profile), and
links a device dylib with the tree's recorded link line, retargeted.

usage: scripts/ios/build_device_composite.py [OUTDIR]   (default build/ios-device-composite)
Environment: BLUEWAKE_COMPOSITE_BUILD (default build/composite-cycle-hybrid-o2-v2),
             BLUEWAKE_DEVICE_CPU (default apple-a13), JOBS (default cores - 2)
"""
import json
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
os.chdir(root)
build = os.environ.get("BLUEWAKE_COMPOSITE_BUILD", "build/composite-cycle-hybrid-o2-v2")
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "build/ios-device-composite")
cpu = os.environ.get("BLUEWAKE_DEVICE_CPU", "apple-a13")
jobs = int(os.environ.get("JOBS", max(1, (os.cpu_count() or 4) - 2)))
os.makedirs(os.path.join(out, "obj"), exist_ok=True)

sdk = subprocess.run(["xcrun", "--sdk", "iphoneos", "--show-sdk-path"],
                     capture_output=True, text=True, check=True).stdout.strip()
target = ["-target", "arm64-apple-ios17.0", "-mcpu=" + cpu]

# Profiles, by the object each one was trained for (pgo_composite_hot.py).
profiles = {}
for pgo in ("build/composite-pgo", "build/composite-pgo-rt"):
    prof = os.path.abspath(os.path.join(pgo, "prof", "merged.profdata"))
    use = os.path.join(pgo, "use")
    if os.path.exists(prof) and os.path.isdir(use):
        for obj in os.listdir(use):
            if obj.endswith(".c.o"):
                profiles[obj] = prof

entries = json.load(open(os.path.join(build, "compile_commands.json")))


def retarget(tokens):
    res, skip = [], False
    for i, tok in enumerate(tokens):
        if skip:
            skip = False
            continue
        if tok in ("-arch", "-isysroot"):
            skip = True
            continue
        if tok.startswith("-mmacosx-version-min") or tok.startswith("-mcpu="):
            continue
        res.append(tok)
    return res[:1] + target + ["-isysroot", sdk] + res[1:]


def compile_one(e):
    tokens = shlex.split(e["command"])
    o = tokens.index("-o")
    orig = tokens[o + 1]
    name = os.path.basename(orig)
    obj = os.path.join(out, "obj", name)
    tokens[o + 1] = obj
    tokens = retarget(tokens)
    if name in profiles:
        tokens += ["-fprofile-instr-use=" + profiles[name], "-Wno-profile-instr-unprofiled",
                   "-Wno-profile-instr-out-of-date", "-Wno-backend-plugin"]
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(e["file"]):
        return orig, obj, None
    r = subprocess.run(["nice", "-n", "10"] + tokens, cwd=e["directory"], capture_output=True, text=True)
    return orig, obj, (r.stderr[-1500:] if r.returncode else None)


objmap, failures = {}, []
with ThreadPoolExecutor(jobs) as pool:
    for n, (orig, obj, err) in enumerate(pool.map(compile_one, entries), 1):
        if err:
            failures.append((orig, err))
        objmap[os.path.basename(orig)] = obj
        if n % 50 == 0:
            print("device-composite: %d/%d compiled" % (n, len(entries)), flush=True)
if failures:
    for orig, err in failures[:3]:
        print("device-composite: FAILED %s\n%s" % (orig, err))
    sys.exit(1)

link = shlex.split(open(os.path.join(build, "CMakeFiles/gGZLE01_recomp.dir/link.txt")).read())
dylib = os.path.join(out, "gGZLE01_recomp.dylib")
res, skip = [], False
for tok in link:
    if skip:
        skip = False
        continue
    if tok in ("-arch", "-isysroot"):
        skip = True
        continue
    if tok == "-o":
        res += ["-o", dylib]
        skip = True
        continue
    base = os.path.basename(tok)
    res.append(objmap.get(base, tok) if tok.endswith(".o") else tok)
res = res[:1] + target + ["-isysroot", sdk] + res[1:]
subprocess.run(res, cwd=build, check=True)
print("device-composite: %s (%d files, %d with PGO, cpu %s)" % (dylib, len(entries), len(profiles), cpu))
