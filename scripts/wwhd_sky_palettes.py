#!/usr/bin/env python3
"""HD's sky and light palettes, in the GameCube's layout, for the host to put over each stage's own (BLUEWAKE_HD_SKY).
The importer writes them (optional/hd-sky/stages.bin); this makes them from stage files directly. Formats: wwhd/sky.py.

Usage: wwhd_sky_palettes.py GC_DZS HD_DZS OUT        one stage (the GameCube's res/Stage/STAGE/Stage.arc dzs/stage.dzs,
                                                     HD's STAGE_Stage.szs Stage.bfres's embedded dzs)
   or: wwhd_sky_palettes.py --all GC_DIR HD_DIR OUT  every STAGE.dzs in both folders into one stages.bin"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wwhd.sky import convert, pack_all

if len(sys.argv) == 5 and sys.argv[1] == '--all':
    gc_dir, hd_dir, out = sys.argv[2:5]
    names = sorted(n[:-4] for n in os.listdir(gc_dir) if n.endswith('.dzs') and os.path.exists(os.path.join(hd_dir, n)))
    data, skipped = pack_all({n: (open(os.path.join(gc_dir, n + '.dzs'), 'rb').read(),
                                  open(os.path.join(hd_dir, n + '.dzs'), 'rb').read()) for n in names})
    open(out, 'wb').write(data)
    for name, why in skipped:
        print(f'{name}: skipped ({why})')
    print(f'{struct.unpack(">I", data[4:8])[0]} stages -> {out}')
elif len(sys.argv) == 4:
    data = convert(open(sys.argv[1], 'rb').read(), open(sys.argv[2], 'rb').read())
    open(sys.argv[3], 'wb').write(data)
    pn, vn = struct.unpack('>II', data[8:16])
    print(f'{pn} Pale, {vn} Virt -> {sys.argv[3]}')
else:
    raise SystemExit(__doc__)
