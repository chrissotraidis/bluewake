#!/usr/bin/env bash
# Profile BlueWake's game thread on a Steam Deck (or any Linux host) with perf.
#
# Automates the procedure in docs/PROFILING.md: launch the game into the heavy
# scene, attach perf for N seconds, and emit a flat self-time report. Two modes:
#
#   headless (default)  BLUEWAKE_RENDERER=headless, unpaced. Reaches the heavy
#                       scene with no display or controller and runs the game
#                       thread flat-out. Produces the game-thread perf profile.
#                       It does NOT produce [fps-dip] / [perf-summary] lines:
#                       an unpaced run runs faster than real time, so the fps
#                       watcher deliberately skips it. FPS numbers come from
#                       --rendered.
#   --rendered          Runs with the real renderer, wall-paced. Needs the
#                       Deck's own desktop session (DISPLAY and, on Wayland,
#                       the Xwayland auth cookie). Its [fps-dip] and
#                       [perf-summary] lines say whether the game thread, the
#                       GX worker, or the GPU is the bottleneck.
#
# Usage:
#   scripts/linux/profile_deck.sh [--seconds 30] [--state PATH] [--out DIR]
#                                 [--appimage PATH] [--rendered] [--hz 99]
#
# Environment (all optional; flags win):
#   BLUEWAKE_APPIMAGE      AppImage or host binary to run
#   BLUEWAKE_STATE         save state to load
#   BLUEWAKE_DATA_DIR      BlueWake data dir (default $XDG_DATA_HOME/BlueWake
#                          or ~/.local/share/BlueWake)
#   BLUEWAKE_RENDERER      backend (headless|aurora); --rendered sets aurora
#   BLUEWAKE_WALL_PACE     pacing (0 = unpaced); default 0 headless, 1 rendered
#   BLUEWAKE_FPS_WATCH     fps watcher (default 1)
#
# Runs on the Deck itself (or any Linux host with the AppImage and a state).
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)

seconds=30
hz=99
state=""
out=""
appimage=""
rendered=0
renderer=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --seconds) seconds=$2; shift 2 ;;
        --hz) hz=$2; shift 2 ;;
        --state) state=$2; shift 2 ;;
        --out) out=$2; shift 2 ;;
        --appimage) appimage=$2; shift 2 ;;
        --rendered) rendered=1; shift ;;
        --renderer) renderer=$2; shift 2 ;;
        -h|--help) sed -n '2,32p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

die() { echo "profile_deck: $*" >&2; exit 1; }

command -v perf >/dev/null 2>&1 || die "perf not found on PATH (the Deck ships it)"

# --- resolve the data dir (where saves, states and session logs live) ---------
if [[ -n "${BLUEWAKE_DATA_DIR:-}" ]]; then
    data_dir="${BLUEWAKE_DATA_DIR%/}/"
elif [[ -n "${XDG_DATA_HOME:-}" ]]; then
    data_dir="${XDG_DATA_HOME%/}/BlueWake/"
else
    data_dir="$HOME/.local/share/BlueWake/"
fi

# --- resolve the save state ---------------------------------------------------
# The state dir is the data dir root (linux_entry.c defaults BLUEWAKE_STATE_DIR
# to the data dir), but older layouts used a states/ subdir, so look in both.
if [[ -z "$state" ]]; then
    state="${BLUEWAKE_STATE:-}"
fi
if [[ -z "$state" ]]; then
    state=$(ls -t "$data_dir"/quick-*.bwstate "$data_dir"/states/quick-*.bwstate 2>/dev/null | head -1 || true)
fi
[[ -n "$state" && -e "$state" ]] || die "no save state: pass --state PATH, or put a quick-*.bwstate under $data_dir"
echo "profile_deck: state = $state"

# The retrace ceiling must exceed the state's already-counted retrace or the
# run stops after 0 blocks. Parse quick-<N>.bwstate for N; fall back to a large
# bound when the name does not carry it.
margin=$((seconds * 200 + 5000))
state_retrace=$(basename "$state" | sed -n 's/^quick-\([0-9]*\)\.bwstate$/\1/p')
if [[ -n "${state_retrace:-}" ]]; then
    max_retraces=$((state_retrace + margin))
else
    max_retraces=100000000
    echo "profile_deck: state name has no retrace; using MAX_RETRACES=$max_retraces"
fi

# --- resolve the AppImage / binary --------------------------------------------
if [[ -z "$appimage" ]]; then
    appimage="${BLUEWAKE_APPIMAGE:-}"
fi
if [[ -z "$appimage" ]]; then
    # Prefer the Deck's SD-card install, then the current directory.
    appimage=$(ls -t /run/media/deck/GF8S5/BlueWake/*.AppImage ./*.AppImage 2>/dev/null | head -1 || true)
fi
[[ -n "$appimage" && -e "$appimage" ]] || die "no AppImage: pass --appimage PATH"
echo "profile_deck: app = $appimage"

# --- output dir ---------------------------------------------------------------
out="${out:-/tmp/bw-profile/$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$out"
run_log="$out/run.log"
perf_data="$out/perf.data"
report="$out/report.txt"

# --- launch -------------------------------------------------------------------
export BLUEWAKE_STATE_DIR="$data_dir"
export BLUEWAKE_LOAD_STATE="$state"
export BLUEWAKE_MAX_RETRACES="$max_retraces"

if [[ $rendered -eq 1 ]]; then
    export BLUEWAKE_RENDERER="${renderer:-aurora}"
    export BLUEWAKE_WALL_PACE="${BLUEWAKE_WALL_PACE:-1}"
    export BLUEWAKE_FPS_WATCH="${BLUEWAKE_FPS_WATCH:-1}"
    [[ -n "${DISPLAY:-}" ]] || die "--rendered needs DISPLAY (run it from the Deck's desktop session)"
else
    export BLUEWAKE_RENDERER="${renderer:-headless}"
    export BLUEWAKE_WALL_PACE="${BLUEWAKE_WALL_PACE:-0}"
    export BLUEWAKE_FPS_WATCH="${BLUEWAKE_FPS_WATCH:-0}"
fi

echo "profile_deck: launching ($BLUEWAKE_RENDERER, wall_pace=$BLUEWAKE_WALL_PACE)"
# setsid + disown survives SSH-session close; plain nohup does not (the
# AppImage's FUSE mount tree is torn down with the session).
setsid bash -c "\"$appimage\" </dev/null >\"$run_log\" 2>&1" &
disown

# --- find the real PID --------------------------------------------------------
# An AppImage unpacks to /tmp/.mount_*/usr/bin/bluewake, whose path changes per
# run, so match that rather than the wrapper.
pid=""
if [[ "$appimage" == *.AppImage ]]; then
    pattern='mount_.*bluewake'
else
    pattern='bluewake'
fi
for _ in $(seq 1 120); do
    pid=$(pgrep -f "$pattern" | head -1 || true)
    [[ -n "${pid:-}" ]] && break
    sleep 1
done
[[ -n "${pid:-}" ]] || { echo "profile_deck: game never started; see $run_log" >&2; exit 1; }
echo "profile_deck: game pid = $pid"

cleanup() {
    # Kill by exact PID, never pkill -f (that matches this script's own shell).
    if [[ -n "${pid:-}" ]] && kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# --- profile ------------------------------------------------------------------
echo "profile_deck: sampling $seconds s at ${hz} Hz"
perf record -g -F "$hz" -p "$pid" -o "$perf_data" -- sleep "$seconds" >/dev/null 2>&1 \
    || die "perf record failed (see $out)"

echo "profile_deck: report -> ${report#$root/}"
perf report -i "$perf_data" --stdio --no-children > "$report"

echo
echo "=== top self-time symbols (first $seconds-second capture) ==="
head -60 "$report"

# --- pull the newest session log for the fps story ----------------------------
newest_log=$(ls -t "$data_dir"/logs/session-*.log 2>/dev/null | head -1 || true)
if [[ -n "${newest_log:-}" ]]; then
    cp "$newest_log" "$out/session.log"
    echo
    echo "=== session log: $newest_log ==="
    if [[ $rendered -eq 1 ]]; then
        grep -a -E "fps-dip|perf-summary" "$out/session.log" | tail -20 || \
            echo "(no fps-dip / perf-summary lines yet)"
    else
        echo "(headless run: unpaced, so the fps watcher skipped it. "
        echo " Re-run with --rendered on the Deck's desktop for [fps-dip] and [perf-summary].)"
    fi
fi

echo
echo "profile_deck: done. artifacts in ${out#$root/}"
echo "  perf.data   raw perf capture (open with 'perf report -i $perf_data')"
echo "  report.txt  flat self-time ranking"
echo "  run.log     the game's own stdout/stderr"
[[ -n "${newest_log:-}" ]] && echo "  session.log copy of $newest_log"
