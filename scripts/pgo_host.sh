#!/usr/bin/env bash
# Profile-guided build of the macOS host (runtime/host).
# usage: scripts/pgo_host.sh gen|use [PROFDATA]
set -euo pipefail
cd "$(dirname "$0")/.."
mode=$1
donor_build=$PWD/build/recompcore-dsp-ipo
if [ "$mode" = gen ]; then
    flags="-fprofile-instr-generate"; dir=build/host-pgo-gen
else
    flags="-fprofile-instr-use=$(cd "$(dirname "$2")" && pwd)/$(basename "$2") -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"; dir=build/host-pgo-use
fi
cmake -S runtime/host -B "$dir" -DCMAKE_BUILD_TYPE=Release \
    -DBLUEWAKE_ENABLE_DSP_ADAPTER=ON -DBLUEWAKE_ENABLE_DSP_IPO=ON \
    -DBLUEWAKE_DSP_DONOR_DIR="$PWD/ref/recompcore" -DBLUEWAKE_DSP_DONOR_BUILD_DIR="$donor_build" \
    -DCMAKE_C_FLAGS="$flags" -DCMAKE_CXX_FLAGS="$flags" \
    -DCMAKE_EXE_LINKER_FLAGS="$( [ "$mode" = gen ] && echo -fprofile-instr-generate || true )" >/dev/null
cmake --build "$dir" --target bluewake_host --parallel "$(sysctl -n hw.ncpu)"
ls -la "$dir/bluewake_host"
