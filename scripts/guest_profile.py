"""Game-thread samples -> guest functions.
guest_profile.py PERF.data TID MODULE.so SYMBOLS.txt [--top N]
Each sample in the translated module is placed in its func_ chunk, then at the guest
instruction whose label (the chunk's pc_table entry) is the nearest at or before it,
then in the decompilation's function containing that guest address."""
import bisect, collections, re, subprocess, sys
perf, tid, mod, syms = sys.argv[1:5]
top = int(sys.argv[sys.argv.index("--top") + 1]) if "--top" in sys.argv else 40
# load bias from the mmap event
mm = subprocess.run(["perf", "script", "-i", perf, "--tid", tid, "--show-mmap-events", "-F", "ip"],
                    capture_output=True, text=True).stdout
m = re.search(r"\[0x([0-9a-f]+)\(0x[0-9a-f]+\) @ 0x([0-9a-f]+) .*gGZLE01_recomp\.so", mm)
start, off = int(m.group(1), 16), int(m.group(2), 16)
# file offset -> vaddr for the text segment (LOAD R E)
segs = subprocess.run(["readelf", "-lW", mod], capture_output=True, text=True).stdout
for line in segs.splitlines():
    f = line.split()
    if f and f[0] == "LOAD" and "E" in f[6:8]:
        seg_off, seg_vaddr = int(f[1], 16), int(f[2], 16)
bias = seg_vaddr - seg_off
ips = subprocess.run(["perf", "script", "-i", perf, "--tid", tid, "-F", "ip,dso"], capture_output=True, text=True).stdout
total = 0; host = collections.Counter(); inmod = []
for line in ips.splitlines():
    p = line.split(None, 1)
    if len(p) < 2: continue
    total += 1
    if "gGZLE01_recomp.so" in p[1]:
        inmod.append(int(p[0], 16) - start + off + bias)
    else:
        host[p[1].strip().split("/")[-1].strip("()")] += 1
# chunk symbols
nm = subprocess.run(["nm", "-n", mod], capture_output=True, text=True).stdout
funcs = []; tables = {}
for line in nm.splitlines():
    f = line.split()
    if len(f) != 3: continue
    a, t, name = int(f[0], 16), f[1], f[2]
    if t in "tT": funcs.append((a, name))
    mt = re.match(r"func_[0-9A-F]{8}\.pc_table_([0-9A-F]{8})$", name)
    if mt: tables[int(mt.group(1), 16)] = a
fa = [a for a, _ in funcs]
def chunk_of(v):
    i = bisect.bisect_right(fa, v) - 1
    return funcs[i] if i >= 0 else (0, "?")
# which chunks are hot
chunk_count = collections.Counter(chunk_of(v)[1] for v in inmod)
hot = [n for n, _ in chunk_count.most_common(400) if re.match(r"func_[0-9A-F]{8}$", n)]
# pc_table entries for the hot chunks, from the RELATIVE relocations
want = {}
for n in hot:
    g = int(n[5:], 16)
    if g in tables: want[tables[g]] = g
# pc_table entries for the hot chunks: with packed (RELR) relocations the label addresses sit in the table
import struct
rw = []
for line in segs.splitlines():
    f = line.split()
    if f and f[0] == "LOAD" and any("W" in x for x in f[6:8]):
        rw.append((int(f[2], 16), int(f[1], 16), int(f[4], 16)))
def file_off(v):
    for va, fo, sz in rw:
        if va <= v < va + sz: return v - va + fo
labels = collections.defaultdict(list)  # guest chunk -> [(native, guest pc)]
with open(mod, "rb") as fh:
    for t, g in want.items():
        fh.seek(file_off(t)); raw = fh.read(4096 * 8)
        for idx, val in enumerate(struct.unpack("<4096Q", raw)):
            if val: labels[g].append((val, g + idx * 4))
for g in labels: labels[g].sort()
# decompilation symbols
sy = []
for line in open(syms):
    m = re.match(r"(\S+) = \.\w+:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)", line)
    if m: sy.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
sy.sort(); sa = [s[0] for s in sy]
def guest_name(pc):
    i = bisect.bisect_right(sa, pc) - 1
    if i >= 0 and sy[i][0] <= pc < sy[i][0] + sy[i][1]: return sy[i][2]
    return "?%08X" % pc
by_fn = collections.Counter(); other = collections.Counter(); unmapped = 0
for v in inmod:
    a, n = chunk_of(v)
    mt = re.match(r"func_([0-9A-F]{8})$", n)
    if mt and int(mt.group(1), 16) in labels:
        L = labels[int(mt.group(1), 16)]
        j = bisect.bisect_right(L, (v, 0xFFFFFFFF)) - 1
        if j >= 0: by_fn[guest_name(L[j][1])] += 1
        else: unmapped += 1
    else:
        other[n] += 1
print(f"samples {total}, translated module {len(inmod)} ({100*len(inmod)/total:.1f}%), host/libs {total-len(inmod)}")
print("top game functions (share of all game-thread samples):")
for name, c in by_fn.most_common(top): print(f"  {100*c/total:5.2f}%  {name}")
print("module code outside the mapped chunks (helpers, natives):")
for name, c in other.most_common(10): print(f"  {100*c/total:5.2f}%  {name}")
print("host/libs:", ", ".join(f"{k} {100*v/total:.1f}%" for k, v in host.most_common(4)))
if "--json" in sys.argv:
    import json
    out = {"total": total, "functions": dict(by_fn), "module_other": dict(other), "host": dict(host)}
    json.dump(out, open(sys.argv[sys.argv.index("--json") + 1], "w"), indent=0)
