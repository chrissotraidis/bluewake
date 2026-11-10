#!/usr/bin/env python3
"""Route cBgW::GroundCrossGrpRp (0x802491F4) to its native walk (#592).

Run with the other native preparations, before the CPU/block/direct-call
rewrites. The function is certified by its guest instructions (the
translation's `// ADDRESS: instruction` comments, which the later rewrites
keep): a different translation is left alone and the build fails. At run
time the native is off unless BLUEWAKE_NATIVE_GROUND_WALK=1, and it declines
to the unmodified translation whenever its checks don't hold.
No game source is distributed by this script; it records hashes only.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

ENTRY, END = 0x802491F4, 0x80249368
CHUNK = '802456E0'
# The 93 guest instructions of GZLE01's GroundCrossGrpRp, as DolRecomp prints them.
EXPECTED = 'a9b0d2df2978338a8646a1872c7befcc5f15147c819655bf31c265c9efb2109d'
INCLUDE = '#include "native_ground_walk.h"\n'
GENERATED_INCLUDE = '#include "../generated.h"\n'
MARKER = '#define BLUEWAKE_NATIVE_GROUND_WALK_PREPARED 1\n'
HOOK = (f"    /* bluewake: native ground walk {ENTRY:08X} (opt-in, #592) */\n"
        "    if (bluewake_native_ground_walk_try(ctx)) {\n"
        "        ctx->pc = ctx->lr & ~3u;\n"
        f"        goto return_dispatch_{CHUNK};\n"
        "    }\n")


def instructions(body):
    lines = [l.strip() for l in body.splitlines() if re.match(r'\s*// [0-9A-F]{8}: ', l)]
    return hashlib.sha256('\n'.join(lines).encode()).hexdigest()


def prepare(root):
    paths = sorted(root.rglob(f'*{CHUNK}*.c'))
    if not paths:
        raise ValueError(f'missing translated collision chunk {CHUNK}')
    header = root / 'generated.h'
    if not header.is_file():
        raise ValueError('missing generated.h')
    prepared = {}
    for path in paths:
        text = path.read_text()
        if GENERATED_INCLUDE not in text or f'\nreturn_dispatch_{CHUNK}:' not in text:
            raise ValueError(f'unsupported collision chunk {path}')
        begin = text.find(f'\nlabel_{ENTRY:08X}:')
        finish = text.find(f'\nlabel_{END:08X}:')
        if begin < 0 or finish <= begin:
            raise ValueError(f'missing GroundCrossGrpRp in {path}')
        body = text[begin:finish]
        if instructions(body) != EXPECTED:
            raise ValueError(f'changed GroundCrossGrpRp in {path}; the native ground walk is not certified')
        if HOOK not in body:
            if 'bluewake_native_ground_walk_try' in body:
                raise ValueError(f'modified ground walk hook in {path}')
            label = f'\nlabel_{ENTRY:08X}:\n'
            text = text.replace(label, label + HOOK, 1)
        if INCLUDE not in text:
            text = text.replace(GENERATED_INCLUDE, GENERATED_INCLUDE + INCLUDE, 1)
        prepared[path] = text
    files = {}
    for path, text in prepared.items():
        if path.read_text() != text:
            temporary = path.with_suffix('.c.tmp')
            with temporary.open('w', encoding='utf-8', newline='\n') as stream:
                stream.write(text)
            temporary.replace(path)
        files[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = root / 'native_ground_walk.json'
    data = json.dumps({'abi': 1, 'files': files}, indent=2) + '\n'
    if not manifest.exists() or manifest.read_text() != data:
        manifest.write_text(data)
    if MARKER not in header.read_text():
        header.write_text(header.read_text() + '\n' + MARKER)
    print(f'native ground walk: GroundCrossGrpRp certified and routed in {len(files)} chunks')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        raise SystemExit(f'native_ground_walk.py: {error}')
