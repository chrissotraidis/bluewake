#!/usr/bin/env python3
"""Certify the exact translated SDK bodies accepted by native_math.c.

Only hashes are distributed. Inputs come from the player's own disc. Check
all variants before writing the manifest; even a body edit that retains the
instruction comments must invalidate this optimization.
"""
import argparse
import hashlib
import json
from pathlib import Path

LEAVES = (
    (0x8030D0C8, 0x8030D0FC, '803096E0', '8d89ffc5251b41bc39532c918e0855ceceab319616a5c0ef12acfb9635d9b55d'),
    (0x8030D0FC, 0x8030D1C8, '803096E0', '8ab502958cfcc5e434d35b0848d89ae59a9af3b00b160835ec3dde947709f1ea'),
    (0x8030DA44, 0x8030DA98, '8030D6E0', 'cf4cfa14c036cdcc192f4b3ee9ed8ceda627c576a3a26de1fd922be96b51287e'),
    (0x8030DA98, 0x8030DB24, '8030D6E0', 'ecba4bf605b5dfa693e2a14525f36a68a8a373881105bab2872d7118753111cd'),
)


DECLARATION = """// BlueWake native math entries are resolved only on a PC-cache miss.
#define BLUEWAKE_NATIVE_MATH_CACHED 1
static DolRecompFunction bluewake_native_math_find(u32 address);
"""
LOOKUP = """    DolRecompFunction native = bluewake_native_math_find(address);
    if (native) {
        s_cached_pc[cache_index] = address;
        s_cached_pc_fn[cache_index] = native;
        return native;
    }
"""
TYPE = 'typedef void (*DolRecompFunction)(CPUState* ctx);\n'
CACHE = """    if (s_cached_pc[cache_index] == address)
        return s_cached_pc_fn[cache_index];
"""


def prepare(root):
    files = {}
    for start, end, chunk, expected in LEAVES:
        matches = sorted(root.rglob(f'*{chunk}*.c'))
        if not matches:
            raise ValueError(f'missing translated SDK chunk {chunk}')
        for path in matches:
            source = path.read_text()
            begin = source.find(f'\nlabel_{start:08X}:')
            finish = source.find(f'\nlabel_{end:08X}:')
            if begin < 0 or finish <= begin:
                raise ValueError(f'missing SDK function {start:08X} in {path}')
            body = ' '.join(source[begin:finish].split())
            if hashlib.sha256(body.encode()).hexdigest() != expected:
                raise ValueError(f'changed SDK function {start:08X} in {path}; native math not certified')
            files[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    header = root / 'generated_composite.h'
    source = header.read_text()
    if 'BLUEWAKE_NATIVE_MATH_CACHED' in source:
        if source.count(DECLARATION) != 1 or source.count(LOOKUP) != 1:
            raise ValueError('modified native math dispatch hook')
    else:
        if source.count(TYPE) != 1 or source.count(CACHE) != 1:
            raise ValueError('unsupported composite dispatcher')
        source = source.replace(TYPE, TYPE + DECLARATION)
        source = source.replace(CACHE, CACHE + LOOKUP)
    # Finish validation before changing the header or its certification.
    header.write_text(source)
    files[header.name] = hashlib.sha256(header.read_bytes()).hexdigest()
    manifest = root / 'native_math.json'
    temporary = manifest.with_suffix('.json.tmp')
    temporary.write_text(json.dumps({'abi': 1, 'files': files}, indent=2) + '\n')
    temporary.replace(manifest)
    print(f'native math: {len(LEAVES)} SDK leaves verified across {len(files)} source files')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
