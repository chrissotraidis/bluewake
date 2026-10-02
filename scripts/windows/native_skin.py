#!/usr/bin/env python3
"""Certify and route J3D skinning functions.

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

# BlueWake pre-transform body, independently compared against the personal
# translated module with every CPU byte and all MEM1 bytes. The donor script
# certified 401b6d74be3c73babb47e9a329fb6173007fe0c907b94780388a463ab09b6013
# after its Windows rewrites; BlueWake certifies before optional transforms.
LEAVES = ((0x802EE67C, 0x802EE874, {"33370ed970c0bfdd1e53b4b63ed3b7c682ed2a7481256047531f14ca3bbe2fc1"}),)

INCLUDE = '#include "native_skin.h"\n'
GENERATED_INCLUDE = '#include "../generated.h"\n'
MARKER = '#define BLUEWAKE_NATIVE_SKIN_PREPARED 1\n'


def hook(start):
    return (f"    /* bluewake: J3D skinning leaf {start:08X} */\n"
            "    if (bluewake_native_skin_try(ctx))\n"
            "        goto return_dispatch_802ED6E0;\n")


def prepare(root):
    paths = sorted(root.rglob('*802ED6E0*.c'))
    if not paths:
        raise ValueError('missing translated skinning chunk 802ED6E0')
    header = root / 'generated.h'
    if not header.is_file():
        raise ValueError('missing generated.h')
    prepared = {}
    for path in paths:
        text = path.read_text()
        if GENERATED_INCLUDE not in text or 'return_dispatch_802ED6E0:' not in text:
            raise ValueError(f'unsupported skinning chunk {path}')
        if text.count(INCLUDE) > 1:
            raise ValueError(f'duplicate skinning include in {path}')
        for start, end, expected in LEAVES:
            begin = text.find(f'\nlabel_{start:08X}:')
            finish = text.find(f'\nlabel_{end:08X}:')
            if begin < 0 or finish <= begin:
                raise ValueError(f'missing skinning function {start:08X} in {path}')
            body = text[begin:finish]
            native = hook(start)
            if 'bluewake_native_skin_' in body:
                if body.count(native) != 1:
                    raise ValueError(f'modified skinning hook {start:08X} in {path}')
                body = body.replace(native, '', 1)
            digest = hashlib.sha256(' '.join(body.split()).encode()).hexdigest()
            if digest not in expected:
                raise ValueError(f'changed skinning function {start:08X} in {path}; native skinning not certified')
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
            temporary.write_text(text, newline='\n')
            temporary.replace(path)
        files[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = root / 'native_skin.json'
    data = json.dumps({'abi': 1, 'files': files}, indent=2) + '\n'
    if not manifest.exists() or manifest.read_text() != data:
        temporary = manifest.with_suffix('.json.tmp')
        temporary.write_text(data, newline='\n')
        temporary.replace(manifest)
    if MARKER not in header.read_text():
        header.write_text(header.read_text() + '\n' + MARKER)
    print(f'native skinning: {len(LEAVES)} J3D skinning functions certified in {len(files)} chunks')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
