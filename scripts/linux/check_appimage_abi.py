#!/usr/bin/env python3
"""Check that a BlueWake AppImage is safe and compatible with SteamOS."""

from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


# SteamOS currently provides glibc, but not necessarily the same libstdc++ as
# the build host. BlueWake therefore caps glibc at the Ubuntu 24.04 build
# baseline and bundles libc++.
GLIBC_VERSION = re.compile(r"\bGLIBC_(\d+(?:\.\d+)*)\b")
GLIBCXX_VERSION = re.compile(r"\bGLIBCXX_(\d+(?:\.\d+)*)\b")

# These files must never enter public CI output. CI supplies an empty synthetic
# module; a real translated module is allowed only in maintainer-owned releases.
PRIVATE_FILE_SUFFIXES = {
    ".iso", ".gcm", ".rvz", ".wbfs", ".wia", ".ciso", ".gcz", ".nfs",
    ".dol", ".rel", ".card", ".gci", ".sav", ".raw",
}

REQUIRED_PAYLOAD = (
    "AppRun",
    "BlueWake.desktop",
    "usr/bin/bluewake",
    "usr/bin/gGZLE01_recomp.so",
)


def parse_version(value):
    """Turn a dotted ABI version into a tuple that compares numerically."""
    return tuple(int(part) for part in value.split("."))


def extract_appimage(image, destination):
    """Extract an AppImage without requiring FUSE on the CI runner."""
    result = subprocess.run(
        [str(image), "--appimage-extract"],
        cwd=destination,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode:
        message = result.stderr.strip() or "unknown extraction error"
        raise RuntimeError(message)
    return Path(destination) / "squashfs-root"


def check_required_payload(root):
    """Return errors for files every BlueWake AppImage must contain."""
    missing = [relative for relative in REQUIRED_PAYLOAD
               if not (root / relative).is_file()]
    if not missing:
        return []
    return ["missing payload: " + ", ".join(missing)]


def check_for_private_files(root):
    """Return errors for disc data, saves, or other prohibited user files."""
    errors = []
    for path in root.rglob("*"):
        if path.is_file() and path.suffix.lower() in PRIVATE_FILE_SUFFIXES:
            errors.append(f"prohibited private file: {path.relative_to(root)}")
    return errors


def elf_files(image, root):
    """Yield the outer AppImage runtime and every ELF in its filesystem."""
    candidates = [(image, "AppImage runtime")]
    candidates.extend(
        (path, str(path.relative_to(root))) for path in root.rglob("*")
    )
    for path, label in candidates:
        if not path.is_file():
            continue
        try:
            with path.open("rb") as stream:
                is_elf = stream.read(4) == b"\x7fELF"
        except OSError as error:
            yield path, label, error
            continue
        if is_elf:
            yield path, label, None


def audit_elf_versions(image, root, readelf, maximum_glibc):
    """Check glibc compatibility and reject accidental libstdc++ linkage."""
    errors = []
    highest_glibc = "0"
    elf_count = 0

    for path, label, open_error in elf_files(image, root):
        if open_error is not None:
            errors.append(f"{label}: {open_error}")
            continue

        elf_count += 1
        result = subprocess.run(
            [readelf, "--version-info", str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if result.returncode:
            errors.append(f"{label}: readelf failed: {result.stderr.strip()}")
            continue

        glibc_versions = GLIBC_VERSION.findall(result.stdout)
        if glibc_versions:
            file_maximum = max(glibc_versions, key=parse_version)
            highest_glibc = max(
                (highest_glibc, file_maximum), key=parse_version
            )
            if parse_version(file_maximum) > parse_version(maximum_glibc):
                errors.append(
                    f"{label} requires GLIBC_{file_maximum} "
                    f"(maximum GLIBC_{maximum_glibc})"
                )

        # A GLIBCXX requirement means something linked against the host's
        # libstdc++. That dependency would vary between Linux distributions.
        glibcxx_versions = GLIBCXX_VERSION.findall(result.stdout)
        if glibcxx_versions:
            required = max(glibcxx_versions, key=parse_version)
            errors.append(
                f"{label} requires GLIBCXX_{required}; "
                "the AppImage must use its bundled LLVM libc++"
            )

    if elf_count == 0:
        errors.append("no ELF payloads found")
    return errors, elf_count, highest_glibc


def main(args):
    if len(args) not in (1, 2):
        print("usage: check_appimage_abi.py IMAGE [MAX_GLIBC]", file=sys.stderr)
        return 2

    image = Path(args[0]).resolve()
    maximum_glibc = args[1] if len(args) == 2 else "2.39"
    if not image.is_file():
        print(f"appimage audit: not a file: {image}", file=sys.stderr)
        return 2

    try:
        parse_version(maximum_glibc)
    except ValueError:
        print(f"appimage audit: invalid GLIBC version: {maximum_glibc}", file=sys.stderr)
        return 2

    readelf = shutil.which("readelf")
    if readelf is None:
        print("appimage audit: readelf is not on PATH", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="bluewake-appimage-audit-") as work:
        try:
            root = extract_appimage(image, work)
        except RuntimeError as error:
            print(f"appimage audit: extraction failed: {error}", file=sys.stderr)
            return 1

        errors = check_required_payload(root)
        errors.extend(check_for_private_files(root))
        abi_errors, elf_count, highest_glibc = audit_elf_versions(
            image, root, readelf, maximum_glibc
        )
        errors.extend(abi_errors)

        if errors:
            for error in errors:
                print(f"appimage audit: {error}", file=sys.stderr)
            return 1

        print(
            f"appimage audit: {elf_count} ELF files; "
            f"maximum GLIBC_{highest_glibc}; no private game files"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
