#!/usr/bin/env python3
"""Assemble a local Mac app without touching installed apps or player storage."""
import argparse
import hashlib
import json
import plistlib
from pathlib import Path
import shutil
import subprocess
import tempfile


def sha(path):
    with path.open("rb") as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
        return digest.hexdigest()


def assemble(args):
    personal = args.module is not None
    if personal != (args.game is not None) or personal != (args.disc is not None):
        raise ValueError("--module, --game and --disc must be supplied together")
    executable = args.app / "Contents/MacOS/BlueWake"
    if not executable.is_file():
        raise ValueError("the source app has no BlueWake executable")
    if (args.app / "Contents/Resources/Game").exists() or (args.app / "Contents/Frameworks/gGZLE01_recomp.dylib").exists():
        raise ValueError("the source app already contains personal game inputs")
    # A player app must not depend on libraries in a developer checkout/Homebrew.
    linked = subprocess.check_output(["otool", "-L", executable], text=True)
    for line in linked.splitlines()[1:]:
        library = line.strip().split(" (", 1)[0]
        if not library.startswith(("/System/Library/", "/usr/lib/")):
            raise ValueError(f"host dependency is not self-contained: {library}")
    if personal:
        if not args.module.is_file() or not args.disc.is_file() or not (args.game / "main.dol").is_file():
            raise ValueError("missing personal module, disc or extracted executable")
        if len(list((args.game / "rels").glob("*.rel"))) != 415:
            raise ValueError("expected all 415 extracted RELs")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix="mac-stage-", dir=args.output.parent))
    app = stage / "BlueWake.app"
    shutil.copytree(args.app, app)
    resources = app / "Contents/Resources"
    info_path = app / "Contents/Info.plist"
    info = plistlib.loads(info_path.read_bytes())
    info["LSMinimumSystemVersion"] = "14.0"
    info_path.write_bytes(plistlib.dumps(info))
    provenance = {
        "platform": "macos", "source_commit": args.source_commit,
        "source_modified": args.source_modified,
        "runtime_commit": subprocess.check_output(["git", "-C", args.runtime, "rev-parse", "HEAD"], text=True).strip(),
        "translator_commit": subprocess.check_output(["git", "-C", args.runtime / "DolRecomp", "rev-parse", "HEAD"], text=True).strip(),
        "containsTranslatedGameCode": personal,
    }
    if personal:
        game = resources / "Game"
        game.mkdir()
        shutil.copy2(args.game / "main.dol", game / "main.dol")
        shutil.copytree(args.game / "rels", game / "rels")
        shutil.copy2(args.disc, game / "GZLE01.iso")
        frameworks = app / "Contents/Frameworks"
        frameworks.mkdir(exist_ok=True)
        module = frameworks / "gGZLE01_recomp.dylib"
        shutil.copy2(args.module, module)
        subprocess.run(["codesign", "--force", "--sign", args.identity, module], check=True)
        provenance.update(module_sha256=sha(module), disc_sha256=sha(game / "GZLE01.iso"))
        if provenance["disc_sha256"] != sha(args.disc):
            raise ValueError("disc copy failed verification")
    # These runtime resources are versioned with RecompCore, not private caches.
    dsp = resources / "DSP"
    dsp.mkdir(exist_ok=True)
    for name in ("dsp_rom.bin", "dsp_coef.bin"):
        shutil.copy2(args.runtime / "Data/Sys/GC" / name, dsp / name)
    (resources / "BuilderProvenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    subprocess.run(["codesign", "--force", "--sign", args.identity, app], check=True)
    subprocess.run(["codesign", "--verify", "--deep", "--strict", app], check=True)
    # Only replace the builder's output after the complete candidate verifies.
    # Retain the old app so interruptions or changed inputs never discard it.
    if args.output.exists():
        previous = Path(tempfile.mkdtemp(prefix="mac-previous-", dir=args.output.parent))
        args.output.rename(previous / args.output.name)
        print(f"Previous app preserved: {previous / args.output.name}")
    app.rename(args.output)
    stage.rmdir()
    print(f"{'Personal' if personal else 'App-only'} Mac app: {args.output}")
    return provenance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("app", "output", "runtime"):
        parser.add_argument("--" + name, required=True, type=Path)
    for name in ("module", "game", "disc"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--source-modified", action="store_true")
    parser.add_argument("--identity", default="-")
    args = parser.parse_args()
    if args.output.suffix != ".app" or args.output.resolve() == args.app.resolve():
        parser.error("--output must name a separate .app")
    try:
        assemble(args)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"package-macos: {error}\n")


if __name__ == "__main__":
    main()
