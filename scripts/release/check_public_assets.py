#!/usr/bin/env python3
"""Validate source archives, normalize members to ZIP, then run the release gate.

Archive contents are streamed into a temporary ZIP, never extracted onto disk.
Only regular files and directories are supported; links and ambiguous paths fail.
"""
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile

FORBIDDEN = re.compile(
    r"(?:^|/)(?:gGZLE01_recomp\.dylib|[^/]*\.(?:iso|gcm|rvz|wbfs|wia|ciso|gcz|nfs|dol|rel|"
    r"card|gci|sav|raw|p12|mobileprovision|provisionprofile|profraw))$", re.I)


def check_name(name, seen, directory=False):
    if '\\' in name or name.startswith('/'):
        raise ValueError(f'unsafe archive path: {name!r}')
    clean = name[:-1] if directory and name.endswith('/') else name
    parts = clean.split('/')
    if any(p in ('', '.', '..') for p in parts) or ':' in parts[0]:
        raise ValueError(f'unsafe archive path: {name!r}')
    if clean in seen:
        raise ValueError(f'duplicate archive path: {name!r}')
    seen.add(clean)
    if FORBIDDEN.search(clean):
        raise ValueError(f'prohibited private file: {name}')
    return clean


def copy_member(src, dst, name):
    # The gate does not apply the filename rules recursively. Source release
    # archives need no nested containers; refuse them rather than hide inputs.
    prefix = src.read(512)
    lower = name.lower()
    if (lower.endswith(('.zip', '.ipa', '.apk', '.aab', '.tar', '.tgz', '.gz', '.bz2', '.xz', '.7z', '.rar')) or
            prefix.startswith((b'PK\x03\x04', b'PK\x05\x06', b'\x1f\x8b', b'BZh', b'\xfd7zXZ\x00', b'7z\xbc\xaf\x27\x1c', b'Rar!')) or
            prefix[257:262] == b'ustar'):
        raise ValueError(f'nested archive unsupported; flatten source contents first: {name}')
    dst.write(prefix)
    shutil.copyfileobj(src, dst, length=1024 * 1024)


def normalize(source, target, kind):
    seen = set()
    with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_DEFLATED) as out:
        if kind == 'zip':
            with zipfile.ZipFile(source) as archive:
                for member in archive.infolist():
                    mode = stat.S_IFMT(member.external_attr >> 16)
                    directory = member.is_dir()
                    if (mode not in (0, stat.S_IFREG, stat.S_IFDIR) or
                            (mode == stat.S_IFDIR and not directory) or
                            (directory and (mode == stat.S_IFREG or member.file_size != 0))):
                        raise ValueError(f'nonregular ZIP member: {member.filename}')
                    name = check_name(member.filename, seen, directory)
                    if directory:
                        continue
                    with archive.open(member) as src, out.open(name, 'w', force_zip64=True) as dst:
                        copy_member(src, dst, name)
        else:
            with tarfile.open(source, mode='r:*') as archive:
                for member in archive:
                    if not (member.isfile() or member.isdir()):
                        raise ValueError(f'nonregular TAR member: {member.name}')
                    name = check_name(member.name, seen, member.isdir())
                    if member.isdir():
                        continue
                    with archive.extractfile(member) as src, out.open(name, 'w', force_zip64=True) as dst:
                        copy_member(src, dst, name)


def archive_kind(source):
    # Renaming a container must not turn it into an opaque binary to the gate.
    with source.open('rb') as stream:
        prefix = stream.read(512)
    lower = source.name.lower()
    if (lower.endswith(('.zip', '.ipa', '.apk', '.aab')) or
            prefix.startswith((b'PK\x03\x04', b'PK\x05\x06')) or zipfile.is_zipfile(source)):
        return 'zip'
    if (lower.endswith(('.tar', '.tar.gz', '.tgz', '.tar.bz2', '.tbz2', '.tar.xz', '.txz')) or
            prefix[257:262] == b'ustar' or tarfile.is_tarfile(source)):
        return 'tar'
    if (lower.endswith(('.gz', '.bz2', '.xz', '.zst', '.7z', '.rar')) or
            prefix.startswith((b'\x1f\x8b', b'BZh', b'\xfd7zXZ\x00', b'\x28\xb5\x2f\xfd',
                               b'7z\xbc\xaf\x27\x1c', b'Rar!'))):
        raise ValueError('unsupported or malformed compressed input; provide a source ZIP or TAR')
    return None


def main(args):
    gate = Path(os.environ.get('BLUEWAKE_RELEASE_GATE', str(Path.home() / '.codex/release-gate/release_gate.py')))
    if not args:
        print('usage: check_public_assets.sh FILE...', file=sys.stderr)
        return 2
    if not gate.is_file():
        print(f'FAIL: release gate not found at {gate}; nothing may be published', file=sys.stderr)
        return 1
    failed = False
    for value in args:
        source = Path(value)
        try:
            if not source.is_file():
                raise ValueError('not a file')
            if FORBIDDEN.search(source.name):
                raise ValueError('prohibited private filename')
            kind = archive_kind(source)
            with tempfile.TemporaryDirectory(prefix='bluewake-release-check-') as work:
                inspected = source
                if kind:
                    inspected = Path(work) / 'source.zip'
                    normalize(source, inspected, kind)
                # The existing gate skips ZIP members below 64 bytes. Scan those
                # directly too, preserving extensions so tiny JSON provenance
                # and key-containing files receive the same content checks.
                small = []
                if kind:
                    with zipfile.ZipFile(inspected) as archive:
                        for index, member in enumerate(archive.infolist()):
                            if member.file_size < 64:
                                tiny = Path(work) / f'small-{index}{Path(member.filename).suffix}'
                                tiny.write_bytes(archive.read(member))
                                small.append(str(tiny))
                result = subprocess.run([sys.executable, str(gate), str(inspected), *small])
                if result.returncode:
                    raise ValueError(f'content gate failed (exit {result.returncode})')
            print(f'PASS {source}')
        except (OSError, EOFError, ValueError, RuntimeError, tarfile.TarError, zipfile.BadZipFile, NotImplementedError) as error:
            print(f'FAIL {source}: {error}', file=sys.stderr)
            failed = True
    print('check_public_assets: FAILED; do not publish' if failed else
          f'check_public_assets: all {len(args)} file(s) passed', file=sys.stderr if failed else sys.stdout)
    return int(failed)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
