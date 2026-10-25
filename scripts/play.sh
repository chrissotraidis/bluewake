#!/usr/bin/env bash
# BlueWake: launch the game the way a human plays it.
#
# M1 of docs/archive/GOAL_PROMPT_2026-09-14.md. Needs no exported variables: it finds
# the repository from its own location, opens the Aurora window, plays audio
# through the default output device, and merges live keyboard and pad input.
#
# Keyboard, Aurora port-0 fallback bindings (a physical pad also works):
#   arrows  D-pad            J  A       K  B       U  X       I  Y
#   W/A/S/D left stick       H/F/T/G  C-stick
#   E/R     L/R triggers     Q  Z       RETURN  START
#
# Usage:
#   scripts/play.sh                  play from a writable copy of the route card
#   scripts/play.sh --save           play from your persistent save slot
#   scripts/play.sh --retraces 700   bounded smoke launch
#   scripts/play.sh --capture-wav F  also record the paced PCM to F
#   scripts/play.sh --route          unattended: synthetic A press to Outset
#   scripts/play.sh --route-full     unattended all the way to controllable
#                                    gameplay, cutscene included
#   scripts/play.sh --capture-frame F --capture-retrace N
#                                    screenshot the window at retrace N to F
#   scripts/play.sh --timing         report fps/RSS/digest for this playback
#   scripts/play.sh --bundle         launch the app bundle with an empty env
#   scripts/play.sh --headless       no window, for machines with no display
#   scripts/play.sh --check          verify every input, launch nothing

set -eo pipefail

cd "$(dirname "$0")/.."
root=$PWD

retraces=""
use_save_slot=0
capture_wav=""
capture_frame=""
capture_retrace=""
route=0
route_full=0
timing=0
window="${BLUEWAKE_PLAY_WINDOW:-13910:14100}"
bundle=0
renderer=aurora
check_only=0
card_override=""

while [ $# -gt 0 ]; do
    case "$1" in
        --retraces)
            if [ $# -lt 2 ]; then echo "play: --retraces needs a value" >&2; exit 2; fi
            retraces=$2; shift 2 ;;
        --card)
            if [ $# -lt 2 ]; then echo "play: --card needs a path" >&2; exit 2; fi
            card_override=$2; shift 2 ;;
        --capture-wav)
            if [ $# -lt 2 ]; then echo "play: --capture-wav needs a path" >&2; exit 2; fi
            capture_wav=$2; shift 2 ;;
        --capture-frame)
            if [ $# -lt 2 ]; then echo "play: --capture-frame needs a path" >&2; exit 2; fi
            capture_frame=$2; shift 2 ;;
        --capture-retrace)
            if [ $# -lt 2 ]; then echo "play: --capture-retrace needs a number" >&2; exit 2; fi
            capture_retrace=$2; shift 2 ;;
        --route) route=1; shift ;;
        --route-full) route=1; route_full=1; shift ;;
        --timing) timing=1; shift ;;
        --window)
            if [ $# -lt 2 ]; then echo "play: --window needs START:END" >&2; exit 2; fi
            window=$2; shift 2 ;;
        --save) use_save_slot=1; shift ;;
        --bundle) bundle=1; shift ;;
        --headless) renderer=headless; shift ;;
        --check) check_only=1; shift ;;
        -h|--help) sed -n "2,32p" "$0"; exit 0 ;;
        *) echo "play: unknown argument: $1" >&2; exit 2 ;;
    esac
done

# The authored `awake` cutscene - event 38 in the guest's own event table - is
# the one stretch the latched new-game schedules cannot walk. It confirms nine
# text pages between retrace 17,868 and 19,773, and the guest only admits
# control at retrace 20,256 (measured; [player-milestone] control-admitted).
# That is what BLUEWAKE_PAD_SCRIPT exists for. A press every 150 retraces from
# 17,800 covers the pages with room on both sides, and the guest ignores A when
# no prompt is up, so an early or late press is harmless.
full_script=""
if [ $route_full -eq 1 ]; then
    press=17800
    while [ $press -le 20400 ]; do
        full_script="${full_script:+$full_script,}${press}:0x0100:2"
        press=$((press + 150))
    done
fi

if [ -n "$capture_frame" ] && [ -z "$capture_retrace" ]; then
    echo "play: --capture-frame needs --capture-retrace" >&2; exit 2
fi
if [ -n "$capture_frame" ] && [ $renderer = headless ]; then
    echo "play: --capture-frame only works with the window (drop --headless)" >&2; exit 2
fi

host=${BLUEWAKE_PLAY_HOST:-$root/build/runtime-host-dsp/bluewake_host}
composite=${BLUEWAKE_PLAY_COMPOSITE:-$root/build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib}
card_source=${BLUEWAKE_PLAY_CARD:-$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card}
dol=$root/generated/full/main.dol
rels=$root/generated/full/rels
disc="$root/ref/The Legend Of Zelda The Wind Waker.iso"
dsp_irom=$root/ref/recompcore/Data/Sys/GC/dsp_rom.bin
dsp_coef=$root/ref/recompcore/Data/Sys/GC/dsp_coef.bin
app_binary=$root/build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake

missing=0
require_file() {
    if [ ! -e "$2" ]; then
        echo "play: missing required input ($1): $2" >&2
        missing=1
    fi
}
require_file host "$host"
require_file composite "$composite"
require_file dol "$dol"
require_file disc "$disc"
require_file dsp_irom "$dsp_irom"
require_file dsp_coef "$dsp_coef"
if [ ! -d "$rels" ]; then
    echo "play: missing required REL directory: $rels" >&2
    missing=1
fi

if [ $missing -ne 0 ]; then
    echo "play: cannot launch until the inputs above exist." >&2
    echo "play: route recipe: local-research/evidence/outset-performance-route-v1-20260901/README.md" >&2
    exit 1
fi

if [ $check_only -ne 0 ]; then
    echo "play: all required inputs are present."
    echo "play: host      = $host"
    echo "play: composite = $composite"
    exit 0
fi

# Match the executable name, not the whole command line. `pgrep -f` matches any
# process whose command line mentions the string, which includes the shell that
# happens to be running a command about bluewake_host - a harness, a monitoring
# script, or an agent's own wrapper - so this guard refused to launch three times
# in a row for a host that did not exist. `-x` compares the process name only.
if pgrep -x bluewake_host >/dev/null 2>&1; then
    echo "play: a bluewake_host process is already running; refusing to overlap" >&2
    exit 1
fi

card=""
if [ $use_save_slot -eq 0 ]; then
    if [ -n "$card_override" ]; then
        card=$card_override
    elif [ -e "$card_source" ]; then
        play_dir=$root/local-research/play/$(date +%Y%m%d-%H%M%S)
        mkdir -p "$play_dir"
        card=$play_dir/play.card
        cp "$card_source" "$card"
    else
        echo "play: route card not found; falling back to your save slot" >&2
    fi
fi

echo "play: root     = $root"
echo "play: renderer = $renderer"
if [ -n "$card" ]; then
    echo "play: card     = $card"
else
    echo "play: card     = user save slot (~/Library/Application Support/BlueWake)"
fi
if [ -n "$retraces" ]; then echo "play: retraces = $retraces"; fi
if [ -n "$capture_wav" ]; then echo "play: wav      = $capture_wav"; fi
if [ -n "$capture_frame" ]; then
    echo "play: frame    = $capture_frame (retrace $capture_retrace)"
fi
if [ $route -eq 1 ]; then
    echo "play: route    = synthetic A press at title; unattended run to Outset"
fi
if [ $route_full -eq 1 ]; then
    echo "play: route    = full; unattended past the awake cutscene to control"
fi
if [ $timing -eq 1 ]; then
    echo "play: timing   = reporting fps/RSS/digest over window $window"
fi
echo "play: keys     = arrows D-pad, J=A K=B U=X I=Y, WASD stick, RETURN=start"
echo "play: the title screen wants an A press (J) to reach the file menu."

if [ $bundle -eq 1 ]; then
    echo "play: launching the app bundle with an empty environment"
    # The point of this path is that nothing but HOME survives: the app must
    # find the repository, the composite, the disc and the DSP ROMs by walking
    # up from its own executable. Only user-facing choices are passed through.
    bundle_env=(HOME="$HOME")
    if [ -n "$card" ]; then bundle_env+=(BLUEWAKE_CARD_PATH="$card"); fi
    if [ -n "$retraces" ]; then bundle_env+=(BLUEWAKE_MAX_RETRACES="$retraces"); fi
    if [ -n "$capture_wav" ]; then bundle_env+=(BLUEWAKE_CAPTURE_AUDIO_WAV="$capture_wav"); fi
    if [ -n "$capture_frame" ]; then
        bundle_env+=(BLUEWAKE_CAPTURE_OPENING_FRAME="$capture_frame")
        bundle_env+=(BLUEWAKE_CAPTURE_RETRACE="$capture_retrace")
    fi
    if [ $route -eq 1 ]; then
        bundle_env+=(BLUEWAKE_PAD_BUTTONS=0x0100)
        bundle_env+=(BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1)
        bundle_env+=(BLUEWAKE_PAD_PULSE_LENGTH=2)
    fi
    if [ -n "$full_script" ]; then
        bundle_env+=(BLUEWAKE_PAD_SCRIPT="$full_script")
        bundle_env+=(BLUEWAKE_PLAYER_PROBE=1)
    fi
    if [ $renderer != aurora ]; then bundle_env+=(BLUEWAKE_RENDERER="$renderer"); fi
    exec env -i "${bundle_env[@]}" "$app_binary"
fi

export BLUEWAKE_ROOT="$root"
export BLUEWAKE_RENDERER="$renderer"
# The same cap the benchmark now ships as its default, with the pair that
# adopted it recorded in docs/archive/GOAL_PROMPT_V16_2026-09-14.md.
# This used to pin `dynamic`, the policy commit 8da70d2 adopted and commit
# be84bdf superseded. The runtime's own default moved to a fixed 16384 window
# (docs/archive/status/CURRENT.md, "the cycle window is a free lever"), so pinning
# `dynamic` here made the human play path slower than a double-click of the
# same app: play-window median 31.38 fps against 33.55 and 87.56 s against
# 70.68 s on the route, digest UNCHANGED either way. Aligned with the benchmark
# and the runtime, and still overridable for a regression.
export BLUEWAKE_CYCLE_CAP="${BLUEWAKE_CYCLE_CAP:-16384}"
export BLUEWAKE_MAX_BLOCKS=100000000000
export BLUEWAKE_DOL="$dol"
export BLUEWAKE_DISC="$disc"
export BLUEWAKE_RELS_DIR="$rels"
export BLUEWAKE_DSP_IROM="$dsp_irom"
export BLUEWAKE_DSP_COEF="$dsp_coef"
if [ -n "$card" ]; then export BLUEWAKE_CARD_PATH="$card"; fi
if [ -n "$retraces" ]; then export BLUEWAKE_MAX_RETRACES="$retraces"; fi
if [ -n "$capture_wav" ]; then export BLUEWAKE_CAPTURE_AUDIO_WAV="$capture_wav"; fi
if [ -n "$capture_frame" ]; then
    export BLUEWAKE_CAPTURE_OPENING_FRAME="$capture_frame"
    export BLUEWAKE_CAPTURE_RETRACE="$capture_retrace"
fi
# The route is the same synthetic input the benchmark uses to walk from the
# title to controllable Outset: the game is played by the same code a human
# drives, only the button comes from the schedule instead of the keyboard.
if [ $route -eq 1 ]; then
    export BLUEWAKE_PAD_BUTTONS=0x0100
    export BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1
    export BLUEWAKE_PAD_PULSE_LENGTH=2
fi
if [ -n "$full_script" ]; then
    export BLUEWAKE_PAD_SCRIPT="$full_script"
    export BLUEWAKE_PLAYER_PROBE=1
fi
if [ $timing -eq 1 ]; then
    export BLUEWAKE_FRAME_TIMING=1
    stamp=$(date +%Y%m%d-%H%M%S)
    play_out="$root/local-research/play/$stamp"
    mkdir -p "$play_out"
    log="$play_out/play.log"
    echo "play: log      = "${play_out#$root/}"/play.log"
    echo "play: running..."
    /usr/bin/time -l "$host" "$composite" >"$log" 2>&1
    if [ -n "$card" ]; then
        report_card=$card
    else
        report_card=$card_source
    fi
    # The D2 baseline, not D1's. This default was left on D1's value
    # 0d4cee87... when decision D2 re-derived the route digest, and D1's value
    # is reachable only with the DSP idle skip switched off, so every rendered
    # `--timing` run reported "route digest diverged" against a digest it could
    # never have produced. Same value scripts/bench.sh pins.
    python3 "$root/scripts/bench_report.py" \
        --log "$log" \
        --card "$report_card" \
        --retraces "${retraces:-14100}" \
        --window "$window" \
        --baseline-digest "${BLUEWAKE_PLAY_BASELINE_DIGEST:-92dd816c6a382531d597c8da0cc3e447c716aaa8240912d10a78bee853478782}" \
        --baseline-retraces 14100
    exit $?
fi

exec "$host" "$composite"
