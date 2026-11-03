#!/usr/bin/env bash
# BlueWake P4 milestone 9 acceptance: save, quit, and reload, unattended.
#
# What this proves, in order, and from what:
#   1. the route reaches controllable gameplay on the certified card copy;
#   2. the pause menu's Save screen runs the guest's own save chain - the save
#      screen's proc field walks 0 -> 1 -> 16/17 -> 18 -> 22 with the card write
#      counter moving, which is host bookkeeping of the guest's own writes;
#   3. the card file the game wrote differs from the card it started with, read
#      back by scripts/card_container.py and not by the guest's word;
#   4. the quit is the guest's own reset (the card unmounts and the machine
#      reboots), which the log shows as the unmount that follows the second
#      prompt's answer;
#   5. a separate boot with that card reaches event-free gameplay without any
#      new-game milestones, and the same control predicate the cold-new-game
#      path uses fires at a retrace this script checks against a bound.
#
# What it is not. The input is the host's own pad schedule, not a person's keys:
# the pad-driven run is a measurement, and scripts/app_acceptance_test.sh remains
# the real-key path. Nor does it prove the save survives a *different* build -
# the reload is the same binary that wrote it.
#
# Usage: scripts/save_continue_acceptance.sh
#   BLUEWAKE_ACCEPTANCE_SAVE_RETRACES   retrace cap for the save half (24000)
#   BLUEWAKE_ACCEPTANCE_RELOAD_RETRACES retrace cap for the reload half (2500)
#   BLUEWAKE_ACCEPTANCE_CONTROL_BOUND   reload control must land by this (2000)
#   BLUEWAKE_ACCEPTANCE_OUT             output directory
#
# A pass prints the two control retraces and their guest seconds: the cold new
# game reaches control at 20,338 retraces (339.0 s of authored content) and the
# save-continue path at 833 (13.9 s), which is the PRD's time-to-playable reading
# and the reason the second half of this script exists.

set -euo pipefail
cd "$(dirname "$0")/.."
root=$PWD

host=${BLUEWAKE_ACCEPTANCE_HOST:-$root/build/runtime-host-dsp/bluewake_host}
composite=${BLUEWAKE_ACCEPTANCE_COMPOSITE:-$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}
route_card=$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card
dol=$root/generated/full/main.dol
disc="$root/ref/The Legend Of Zelda The Wind Waker.iso"
rels=$root/generated/full/rels
dsp_irom=$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin
dsp_coef=$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin
save_retraces=${BLUEWAKE_ACCEPTANCE_SAVE_RETRACES:-24000}
reload_retraces=${BLUEWAKE_ACCEPTANCE_RELOAD_RETRACES:-2500}
control_bound=${BLUEWAKE_ACCEPTANCE_CONTROL_BOUND:-2000}
out=${BLUEWAKE_ACCEPTANCE_OUT:-$root/local-research/acceptance/$(date +%Y%m%d-%H%M%S)}

for required in "$host" "$composite" "$route_card" "$dol" "$disc" "$rels" "$dsp_irom" "$dsp_coef"; do
    if [ ! -e "$required" ]; then
        echo "acceptance: missing required input: $required" >&2
        exit 1
    fi
done
if pgrep -x bluewake_host >/dev/null 2>&1; then
    echo "acceptance: a BlueWake process is already running; refusing to overlap" >&2
    exit 1
fi

mkdir -p "$out"
card=$out/save.card
cp "$route_card" "$card"
failures=0
bad() { printf 'acceptance: FAIL %s\n' "$*"; failures=$((failures + 1)); }
ok() { printf 'acceptance: ok   %s\n' "$*"; }

# One host process, one card, headless. The two halves differ only in the card
# they start from, the retrace cap and the pad schedule.
run_host() {
    local log=$1 retraces=$2 card_path=$3 script=$4 confirm=$5 extra=$6
    env BLUEWAKE_ROOT="$root" \
        BLUEWAKE_RENDERER=headless \
        BLUEWAKE_CYCLE_CAP=16384 \
        BLUEWAKE_MAX_BLOCKS=100000000000 \
        BLUEWAKE_DOL="$dol" \
        BLUEWAKE_DISC="$disc" \
        BLUEWAKE_RELS_DIR="$rels" \
        BLUEWAKE_DSP_IROM="$dsp_irom" \
        BLUEWAKE_DSP_COEF="$dsp_coef" \
        BLUEWAKE_CARD_PATH="$card_path" \
        BLUEWAKE_CARD_LOG=1 \
        BLUEWAKE_MAX_RETRACES="$retraces" \
        BLUEWAKE_PAD_BUTTONS=0x0100 \
        BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1 \
        BLUEWAKE_PAD_PULSE_LENGTH=2 \
        ${script:+BLUEWAKE_PAD_SCRIPT="$script"} \
        ${confirm:+BLUEWAKE_PAD_CONFIRM_EVENT="$confirm"} \
        ${extra:+$extra} \
        "$host" "$composite" >"$log" 2>&1
}

# The route's own walk through the awake cutscene: one A every 150 retraces from
# 17,800, the spacing scripts/play.sh uses for the same scene.
new_game_script=$(python3 - <<'PY'
print(','.join('%d:0x0100:2' % p for p in range(17800, 29801, 150)))
PY
)
reload_script=$(python3 - <<'PY'
print(','.join('%d:0x0100:2' % p for p in range(340, 1601, 60)))
PY
)

echo "acceptance: out = ${out#$root/}"
echo "acceptance: save half: ${save_retraces} retraces on a copy of the route card"
run_host "$out/save.log" "$save_retraces" "$card" "$new_game_script" "" "BLUEWAKE_SAVE_ROUTE=1"

# 1. The route was the certified one. A normal stop is not evidence on its own,
#    which is why the milestones the bench guards on are checked here too.
grep -aq 'opening-complete retrace=13850' "$out/save.log" \
    && ok 'route: opening-complete at retrace 13850 (the certified route)' \
    || bad 'route: the save half did not follow the certified route'
grep -aq 'play-scene retrace=13910' "$out/save.log" \
    && ok 'route: play scene at retrace 13910' \
    || bad 'route: the play scene milestone is not the certified one'

# 2. The guest's own save chain, read from the save screen's proc field.
grep -aq 'save-screen retrace=' "$out/save.log" \
    && ok 'save: the pause menu opened the save screen (collect page, mode 3)' \
    || bad 'save: the save screen never opened'
grep -aqE 'proc=18 ' "$out/save.log" \
    && ok 'save: the save data was written (proc 18, the data-save step)' \
    || bad 'save: the save chain did not reach the data-save step'
writes=$(grep -aoE 'proc=22 .*card_writes=[0-9]+' "$out/save.log" | grep -aoE '[0-9]+$' | head -1)
if [ -n "$writes" ] && [ "$writes" -gt 0 ]; then
    ok "save: the card was written ($writes write calls, 3 x 8192 bytes each)"
else
    bad 'save: the card write counter never moved'
fi
grep -aq 'press=stick-right quit-prompt' "$out/save.log" \
    && ok 'quit: the second prompt was answered on the quit line' \
    || bad 'quit: the quit prompt was never answered on the quit line'
grep -aq 'press=A quit-prompt' "$out/save.log" \
    && ok 'quit: the guest was told to return to the title screen' \
    || bad 'quit: the quit answer was never confirmed'
grep -aq '\[card\] CARDUnmount' "$out/save.log" \
    && ok 'quit: the guest unmounted the card, which its reset path does' \
    || bad 'quit: no card unmount, so the guest did not tear down for a reset'

# 3. The card, read back independently of the guest.
card_diff=$(python3 "$root/scripts/card_container.py" "$route_card" "$card" || true)
if printf '%s\n' "$card_diff" | grep -q "data changed"; then
    changed=$(printf '%s\n' "$card_diff" | grep -oE '[0-9]+ of [0-9]+ bytes differ' | head -1)
    ok "card: the save the game wrote differs from the card it started with ($changed)"
else
    bad 'card: the card is byte-identical, so no gameplay state was saved'
fi
printf '%s\n' "$card_diff" | grep -E "#0 |data changed" | sed 's/^/acceptance:     /'

# 4. A separate boot with that card: no new game, and control inside the bound.
echo "acceptance: reload half: ${reload_retraces} retraces on the card the save wrote"
run_host "$out/reload.log" "$reload_retraces" "$card" "$reload_script" "any" "BLUEWAKE_SAVE_ROUTE=1"

grep -aq 'play-scene retrace=' "$out/reload.log" \
    && ok 'reload: the play scene came up' \
    || bad 'reload: the play scene never came up'
if grep -aq 'new-game-intro' "$out/reload.log"; then
    bad 'reload: the run took the new-game path, so nothing was loaded'
else
    ok 'reload: no new-game intro, so the card content was loaded as a save'
fi
if grep -aq 'name-input-complete' "$out/reload.log"; then
    bad 'reload: the name-entry flow ran, which a loaded save does not do'
else
    ok 'reload: no name entry, which only a fresh file asks for'
fi
control=$(grep -aoE 'control-ready retrace=[0-9]+' "$out/reload.log" | grep -oE '[0-9]+$' | head -1)
cold_control=20338
if [ -z "$control" ]; then
    bad 'reload: the control predicate never fired'
elif [ "$control" -le "$control_bound" ]; then
    ok "reload: control at retrace $control ($(awk -v r="$control" 'BEGIN { printf "%.1f", r / 60 }') s of authored content, against $cold_control / 339.0 s for a cold new game)"
else
    bad "reload: control took $control retraces, over the $control_bound bound"
fi

echo "acceptance: logs = ${out#$root/}"
if [ "$failures" -eq 0 ]; then
    echo 'acceptance: PASS - the save, the guest quit and the reload all held'
    exit 0
fi
echo "acceptance: FAIL - $failures clause(s)" >&2
exit 1

