#!/usr/bin/env bash
# Rebuild the composite's runtime objects (GXRuntime cpu.c and friends) and
# relink the dylib, without recompiling the 748 generated chunks.
#
# The composite build tree's dependency state is stale enough that a plain
# `make` recompiles every chunk (hours). The runtime sources change far more
# often than the generated code, so this script recompiles only them, with the
# exact commands recorded in compile_commands.json, and relinks with the
# recorded link.txt. The previous dylib is kept next to the new one.
#
# Usage: scripts/relink_composite_runtime.sh [BUILD_DIR]
#   BUILD_DIR defaults to build/composite-cycle-hybrid-o2-v2
#
# Profile-guided objects in build/composite-pgo/use (hot chunks) and
# build/composite-pgo-rt/use (runtime and dispatch), both from
# scripts/pgo_composite_hot.py, replace the plain objects in the link, so a
# relink keeps the promoted PGO composite. BLUEWAKE_NO_PGO=1 skips them.
set -euo pipefail
cd "$(dirname "$0")/.."
build=${1:-build/composite-cycle-hybrid-o2-v2}
cd "$build"
[ -f compile_commands.json ] && [ -f CMakeFiles/gGZLE01_recomp.dir/link.txt ] || {
    echo "relink: $build is not a composite build tree" >&2; exit 1; }

python3 - <<'PY'
import json, shlex, subprocess, sys
entries = json.load(open('compile_commands.json'))
runtime = [e for e in entries if '/ref/recompcore/GXRuntime/src/' in e['file']]
if not runtime:
    sys.exit('relink: no runtime sources in compile_commands.json')
for e in runtime:
    print('relink: compile', e['file'].split('/GXRuntime/')[1])
    subprocess.run(e['command'], shell=True, cwd=e['directory'], check=True)
PY

if [ -f gGZLE01_recomp.dylib ]; then
    cp -p gGZLE01_recomp.dylib "gGZLE01_recomp.dylib.prev"
fi
echo "relink: link"
link=$(cat CMakeFiles/gGZLE01_recomp.dir/link.txt)
repo=$(cd - >/dev/null; pwd)
if [ "${BLUEWAKE_NO_PGO:-0}" != 1 ]; then
    link=$(python3 - "$link" "$repo/build/composite-pgo/use" "$repo/build/composite-pgo-rt/use" <<'PY'
import os, sys
link = sys.argv[1]
n = 0
for pgo in sys.argv[2:]:
    if not os.path.isdir(pgo):
        continue
    for obj in sorted(os.listdir(pgo)):
        if not obj.endswith('.c.o'):
            continue
        for tok in link.split(' '):
            if tok.strip('"').endswith('/' + obj):
                link = link.replace(' ' + tok, ' "%s"' % os.path.join(pgo, obj), 1)
                n += 1
                break
print(link)
print('relink: %d PGO objects' % n, file=sys.stderr)
PY
)
fi
sh -c "$link"
ls -la gGZLE01_recomp.dylib
