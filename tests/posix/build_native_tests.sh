#!/bin/bash
# Build the native_* comparison tests for Linux, against a gGZLE01_recomp.so.
# Usage: build_native_tests.sh <gGZLE01_recomp.so> [out_dir]
# Optional: ONLY=native_bg_test (or a comma-separated list), RECOMPCORE=...,
# and GRTLIB=... select a focused test and its matching private-build inputs.
set -euo pipefail
SO="${1:?usage: build_native_tests.sh <gGZLE01_recomp.so> [out_dir]}"
OUT="${2:-build/linux-native/native-tests}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
RECOMPCORE="${RECOMPCORE:-$ROOT/ref/recompcore}"
GXR="$RECOMPCORE/GXRuntime"
GRTLIB="${GRTLIB:-$ROOT/build/linux-native/app/gxruntime_build/libgxruntime.a}"
STATICRECOMP="$RECOMPCORE/Source/Core/Core/PowerPC/StaticRecomp"
CC="${CC:-cc}"
ONLY="${ONLY:-all}"
CFLAGS="-O2 -march=x86-64-v3 -ffp-contract=off -I$ROOT/cmake/composite -I$GXR/include -I$STATICRECOMP -I$ROOT/tests/posix -Wno-unused-function -Wno-unused-variable"

mkdir -p "$OUT"

if [[ ! -f "$GRTLIB" ]]; then
    echo "GXRuntime library not found: $GRTLIB" >&2
    echo "Set GRTLIB to the build's libgxruntime.a path." >&2
    exit 2
fi
if [[ ! -f "$GXR/include/core/cpu.h" || ! -d "$STATICRECOMP" ]]; then
    echo "RecompCore headers not found: $RECOMPCORE" >&2
    echo "Set RECOMPCORE to the pinned checkout used for the module build." >&2
    exit 2
fi

build_one() {
    local test="$1"; shift
    local srcs="$1"; shift
    local name="${test%.c}"
    echo "=== building $name ==="
    $CC $CFLAGS -o "$OUT/$name" "$ROOT/tests/$test" "$ROOT/cmake/composite/direct_calls.c" $srcs $GRTLIB -ldl -lm -pthread
    echo "    -> $OUT/$name"
}

want() {
    [[ "$ONLY" == "all" || ",$ONLY," == *",$1,"* ]]
}

if want native_search_test; then build_one native_search_test.c "$ROOT/cmake/composite/native_search.c"; fi
if want native_mtxcalc_test; then build_one native_mtxcalc_test.c "$ROOT/cmake/composite/native_mtxcalc.c"; fi
if want native_bg_test; then build_one native_bg_test.c "$ROOT/cmake/composite/native_bg.c"; fi
if want native_leaf_test; then
    build_one native_leaf_test.c "$ROOT/cmake/composite/native_libm.c $ROOT/cmake/composite/native_bgblk.c $ROOT/cmake/composite/native_rot.c $ROOT/cmake/composite/native_calc.c $ROOT/cmake/composite/native_geom.c $ROOT/cmake/composite/native_jas.c"
fi

echo "done. run, for example: $OUT/native_bg_test $SO"
