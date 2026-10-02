#!/usr/bin/env python3
"""Certify and route SDK vector functions.

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

# Fixed donor body hashes retained from scripts/windows/direct_calls.py.
LEAVES = (
    (0x8030DCE0, 0x8030DD04, {"58694b1d26c00948de83054d0bf3be20e59c95bc45172aa813b04f5f45e3799f"}),
    (0x8030DD04, 0x8030DD28, {"9c5e38c526a3a92ef067c8283b8b8c0dfa3a728fd860eacbfb181b9a65871916"}),
    (0x8030DD28, 0x8030DD44, {"c1c5adae1761a7944e710bf410dac0b608e5f9953cfdb65db1acd99353b981f9"}),
    (0x8030DE50, 0x8030DE68, {"0a5077731582f5f871952bee6ba70d1ac05eeb54feb0c8a498eb307774cd7ea8"}),
    (0x8030DEAC, 0x8030DECC, {"62a35df011a51e2aef7a0db1bfa512390017e42567a52275ea91e01c3398607b"}),
    (0x8030DECC, 0x8030DF08, {"a1eedeef0d07eb445d5d0b52313325a54945cb299e4abc81b058c2107676fe04"}),
    (0x8030E0B4, 0x8030E0DC, {"6f48e68ae5da2ea1221917e6486557c9016dc527a74d3e503dce9306b313f9fb"}),
    (0x8030DE0C, 0x8030DE50, {"3c64781d1609e952cae0ef86ac223dbc9847175b09b00487805b8fb1c3bab731"}),
    (0x8030DE68, 0x8030DEAC, {"ed8be5c3e04ec0bb6b8b8af13095e391acb4291521073be27f3c74ec63909313"}),
)

INCLUDE = '#include "native_vec.h"\n'
GENERATED_INCLUDE = '#include "../generated.h"\n'
MARKER = '#define BLUEWAKE_NATIVE_VEC_PREPARED 1\n'


def hook(start):
    return (f"    /* bluewake: SDK vector leaf {start:08X} */\n"
            f"    if (bluewake_native_vec_try(ctx, 0x{start:08X}u))\n"
            "        goto return_dispatch_8030D6E0;\n")


def prepare(root):
    paths = sorted(root.rglob('*8030D6E0*.c'))
    if not paths:
        raise ValueError('missing translated vector chunk 8030D6E0')
    header = root / 'generated.h'
    if not header.is_file():
        raise ValueError('missing generated.h')
    prepared = {}
    for path in paths:
        text = path.read_text()
        if GENERATED_INCLUDE not in text or 'return_dispatch_8030D6E0:' not in text:
            raise ValueError(f'unsupported vector chunk {path}')
        if text.count(INCLUDE) > 1:
            raise ValueError(f'duplicate vector include in {path}')
        for start, end, expected in LEAVES:
            begin = text.find(f'\nlabel_{start:08X}:')
            finish = text.find(f'\nlabel_{end:08X}:')
            if begin < 0 or finish <= begin:
                raise ValueError(f'missing vector function {start:08X} in {path}')
            body = text[begin:finish]
            native = hook(start)
            if 'bluewake_native_vec_' in body:
                if body.count(native) != 1:
                    raise ValueError(f'modified vector hook {start:08X} in {path}')
                body = body.replace(native, '', 1)
            digest = hashlib.sha256(' '.join(body.split()).encode()).hexdigest()
            if digest not in expected:
                raise ValueError(f'changed vector function {start:08X} in {path}; native vector not certified')
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
    manifest = root / 'native_vec.json'
    data = json.dumps({'abi': 1, 'files': files}, indent=2) + '\n'
    if not manifest.exists() or manifest.read_text() != data:
        temporary = manifest.with_suffix('.json.tmp')
        temporary.write_text(data, newline='\n')
        temporary.replace(manifest)
    if MARKER not in header.read_text():
        header.write_text(header.read_text() + '\n' + MARKER)
    print(f'native vector: {len(LEAVES)} SDK vector functions certified in {len(files)} chunks')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
