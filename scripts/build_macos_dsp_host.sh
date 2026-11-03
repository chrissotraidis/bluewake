#!/usr/bin/env bash
# Build the pinned donor DSP and the native macOS host with ThinLTO.
set -euo pipefail

cd "$(dirname "$0")/.."

donor_build=${BLUEWAKE_DSP_DONOR_BUILD_DIR:-build/recompcore-dsp-ipo}
host_build=${BLUEWAKE_HOST_BUILD_DIR:-build/runtime-host-dsp}
jobs=${BLUEWAKE_BUILD_JOBS:-$(sysctl -n hw.ncpu)}
developer_tracing=${BLUEWAKE_ENABLE_DEVELOPER_TRACING:-OFF}

case "$donor_build" in
    /*) donor_build_abs=$donor_build ;;
    *) donor_build_abs=$PWD/$donor_build ;;
esac

cmake -S ref/recompcore -B "$donor_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DENABLE_QT=OFF \
    -DENABLE_TESTS=OFF
cmake --build "$donor_build" --target core --parallel "$jobs"

cmake -S runtime/host -B "$host_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBLUEWAKE_ENABLE_DSP_ADAPTER=ON \
    -DBLUEWAKE_ENABLE_DSP_IPO=ON \
    -DBLUEWAKE_ENABLE_DEVELOPER_TRACING="$developer_tracing" \
    -DBLUEWAKE_DSP_DONOR_DIR="$PWD/ref/recompcore" \
    -DBLUEWAKE_DSP_DONOR_BUILD_DIR="$donor_build_abs"
cmake --build "$host_build" --parallel "$jobs"
ctest --test-dir "$host_build" --output-on-failure -j1 -E '_NOT_BUILT$'
