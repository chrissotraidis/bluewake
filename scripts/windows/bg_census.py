#!/usr/bin/env python3
"""Insert the ground-collision census's observer calls (#542 M0, --bg-census).

Diagnostic builds only. At the entry label of each function in FUNCTIONS, in
every chunk variant that has it, insert a call to
bluewake_bg_census_module(ctx, address) (cmake/composite/bg_census_module.c),
which only reads registers. Run after mod variants, before the optional
CPU/block/direct-call rewrites, as native_skin.py is. Repeatable.
"""
import argparse
from pathlib import Path

FUNCTIONS = (0x802468E4, 0x802491F4, 0x800A9684, 0x80248C38)
INCLUDE = '#include "bg_census_module.h"\n'
GENERATED_INCLUDE = '#include "../generated.h"\n'


def hook(address):
    return f"    bluewake_bg_census_module(ctx, 0x{address:08X}u); /* bluewake: --bg-census */\n"


def prepare(root):
    found = {address: 0 for address in FUNCTIONS}
    for path in sorted(root.rglob('chunk*.c')):
        text = path.read_text()
        changed = text
        for address in FUNCTIONS:
            label = f'\nlabel_{address:08X}:\n'
            if label not in changed:
                continue
            found[address] += 1
            if label + hook(address) not in changed:
                changed = changed.replace(label, label + hook(address), 1)
        if changed != text:
            if GENERATED_INCLUDE not in changed:
                raise ValueError(f'{path}: no generated.h include')
            if INCLUDE not in changed:
                changed = changed.replace(GENERATED_INCLUDE, GENERATED_INCLUDE + INCLUDE, 1)
            temporary = path.with_suffix('.c.tmp')
            with temporary.open('w', encoding='utf-8', newline='\n') as stream:
                stream.write(changed)
            temporary.replace(path)
    missing = [f'{a:08X}' for a, n in found.items() if n == 0]
    if missing:
        raise ValueError(f'ground census: no entry label for {", ".join(missing)}')
    print('ground census: observers at ' + ', '.join(f'{a:08X} ({n} chunks)' for a, n in found.items()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
