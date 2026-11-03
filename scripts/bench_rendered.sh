#!/usr/bin/env bash
# The rendered differential: instructions per play retrace, rendered and headless.
#
# scripts/bench_instructions.sh measures only the headless configuration, and
# headless installs the headless backend, so the GX frontend never runs and the
# rendered path is invisible to it. This takes the same differential on both and
# reports the renderer's share - the number that says what the product actually
# spends in its play window.
#
# Two ceilings are used, both INSIDE the certified route (13,800 and 14,100), so
# the guest work is provably identical on both sides: the runs must report the
# same host turn count and the same normal-stop pc, and this script refuses to
# report at all if they do not. That check is not decoration. An earlier rendered
# differential differenced a run that had aborted on a shader-compile failure and
# produced 214 M instructions per retrace of renderer cost where the true figure
# is 101 M; and a second attempt produced 161 M, below the headless count, which
# is impossible and was also an aborted run. Both would have been caught by
# looking for the normal-stop line.
#
# Usage: scripts/bench_rendered.sh

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD

# The same two ceilings scripts/bench_instructions.sh uses, with the references
# that harness records for them. They were 13,800/14,100 with the turn counts of
# the 63 MB artifact generation and had gone stale the way the instruction
# harness's own reference did: the composite has lost turns since, so this
# script refused to report rather than produce a rendered differential of a
# route that had moved.
low=13900
high=14700
expected_turns_low=14287831
expected_turns_high=20138490
expected_pc_low=0x80307ef4
expected_pc_high=0x8027fa30

host=$root/build/runtime-host-dsp/bluewake_host
composite=$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib
card_source=$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card

tmp=$(mktemp -d)
cp "$card_source" "$tmp/route.card"

run() { # retraces renderer(yes|no) logfile
    local n=$1 renderer=$2 log=$3
    local r=""
    [ "$renderer" = no ] && r="BLUEWAKE_RENDERER=headless"
    env BLUEWAKE_ROOT="$root" $r \
        BLUEWAKE_CYCLE_CAP=16384 \
        BLUEWAKE_MAX_BLOCKS=100000000000 \
        BLUEWAKE_DOL="$root/generated/full/main.dol" \
        BLUEWAKE_DISC="$root/ref/The Legend Of Zelda The Wind Waker.iso" \
        BLUEWAKE_RELS_DIR="$root/generated/full/rels" \
        BLUEWAKE_DSP_IROM="$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin" \
        BLUEWAKE_DSP_COEF="$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin" \
        BLUEWAKE_CARD_PATH="$tmp/route.card" \
        BLUEWAKE_MAX_RETRACES="$n" \
        BLUEWAKE_PAD_BUTTONS=0x0100 \
        BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1 \
        BLUEWAKE_PAD_PULSE_LENGTH=2 \
        /usr/bin/time -l "$host" "$composite" >"$log" 2>&1 || true
}

for renderer in yes no; do
    for n in $low $high; do
        echo "bench_rendered: running $renderer at $n retraces"
        run "$n" "$renderer" "$tmp/$renderer-$n.log"
    done
done

python3 - "$tmp" "$low" "$high" "$expected_turns_low" "$expected_turns_high" \
         "$expected_pc_low" "$expected_pc_high" <<'PY'
import re, sys
tmp, low, high, etl, eth, epc_low, epc_high = sys.argv[1:8]

def parse(path):
    text = open(path, errors="replace").read()
    # Capture the address alone, not the "pc=" prefix: the first version of this
    # pattern kept the prefix, so every pc compared unequal to the expected
    # address and the guard refused four perfectly good runs.
    m = re.search(r"stopped: normal after (\d+) blocks at pc=(0x[0-9a-f]+)", text)
    ins = re.search(r"(\d+)\s+instructions retired", text)
    return {"turns": int(m.group(1)) if m else None,
            "pc": m.group(2) if m else None,
            "instructions": int(ins.group(1)) if ins else 0}

total = int(high) - int(low)
rows = {}
bad = []
for renderer in ("yes", "no"):
    a = parse("%s/%s-%s.log" % (tmp, renderer, low))
    b = parse("%s/%s-%s.log" % (tmp, renderer, high))
    for label, got, want in ((low, a["turns"], etl), (high, b["turns"], eth)):
        if got != int(want):
            bad.append("%s at %s retraces: %s turns, expected %s"
                       % (renderer, label, got, want))
    for label, rec, want_pc in ((low, a, epc_low), (high, b, epc_high)):
        if rec["pc"] != want_pc:
            bad.append("%s at %s retraces: stop %s, expected %s"
                       % (renderer, label, rec["pc"], want_pc))
    rows[renderer] = (b["instructions"] - a["instructions"]) / total

if bad:
    sys.stderr.write("bench_rendered: refusing to report. The runs did not both "
                     "complete the certified route:\n")
    for b in bad:
        sys.stderr.write("  " + b + "\n")
    sys.exit(1)

headless, rendered = rows["no"], rows["yes"]
print()
print("live-play window, %s -> %s retraces (%s):" % (low, high, total))
print("  headless   %.1f M instructions per retrace" % (headless / 1e6))
print("  rendered   %.1f M instructions per retrace" % (rendered / 1e6))
print("  renderer   +%.1f M per retrace (+%.1f%%)"
      % ((rendered - headless) / 1e6, 100.0 * (rendered - headless) / headless))
print()
print("  both configurations stopped normally with the certified turn counts at "
      "each ceiling (%s at %s, %s at %s), so the guest work is identical and the "
      "difference is host work." % (epc_low, low, epc_high, high))
PY
