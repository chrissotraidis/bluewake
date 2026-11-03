#!/usr/bin/env bash
# The simulator's village walk: load the Outset pier save, steer Link along the
# route past his house toward the village (BLUEWAKE_PAD_PLAYER_ROUTE), and time
# the heaviest stretch, retraces 1,300-1,730 (the grass behind the house to the
# ramp above the beach, the village and its actors in view). This replaces the
# 23,000-retrace new-game route for village speed checks.
#
# usage: scripts/ios/sim_village_view.sh [--runs N] [--no-build] [--env K=V]...
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
        *) echo "sim_village_view: unknown argument $1" >&2; exit 2 ;;
    esac
done
card=$root/local-research/ipad/acceptance-20260923-095148/save.card
[ -f "$card" ] || { echo "sim_village_view: missing $card (a save on the Outset pier)" >&2; exit 1; }
script=$(python3 -c "print(','.join('%d:0x0100:2' % p for p in range(340, 821, 60)))")
route='-194854.312,314302.333;-194720.526,314468.365;-194616.260,314832.344;-194495.005,314956.375;-194397.396,315267.510;-194239.273,315454.206;-194185.881,315589.061;-193984.063,315877.523;-193828.953,316060.664;-193635.354,316477.094;-193433.047,316762.250;-193557.562,316861.812;-193508.453,317174.719;-193060.156,317311.656;-192868.875,317630.344;-193286.984,317938.594;-193351.000,317986.000;-193640.375,317943.594;-193577.828,318049.656;-193477.969,318106.094;-193436.250,318259.375;-193584.859,318324.344;-193632.500,318447.656;-193515.828,318541.562;-193606.797,318743.750;-193493.516,318783.458;-193428.766,318691.188;-193396.734,318733.062;-193397.250,318680.344;-193479.363,318708.051'
for i in $(seq 1 "$runs"); do
    out=$root/local-research/ipad/village-$(date +%Y%m%d-%H%M%S)
    scripts/ios/sim_run.sh $build ${pass[@]+"${pass[@]}"} --card "$card" --out "$out" --wait \
        --env BLUEWAKE_FRAME_TIMING=1 --env BLUEWAKE_MAX_RETRACES=1800 \
        --env BLUEWAKE_PAD_SCRIPT="$script" --env BLUEWAKE_PAD_CONFIRM_EVENT=any \
        --env BLUEWAKE_PAD_PLAYER_TARGET_X=-194854.312 --env BLUEWAKE_PAD_PLAYER_TARGET_Z=314302.333 \
        --env BLUEWAKE_PAD_PLAYER_STICK_LENGTH=6000 --env BLUEWAKE_PAD_PLAYER_ROUTE="$route" >/dev/null
    build=--no-build
    python3 - "$out/stderr.log" "$out" <<'EOF'
import re, statistics, sys
t = {}
for line in open(sys.argv[1], errors='replace'):
    m = re.match(r'\[frame-timing\] retrace=(\d+) us=(\d+)', line)
    if m:
        t[int(m.group(1))] = int(m.group(2))
a, b = 1300, 1730
if a not in t or b not in t:
    sys.exit('sim_village_view: the run did not reach retrace %d (log %s)' % (b, sys.argv[1]))
d = sorted((t[k + 1] - t[k]) / 1000 for k in range(a, b) if k in t and k + 1 in t)
wall = (t[b] - t[a]) / 1e6
route = 'complete' if 'pad-route] complete' in open(sys.argv[1], errors='replace').read() else 'INCOMPLETE'
print('village %d-%d: %.2f retraces/s, median %.1f ms, p90 %.1f ms, route %s  (%s)' % (
    a, b, (b - a) / wall, statistics.median(d), d[int(.9 * len(d))], route, sys.argv[2]))
EOF
done
