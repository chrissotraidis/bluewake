#!/usr/bin/env python3
"""Certified GZLE01 game math entry hooks (native_game_math.c).

  native_game_math.py COMPOSITE_SRC

Run after mod variants, before other native hooks and optional CPU/block/direct
rewrites. All base/mod bodies and nested dependencies must be certified before
any source changes. Missing or changed required entries reject preparation.
Certificates describe this stage; the builder fingerprints the final tree.
The native guard leaves the original translation available on unsupported inputs.
"""
import argparse
import json
import hashlib
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from direct_calls import watched_addresses

MARK = "/* bluewake: certified game math (scripts/windows/native_game_math.py) */\n"
INCLUDE = '#include "../generated.h"\n'
HOOK = re.compile(r"    if \(bluewake_native_game_math_try\(ctx, 0x([0-9A-F]{8})u\)\)\n"
                  r"        goto return_dispatch_([0-9A-F]{8});\n")

# name -> (chunk start, first label, end label or the chunk's return table,
#          SHA256 of the canonical translated fragment).
# Independently compared pre-transform bodies; certify before other native hooks.
FRAGMENTS = {
    'xyz_add': (0x802416E0, 0x80245674, 0x802456C4,
        '9dc5cc389f1ae93525d1efb111503e536a6f9f5a2abd02c175e099f6e5933649'),
    'xyz_sub_entry': (0x802416E0, 0x802456C4, None,
        '1cab4e32368adc98471fcc434e5cd5c9ba3df3624724cb79e502041f06853a0f'),
    'xyz_sub_tail': (0x802456E0, 0x802456E0, 0x80245714,
        '58a1cd6a179ff6257f2426b46a9eca5d6e46e819fbe4a069f132d649bd14ba2b'),
    'xyz_scale': (0x802456E0, 0x80245714, 0x80245760,
        '13ef66b79d8a39a2fc4512ae3ab155e13d82fee1b51d5466c0383f9186964d45'),
    'aab_cyl': (0x802496E0, 0x8024A8E0, 0x8024A988,
        '564cd18f6294430a1dd2d9104299e81590559eb62466b29fecd017a81a3461c0'),
    'xrot_s': (0x800096E0, 0x8000CD28, 0x8000CD88,
        'b4e6e6cc76beed4c6fc92087b557983008376363aaa4d3db46554b90c25e5de9'),
    'yrot_s': (0x800096E0, 0x8000CDC8, 0x8000CE28,
        'aac461900373c8bc91d7c0d8fc0974d23ff1e0eec9933de91e545095c4ad7fcb'),
    'zrot_s': (0x800096E0, 0x8000CE68, 0x8000CEC8,
        '9cd5e46e5f6330c248f90053e5b1be235ce6a0295de4f7bb93250db6e2b767ce'),
    'box_line': (0x802496E0, 0x8024AE3C, 0x8024BA18,
        '4c6393ec874dc07852f9a7de15a0a8143d7a2028351ba6ee5ae7aa22f6eae061'),
    'sphere_clip': (0x802556E0, 0x80256888, 0x802569D0,
        '156ce22cdac43f93f8fa3ce925aee8261210b191e38ffbe74b8c6e776ff58130'),
    'box_clip': (0x802556E0, 0x802569D0, 0x80256CB8,
        '53ca85494d558fda197dc771b989d20739382ed94f53d9de00905510fb14faa4'),
    'clip_zero_loop': (0x802556E0, 'loop_80256A08', None,
        '32ac0f78c9875f02a2087c5143131cc0e35134ce9051ffd429b1bbec05566f4d'),
    'key_s': (0x802ED6E0, 0x802F072C, 0x802F0954,
        '84801732c70ac69c35446f028e8e0921afa9ff7e0ab178a6565e50cced9ea3d9'),
    'hermite_s': (0x802ED6E0, 0x802F06D8, 0x802F072C,
        'a69c6cac2656fbb847d1b6ad99a5205b95fb3e00fb94008bbffdfeae92150a3a'),
    'sdk_add': (0x8030D6E0, 0x8030DCE0, 0x8030DD04,
        '58694b1d26c00948de83054d0bf3be20e59c95bc45172aa813b04f5f45e3799f'),
    'sdk_sub': (0x8030D6E0, 0x8030DD04, 0x8030DD28,
        '9c5e38c526a3a92ef067c8283b8b8c0dfa3a728fd860eacbfb181b9a65871916'),
    'sdk_scale': (0x8030D6E0, 0x8030DD28, 0x8030DD44,
        'c1c5adae1761a7944e710bf410dac0b608e5f9953cfdb65db1acd99353b981f9'),
    'sdk_multvec': (0x8030D6E0, 0x8030DA44, 0x8030DA98,
        'cf4cfa14c036cdcc192f4b3ee9ed8ceda627c576a3a26de1fd922be96b51287e'),
    'transform_simple': (0x802ED6E0, 0x802F0954, 0x802F0E20,
        'b1c6dd06f5b96448eab1e5288a5bc794bb19dc3cd1f8f897f2c192baa7b714b7'),
}
# entry -> required fragments (including all native-emulated calls).
ENTRIES = {
    0x80245674: ('xyz_add', 'sdk_add'),
    0x802456C4: ('xyz_sub_entry', 'xyz_sub_tail', 'sdk_sub'),
    0x80245714: ('xyz_scale', 'sdk_scale'),
    0x8024A8E0: ('aab_cyl',),
    0x8000CD28: ('xrot_s',),
    0x8000CDC8: ('yrot_s',),
    0x8000CE68: ('zrot_s',),
    0x8024AE3C: ('box_line',),
    0x80256888: ('sphere_clip', 'sdk_multvec'),
    0x802569D0: ('box_clip', 'clip_zero_loop', 'sdk_multvec'),
    0x802F072C: ('key_s', 'hermite_s'),
    0x802F0954: ('transform_simple',),
}


def canonical(text):
    text = HOOK.sub("", text)
    text = re.sub(r"^    if \(cycle_block_prepaid\) goto bwfast_\w+;\n", "", text, flags=re.M)
    text = re.sub(r"^(?:bwslow|bwend)_\w+: ;\n", "", text, flags=re.M)
    return " ".join(text.split())


def fragment(text, chunk, start, end):
    if isinstance(start, str):
        # A loop extracted by the translator into a static function. The fast
        # copy lives at the end of that function, after the original body.
        match = re.search(rf"^static void {start}\(CPUState\* ctx(?:_param)?\) \{{", text, re.M)
        if not match:
            return None
        finish = re.search(r"^(?:static )?void \w+\(CPUState\* ctx(?:_param)?\)", text[match.end():], re.M)
        body = text[match.start():match.end() + finish.start()] if finish else text[match.start():]
        fast = body.find("\nbwfast_")
        if fast >= 0:
            body = body[:fast] + "\n}\n"
        return canonical(body)
    begin = text.find(f"\nlabel_{start:08X}:\n")
    finish = text.find(f"\nlabel_{end:08X}:\n", begin) if end else text.find(f"\nreturn_dispatch_{chunk:08X}:\n", begin)
    if begin < 0 or finish <= begin:
        return None
    return canonical(text[begin:finish])


def certify(chunks, watched):
    certified = set()
    for name, (chunk, start, end, expected) in FRAGMENTS.items():
        paths = [p for p in chunks if p.name.endswith(f"_{chunk:08X}.c")]
        valid = bool(paths)
        for path in paths:
            text = path.read_text(encoding="utf-8")
            for existing in HOOK.finditer(text):
                entry = int(existing[1], 16)
                if (not text[:existing.start()].endswith(f'\nlabel_{entry:08X}:\n') or
                        int(existing[2], 16) != chunk):
                    raise ValueError(f'modified game-math hook in {path}')
            body = fragment(text, chunk, start, end)
            if body is None or hashlib.sha256(body.encode()).hexdigest() != expected:
                print(f"{path.name}: unverified {name}; dependent natives disabled")
                valid = False
            # Include the internal labels/returns and callees, in both mirrors.
            if body:
                observed = body
                if name == 'box_line':
                    # Only this fixed call is additionally approved by the
                    # versioned host predicate in native_game_math_try.
                    observed = observed.replace(
                        'ctx->lr = 0x8024AEC8u; ctx->pc = 0x80328F40u;',
                        'ctx->lr = 0x8024AEC8u;')
                names = re.findall(r"label_([8C][0-9A-Fa-f]{7})|// ([8C][0-9A-Fa-f]{7}):|"
                                   r"ctx->(?:pc|lr) = 0x([8C][0-9A-Fa-f]{7})", observed)
                pcs = [int(next(value for value in match if value), 16) for match in names]
                if any(pc in watched or (pc & ~0x40000000) in watched for pc in pcs):
                    print(f"{name}: host watches an internal address; dependent natives disabled")
                    valid = False
        if valid:
            certified.add(name)
    return {entry for entry, names in ENTRIES.items() if set(names) <= certified}


def transform(text, chunk, entries):
    # A rerun after a dependency/mod changes must remove its old hooks too.
    text = HOOK.sub(lambda m: m[0] if int(m[1], 16) in entries else "", text)
    done = 0
    for entry in sorted(entries):
        label = f"\nlabel_{entry:08X}:\n"
        if label not in text:
            continue
        hook = (f"    if (bluewake_native_game_math_try(ctx, 0x{entry:08X}u))\n"
                f"        goto return_dispatch_{chunk:08X};\n")
        if hook in text:
            continue
        if f"\nreturn_dispatch_{chunk:08X}:\n" not in text:
            raise ValueError(f"no return table for {entry:08X}")
        text = text.replace(label, label + hook, 1)
        done += 1
    if done and MARK not in text:
        if INCLUDE not in text:
            raise ValueError("no generated.h include")
        text = text.replace(INCLUDE, INCLUDE + MARK + '#include "native_game_math.h"\n', 1)
    return text, done


def prepare(root):
    header = root / 'generated.h'
    if not header.is_file():
        raise ValueError('missing generated.h')
    chunks = sorted(root.glob("chunks_*/*.c"))
    entries = certify(chunks, watched_addresses())
    if entries != set(ENTRIES):
        raise ValueError('uncertified required game-math entries: ' +
                         ', '.join(f'{entry:08X}' for entry in sorted(set(ENTRIES) - entries)))
    prepared = {}
    hooks = 0
    for path in chunks:
        match = re.search(r"_([0-9A-F]{8})\.c$", path.name)
        if not match:
            continue
        original = path.read_text(encoding="utf-8")
        converted, count = transform(original, int(match[1], 16), entries)
        if MARK in converted:
            prepared[path] = converted
        hooks += count
    # Validate every variant before changing any file.
    files = {}
    for path, converted in prepared.items():
        if path.read_text() != converted:
            temporary = path.with_suffix('.c.tmp')
            temporary.write_text(converted, encoding='utf-8', newline='\n')
            temporary.replace(path)
        files[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    manifest = root / 'native_game_math.json'
    data = json.dumps({'abi': 1, 'entries': sorted(entries), 'files': files}, indent=2) + '\n'
    if not manifest.exists() or manifest.read_text() != data:
        temporary = manifest.with_suffix('.json.tmp')
        temporary.write_text(data, newline='\n'); temporary.replace(manifest)
    marker = '#define BLUEWAKE_NATIVE_GAME_MATH_PREPARED 1\n'
    if marker not in header.read_text():
        header.write_text(header.read_text() + '\n' + marker)
    print(f'native game math: {len(entries)} certified entries, {hooks} new hooks in {len(files)} chunks')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('composite', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.composite)
    except ValueError as error:
        parser.exit(1, f'{error}\n')
