#!/usr/bin/env bash
# Standalone microbenchmark of one generated chunk.
#
# Why this exists. Every planning number in this project has been a code-size
# proxy, and those proxies have measured wrong repeatedly. The instrument that
# reports the emitted body's cost directly - one chunk object, linked against
# the runtime, driven with a synthetic CPUState - was asked for in three
# consecutive ledger entries and never built. This is it.
#
# What it measures. An entry sweep: every instruction slot in the chunk is used
# as a dispatch entry point, with lr outside the chunk so each entry runs until
# the guest's first return. The harness reports guest cycles charged and host
# instructions retired over the whole sweep. The ratio is host instructions per
# guest cycle for that chunk's emitted body alone - no dispatcher, no device
# sync, no edge service.
#
# What it does not measure. Wall-clock speed. Every entry starts cold, so
# I-cache and branch-predictor state are unrepresentative and ns/cycle here is
# pessimistic; read the instruction count, not the seconds. Nor is the sweep a
# weighted sample of the play-scene instruction mix - it exercises every block
# once per pass rather than in proportion.
#
# Usage: scripts/bench_chunk.sh [chunk_id] [entries]
#        scripts/bench_chunk.sh 0144
#        scripts/bench_chunk.sh 0201 400000
#        scripts/bench_chunk.sh --all

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD
build=$root/build/composite-cycle-hybrid-o2-v2
composite_src=$root/local-research/work/cycle-extent-v6-20260831/composite-r2
gx_include=$root/ref/recompcore/GXRuntime/include
abi_include=$root/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp
cc=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
sdk=$(xcrun --show-sdk-path)
work="${BLUEWAKE_CHUNK_BENCH_DIR:-/tmp/bw-chunk-bench}"

# The hottest generated chunks in the live play window, from the rendered
# sample in docs/archive/status/CURRENT.md. 0201 is 15.7% of the sampled main thread.
HOT_CHUNKS="0201 0144 0015 0145 0181 0200 0187 0203 0188 0148"

chunk_ids=()
entries="${BLUEWAKE_CHUNK_BENCH_ENTRIES:-200000}"
# Bound every sweep in wall clock. An ablation is a price and not a patch, so
# some of them are semantically unsafe by design - and an unsafe memory path can
# drive the synthetic state into a guest loop that never returns. `unchecked` did
# exactly that on 2026-09-22: it held a core at 99 percent for seventeen minutes
# and the run it was pricing produced no number at all. A sweep that has to be
# killed is a null, not a price, so this says so rather than leaving it to be
# inferred from a hung process.
BLUEWAKE_CHUNK_BENCH_TIMEOUT="${BLUEWAKE_CHUNK_BENCH_TIMEOUT:-180}"
sweep() {
    local out="$1"
    local time_file="$2"
    shift 2
    /usr/bin/time -l "$@" >"$out" 2>"$time_file" &
    local pid=$!
    local waited=0
    while kill -0 "$pid" 2>/dev/null; do
        if [ "$waited" -ge "$BLUEWAKE_CHUNK_BENCH_TIMEOUT" ]; then
            kill -9 "$pid" 2>/dev/null || true
            wait "$pid" 2>/dev/null || true
            echo "bench_chunk: sweep killed after ${BLUEWAKE_CHUNK_BENCH_TIMEOUT}s of wall clock - this ablation is not sweepable" >&2
            return 1
        fi
        sleep 1
        waited=$((waited + 1))
    done
    wait "$pid" 2>/dev/null || true
    return 0
}
if [ "${1:-}" = "--all" ]; then
    read -r -a chunk_ids <<< "$HOT_CHUNKS"
elif [ -n "${1:-}" ]; then
    chunk_ids=("${1}")
else
    chunk_ids=(0144)
fi
if [ -n "${2:-}" ]; then entries="${2}"; fi

mkdir -p "$work"

runtime_objects=(
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu.c.o"
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu_exception.c.o"
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu_interpreter.c.o"
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu_interpreter_float.c.o"
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu_interpreter_integer.c.o"
    "Users/chrissotraidis/GitHub/bluewake/ref/recompcore/GXRuntime/src/core/cpu_interpreter_table.c.o"
)

# Snapshot mode: replay real in-game entry states captured by
# scripts/dump_chunk_state.sh instead of a synthetic sweep. Two runs at
# different dispatch counts are differenced, so process startup and dynamic
# loader work cancel and only the replayed dispatches are measured.
if [ "${1:-}" = "--snapshot" ]; then
    snap_dir="${2:-/tmp/bw-chunk-state}"
    snap_entries="${3:-100000}"
    for bin in "$snap_dir"/chunk_*.bin; do
        [ -e "$bin" ] || continue
        stem=$(basename "$bin" .bin)
        id=$(printf '%s' "$stem" | awk -F_ '{ print $2 }')
        snap_entry=$(printf '%s' "$stem" | awk -F_ '{ print $3 }')
        chunk_obj=$(find "$build/CMakeFiles/gGZLE01_recomp.dir" \
            -name "chunk_${id}_*.c.o" 2>/dev/null | head -1 || true)
        if [ -z "$chunk_obj" ]; then
            echo "bench_chunk: no built object for chunk $id" >&2
            continue
        fi
        func="func_${snap_entry}"
        "$cc" -DDOLRECOMP_CPU_HEADER=\"core/cpu.h\" \
            -I"$composite_src" -I"$gx_include" \
            -DBENCH_FUNC="$func" -DBENCH_ENTRY="0x${snap_entry}u" \
            -DBENCH_SLOTS="4096u" -DBENCH_NAME="\"chunk_${id}\"" \
            -O2 -DNDEBUG -std=gnu11 -arch arm64 -isysroot "$sdk" \
            -ffp-contract=off -c "$root/scripts/chunk_bench.c" -o "$work/chunk_bench.o"
        objs=("$work/chunk_bench.o" "$chunk_obj")
        for rel in "${runtime_objects[@]}"; do
            objs+=("$build/CMakeFiles/gGZLE01_recomp.dir/$rel")
        done
        "$cc" -O2 -arch arm64 -isysroot "$sdk" -o "$work/chunk_bench" "${objs[@]}"

        # Warm the snapshot through the page cache first. A cold first read and
        # a cached second read differ by millions of instructions, which would
        # otherwise swamp the dispatch delta this differences.
        sweep /dev/null /dev/null "$work/chunk_bench" --state-sweep "$bin" 4096 || exit 3
        sweep "$work/s1.txt" "$work/t1.txt" "$work/chunk_bench" --state-sweep "$bin" "$snap_entries" || exit 3
        sweep "$work/s2.txt" "$work/t2.txt" "$work/chunk_bench" --state-sweep "$bin" "$(( snap_entries * 2 ))" || exit 3

        i1=$(awk '/instructions retired/ { print $1 }' "$work/t1.txt")
        i2=$(awk '/instructions retired/ { print $1 }' "$work/t2.txt")
        c1=$(awk '/cycles elapsed/ { print $1 }' "$work/t1.txt")
        c2=$(awk '/cycles elapsed/ { print $1 }' "$work/t2.txt")
        g1=$(awk '/^guest cycles/ { print $3 }' "$work/s1.txt")
        g2=$(awk '/^guest cycles/ { print $3 }' "$work/s2.txt")
        pc=$(awk '/^entry pc/ { print $3 }' "$work/s1.txt")
        cpd=$(awk '/^cycles\/dispatch/ { print $3 }' "$work/s1.txt")

        echo "=== chunk_${id} real entry ${pc:-?} (cycles/dispatch ${cpd:-?}) ==="
        awk -v i1="$i1" -v i2="$i2" -v c1="$c1" -v c2="$c2" \
            -v g1="$g1" -v g2="$g2" '
            BEGIN {
                di = i2 - i1; dc = c2 - c1; dg = g2 - g1;
                if (dg > 0) printf "instructions/guest cycle  %.2f\n", di / dg;
                if (dc > 0) printf "IPC                      %.2f\n", di / dc;
                printf "dispatch delta           %d host instructions over %d guest cycles\n", di, dg;
            }'
        echo
    done
    exit 0
fi

# Ablation mode: apply a named transform from scripts/ablate_chunk.py to one
# generated chunk, compile it, and measure it with the same real-state sweep.
# This prices a named emitted construct in seconds instead of a rebuild.
if [ "${1:-}" = "--ablate" ]; then
    abl_id="${2:?usage: bench_chunk.sh --ablate <chunk_id> <ablation> [entries]}"
    ablation="${3:?usage: bench_chunk.sh --ablate <chunk_id> <ablation> [entries]}"
    abl_entries="${4:-200000}"
    snap_dir="${BLUEWAKE_DUMP_DIR:-/tmp/bw-chunk-state}"
    abl_plain=$(find "$composite_src" -name "chunk_${abl_id}_*.c" | head -1)
    if [ -z "$abl_plain" ]; then
        echo "bench_chunk: no generated source for chunk $abl_id" >&2
        exit 1
    fi
    if [ "$ablation" = "baseline" ]; then
        abl_src="$abl_plain"
        src_label="baseline"
    else
        abl_src="$work/chunk_${abl_id}_abl.c"
        python3 "$root/scripts/ablate_chunk.py" "$abl_plain" "$abl_src" "$ablation"
        src_label="$ablation"
    fi
    abl_entry=$(basename "$abl_plain" .c | awk -F_ '{ print $NF }')
    abl_obj="$work/chunk_${abl_id}_abl.o"
    "$cc" -DDOLRECOMP_CPU_HEADER=\"core/cpu.h\" -DMODULE_GAME_ID=\"GZLE01\" \
        -I"$composite_src" -I"$gx_include" -I"$abi_include" \
        -DNDEBUG -std=gnu11 -arch arm64 -isysroot "$sdk" \
        -fPIC -fvisibility=hidden -ffp-contract=off -O2 \
        -c "$abl_src" -o "$abl_obj"
    "$cc" -DDOLRECOMP_CPU_HEADER=\"core/cpu.h\" \
        -I"$composite_src" -I"$gx_include" \
        -DBENCH_FUNC="func_${abl_entry}" -DBENCH_ENTRY="0x${abl_entry}u" \
        -DBENCH_SLOTS="4096u" -DBENCH_NAME="\"chunk_${abl_id}\"" \
        -O2 -DNDEBUG -std=gnu11 -arch arm64 -isysroot "$sdk" \
        -ffp-contract=off -c "$root/scripts/chunk_bench.c" -o "$work/chunk_bench.o"
    objs=("$work/chunk_bench.o" "$abl_obj")
    for rel in "${runtime_objects[@]}"; do
        objs+=("$build/CMakeFiles/gGZLE01_recomp.dir/$rel")
    done
    "$cc" -O2 -arch arm64 -isysroot "$sdk" -o "$work/chunk_bench" "${objs[@]}"

    snap=""
    for candidate in "$snap_dir"/chunk_"$abl_id"_*.bin; do
        [ -e "$candidate" ] && snap=$candidate
    done
    if [ -z "$snap" ]; then
        echo "bench_chunk: no snapshot for chunk $abl_id in $snap_dir" >&2
        exit 1
    fi
    sweep /dev/null /dev/null "$work/chunk_bench" --state-sweep "$snap" 4096 || exit 3
    sweep "$work/a1.txt" "$work/b1.txt" "$work/chunk_bench" --state-sweep "$snap" "$abl_entries" || exit 3
    sweep "$work/a2.txt" "$work/b2.txt" "$work/chunk_bench" --state-sweep "$snap" "$(( abl_entries * 2 ))" || exit 3
    i1=$(awk '/instructions retired/ { print $1 }' "$work/b1.txt")
    i2=$(awk '/instructions retired/ { print $1 }' "$work/b2.txt")
    g1=$(awk '/^guest cycles/ { print $3 }' "$work/a1.txt")
    g2=$(awk '/^guest cycles/ { print $3 }' "$work/a2.txt")
    cyc1=$(awk '/^guest cycles/ { print $3 }' "$work/a1.txt")
    awk -v id="$abl_id" -v ab="$src_label" -v i1="$i1" -v i2="$i2" \
        -v g1="$g1" -v g2="$g2" '
        BEGIN {
            di = i2 - i1; dg = g2 - g1;
            printf "chunk_%-6s %-22s instructions/guest cycle %7.2f\n",
                   id, ab, (dg > 0) ? di / dg : -1;
        }'
    exit 0
fi

for id in "${chunk_ids[@]}"; do
    chunk_obj=$(find "$build/CMakeFiles/gGZLE01_recomp.dir" \
        -name "chunk_${id}_*.c.o" 2>/dev/null | head -1 || true)
    if [ -z "$chunk_obj" ]; then
        echo "bench_chunk: no built object for chunk $id" >&2
        continue
    fi
    base=$(basename "$chunk_obj" .c.o)
    entry=$(printf '%s' "$base" | awk -F_ '{ print $NF }')
    if [ "${#entry}" -ne 8 ]; then
        echo "bench_chunk: cannot read an entry address from $base" >&2
        continue
    fi
    func="func_${entry}"
    slots=4096

    "$cc" -DDOLRECOMP_CPU_HEADER=\"core/cpu.h\" \
        -I"$composite_src" -I"$gx_include" \
        -DBENCH_FUNC="$func" -DBENCH_ENTRY="0x${entry}u" \
        -DBENCH_SLOTS="${slots}u" -DBENCH_NAME="\"chunk_${id}\"" \
        -O2 -DNDEBUG -std=gnu11 -arch arm64 -isysroot "$sdk" \
        -ffp-contract=off -c "$root/scripts/chunk_bench.c" \
        -o "$work/chunk_bench.o"

    objs=("$work/chunk_bench.o" "$chunk_obj")
    for rel in "${runtime_objects[@]}"; do
        objs+=("$build/CMakeFiles/gGZLE01_recomp.dir/$rel")
    done
    "$cc" -O2 -arch arm64 -isysroot "$sdk" -o "$work/chunk_bench" "${objs[@]}"

    sweep "$work/out.txt" "$work/time.txt" "$work/chunk_bench" "$entries" || exit 3
    guest_cycles=$(awk '/^guest cycles/ { print $3 }' "$work/out.txt")
    instr=$(awk '/instructions retired/ { print $1 }' "$work/time.txt")
    elapsed=$(awk '/cycles elapsed/ { print $1 }' "$work/time.txt")

    echo "=== chunk_${id} (${func}, ${slots} slots, ${entries} entries) ==="
    grep -E '^entries|^entry stalls|^guest cycles|^cycles/entry|^wall seconds' "$work/out.txt"
    if [ -n "$instr" ] && [ -n "$guest_cycles" ] && [ "$guest_cycles" != 0 ]; then
        awk -v i="$instr" -v c="$guest_cycles" \
            'BEGIN { printf "instructions/guest cycle  %.2f\n", i / c }'
    fi
    if [ -n "$instr" ] && [ -n "$elapsed" ] && [ "$elapsed" != 0 ]; then
        awk -v i="$instr" -v e="$elapsed" 'BEGIN { printf "IPC                      %.2f\n", i / e }'
    fi
    echo
done
