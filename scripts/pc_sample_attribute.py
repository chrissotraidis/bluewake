#!/usr/bin/env python3
"""Attribute BLUEWAKE_PC_SAMPLE counts to tww functions and subsystems.

The sampler (runtime/host/src/main.c) writes "PC COUNT" lines; zeldaret/tww's
config/GZLE01/symbols.txt names the functions. REL code is bucketed by 64 KiB.
Subsystems are grouped by name pattern, so callees are not credited to callers.

Shares per guest thread come from SAMPLES.threads when the host wrote one.

usage: pc_sample_attribute.py SYMBOLS.txt SAMPLES.pcs [top]
"""
import bisect, re, sys
from collections import Counter

SYM = re.compile(r"^(\S+) = \.(\S+):0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")

def load(path):
    funcs = []
    for line in open(path):
        m = SYM.match(line)
        if m:
            name, _sec, addr, size = m.groups()
            funcs.append((int(addr, 16), int(size, 16), name))
    funcs.sort()
    return funcs

# Prefix -> subsystem, first match wins. Mangled C++ names carry the class
# after "__"; plain C names start with the prefix.
GROUPS = [
    ("J3D", r"J3D|calcWeightEnvelope|J3DMtx|J3DShape|J3DMaterial|J3DModel|J3DJoint|J3DAnm|J3DSkin|J3DDeform|J3DSys|J3DTev|J3DTexMtx|J3DDraw"),
    ("matrix/vector (PS*, C_MTX, cMtx, mDoMtx)", r"^PSMTX|^PSVEC|^PSMTX|^C_MTX|^C_VEC|^PSQUAT|^C_QUAT|cMtx_|mDoMtx|cXyz|^PSMtx|^MTX|^VEC"),
    ("save/restore gpr", r"^__(save|restore)_(gpr|fpr)"),
    ("actor search/iter (cTgIt, fpcSch, cNdIt, fopAc)", r"cTgIt|fpcSch|cNdIt|cLsIt|fpcLy|fpcEx_Search|fopAcIt|fpcM_Search|fpcCtIt|fpcLnIt"),
    ("process framework (fpc*, fop*)", r"fpc|fop"),
    ("collision (cBg, dBg, cCc, dCc, cM3d)", r"cBg|dBg|cCc|dCc|cM3d|dBgS|cCcS|dCcS|cM2d"),
    ("particles (JPA, dPa)", r"JPA|dPa_|dPa"),
    ("GX/GD", r"^GX|^GD|^__GX|^__GD"),
    ("JUT/JKR/JSU (system libs)", r"JUT|JKR|JSU|JGeometry|JMath|JMA"),
    ("2D/UI (J2D, dDlst, dMenu)", r"J2D|dDlst|dMenu|dMeter|dFile"),
    ("event/msg (dEv, dMsg)", r"dEv|dMsg|dEvent"),
    ("stage/rooms (dStage, dRes, dComIfG)", r"dStage|dRes|dComIfG|dSv"),
    ("audio (JAI, JAS, mDoAud, Z2)", r"JAI|JAS|JAU|mDoAud|DSP|AI"),
    ("libc/runtime", r"^mem|^str|^__|^sqrt|^sin|^cos|^atan|^fabs|^floor|^ceil|^OS|^DVD|^ARQ|^AR"),
    ("daPy (Link)", r"daPy"),
    ("d_a_* actors (main dol)", r"da[A-Z]|d_a_"),
    ("kankyo/sky/sea (dKy, dScnKy, daSea)", r"dKy|dScnKy|Sea|dSky"),
]
GROUPS = [(g, re.compile(p)) for g, p in GROUPS]

def group(name):
    for g, rx in GROUPS:
        if rx.search(name):
            return g
    return "other"

def print_threads(samples_path, funcs):
    """Shares per guest thread from SAMPLES.threads, named by its OSThread object."""
    try:
        lines = open(samples_path + ".threads").read().split("\n")
    except OSError:
        return
    objects = {}
    for line in open(sys.argv[1]):
        m = re.match(r"^(\S+) = \.(?:s?bss|s?data):0x([0-9A-Fa-f]+); // type:object size:0x318 ", line)
        if m:
            objects[int(m.group(2), 16)] = m.group(1)
    rows = [line.split() for line in lines if line.strip()]
    total = sum(int(n) for _, n in rows)
    print("== by guest thread ==")
    for thread, n in sorted(rows, key=lambda r: -int(r[1])):
        name = objects.get(int(thread, 16), "") if thread != "other" else "(table full)"
        print(f"{100*int(n)/total:6.2f}%  {thread}  {name}")
    print()


def main():
    funcs = load(sys.argv[1])
    print_threads(sys.argv[2], funcs)
    top = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    starts = [f[0] for f in funcs]
    by_fn, by_group, rel = Counter(), Counter(), Counter()
    total = 0
    for line in open(sys.argv[2]):
        pc_s, n_s = line.split()
        pc, n = int(pc_s, 16), int(n_s)
        total += n
        canon = pc & ~0x40000000
        if pc >= 0xC0400000 or not (0x80000000 <= canon < 0x80400000):
            rel[pc & 0xFFFF0000] += n  # REL modules / other: 64 KiB bucket
            by_group["REL modules (d_a_* in RELs)"] += n
            continue
        i = bisect.bisect_right(starts, canon) - 1
        if i >= 0 and canon < funcs[i][0] + funcs[i][1]:
            name = funcs[i][2]
        else:
            name = "<no symbol>"
        by_fn[name] += n
        by_group[group(name)] += n
    print(f"total samples {total}\n")
    print("== by subsystem ==")
    for g, n in by_group.most_common():
        print(f"{100*n/total:6.2f}%  {g}")
    print(f"\n== top {top} functions ==")
    acc = 0
    for name, n in by_fn.most_common(top):
        acc += n
        print(f"{100*n/total:6.2f}%  (cum {100*acc/total:5.1f}%)  {group(name)[:14]:14s}  {name}")
    print("\n== top REL 64KiB buckets ==")
    for b, n in rel.most_common(10):
        print(f"{100*n/total:6.2f}%  {b:08X}")

main()
