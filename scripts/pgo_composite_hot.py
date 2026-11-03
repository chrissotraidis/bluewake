#!/usr/bin/env python3
"""Profile-guided build of the composite's hottest chunks.

usage: pgo_composite_hot.py gen|use HOTLIST OUTDIR [PROFDATA]
  gen: compile HOTLIST chunks with -fprofile-instr-generate into OUTDIR/gen and
       link OUTDIR/gen/gGZLE01_recomp.dylib (other objects unchanged).
  use: compile them with -fprofile-instr-use=PROFDATA into OUTDIR/use and link
       OUTDIR/use/gGZLE01_recomp.dylib.
Reads compile_commands.json and link.txt from the composite build tree.
EXTRA_CFLAGS adds compiler flags to both passes.
"""
import json, os, re, shlex, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

BUILD = os.environ.get('BLUEWAKE_COMPOSITE_BUILD', 'build/composite-cycle-hybrid-o2-v2')
mode, hotlist, outdir = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
prof = os.path.abspath(sys.argv[4]) if mode == 'use' else None
hot = [l.strip() for l in open(hotlist) if l.strip()]
cc = {e['file']: e for e in json.load(open(os.path.join(BUILD, 'compile_commands.json')))}
dest = os.path.join(outdir, mode)
os.makedirs(dest, exist_ok=True)
flag = '-fprofile-instr-generate' if mode == 'gen' else f'-fprofile-instr-use={prof} -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date'
flag += ' ' + os.environ.get('EXTRA_CFLAGS', '')  # e.g. -mcpu=apple-m1
replaced = {}

relink_only = os.environ.get('RELINK_ONLY') == '1'

def build(f):
    e = cc[f]
    m = re.search(r' -o (\S+)', e['command'])
    orig_obj = m.group(1)
    obj = os.path.join(dest, os.path.basename(f) + '.o')
    if relink_only and os.path.exists(obj):
        return f, (orig_obj, obj), None
    cmd = e['command'].replace(f' -o {orig_obj}', f' -o {obj}', 1) + ' ' + flag
    r = subprocess.run(cmd, shell=True, cwd=e['directory'], capture_output=True, text=True)
    if r.returncode != 0:
        return f, None, r.stderr[-2000:]
    return f, (orig_obj, obj), None

with ThreadPoolExecutor(int(os.environ.get('JOBS', '8'))) as ex:
    for i, (f, pair, err) in enumerate(ex.map(build, hot), 1):
        if err:
            sys.exit(f'compile failed: {f}\n{err}')
        replaced[pair[0]] = pair[1]
        print(f'[{i}/{len(hot)}] {os.path.basename(f)}', flush=True)

link = open(os.path.join(BUILD, 'CMakeFiles/gGZLE01_recomp.dir/link.txt')).read().strip()
for orig, new in replaced.items():
    for form in (f'"{orig}"', orig):
        if f' {form} ' in link or link.endswith(' ' + form):
            link = link.replace(f' {form}', f' "{new}"', 1)
            break
    else:
        sys.exit(f'object not in link line: {orig}')
out = os.path.join(dest, 'gGZLE01_recomp.dylib')
link = re.sub(r' -o gGZLE01_recomp\.dylib ', f' -o {out} ', link, count=1)
if mode == 'gen':
    link += ' -fprofile-instr-generate'
subprocess.run(link, shell=True, cwd=BUILD, check=True)
print('linked', out)
