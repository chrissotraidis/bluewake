#!/usr/bin/env bash
# Capture a real in-game CPUState plus MEM1 image at the moment execution is
# dispatched into a chunk.
#
# Why this exists. scripts/chunk_bench.sh measures the emitted body's cost per
# guest cycle, but drives it from a synthetic CPUState, and a synthetic state
# cannot reproduce a real trajectory - that is the honest limit of the 27
# instructions per guest cycle it reports. This produces the real thing.
#
# It builds an instrumented copy of the composite's dispatch loop in a temp
# directory and relinks it there, so the certified dylib in the build tree is
# never touched. The hook is inert unless BLUEWAKE_DUMP_CHUNK_STATE is set.
#
# Usage: scripts/dump_chunk_state.sh [chunk_id ...]
#        scripts/dump_chunk_state.sh 0144 0201
#        scripts/dump_chunk_state.sh --all
#
# Environment:
#   BLUEWAKE_DUMP_DIR        output directory (default /tmp/bw-chunk-state)
#   BLUEWAKE_DUMP_WORK       instrumented build directory
#   BLUEWAKE_DUMP_AFTER_MS   hold the hook off for this long, so the snapshot
#                            comes from the play window rather than boot
#   BLUEWAKE_DUMP_PLAY_ARGS  extra args for play.sh (default: --headless --route)

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD
build=$root/build/composite-cycle-hybrid-o2-v2
composite_src=$root/local-research/work/cycle-extent-v6-20260831/composite-r2
gx_include=$root/ref/recompcore/GXRuntime/include
abi_include=$root/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp
cc=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
sdk=$(xcrun --show-sdk-path)
out="${BLUEWAKE_DUMP_DIR:-/tmp/bw-chunk-state}"
work="${BLUEWAKE_DUMP_WORK:-/tmp/bw-chunk-dump-build}"
after_ms="${BLUEWAKE_DUMP_AFTER_MS:-0}"
play_args="${BLUEWAKE_DUMP_PLAY_ARGS:---headless --route}"
HOT_CHUNKS="0201 0144 0015 0145 0181 0200 0187 0203 0188 0148"

if [ "${1:-}" = "--all" ]; then set -- $HOT_CHUNKS; fi
if [ $# -eq 0 ]; then set -- 0144; fi

mkdir -p "$out" "$work"

# Address ranges come from the generated source: the entry address is in the
# file name and the slot count is in the emitted pc_table, so this does not
# assume every chunk is 4096 instructions.
spec=""
for id in "$@"; do
    src=$(find "$composite_src" -name "chunk_${id}_*.c" 2>/dev/null | head -1 || true)
    if [ -z "$src" ]; then
        echo "dump_chunk_state: no generated source for chunk $id" >&2
        exit 1
    fi
    lo=$(basename "$src" .c | awk -F_ '{ print $NF }')
    slots=$(grep -oE 'pc_table_[0-9A-F]+\[[0-9]+\]' "$src" | head -1 | grep -oE '[0-9]+\]' | tr -d ']')
    if [ -z "$slots" ]; then slots=4096; fi
    hi=$(printf '%08X' $(( 0x$lo + slots * 4 - 4 )))
    path="$out/chunk_${id}_${lo}.bin"
    spec="${spec:+${spec},}${lo}:${hi}:${path}"
done

echo "dump_chunk_state: arm delay ${after_ms} ms"
printf '%s' "$spec" | tr ',' '\n' | sed 's/^/  /'

python3 "$root/scripts/make_chunk_dump_loop.py" \
    "$root/cmake/composite/dispatch_loop.c" \
    "$work/dispatch_loop_dump.c"

"$cc" -DDOLRECOMP_CPU_HEADER=\"core/cpu.h\" -DMODULE_GAME_ID=\"GZLE01\" \
    -DgGZLE01_recomp_EXPORTS \
    -I"$root/cmake/composite" -I"$composite_src" -I"$gx_include" -I"$abi_include" \
    -O2 -DNDEBUG -std=gnu11 -arch arm64 -isysroot "$sdk" \
    -fPIC -fvisibility=hidden -ffp-contract=off \
    -c "$work/dispatch_loop_dump.c" -o "$work/dispatch_loop_dump.o"

dylib="$work/gGZLE01_recomp_dump.dylib"
python3 - "$build" "$work/dispatch_loop_dump.o" "$dylib" <<'PY'
import os, shlex, subprocess, sys
build, dump_obj, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
link = os.path.join(build, "CMakeFiles/gGZLE01_recomp.dir/link.txt")
cmd = shlex.split(open(link).read().strip())
subbed = []
skip_next = False
for tok in cmd:
    if skip_next:
        skip_next = False
        continue
    if tok.endswith("dispatch_loop.c.o"):
        subbed.append(dump_obj)
    elif tok == "-o":
        subbed.extend([tok, out_path])
        skip_next = True
    else:
        subbed.append(tok)
subprocess.run(subbed, cwd=build, check=True)
print("dump_chunk_state: relinked " + out_path)
PY

echo "dump_chunk_state: running the route with the instrumented composite"
# shellcheck disable=SC2086
BLUEWAKE_PLAY_COMPOSITE="$dylib" \
BLUEWAKE_DUMP_CHUNK_STATE="$spec" \
BLUEWAKE_DUMP_AFTER_MS="$after_ms" \
    "$root/scripts/play.sh" $play_args || true

echo
echo "dump_chunk_state: snapshots"
ls -la "$out"/*.bin 2>/dev/null || echo "  (none written)"
