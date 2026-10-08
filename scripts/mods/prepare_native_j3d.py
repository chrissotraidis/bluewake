#!/usr/bin/env python3
"""Certify and route recovered J3D rotation/translation matrix functions.

Run after mod variants, before optional CPU/block/direct-call rewrites.
The manifest records this certification stage; the builder records the final tree.
Only exact GZLE01 bodies are accepted, including every mod variant. The
unmodified translation remains available whenever the native guard declines.
No game source is distributed by this script; it records body hashes only.
"""
import argparse
import hashlib
import json
from pathlib import Path

def write_lf(path, text):
    """Write text with LF line ends on every platform. Path.write_text's
    newline argument needs Python 3.10, and macOS's own python3 is 3.9."""
    with open(path, "w", newline="\n") as f:
        f.write(text)


LEAVES = (
    (0x802DA64C, 0x802DA724, {
        "d8a04c8f0630a9253afbfd0161ea1580993372eb9dbbea9d220085c2c2fe7e11",  # plain
        "5bc3dd2d5c9e95eaefe30ff3b739f35dac0e71067125f5a1c885c10fad2330a7",  # prepaid copies
    }),
    (0x802DA724, 0x802DA7E4, {
        "364429ec9fb17e6dbf237ca49863689c05e8f6a3fb8e6ec64c72ca2b1224aa70",
        "d2d8396b88997042032c1a5225de66b30a0980d0cc28a207c60f5e7035ed168a",
    }),
)
INCLUDE = '#include "native_j3d.h"\n'
GENERATED_INCLUDE = '#include "../generated.h"\n'
MARKER = '#define BLUEWAKE_NATIVE_J3D_PREPARED 1\n'


def hook(start):
    return (f"    /* bluewake: recovered J3D matrix {start:08X} */\n"
            f"    if (bluewake_native_j3d_try(ctx, 0x{start:08X}u))\n"
            "        goto return_dispatch_802D96E0;\n")


def prepare(root):
    paths = sorted(root.rglob('*802D96E0*.c'))
    if not paths:
        raise ValueError('missing translated J3D chunk 802D96E0')
    header = root / 'generated.h'
    if not header.is_file():
        raise ValueError('missing generated.h')
    prepared = {}
    for path in paths:
        text = path.read_text()
        if GENERATED_INCLUDE not in text or 'return_dispatch_802D96E0:' not in text:
            raise ValueError(f'unsupported J3D chunk {path}')
        if text.count(INCLUDE) > 1:
            raise ValueError(f'duplicate J3D include in {path}')
        for start, end, expected in LEAVES:
            begin = text.find(f'\nlabel_{start:08X}:')
            finish = text.find(f'\nlabel_{end:08X}:')
            if begin < 0 or finish <= begin:
                raise ValueError(f'missing J3D function {start:08X} in {path}')
            body = text[begin:finish]
            native = hook(start)
            if 'bluewake_native_j3d_' in body:
                if body.count(native) != 1:
                    raise ValueError(f'modified J3D hook {start:08X} in {path}')
                body = body.replace(native, '', 1)
            digest = hashlib.sha256(' '.join(body.split()).encode()).hexdigest()
            if digest not in expected:
                raise ValueError(f'changed J3D function {start:08X} in {path}; native J3D not certified')
            if native not in text[begin:finish]:
                label = f'\nlabel_{start:08X}:\n'
                text = text.replace(label, label + native, 1)
        if INCLUDE not in text:
            text = text.replace(GENERATED_INCLUDE, GENERATED_INCLUDE + INCLUDE, 1)
        prepared[path] = text
    # Validate all variants before mutating any source or its manifest.
    files = {}
    for path, text in prepared.items():
        if path.read_text() != text:
            temporary = path.with_suffix('.c.tmp')
            write_lf(temporary, text)
            temporary.replace(path)
        files[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = root / 'native_j3d.json'
    data = json.dumps({'abi': 1, 'files': files}, indent=2) + '\n'
    if not manifest.exists() or manifest.read_text() != data:
        temporary = manifest.with_suffix('.json.tmp')
        write_lf(temporary, data)
        temporary.replace(manifest)
    if MARKER not in header.read_text():
        header.write_text(header.read_text() + '\n' + MARKER)
    print(f'native J3D: {len(LEAVES)} recovered matrix functions certified in {len(files)} chunks')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
