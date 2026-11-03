#!/usr/bin/env bash
# Run BlueWake in ONE iPad simulator against this repository's prepared data.
#
# usage: scripts/ios/sim_run.sh [--device NAME] [--retraces N] [--route] [--route-full]
#                               [--card PATH] [--screenshot-after SECONDS] [--no-build]
#                               [--env KEY=VALUE]...
# --card starts from a copy of an existing memory card (for example a save in
# Outset) instead of the route's new-game card.
# --out DIR writes logs there; --card-in-place uses --card directly instead of a
# copy (so a save the game writes lands in that file); --wait blocks until the
# app exits and returns its log directory on the last line.
# --fresh reinstalls the app with an empty container holding only the composite
# (a new player's iPad), so the first-run screen appears; --import-disc then
# feeds it this repository's disc through BLUEWAKE_IMPORT_DISC instead of the
# document picker. A fresh run uses the container's own card.
# --container runs against whatever the installed app's container already holds
# (a returning player), adding only the composite if it is missing.
# Output: local-research/ipad/<stamp>/{stderr.log,stdout.log,*.png}
set -euo pipefail
cd "$(dirname "$0")/../.."
root=$PWD
device="iPad Pro 11-inch (M5)"
retraces=""
route=0
route_full=0
shot_after=""
build=1
extra_env=()
card_override=""
out_override=""
card_in_place=0
wait_exit=0
fresh=0
import_disc=0
while [ $# -gt 0 ]; do
    case "$1" in
        --device) device=$2; shift 2 ;;
        --retraces) retraces=$2; shift 2 ;;
        --route) route=1; shift ;;
        --route-full) route=1; route_full=1; shift ;;
        --screenshot-after) shot_after=$2; shift 2 ;;
        --no-build) build=0; shift ;;
        --card) card_override=$2; shift 2 ;;
        --out) out_override=$2; shift 2 ;;
        --card-in-place) card_in_place=1; shift ;;
        --wait) wait_exit=1; shift ;;
        --fresh) fresh=1; shift ;;
        --container) fresh=2; shift ;;
        --import-disc) import_disc=1; shift ;;
        --env) extra_env+=("SIMCTL_CHILD_$2"); shift 2 ;;
        *) echo "sim_run: unknown argument $1" >&2; exit 2 ;;
    esac
done

bundle_id=dev.bluewake.BlueWake
app=$root/build/ios-sim/BlueWake.app
composite_src=${BLUEWAKE_PLAY_COMPOSITE:-$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}
composite=$root/build/ios-sim-composite/gGZLE01_recomp.dylib
card_source=${card_override:-$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card}

if [ $build -eq 1 ]; then
    cmake --build "$root/build/ios-sim" --target BlueWake -j"$(sysctl -n hw.ncpu)" >/dev/null
    codesign -f -s - "$app" >/dev/null 2>&1
fi
# Retag when the source is newer or is a different file than last time (A/B runs).
if [ ! -e "$composite" ] || [ "$composite_src" -nt "$composite" ] ||
   [ "$(cat "$composite.source" 2>/dev/null)" != "$composite_src" ]; then
    mkdir -p "$(dirname "$composite")"
    python3 "$root/scripts/ios/retag_macho_platform.py" --platform iossim --minos 17.0 \
        "$composite_src" "$composite"
    codesign -f -s - "$composite" >/dev/null 2>&1
    printf '%s' "$composite_src" > "$composite.source"
fi

# One simulator at a time: shut down any other booted device first.
udid=$(xcrun simctl list devices available -j | python3 -c '
import json,sys
name=sys.argv[1]
for rt,devs in json.load(sys.stdin)["devices"].items():
    for d in devs:
        if d["name"]==name: print(d["udid"]); sys.exit()
' "$device")
if [ -z "$udid" ]; then echo "sim_run: no simulator named $device" >&2; exit 1; fi
for other in $(xcrun simctl list devices booted -j | python3 -c '
import json,sys
for rt,devs in json.load(sys.stdin)["devices"].items():
    for d in devs:
        if d["state"]=="Booted": print(d["udid"])
'); do
    if [ "$other" != "$udid" ]; then xcrun simctl shutdown "$other"; fi
done
xcrun simctl bootstatus "$udid" -b >/dev/null
open -a Simulator --args -CurrentDeviceUDID "$udid" || true

# One game at a time: no Dolphin or macOS host alongside the simulator.
"$root/scripts/one_game_guard.sh" --ignore-sim
xcrun simctl terminate "$udid" "$bundle_id" >/dev/null 2>&1 || true
if [ "$fresh" -eq 1 ]; then
    xcrun simctl uninstall "$udid" "$bundle_id" >/dev/null 2>&1 || true
fi
xcrun simctl install "$udid" "$app"

out=${out_override:-$root/local-research/ipad/$(date +%Y%m%d-%H%M%S)}
mkdir -p "$out"
if [ "$card_in_place" -eq 1 ]; then
    card=$card_source
else
    card=$out/play.card
    cp "$card_source" "$card"
fi

if [ "$fresh" -ne 0 ]; then
    # Only what a Mac build would embed: the composite. The container's data
    # directory is where the entry shim looks for it in the simulator.
    container=$(xcrun simctl get_app_container "$udid" "$bundle_id" data)
    mkdir -p "$container/Documents/BlueWake"
    if [ "$fresh" -eq 1 ] || ! cmp -s "$composite" "$container/Documents/BlueWake/gGZLE01_recomp.dylib"; then
        cp -c "$composite" "$container/Documents/BlueWake/gGZLE01_recomp.dylib"
    fi
    env_args=(SIMCTL_CHILD_BLUEWAKE_FRESH=1)
    if [ "$import_disc" -eq 1 ]; then
        env_args+=(SIMCTL_CHILD_BLUEWAKE_IMPORT_DISC="$root/ref/The Legend Of Zelda The Wind Waker.iso")
    fi
    echo "sim_run: container $container"
else
env_args=(
    SIMCTL_CHILD_BLUEWAKE_ROOT="$root"
    SIMCTL_CHILD_BLUEWAKE_COMPOSITE="$composite"
    SIMCTL_CHILD_BLUEWAKE_DOL="$root/generated/full/main.dol"
    SIMCTL_CHILD_BLUEWAKE_RELS_DIR="$root/generated/full/rels"
    SIMCTL_CHILD_BLUEWAKE_DISC="$root/ref/The Legend Of Zelda The Wind Waker.iso"
    SIMCTL_CHILD_BLUEWAKE_DSP_IROM="$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin"
    SIMCTL_CHILD_BLUEWAKE_DSP_COEF="$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin"
    SIMCTL_CHILD_BLUEWAKE_CARD_PATH="$card"
)
fi
if [ -n "$retraces" ]; then env_args+=(SIMCTL_CHILD_BLUEWAKE_MAX_RETRACES="$retraces"); fi
if [ ${#extra_env[@]} -gt 0 ]; then env_args+=("${extra_env[@]}"); fi
if [ $route -eq 1 ]; then
    env_args+=(SIMCTL_CHILD_BLUEWAKE_PAD_BUTTONS=0x0100
               SIMCTL_CHILD_BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1
               SIMCTL_CHILD_BLUEWAKE_PAD_PULSE_LENGTH=2)
fi
if [ $route_full -eq 1 ]; then
    script=""
    press=17800
    while [ $press -le 20400 ]; do
        script="${script:+$script,}${press}:0x0100:2"; press=$((press + 150))
    done
    env_args+=(SIMCTL_CHILD_BLUEWAKE_PAD_SCRIPT="$script" SIMCTL_CHILD_BLUEWAKE_PLAYER_PROBE=1)
fi

env "${env_args[@]}" xcrun simctl launch --terminate-running-process \
    --stdout="$out/stdout.log" --stderr="$out/stderr.log" "$udid" "$bundle_id"
echo "sim_run: device=$device udid=$udid"
echo "sim_run: logs=$out"
if [ -n "$shot_after" ]; then
    sleep "$shot_after"
    xcrun simctl io "$udid" screenshot "$out/shot-${shot_after}s.png" >/dev/null 2>&1 || true
    echo "sim_run: screenshot=$out/shot-${shot_after}s.png"
fi
if [ "$wait_exit" -eq 1 ]; then
    # The simulator's app is an ordinary host process under this device's tree.
    sleep 3
    while pgrep -f "$udid/data/Containers/Bundle/Application/.*/BlueWake.app/BlueWake" >/dev/null 2>&1; do
        sleep 5
    done
    echo "sim_run: exited"
fi
