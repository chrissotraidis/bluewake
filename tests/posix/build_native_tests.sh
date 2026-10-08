#!/bin/bash
# Build the native_* comparison tests for Linux, against a gGZLE01_recomp.so.
# Usage: build_native_tests.sh <gGZLE01_recomp.so> [out_dir]
set -euo pipefail
SO="${1:?usage: build_native_tests.sh <gGZLE01_recomp.so> [out_dir]}"
OUT="${2:-build/linux-native/native-tests}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
GXR="$ROOT/ref/recompcore/GXRuntime"
GRTLIB="$ROOT/build/linux-native/app/gxruntime_build/libgxruntime.a"
STATICRECOMP="$ROOT/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp"
CC="${CC:-cc}"
CFLAGS="-O2 -march=x86-64-v3 -ffp-contract=off -I$ROOT/cmake/composite -I$GXR/include -I$STATICRECOMP -I$ROOT/tests/posix -Wno-unused-function -Wno-unused-variable"

mkdir -p "$OUT"

build_one() {
    local test="$1"; shift
    local srcs="$1"; shift
    local name="${test%.c}"
    echo "=== building $name ==="
    $CC $CFLAGS -o "$OUT/$name" "$ROOT/tests/$test" "$ROOT/cmake/composite/direct_calls.c" $srcs $GRTLIB -ldl -lm -pthread
    echo "    -> $OUT/$name"
}

build_one native_search_test.c "$ROOT/cmake/composite/native_search.c"
build_one native_mtxcalc_test.c "$ROOT/cmake/composite/native_mtxcalc.c"

echo "done. run: $OUT/native_search_test $SO"
