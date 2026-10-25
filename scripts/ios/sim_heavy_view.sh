#!/usr/bin/env bash
# The simulator's heavy Outset view: load the acceptance save, stand on the pier
# facing the island, and time retraces 1000-3100 from the host's frame timing.
# This is the view the speed table in docs/archive/status/CURRENT.md tracks.
#
# usage: scripts/ios/sim_heavy_view.sh [--runs N] [--no-build] [--env K=V]...
# Prints one line per run: retraces a second, median and p90 retrace time.
set -euo pipefail
cd "$(dirname "$0")/../.."
root=$PWD
runs=1
pass=()
build=
while [ $# -gt 0 ]; do
    case "$1" in
        --runs) runs=$2; shift 2 ;;
        --no-build) build=--no-build; shift ;;
        --env) pass+=(--env "$2"); shift 2 ;;
        *) echo "sim_heavy_view: unknown argument $1" >&2; exit 2 ;;
    esac
done
card=$root/local-research/ipad/acceptance-20260923-095148/save.card
[ -f "$card" ] || { echo "sim_heavy_view: missing $card (a save in Outset)" >&2; exit 1; }
script=$(python3 -c "print(','.join('%d:0x0100:2' % p for p in range(340, 1601, 60)))")
for i in $(seq 1 "$runs"); do
    out=$root/local-research/ipad/heavy-$(date +%Y%m%d-%H%M%S)
    scripts/ios/sim_run.sh $build ${pass[@]+"${pass[@]}"} --route --card "$card" --out "$out" --wait \
        --env BLUEWAKE_FRAME_TIMING=1 --env BLUEWAKE_MAX_RETRACES=3200 \
        --env BLUEWAKE_PAD_SCRIPT="$script" --env BLUEWAKE_PAD_CONFIRM_EVENT=any >/dev/null
    build=--no-build
    python3 - "$out/stderr.log" "$out" <<'EOF'
import re, statistics, sys
t = {}
for line in open(sys.argv[1], errors='replace'):
    m = re.match(r'\[frame-timing\] retrace=(\d+) us=(\d+)', line)
    if m:
        t[int(m.group(1))] = int(m.group(2))
a, b = 1000, 3100
if a not in t or b not in t:
    sys.exit('sim_heavy_view: the run did not reach retrace %d (log %s)' % (b, sys.argv[1]))
d = sorted((t[k + 1] - t[k]) / 1000 for k in range(a, b) if k in t and k + 1 in t)
wall = (t[b] - t[a]) / 1e6
print('heavy view %d-%d: %.2f retraces/s, median %.1f ms, p90 %.1f ms, over 100 ms: %d  (%s)' % (
    a, b, (b - a) / wall, statistics.median(d), d[int(.9 * len(d))], sum(x > 100 for x in d), sys.argv[2]))
EOF
done
