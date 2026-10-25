#!/usr/bin/env bash
# BlueWake iPad simulator acceptance: new game -> save -> the guest's quit ->
# reload, plus an audio capture, in ONE simulator.
#
# Same clauses as scripts/save_continue_acceptance.sh (the macOS headless
# version), run through the iOS app with Metal rendering and the SDL audio
# device. The input is the host's pad schedule, not touch; touch is covered by
# the manual simulator evidence in docs/archive/status/IPADOS_REORIENTATION_2026-09-23.md.
#
# Usage: scripts/ios/sim_save_acceptance.sh [--device NAME]  (default iPad Pro 11-inch (M5))
set -euo pipefail
cd "$(dirname "$0")/../.."
root=$PWD
device=${BLUEWAKE_SIM_DEVICE:-iPad Pro 11-inch (M5)}
if [ "${1:-}" = "--device" ]; then device=$2; fi
dsp_mode=${BLUEWAKE_DSP_MODE:-hle}   # the iOS app default

route_card=$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card
save_retraces=${BLUEWAKE_ACCEPTANCE_SAVE_RETRACES:-24000}
reload_retraces=${BLUEWAKE_ACCEPTANCE_RELOAD_RETRACES:-2500}
control_bound=${BLUEWAKE_ACCEPTANCE_CONTROL_BOUND:-2000}
out=${BLUEWAKE_ACCEPTANCE_OUT:-$root/local-research/ipad/acceptance-$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out/save" "$out/reload"
card=$out/save.card
cp "$route_card" "$card"
failures=0
bad() { printf 'ipad-acceptance: FAIL %s\n' "$*"; failures=$((failures + 1)); }
ok() { printf 'ipad-acceptance: ok   %s\n' "$*"; }

new_game_script=$(python3 -c "print(','.join('%d:0x0100:2' % p for p in range(17800, 29801, 150)))")
reload_script=$(python3 -c "print(','.join('%d:0x0100:2' % p for p in range(340, 1601, 60)))")

echo "ipad-acceptance: out = ${out#$root/}"
echo "ipad-acceptance: save half, $save_retraces retraces"
scripts/ios/sim_run.sh --device "$device" --route --card "$card" --card-in-place --out "$out/save" --wait \
    --env BLUEWAKE_CARD_LOG=1 --env BLUEWAKE_SAVE_ROUTE=1 --env BLUEWAKE_DSP_MODE="$dsp_mode" \
    --env BLUEWAKE_MAX_RETRACES="$save_retraces" \
    --env BLUEWAKE_PAD_SCRIPT="$new_game_script" \
    --env BLUEWAKE_CAPTURE_AUDIO_WAV="$out/save/audio.wav" >/dev/null
save_log=$out/save/stderr.log

# The LLE DSP follows the certified route exactly; the high-level DSP (the iOS
# default) shifts mail timing, which moves these two milestones by about 11
# retraces. Accept the certified value, or a bounded shift under HLE.
milestone_ok() { # name certified slack
    local got
    got=$(grep -aoE "$1 retrace=[0-9]+" "$save_log" | head -1 | grep -oE '[0-9]+$' || true)
    [ -n "$got" ] && [ $(( got > $2 ? got - $2 : $2 - got )) -le "$3" ] && echo "$got"
}
slack=0
[ "$dsp_mode" = hle ] && slack=40
if got=$(milestone_ok opening-complete 13850 $slack); then
    ok "route: opening-complete at retrace $got (certified 13850, DSP $dsp_mode)"
else bad 'route: the save half did not follow the route'; fi
if got=$(milestone_ok play-scene 13910 $slack); then
    ok "route: play scene at retrace $got (certified 13910, DSP $dsp_mode)"
else bad 'route: the play scene milestone is off the route'; fi
grep -aq 'save-screen retrace=' "$save_log" \
    && ok 'save: the pause menu opened the save screen' \
    || bad 'save: the save screen never opened'
grep -aqE 'proc=18 ' "$save_log" \
    && ok 'save: the save data was written (proc 18)' \
    || bad 'save: the save chain did not reach the data-save step'
writes=$(grep -aoE 'proc=22 .*card_writes=[0-9]+' "$save_log" | grep -aoE '[0-9]+$' | head -1 || true)
if [ -n "$writes" ] && [ "$writes" -gt 0 ]; then
    ok "save: the card was written ($writes write calls)"
else
    bad 'save: the card write counter never moved'
fi
grep -aq '\[card\] CARDUnmount' "$save_log" \
    && ok "quit: the guest unmounted the card for its own reset" \
    || bad 'quit: no card unmount'
card_diff=$(python3 "$root/scripts/card_container.py" "$route_card" "$card" || true)
if printf '%s\n' "$card_diff" | grep -q "data changed"; then
    ok "card: the saved card differs from the start card ($(printf '%s\n' "$card_diff" | grep -oE '[0-9]+ of [0-9]+ bytes differ' | head -1))"
else
    bad 'card: the card is byte-identical, so nothing was saved'
fi

audio_report=$(python3 - "$out/save/audio.wav" <<'PY'
import array, math, sys, wave
try:
    w = wave.open(sys.argv[1])
except Exception as e:
    print("missing", e); sys.exit(0)
frames = w.getnframes(); rate = w.getframerate(); ch = w.getnchannels()
data = array.array('h', w.readframes(frames))
nonzero = sum(1 for s in data if s)
rms = math.sqrt(sum(s * s for s in data) / max(1, len(data)))
print(f"seconds={frames / rate:.1f} rate={rate} channels={ch} nonzero={nonzero / max(1, len(data)):.3f} rms={rms:.0f}")
PY
)
if printf '%s' "$audio_report" | grep -qE 'nonzero=0\.[0-9]*[1-9]'; then
    ok "audio: the game's own mix was captured ($audio_report)"
else
    bad "audio: no audible capture ($audio_report)"
fi

echo "ipad-acceptance: reload half, $reload_retraces retraces on the card the save wrote"
scripts/ios/sim_run.sh --device "$device" --no-build --route --card "$card" --card-in-place --out "$out/reload" --wait \
    --env BLUEWAKE_CARD_LOG=1 --env BLUEWAKE_SAVE_ROUTE=1 --env BLUEWAKE_DSP_MODE="$dsp_mode" \
    --env BLUEWAKE_MAX_RETRACES="$reload_retraces" \
    --env BLUEWAKE_PAD_SCRIPT="$reload_script" \
    --env BLUEWAKE_PAD_CONFIRM_EVENT=any >/dev/null
reload_log=$out/reload/stderr.log
grep -aq 'play-scene retrace=' "$reload_log" \
    && ok 'reload: the play scene came up' \
    || bad 'reload: the play scene never came up'
grep -aq 'new-game-intro' "$reload_log" \
    && bad 'reload: the run took the new-game path' \
    || ok 'reload: no new-game intro, so the save was loaded'
control=$(grep -aoE 'control-ready retrace=[0-9]+' "$reload_log" | grep -oE '[0-9]+$' | head -1 || true)
if [ -z "$control" ]; then
    bad 'reload: the control predicate never fired'
elif [ "$control" -le "$control_bound" ]; then
    ok "reload: control at retrace $control"
else
    bad "reload: control took $control retraces, over the $control_bound bound"
fi

echo "ipad-acceptance: logs = ${out#$root/}"
if [ "$failures" -eq 0 ]; then
    echo 'ipad-acceptance: PASS'
    exit 0
fi
echo "ipad-acceptance: FAIL - $failures clause(s)" >&2
exit 1
