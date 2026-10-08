#!/usr/bin/env python3
"""The Windows download: the app the builder made, without the disc.

  python scripts/windows/package_release.py VERSION [--app build/windows/BlueWake] [--out build/windows/release]

Writes, in --out:
  BlueWake-vVERSION-windows-x64.zip   the app, from an allowlist (below)
  BlueWake-vVERSION-source.zip        the source of the commit it was built from
  SHA256SUMS                          both zips' digests

The app zip holds BlueWake.exe and the DLLs beside it (gGZLE01_recomp.dll among
them), Aurora's pipeline seed, the DSP files, BuilderProvenance.json, a README
and the licenses. The build folder also holds the disc (game/GZLE01.iso),
files from it and maybe nodtool.exe (it embeds Wii keys): none of them is on
the allowlist, and the script refuses to write the zip if one gets in anyway.
The first launch asks for the player's own .iso or .gcm and prepares it.

Adapted from Elliott Tate's Wind Waker Recomp package_release.py.
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NAME = "BlueWake"
# Never in a download: the disc and anything from it, saves, keys, debug
# databases, optimization profiles, logs and the portable-mode marker.
PRIVATE = re.compile(r"(\.(iso|gcm|rvz|wia|gcz|ciso|nfs|wbfs|dol|rel|arc|card|gci|sav|raw|pdb|profdata|profraw|log)"
                     r"|^nodtool\.exe|^portable\.txt|^sram\.bin|^settings\.ini)$", re.I)
NOT_SHIPPED = {"nodtool.exe", "bluewake_disc_extract.exe"}
DEPS = ROOT / "build/windows/app/_deps"
REQUIRED_LICENSES = {
    "BlueWake-GPL-3.0.txt": ROOT / "LICENSE",
    "RecompCore-COPYING.txt": ROOT / "ref/recompcore/COPYING",
    "Aurora-MIT.txt": ROOT / "ref/recompcore/GXRuntime/graphics/aurora/LICENSE",
    "SDL3-zlib.txt": DEPS / "sdl3_prebuilt-src/licenses/SDL3/LICENSE.txt",
    # The prebuilt Dawn package and DXC's DLLs carry no license text of their own.
    "Dawn-BSD-3-Clause.txt": ROOT / "windows/licenses/Dawn-BSD-3-Clause.txt",
    "DirectXShaderCompiler.txt": ROOT / "windows/licenses/DirectXShaderCompiler-LICENSE.txt",
}
LICENSE_NAME = re.compile(r"^(LICEN[CS]E|COPYING|NOTICE)(\.(txt|md|TXT))?$", re.I)
NOT_SHIPPED_DEPS = {"googletest"}  # tests only

README = """BlueWake {version} for Windows (beta)

The Legend of Zelda: The Wind Waker (GameCube, USA), statically recompiled to
run natively on Windows x64 with Direct3D 12.

You need your own copy of the game: the GameCube USA disc (GZLE01, revision 0)
as an .iso or .gcm disc image. None is included. (A Dolphin .rvz can be
converted to an ISO in Dolphin: right-click the game, Convert File..., ISO.)

Start
  1. Unpack this whole folder anywhere and run BlueWake.exe. BlueWake is not
     signed, so Windows may say it protected your PC: choose "More info",
     then "Run anyway".
  2. The first time, BlueWake asks for your disc image. It checks that it is
     the USA disc, prepares it once and remembers where it is.
  3. Later launches start the game straight away.

Needs Windows 10 or 11 (64-bit), a Direct3D 12 GPU, and a CPU with AVX2
(Intel Haswell, AMD Zen or newer).

Keyboard: W A S D control stick, T F G H C-stick, arrow keys D-pad,
J K U I = A B X Y, E R Q = L R Z, Return START.
Mouse: click the game, then move the mouse to turn the camera (Esc gives the
mouse back). Game controllers work too (Xbox, PlayStation, Switch Pro, ...).
F1 or Esc settings, F11 fullscreen, F10 Smooth Motion (off by default), F9 frame rate.
BlueWake.exe --help lists the command-line options.

Saves, settings and session logs: %APPDATA%\\BlueWake. To keep them beside
BlueWake.exe instead, create an empty file named portable.txt next to it.
Please attach the newest session log (logs\\session-*.log) to a bug report.

Source and documentation: https://github.com/chrissotraidis/bluewake
(docs/WINDOWS.md), and the source zip beside this download. BlueWake's code is
under the GNU GPL, version 3 or later; the licenses of it and of the libraries
it uses are in licenses\\.

BlueWake is an unofficial project, not affiliated with or endorsed by
Nintendo. The Legend of Zelda: The Wind Waker is Nintendo's; play it from a
disc you own.
"""

# What every Windows 10 and 11 PC has; any other DLL a shipped binary imports
# must be in the download (the Visual C++ runtime included).
SYSTEM_DLLS = {
    "kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll", "shell32.dll", "ole32.dll", "oleaut32.dll",
    "imm32.dll", "version.dll", "winmm.dll", "setupapi.dll", "ntdll.dll", "bcrypt.dll", "bcryptprimitives.dll",
    "comctl32.dll", "comdlg32.dll", "dxgi.dll", "d3d12.dll", "d3d11.dll", "dwmapi.dll", "uxtheme.dll", "hid.dll",
    "cfgmgr32.dll", "ws2_32.dll", "crypt32.dll", "psapi.dll", "dbghelp.dll", "shlwapi.dll", "userenv.dll",
    "secur32.dll", "ncrypt.dll", "powrprof.dll", "winhttp.dll", "iphlpapi.dll", "mfplat.dll", "avrt.dll",
    "d3dcompiler_47.dll", "dxcore.dll", "msvcrt.dll", "sechost.dll", "rpcrt4.dll",
}


def pe_imports(path):
    """The DLL names a PE file imports (its import directory)."""
    data = path.read_bytes()
    pe = int.from_bytes(data[0x3C:0x40], "little")
    if data[pe:pe + 4] != b"PE\0\0":
        return []
    sections = int.from_bytes(data[pe + 6:pe + 8], "little")
    optional = pe + 24
    size = int.from_bytes(data[pe + 20:pe + 22], "little")
    magic = int.from_bytes(data[optional:optional + 2], "little")
    directories = optional + (112 if magic == 0x20B else 96)
    import_rva = int.from_bytes(data[directories + 8:directories + 12], "little")
    table = optional + size
    spans = []
    for i in range(sections):
        s = table + 40 * i
        va, raw_size, raw = (int.from_bytes(data[s + o:s + o + 4], "little") for o in (12, 16, 20))
        virtual_size = int.from_bytes(data[s + 8:s + 12], "little")
        spans.append((va, max(virtual_size, raw_size), raw))

    def offset(rva):
        for va, length, raw in spans:
            if va <= rva < va + length:
                return raw + rva - va
        raise ValueError(f"{path.name}: RVA {rva:#x} outside its sections")

    names = []
    if import_rva == 0:
        return names
    entry = offset(import_rva)
    while True:
        name_rva = int.from_bytes(data[entry + 12:entry + 16], "little")
        if name_rva == 0:
            break
        start = offset(name_rva)
        names.append(data[start:data.index(b"\0", start)].decode("ascii"))
        entry += 20
    return names


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def stage_app(app, stage, version):
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)
    for f in sorted(app.iterdir()):
        if f.is_file() and f.name.lower() not in NOT_SHIPPED and (
                f.suffix.lower() in (".exe", ".dll") or f.name in ("initial_pipeline_cache.db",
                                                                  "BuilderProvenance.json")):
            shutil.copy2(f, stage / f.name)
    (stage / "dsp").mkdir()
    for name in ("dsp_rom.bin", "dsp_coef.bin"):
        shutil.copy2(app / "dsp" / name, stage / "dsp" / name)
    licenses = stage / "licenses"
    licenses.mkdir()
    for name, source in REQUIRED_LICENSES.items():
        if not source.is_file():
            sys.exit(f"no license text for {name} ({source})")
        shutil.copy2(source, licenses / name)
    for dep in sorted(DEPS.glob("*-src")) if DEPS.is_dir() else []:
        if dep.name[:-4] in NOT_SHIPPED_DEPS:
            continue
        for f in sorted(dep.iterdir()):
            if f.is_file() and LICENSE_NAME.match(f.name):
                shutil.copy2(f, licenses / f"{dep.name[:-4]}-{f.name}")
    (stage / "README.txt").write_text(README.format(version=version).replace("\n", "\r\n"), newline="")


def check_stage(stage):
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    bad = [p for p in files if PRIVATE.search(p.name) or p.relative_to(stage).parts[0].lower() in ("game", "user")]
    if bad:
        sys.exit("refusing to package: " + ", ".join(str(p.relative_to(stage)) for p in bad))
    shipped = {p.name.lower() for p in stage.iterdir()}
    missing = sorted({f"{name} (for {p.name})" for p in files if p.suffix.lower() in (".exe", ".dll")
                      for name in pe_imports(p)
                      if name.lower() not in shipped and name.lower() not in SYSTEM_DLLS
                      and not name.lower().startswith(("api-ms-win-", "ext-ms-win-"))})
    if missing:
        sys.exit("refusing to package: these DLLs are imported but neither in the download nor part of Windows: "
                 + ", ".join(missing))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", help="the release's version, e.g. 0.5.0")
    parser.add_argument("--app", type=Path, default=ROOT / "build/windows/BlueWake")
    parser.add_argument("--out", type=Path, default=ROOT / "build/windows/release")
    args = parser.parse_args()

    app = args.app
    for need in ("BlueWake.exe", "gGZLE01_recomp.dll", "BuilderProvenance.json", "dsp/dsp_rom.bin"):
        if not (app / need).is_file():
            sys.exit(f"{app / need} is missing: run scripts/windows/build.py first")
    provenance = json.loads((app / "BuilderProvenance.json").read_text())
    if provenance.get("source_modified"):
        sys.exit("the app was built from a checkout with uncommitted changes: commit, rebuild, then package")
    commit = provenance["source_commit"]

    args.out.mkdir(parents=True, exist_ok=True)
    stage = args.out / NAME
    stage_app(app, stage, args.version)
    files = check_stage(stage)

    zip_path = args.out / f"{NAME}-v{args.version}-windows-x64.zip"
    zip_path.unlink(missing_ok=True)
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for p in files:
            z.write(p, f"{NAME}/{p.relative_to(stage).as_posix()}")

    source_path = args.out / f"{NAME}-v{args.version}-source.zip"
    source_path.unlink(missing_ok=True)
    subprocess.run(["git", "-C", ROOT, "archive", "--format=zip", f"--prefix=bluewake-{args.version}/",
                    "-o", source_path, commit], check=True)

    sums = "".join(f"{sha256(p)}  {p.name}\n" for p in (zip_path, source_path))
    (args.out / "SHA256SUMS").write_text(sums, newline="\n")
    total = sum(p.stat().st_size for p in files)
    print(f"{zip_path} ({zip_path.stat().st_size >> 20} MB; {len(files)} files, {total >> 20} MB unpacked)")
    print(f"{source_path} ({source_path.stat().st_size >> 20} MB, commit {commit})")
    print(sums, end="")
    print(f"module {provenance['module_sha256']}")


if __name__ == "__main__":
    main()
