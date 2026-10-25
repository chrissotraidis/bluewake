#!/usr/bin/env bash
# BlueWake shipping-configuration benchmark.
#
# Runs the exact headless cap-256 Outset route documented in
# local-research/evidence/outset-performance-route-v1-20260901 and reports the
# five governing numbers:
#
#   1. median fps over the live-Outset window
#   2. p99 frame time (ms) over the same window
#   3. wall seconds versus guest seconds (real-speed ratio)
#   4. peak RSS (bytes)
#   5. route digest (scripts/route_digest.py), compared to the accepted baseline
#
# The digest is what makes a speedup admissible: a run that buys fps by
# changing guest behaviour fails the comparison and is not a result. Under D2
# the comparison gates guest state by equality and the host's turn schedule by
# a bound, so `scripts/route_digest.py` also has to be happy, not just this
# digest line - it prints the observed delivery and clock drift it allowed.
#
# Usage:
#   scripts/bench.sh                    # full 14,100-retrace Outset route
#   scripts/bench.sh --retraces 700     # bounded sanity tier
#   scripts/bench.sh --out DIR          # explicit artifact directory
#
# Environment overrides:
#   BLUEWAKE_BENCH_HOST       host binary (default build/runtime-host-dsp/bluewake_host)
#   BLUEWAKE_BENCH_COMPOSITE  composite dylib
#   BLUEWAKE_BENCH_CARD       copied save card used as the route input
#   BLUEWAKE_BENCH_WINDOW     live-play window as START:END (default 13910:14100)

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD

retraces=14100
out_dir=""
window="${BLUEWAKE_BENCH_WINDOW:-13910:14100}"
# The cycle cap owns the turn schedule. It is a measured variable, not a
# constant: the cap A/B has to be able to name the cap it ran.
#
# The shipping default is a fixed 16384-cycle window, adopted 2026-09-17 on the
# regenerated build (docs/archive/status/CURRENT.md, "the cycle window is a free
# lever"): same host, composite and card, play-window median 33.55 fps against
# 33.07 at 4096 and 31.38 under the previous dynamic 256-in-1024 policy, route
# wall 70.68 s against 73.76 s and 87.56 s, host turns 32,203,791 against
# 57,369,093 and 148,527,447, route digest UNCHANGED at every cap tested. At
# 16384 the cap stops mattering: the device deadlines bound the window, so this
# is "no cap" rather than a tuned value. BLUEWAKE_BENCH_CYCLE_CAP=dynamic
# restores the old policy and is what a regression compares against.
cycle_cap="${BLUEWAKE_BENCH_CYCLE_CAP:-16384}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --retraces) retraces=$2; shift 2 ;;
        --out) out_dir=$2; shift 2 ;;
        --window) window=$2; shift 2 ;;
        -h|--help) sed -n "2,28p" "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

host="${BLUEWAKE_BENCH_HOST:-$root/build/runtime-host-dsp/bluewake_host}"
composite="${BLUEWAKE_BENCH_COMPOSITE:-$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}"
card_source="${BLUEWAKE_BENCH_CARD:-$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card}"
dol="${BLUEWAKE_DOL:-$root/generated/full/main.dol}"
rels="${BLUEWAKE_RELS_DIR:-$root/generated/full/rels}"
disc="${BLUEWAKE_DISC:-$root/ref/The Legend Of Zelda The Wind Waker.iso}"
dsp_irom="${BLUEWAKE_DSP_IROM:-$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin}"
dsp_coef="${BLUEWAKE_DSP_COEF:-$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin}"

canonical_card_sha=6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987
# Re-derived once under decision D1 (docs/archive/GOAL_PROMPT_V16_2026-09-14.md): the
# route selector no longer gates on the delivery-timing aggregate, because that
# aggregate is the interrupt-acceptance schedule the cap owns. Both caps produce
# this digest on the completed pair; every guest-state record is unchanged and
# is still gated. The previous value was
# 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b, which no
# longer compares like with like.
#
# Re-derived again under decision D2 (docs/archive/status/CURRENT.md, 2026-09-17, "the
# DSP slice has Dolphin's idle skip switched off", and the D2 row in
# docs/archive/status/DECISIONS.md): the selector now also canonicalises the host
# turn-boundary fields out of the per-delivery records and the route clock, and
# `scripts/route_digest.py` gates those as bounds. D1's previous value was
# 0d4cee87923dea4bc09ef89af34ed438c1d9542368058423c722c3ad50bf7b7a, which was
# reachable only with the DSP idle skip switched off. The DSP idle skip is now
# the shipping default (BLUEWAKE_DSP_BATCH=128), so this value is the one the
# shipping configuration produces; export BLUEWAKE_DSP_BATCH=8 and the same
# digest still comes back, with a one-cycle delivery drift and two cycles of
# route clock the selector now allows and reports.
baseline_digest="${BLUEWAKE_BENCH_BASELINE_DIGEST:-92dd816c6a382531d597c8da0cc3e447c716aaa8240912d10a78bee853478782}"
baseline_retraces="${BLUEWAKE_BENCH_BASELINE_RETRACES:-14100}"

for required in "$host" "$composite" "$card_source" "$dol" "$disc" "$dsp_irom" "$dsp_coef"; do
    if [[ ! -e $required ]]; then
        echo "bench: missing required input: $required" >&2
        exit 1
    fi
done

if pgrep -f "MacOS/BlueWake|bluewake_host" >/dev/null 2>&1; then
    echo "bench: a BlueWake process is already running; refusing to overlap" >&2
    exit 1
fi

stamp=$(date +%Y%m%d-%H%M%S)
if [[ -z $out_dir ]]; then
    out_dir="$root/local-research/bench/$stamp"
fi
mkdir -p "$out_dir"

log="$out_dir/bench.log"
card="$out_dir/bench.card"
cp "$card_source" "$card"

card_sha=$(shasum -a 256 "$card" | awk "{print \$1}")
if [[ $card_sha != "$canonical_card_sha" ]]; then
    echo "bench: warning: card is not the canonical route card" >&2
    echo "bench:   expected $canonical_card_sha" >&2
    echo "bench:   actual   $card_sha" >&2
fi

echo "bench: host      = ${host#$root/}"
echo "bench: composite = ${composite#$root/}"
echo "bench: retraces  = $retraces"
echo "bench: window    = $window"
echo "bench: artifacts = ${out_dir#$root/}"

# Record the identity of what was measured before the run starts, so a timing
# can never be separated from the host and binaries that produced it. fps is
# host-dependent; the digest, the turn count and the card are not.
{
    echo "bench: host-cpu    = $(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo unknown)"
    echo "bench: host-sha256 = $(shasum -a 256 "$host" | awk '{print $1}')"
    echo "bench: comp-sha256 = $(shasum -a 256 "$composite" | awk '{print $1}')"
    echo "bench: card-sha256 = $card_sha"
    echo "bench: retraces    = $retraces"
    echo "bench: window      = $window"
    echo "bench: cycle-cap   = $cycle_cap"
    echo "bench: started     = $(date '+%Y-%m-%dT%H:%M:%S%z')"
} >"$log"

BLUEWAKE_DOL="$dol" \
BLUEWAKE_DISC="$disc" \
BLUEWAKE_RELS_DIR="$rels" \
BLUEWAKE_DSP_IROM="$dsp_irom" \
BLUEWAKE_DSP_COEF="$dsp_coef" \
BLUEWAKE_RENDERER=headless \
BLUEWAKE_CYCLE_CAP="$cycle_cap" \
BLUEWAKE_MAX_RETRACES="$retraces" \
BLUEWAKE_MAX_BLOCKS=100000000000 \
BLUEWAKE_PAD_BUTTONS=0x0100 \
BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1 \
BLUEWAKE_PAD_PULSE_LENGTH=2 \
BLUEWAKE_CARD_PATH="$card" \
BLUEWAKE_FRAME_TIMING=1 \
/usr/bin/time -l "$host" "$composite" >>"$log" 2>&1

/usr/bin/env python3 "$root/scripts/bench_report.py" \
    --log "$log" \
    --card "$card" \
    --retraces "$retraces" \
    --window "$window" \
    --baseline-digest "$baseline_digest" \
    --baseline-retraces "$baseline_retraces" \
    --json-out "$out_dir/bench.json"
