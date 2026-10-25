#!/usr/bin/env bash
# Instructions per live-play retrace, for judging a digest-green change.
#
# scripts/bench.sh reports wall-clock fps, which varies 2-5% between runs of the
# identical artifact - wider than most emitter changes are worth. Instruction
# counts do not vary that way: the guest work is gated by the route digest, so
# two builds with the same digest execute the same guest instructions, and the
# only difference is what the host spent to execute them. On 2026-09-18 this
# measured the live play window at IPC 4.01, which is why instruction count is
# the metric for this workstream rather than speed (docs/archive/status/CURRENT.md).
#
# It works by differencing two headless runs of the same route: the difference
# between N and N+delta retraces is exactly the delta live-play retraces, so
# subtracting their /usr/bin/time -l totals isolates the play window. No rebuild
# needed, about three minutes.
#
# Usage: scripts/bench_instructions.sh [--low N] [--high N]
#
# Environment:
#   BLUEWAKE_INSTR_REFERENCE   instructions per play retrace to compare against
#                              (default 409200000, the tree's pre-tolerance
#                              control, so the printed delta is the landed
#                              zero-charge tolerance's own gain)
#   BLUEWAKE_BENCH_VARIANT     which build's recorded stops to guard against:
#                              landed (default, the tree) or control (the
#                              pre-zero-charge-tolerance artifact). The tolerance
#                              halves the host turn count without changing the
#                              guest work, so the same stopped pc arrives at a
#                              different count and each build needs its own row.

set -euo pipefail

cd "$(dirname "$0")/.."
root=$PWD

low=13900
high=14700
while [[ $# -gt 0 ]]; do
    case "$1" in
        --low) low=$2; shift 2 ;;
        --high) high=$2; shift 2 ;;
        -h|--help) sed -n '2,24p' "$0"; exit 0 ;;
        *) echo "bench_instructions: unknown argument: $1" >&2; exit 2 ;;
    esac
done

# The reference is the bench-window control for the composite in the tree: the
# 0017 artifact measured 491.7 M before the 2026-09-18 increments, the
# 2026-09-21 control measured 486.6 M, and the tree's pre-tolerance control for
# the 2026-09-22 zero-charge tolerance measures 409.2 M. Override it to compare
# against another.
reference=${BLUEWAKE_INSTR_REFERENCE:-409200000}

# Every row is a stop this tree has produced, keyed by the build that produced
# it. A pair is only a measurement if both of its runs match their row exactly;
# a row that does not exist is a refusal, not a pass, because an unguarded count
# is what this table exists to prevent.
variant="${BLUEWAKE_BENCH_VARIANT:-landed}"

host=${BLUEWAKE_BENCH_HOST:-$root/build/runtime-host-dsp/bluewake_host}
"$root/scripts/one_game_guard.sh"
# BLUEWAKE_BENCH_COMPOSITE names the artifact under test. A pair of runs is only
# a measurement of one build if that build cannot change between the two, and
# the composite is a file in the tree that a concurrent build or a screening
# relink can replace mid-pair; point this at a frozen copy to hold it fixed.
composite=${BLUEWAKE_BENCH_COMPOSITE:-$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}
card_source=$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card

echo "bench_instructions: variant   $variant"
echo "bench_instructions: composite $(shasum -a 256 "$composite" | cut -c1-16) $composite"
# Name the host too. A host change is invisible in the composite's hash, and the
# 2026-09-22 window had two changes in flight at once: one in the composite and
# one in the host. A pair is a measurement of both artifacts or it does not name
# what it measured.
echo "bench_instructions: host      $(shasum -a 256 "$host" | cut -c1-16) $host"

tmp=$(mktemp -d)
card="$tmp/route.card"
cp "$card_source" "$card"

run() {
    local n=$1 log=$2
    env BLUEWAKE_ROOT="$root" \
        BLUEWAKE_RENDERER=headless \
        BLUEWAKE_CYCLE_CAP=16384 \
        BLUEWAKE_MAX_BLOCKS=100000000000 \
        BLUEWAKE_DOL="$root/generated/full/main.dol" \
        BLUEWAKE_DISC="$root/ref/The Legend Of Zelda The Wind Waker.iso" \
        BLUEWAKE_RELS_DIR="$root/generated/full/rels" \
        BLUEWAKE_DSP_IROM="$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin" \
        BLUEWAKE_DSP_COEF="$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin" \
        BLUEWAKE_CARD_PATH="$card" \
        BLUEWAKE_MAX_RETRACES="$n" \
        BLUEWAKE_PAD_BUTTONS=0x0100 \
        BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1 \
        BLUEWAKE_PAD_PULSE_LENGTH=2 \
        /usr/bin/time -l "$host" "$composite" >"$log" 2>&1
}

echo "bench_instructions: runs $low and $high retraces on the same route"
run "$low" "$tmp/low.log"
run "$high" "$tmp/high.log"

# Refuse to report unless both runs did what they were supposed to do. An
# instruction count is only interpretable if the guest actually ran the route:
# a run that aborts early reports far fewer instructions, which reads as a
# spectacular speedup. That has already happened once - a DOL_GX_CORE=0 rendered
# run reported 161.4 M per retrace against 706.0 M, because it exited without a
# normal stop at 17 GB of resident memory - and one grep would have caught it, so
# this does the grep.
for log in "$tmp/low.log" "$tmp/high.log"; do
    if ! grep -q 'stopped: normal' "$log"; then
        echo "bench_instructions: $(basename "$log") has no normal-stop line;" >&2
        echo "  the run did not complete the route and its instruction count is" >&2
        echo "  not a measurement. Tail of the log:" >&2
        grep -av 'frame-timing' "$log" | tail -5 | sed 's/^/    /' >&2
        exit 1
    fi
done

# And a normal stop is not evidence that the guest ran the route. A host change
# that breaks the boot stops normally too, having executed far less, and reports
# a spectacular speedup: on 2026-09-21 an edge fast-reject that answered false
# for an intercepted address never reached the title screen, stopped at
# 0x80301510 after 14,495,606 turns, and read as 48.2 M per retrace. So the stop
# pc and turn count are compared against the recorded reference for the ceiling,
# and the boot milestones are checked.
guard_failed=0
for log in "$tmp/low.log" "$tmp/high.log"; do
    case "$log" in
        */low.log) ceiling=$low ;;
        *) ceiling=$high ;;
    esac
    case "$variant:$ceiling" in
        # The pre-tolerance control artifact: 486.6 M instructions per play
        # retrace on this window (docs/archive/status/CURRENT.md, 2026-09-21).
        control:13800) want="after 29737920 blocks at pc=0x80307ef4" ;;
        control:13900) want="after 29937744 blocks at pc=0x80307ef4" ;;
        control:14100) want="after 32203791 blocks at pc=0x80307ef4" ;;
        control:14700) want="after 40502699 blocks at pc=0x8027fa30" ;;
        # The landed zero-charge tolerance, measured on the composite in the
        # tree (sha256 4288b958, the build whose 400-retrace smoke reports
        # 766,380 turns at pc=0x80246960): 400.0 M instructions per play retrace
        # against the 409.2 M control, a gain of 2.25 percent. The 14,532,577 /
        # 20,464,729 pair that docs/archive/status/CURRENT.md first recorded for this
        # tolerance belongs to the earlier private composite 3b7ea820 and is not
        # this artifact's row.
        # Since 2026-09-24 the host writes the GX gather pipe without the
        # device sync and rebudget every other MMIO write takes, so turns run
        # longer and the same stop pc arrives after fewer of them; the route
        # digest over the high ceiling is unchanged (83d2590d, 1,050 records).
        # The pre-2026-09-24 host's rows were 14287831 / 20138490.
        # And since the GroundCross return observation moved into the edge
        # service (2026-09-24), that edge no longer ends a turn: 98 percent of
        # the play window's turn exits are gone. Digest unchanged; the rows
        # before it were 12682901 / 17935124.
        landed:13900) want="after 10557120 blocks at pc=0x80307ef4" ;;
        landed:14700) want="after 11174010 blocks at pc=0x8027fa30" ;;
        *) want="" ;;
    esac
    if [ -z "$want" ]; then
        echo "bench_instructions: no recorded stop for variant $variant at" >&2
        echo "  ceiling $ceiling; add one to the table before using this pair" >&2
        guard_failed=1
    fi
    stopped=$(grep -ao 'after [0-9]* blocks at pc=0x[0-9a-f]*' "$log" | tail -1)
    if [ -n "$want" ] && [ "$stopped" != "$want" ]; then
        echo "bench_instructions: ceiling $ceiling stopped $stopped," >&2
        echo "  expected $want - the guest did not follow the certified route," >&2
        echo "  so this pair is not a measurement. Tail of the log:" >&2
        grep -av 'frame-timing' "$log" | tail -3 | sed 's/^/    /' >&2
        guard_failed=1
    fi
    if [ "$ceiling" -ge 14100 ] && ! grep -aq 'title_ready=1' "$log"; then
        echo "bench_instructions: ceiling $ceiling never reached the title screen" >&2
        guard_failed=1
    fi
    if [ "$ceiling" -ge 14100 ] && ! grep -aq 'play_scene=1' "$log"; then
        echo "bench_instructions: ceiling $ceiling never entered the play scene" >&2
        guard_failed=1
    fi
done
[ "$guard_failed" -eq 0 ] || exit 1

python3 - "$tmp/low.log" "$tmp/high.log" "$low" "$high" "$reference" <<'PY'
import re, sys
low_log, high_log, low_n, high_n, reference = sys.argv[1:6]

def totals(path):
    text = open(path, errors="replace").read()
    def num(pattern):
        m = re.search(pattern, text)
        return int(m.group(1)) if m else 0
    # macOS `time -l` prints one line holding all three: real, user, sys.
    times = {"real": 0.0, "user": 0.0, "sys": 0.0}
    for line in text.splitlines():
        m = re.match(r"\s*([0-9.]+) real\s+([0-9.]+) user\s+([0-9.]+) sys", line)
        if m:
            times = dict(zip(("real", "user", "sys"),
                             (float(m.group(1)), float(m.group(2)), float(m.group(3)))))
    return {
        "instructions": num(r"(\d+)\s+instructions retired"),
        "cycles": num(r"(\d+)\s+cycles elapsed"),
        "user": times["user"],
        "real": times["real"],
    }

a = totals(low_log)
b = totals(high_log)
d_inst = b["instructions"] - a["instructions"]
d_cyc = b["cycles"] - a["cycles"]
play = int(high_n) - int(low_n)
if play <= 0 or d_inst <= 0:
    sys.exit("bench_instructions: the two runs did not differ in guest work")

per_retrace = d_inst / play
ipc = d_inst / d_cyc if d_cyc else 0.0
ref = float(reference)

print()
print("live-play window: %d retraces (%s -> %s)" % (play, low_n, high_n))
print("  instructions        %d" % d_inst)
print("  cycles              %d" % d_cyc)
print("  IPC                 %.2f" % ipc)
print("  instructions/retrace %.1f M" % (per_retrace / 1e6))
print("  user / real         %.2f s / %.2f s" % (b["user"] - a["user"],
                                                 b["real"] - a["real"]))
print()
if ref > 0:
    delta = 100.0 * (ref - per_retrace) / ref
    print("against the reference %.1f M: %+.2f%% fewer instructions"
          % (ref / 1e6, delta))
    if abs(delta) < 0.5:
        print("  (under half a percent - not a result at this resolution)")
PY

# Print the route digest of the high ceiling so a change can be compared against
# the certified value (`92dd816c...` at 14,100) rather than inferred from the
# stop line alone.
echo
python3 "${root:-.}/scripts/route_digest.py" "$tmp/high.log:$card" || true
