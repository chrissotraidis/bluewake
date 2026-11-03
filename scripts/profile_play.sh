#!/usr/bin/env bash
# Sample the host during live Outset play, for renderer-side work.
#
# scripts/bench.sh runs BLUEWAKE_RENDERER=headless, which installs the headless
# backend instead of initialising Aurora, so it never executes the block that
# reads DOL_GX_CORE and installs the gx-core sink observers. Renderer changes
# are therefore invisible to the benchmark, and the rendered tier's wall-clock
# fps moves several percent between runs of the identical artifact. This script
# produces the metric that can see them: owner shares from a `sample` capture
# taken while the live play window is running.
#
# It is deliberately the same route the benchmark uses (scripts/play.sh --route)
# so the sampled frame is the accepted route's play window, and it waits for the
# play scene by reading the frame-timing stamps rather than by sleeping for a
# fixed time.
#
# Usage: scripts/profile_play.sh [OUT_DIR]
#
# Environment:
#   BLUEWAKE_PROFILE_RETRACES   retrace ceiling (default 14700)
#   BLUEWAKE_PROFILE_START      first retrace to accept as live play (14000)
#   BLUEWAKE_PROFILE_SECONDS    sampling duration (default 20)

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD

retraces=${BLUEWAKE_PROFILE_RETRACES:-14700}
start=${BLUEWAKE_PROFILE_START:-14000}
seconds=${BLUEWAKE_PROFILE_SECONDS:-20}
out=${1:-/tmp/bw-profile/$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out"

if pgrep -x bluewake_host >/dev/null 2>&1; then
    echo "profile_play: a bluewake_host process is already running" >&2
    exit 1
fi

echo "profile_play: out = $out"
echo "profile_play: route = rendered, $retraces retraces, sample ${seconds}s at retrace >= $start"

( scripts/play.sh --route --timing --retraces "$retraces" \
      --window 13910:14100 >"$out/play.out" 2>&1 ) &
play_pid=$!

log=""
for _ in $(seq 1 600); do
    log=$(ls -td local-research/play/*/play.log 2>/dev/null | head -1 || true)
    if [ -n "$log" ]; then
        last=$(grep -o 'retrace=[0-9]*' "$log" 2>/dev/null | tail -1 | cut -d= -f2 || true)
        if [ -n "${last:-}" ] && [ "$last" -ge "$start" ] 2>/dev/null; then
            break
        fi
    fi
    if ! kill -0 "$play_pid" 2>/dev/null; then
        echo "profile_play: the route exited before reaching retrace $start" >&2
        exit 1
    fi
    sleep 2
done

pid=$(pgrep -x bluewake_host | head -1 || true)
if [ -z "${pid:-}" ]; then
    echo "profile_play: no bluewake_host process to sample" >&2
    exit 1
fi

echo "profile_play: sampling pid $pid at retrace ${last:-?} for ${seconds}s"
sample "$pid" "$seconds" -file "$out/sample.txt" >/dev/null 2>&1 || true

wait "$play_pid" 2>/dev/null || true

if [ ! -s "$out/sample.txt" ]; then
    echo "profile_play: no sample captured" >&2
    exit 1
fi

echo "profile_play: log    = ${log#$root/}"
echo "profile_play: sample = $out/sample.txt"
echo
python3 "$root/scripts/sample_owners.py" "$out/sample.txt"
