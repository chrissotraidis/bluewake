#!/usr/bin/env bash
# BlueWake app acceptance test: the terminal condition, driven by real keys.
#
# Launches the real app bundle the way a double-click does - an empty
# environment where only HOME survives - then presses real OS-delivered keys
# into that process's window through /tmp/bwfocus and /tmp/bwkey. No synthetic
# pad button is armed anywhere here, so every button the guest sees comes from a
# real key press, and a press only counts when the guest's own controller record
# shows it: [input-chain] cpad_trig, read from 0x803A4E22.
#
# A landed press is necessary but not sufficient, and that difference is why this
# test failed a clause while the app was working. The title's confirm and the
# name-entry screens poll the pad, so a press delivered between polls is recorded
# by the guest and ignored by it. The route's own log says so - the A pulse that
# moves the title is the second one, at title-confirm-ready retrace=386, not the
# one at title-ready retrace=334 - and the negative control at /tmp/bw-live/kb1.log
# records a landed press at retrace=459 after which the title never moved. A
# person in that position taps again. So each driver below taps a real key until
# the guest's own milestone reports, and every tap still has to land.
#
# Two further false failures were chased down before this test could be trusted,
# and both are fixed here rather than worked around:
#   * the guest's own pad record does not use the wire button mask, so a real
#     RETURN lands as cpad_trig=0x0010 and not 0x1000 (see the counters below);
#   * name-input-complete used to be reported only while the route's virtual
#     start pulse was armed, so no live run could ever observe it. The runtime
#     now reports the name-scene milestones from guest state whether or not a
#     route pulse exists, and the walk accepts either that milestone or the
#     guest opening the new-game intro, which subsumes it.
#
# What the default (game) pass proves, in order:
#   1. the app opens with no exported variables and reaches the title screen;
#   2. real J presses reach the guest as cpad_trig=0x0100 and its own title poll
#      takes one, reporting boot-milestone file-select;
#   3. real keys walk the new game - A through the file menu until the guest's
#      own name-entry grid is up, then A to type a character, RETURN to move the
#      guest's own selection to the END cell, and A to confirm - each phase
#      waiting for the guest's own screen state rather than a press count, until
#      the guest reports name-input-complete or opens the new-game intro;
#   4. the intro loads, runs out, and play-scene reports, with the wall time from
#      launch to play-scene read out of the host's own frame-timing stamps;
#   5. a real held W at Outset moves the guest's own player position, so the drive
#      is a measured displacement and not a claimed one, and the guest's stick
#      word shows the stick arrived;
#   6. a real RETURN and a real left arrow are delivered at Outset;
#   7. the run stops normally at the retrace ceiling with no crash, the graphics
#      core shut down with zero rejects and zero failures, and the LLE DSP
#      produced nonzero audio.
#
# The screenshot of Outset under control comes from BLUEWAKE_CAPTURE_PLAYER_READY,
# which fires on the retrace Link leaves the opening cutscene's demo state. Every
# PPM stays in /tmp.
#
# The bar this pass is read against needs a decision, and the arithmetic says so:
# Link takes control at retrace 19,972, which at the console's authentic 60 Hz is
# 332.9 s of authored content. A paced application cannot present content in less
# wall time than it takes to play at any emulator speed, so the cold new game
# cannot meet a five-minute launch-to-interactive bar and never could. The PRD
# forbids a save-state warm start but not continuing from a save the game itself
# wrote, and that path is what fits. See docs/archive/status/PLAN_2026-09-18.md (M2).
#
# The --menu pass is the cheap front half of the same thing: a shorter run with a
# picture series, for the double-click-to-file-menu clause alone.
#
# Usage: scripts/app_acceptance_test.sh [--menu] [--retraces N] [--frames DIR]
#                                       [--card-source PATH]
#
# The run is given a *copy* of a card, never the user's own slot. The default
# source is the canonical route card, which is the empty card the documented
# route uses, so the walk takes the new-game path to Outset. The user's own card
# is hashed before and after and has to come back byte-identical.
#
# Requires: build/runtime-host-dsp/BlueWake.app, the route card at
# local-research/evidence/outset-performance-route-v1-20260901/run1.card, and the
# user's own card at ~/Library/Application Support/BlueWake/GZLE01.card.

set -u
cd "$(dirname "$0")/.."
root=$PWD

mode=game
retraces=""
frames=""
card_source=""
while [ $# -gt 0 ]; do
    case "$1" in
        --menu) mode=menu; shift ;;
        --retraces) retraces=$2; shift 2 ;;
        --frames) frames=$2; shift 2 ;;
        --card-source) card_source=$2; shift 2 ;;
        -h|--help) sed -n '2,47p' "$0"; exit 0 ;;
        *) echo "acceptance: unknown argument: $1" >&2; exit 2 ;;
    esac
done
if [ -z "$retraces" ]; then
    # The menu pass only needs the title and the file menu; the game pass needs
    # play-scene (retrace 13,910 on the route), then the authored Outset `awake`
    # cutscene, which the guest only leaves when its own cutscene-text pages are
    # confirmed with A. The route's own run of the same card confirms nine
    # pages, the first at retrace 17,868 and the last at 19,773, and Link takes
    # control at 19,972 - so a ceiling below about 20,100 stops the run with the
    # player still frozen, which is exactly the false failure this pass was
    # reporting. This host renders the play scene at about 30 ms per retrace
    # (median 33.6 fps, p99 77.6 ms) - the 120 ms figure that used to stand here
    # was measured before W1 and W4 and was wrong by 4x, which is part of why
    # the rendered path went a whole workstream without being profiled; see
    # docs/archive/status/PLAN_2026-09-18.md. The ceiling is still the run's tail: the
    # play scene costs about 30 ms a retrace, so 22,000 leaves time after the W
    # hold and the arrow for the run to stop normally.
    # v19 spent retraces 19,700-22,000 still inside the cutscene because the
    # driver stopped feeding it A, so the ceiling now sits at 24,000: control
    # lands near 20,100 and the tail clauses plus a normal stop need the room.
    if [ "$mode" = menu ]; then retraces=3000; else retraces=24000; fi
fi
if [ -z "$frames" ]; then
    frames=/tmp/bw-acceptance/frames-$mode-$(date +%H%M%S)
fi

failures=0
note() { printf 'acceptance: %s\n' "$*"; }
bad() { printf 'acceptance: FAIL %s\n' "$*"; failures=$((failures + 1)); }

focus=/tmp/bwfocus
key=/tmp/bwkey

# The focus and key helpers are our own code and their sources are in this
# repository, so a clean checkout can rebuild them. They used to exist only as
# binaries under /tmp, which meant a reboot could leave this whole test unable
# to run with no way to rebuild it from the tree.
if [ ! -x "$focus" ]; then
    cc -O2 -o "$focus" "$root/scripts/helpers/bwfocus.m" -framework AppKit ||
        { echo "acceptance: cannot build the focus helper $focus" >&2; exit 1; }
fi
if [ ! -x "$key" ]; then
    cc -O2 -o "$key" "$root/scripts/helpers/bwkey.c" -framework ApplicationServices ||
        { echo "acceptance: cannot build the key helper $key" >&2; exit 1; }
fi
for tool in "$focus" "$key"; do
    if [ ! -x "$tool" ]; then
        echo "acceptance: missing input helper $tool" >&2
        exit 1
    fi
done

bundle=$root/build/runtime-host-dsp/BlueWake.app
app=$bundle/Contents/MacOS/BlueWake
if [ ! -x "$app" ]; then
    echo "acceptance: missing app binary $app" >&2
    exit 1
fi

# Executable names, not command lines: `pgrep -f` also matches the shell that is
# running a command mentioning the string, so this guard fired for a shell.
if pgrep -x 'BlueWake|bluewake_host' >/dev/null 2>&1; then
    echo "acceptance: a BlueWake process is already running; refusing to overlap" >&2
    exit 1
fi

if [ -z "$card_source" ]; then
    card_source=$root/local-research/evidence/outset-performance-route-v1-20260901/run1.card
fi
if [ ! -f "$card_source" ]; then
    echo "acceptance: missing the card source $card_source" >&2
    exit 1
fi

user_card="$HOME/Library/Application Support/BlueWake/GZLE01.card"
if [ ! -f "$user_card" ]; then
    echo "acceptance: missing the user card $user_card" >&2
    exit 1
fi
user_card_before=$(shasum -a 256 "$user_card" | awk '{print $1}')

mkdir -p "$frames" /tmp/bw-acceptance
card=/tmp/bw-acceptance/save.card
cp "$card_source" "$card"
note "card copy $card from ${card_source#$root/} sha256 $(shasum -a 256 "$card" | awk '{print $1}')"
note "the user's own slot sha256 $user_card_before is never opened by this run"

log=/tmp/bw-acceptance/run.log
input_log=/tmp/bw-acceptance/input.log
: >"$log"
: >"$input_log"

# The double-click path, and it has to be this one: `open` launches a real
# bundle the way Finder does, and a bundle is what macOS lets take keyboard
# focus. A bare binary run as a background job comes up with input_focus=false
# and no key press ever reaches the guest - that is how this test found out.
# The environment is empty apart from HOME and the values below, so the app still
# has to find the repository, the composite, the disc and the DSP ROMs by walking
# up from its own executable. The card is a copy, so a save this test writes
# cannot touch the user's own slot.
env_args=(--env HOME="$HOME"
    --env BLUEWAKE_CARD_PATH="$card"
    --env BLUEWAKE_MAX_RETRACES="$retraces"
    --env BLUEWAKE_FRAME_TIMING=1
    --env BLUEWAKE_INPUT_PROBE=1
    --env BLUEWAKE_TRACE_PLAYER=1
    --env BLUEWAKE_TRACE_PAD=1)
if [ "$mode" = menu ]; then
    env_args+=(--env BLUEWAKE_CAPTURE_OPENING_FRAME="$frames/menu"
        --env BLUEWAKE_CAPTURE_RETRACE=300
        --env BLUEWAKE_CAPTURE_INTERVAL=150)
else
    env_args+=(--env BLUEWAKE_CAPTURE_OPENING_FRAME="$frames/outset"
        --env BLUEWAKE_CAPTURE_PLAYER_READY=1
        --env BLUEWAKE_PLAYER_PROBE=1
        --env BLUEWAKE_TRACE_ROOM0=1)
fi

# Perf work has to vary the dispatch granularity and read the deadline census
# without editing this file, so both pass through when the caller exports them.
# Neither is set by default and both are inert when unset, which keeps an
# ordinary acceptance run byte-identical to before this passthrough existed.
if [ -n "${BLUEWAKE_CYCLE_CAP:-}" ]; then
    env_args+=(--env BLUEWAKE_CYCLE_CAP="$BLUEWAKE_CYCLE_CAP")
fi
if [ -n "${BLUEWAKE_DEADLINE_CENSUS:-}" ]; then
    env_args+=(--env BLUEWAKE_DEADLINE_CENSUS=1)
    if [ -n "${BLUEWAKE_DEADLINE_CENSUS_WINDOW:-}" ]; then
        env_args+=(--env BLUEWAKE_DEADLINE_CENSUS_WINDOW="$BLUEWAKE_DEADLINE_CENSUS_WINDOW")
    fi
fi
if [ -n "${BLUEWAKE_CHASSIS_BUDGET:-}" ]; then
    env_args+=(--env BLUEWAKE_CHASSIS_BUDGET=1)
fi

env -i HOME="$HOME" /usr/bin/open -n \
    --stdout "$log" --stderr "$log" \
    "${env_args[@]}" \
    "$bundle"

# `open` reaps its child when the launching shell exits, so this shell has to
# stay alive for the whole run, and it has to take the app down with it.
pid=""
i=0
while [ "$i" -lt 40 ]; do
    pid=$(pgrep -x BlueWake | head -1)
    [ -n "$pid" ] && break
    sleep 0.5
    i=$((i + 1))
done
if [ -z "$pid" ]; then
    echo "acceptance: the bundle never came up" >&2
    exit 1
fi
cleanup() {
    kill -TERM "$pid" 2>/dev/null || true
}

# Preflight: every clause below lands keys, and a key can only land if the app
# holds the key window. On macOS 14 and later a helper process cannot force
# that while another application is being used - activateFromApplication: is
# refused, and the deprecated ignoringOtherApps flag has had no effect since
# macOS 14 - so ask once and say so plainly. A host that cannot give focus
# should fail here in seconds with an environmental verdict, rather than
# failing every press for fifteen minutes and reading like a product failure.
if ! "$focus" "$pid" 600 >/dev/null 2>&1; then
    echo "acceptance: the window server will not give pid $pid the key window, so no synthetic key can land (another application holds focus; this host has to be idle for the key-driven pass)" >&2
    cleanup
    exit 3
fi
trap cleanup EXIT
# A run killed from outside has to take the app with it. Without this the app is
# orphaned and keeps a core busy, and an orphaned app silently corrupts the next
# measurement: the benchmark only refused to overlap a bare bluewake_host
# process, so a surviving app from a killed run was not caught and the numbers
# from those runs read about 60% of real speed instead of the true 88%.
trap 'cleanup; exit 143' TERM INT
note "$mode pass: app pid $pid, log $log, ceiling $retraces retraces"

wait_for() {
    pattern=$1
    limit=$2
    i=0
    while [ "$i" -lt "$limit" ]; do
        if grep -qE "$pattern" "$log" 2>/dev/null; then
            note "saw [$pattern] after $((i / 10)) s"
            return 0
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            note "the process exited while waiting for [$pattern]"
            return 1
        fi
        sleep 0.1
        i=$((i + 1))
    done
    note "timeout after $((limit / 10)) s waiting for [$pattern]"
    return 1
}

# The guest's own trigger word, on change, from the running app. Two button
# layouts are in play and they disagree, and that disagreement is what made this
# test report a false failure for a press that had in fact landed:
#   * on the wire A is 0x0100 and START is 0x1000
#     (ref/recompcore/GXRuntime/graphics/aurora/include/dolphin/pad.h), which is
#     the mask the pad probe prints as port0_button and the same 0x1000 the
#     [input-chain] line shows as jut_trig;
#   * in the guest's own GZLE01 cpad record at 0x803A4E20, which the runtime
#     decodes itself (runtime/host/src/main.c), A is 0x0100 and START is 0x0010:
#     host_guest_cpad_a_released reads 0x803A4E20 & 0x01 and
#     host_guest_cpad_start_released reads 0x803A4E21 & 0x10. So a real RETURN
#     leaves the pad layer as 0x1000 and lands in the guest as cpad_trig=0x0010.
# A landed press adds at least one such line for the bit it carries.
count_a() { grep -cE 'input-chain.*cpad_trig=0x0100' "$log" || true; }
count_start() { grep -cE 'input-chain.*cpad_trig=0x0010' "$log" || true; }
count_any() { grep -cE 'input-chain.*cpad_trig=0x(0100|0010)' "$log" || true; }
# The guest's own D-pad bit for down. Note the mask is not the wire's: the pad
# layer reports the D-pad down as jut_trig=0x0004 while the guest's own cpad
# record carries 0x2000 for the same press, exactly the kind of disagreement this
# script already documents for A (0x0100 both) and START (0x1000 wire against
# 0x0010 guest). Counting the wire bit found zero presses while four had in fact
# landed, which is what made this look like a lost press.
count_down() { grep -cE 'input-chain.*cpad_trig=0x2000' "$log" || true; }

seen() { grep -qE "$1" "$log" 2>/dev/null; }

send() {
    "$focus" "$pid" 300 >>"$input_log" 2>&1
    "$key" "$pid" "$1" "$2" >>"$input_log" 2>&1
}

# One real press. It returns 0 only when the guest's own controller record shows
# a new trigger, so a press that never reached the guest is a failure here and
# can never be counted as a pass.
#
# A press is retried, each retry re-activating the window first and holding
# longer than the last. This is not leniency: the counter still has to increase
# or the press is a failure. It is there because a pass is affected by other
# work on the same machine. v25 lost six real presses in a row at the guest's
# own name-entry grid - the app still had SDL keyboard focus throughout
# (flags=0x20002620, INPUT_FOCUS set) and answered a hand-sent J seconds later -
# while an unrelated project's iOS Simulator UI test was launching a Simulator
# window, which took the key window and swallowed the delivered keys. One lost
# press aborted the whole walk. A person in that position taps again, and so
# does this.
tap() {
    label=$1
    code=$2
    hold=$3
    counter=$4
    before=$(eval "$counter")
    attempt=1
    while [ "$attempt" -le 4 ]; do
        send "$code" "$((hold * attempt))"
        sleep 0.3
        after=$(eval "$counter")
        if [ "$after" -gt "$before" ]; then
            if [ "$attempt" -eq 1 ]; then
                note "press landed ($label): $counter $before -> $after"
            else
                note "press landed on retry $attempt ($label): $counter $before -> $after"
            fi
            return 0
        fi
        attempt=$((attempt + 1))
    done
    bad "the real key press ($label) never reached the guest controller record after 4 real sends"
    return 1
}

# Tap a real key until the guest reports `pattern`, up to `attempts` taps, with a
# short gap between taps because that is the spacing the title took in the run
# that retired this clause (thirteen real presses, one every 0.32 s). A tap that
# does not land stops the driver with a failure, so a driver can never pass by
# pressing nothing.
drive() {
    pattern=$1
    label=$2
    code=$3
    hold=$4
    counter=$5
    attempts=$6
    n=1
    while [ "$n" -le "$attempts" ]; do
        if seen "$pattern"; then
            note "the guest reported [$pattern] after $((n - 1)) real press(es), $label"
            return 0
        fi
        tap "$label press $n" "$code" "$hold" "$counter" || return 1
        j=0
        while [ "$j" -lt 3 ]; do
            seen "$pattern" && break
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.1
            j=$((j + 1))
        done
        n=$((n + 1))
    done
    if seen "$pattern"; then
        note "the guest reported [$pattern] after $((n - 1)) real press(es), $label"
        return 0
    fi
    bad "the guest never reported [$pattern] after $attempts real presses ($label)"
    return 1
}

# The new-file walk is driven by the guest's own screen, not by a press order.
# This is not a stylistic choice, it is the finding that cost two runs: the walk
# used to press New Game, begin-the-file, type-a-character, RETURN, confirm in a
# fixed order and repeat the whole sequence if a round did not take. On
# 2026-09-14 two rounds pressed all five buttons, every press landed in the
# guest's own controller record, and the guest still reported nothing. The
# runtime's [name-scene-state] probe, which reads the same five values the
# route's name driver gates on, says why: the A pressed as "type a character"
# arrived while the guest's active proc was still 5, so it was recorded and
# ignored, the guest reached its name screen with an empty name, and an empty
# name is never accepted. From there the guest sat at sel_menu=4 sel_proc=1|2
# with cur_pos=0 for the rest of both rounds, toggling on every press, while the
# five-press loop kept pressing blind. A press order is only a driver when the
# screen it assumes already exists.
#
# So each phase below waits for the guest to say it is ready, and a phase that
# cannot advance fails and reports the state it saw instead of pressing harder.
# The road map is the route's own gate order, read from its recorded log
# (selected slot 576, new file 624, character 684, name end 691, confirm 696,
# name-input-complete 712) and from the source it drives:
#   * the name-entry screen is active proc 7 - main_proc=7;
#   * the letter grid is showing when sel_proc=0, and A types the character the
#     cursor is on, moving cur_pos off 0;
#   * START (RETURN on the keyboard, wire 0x1000, guest 0x0010) is what the
#     route calls "name end": it moves the guest's own selection to the row that
#     holds the END button, sel_menu=4;
#   * A on the END cell is the confirm, and the guest reports name-input-complete
#     or opens the new-game intro.

# The guest's own name-entry state, parsed out of [name-scene-state]:
# "main_proc sel_proc sel_menu cur_pos name_done", or empty if the probe has not
# printed yet. The probe is gated by BLUEWAKE_INPUT_PROBE, which this run sets.
name_field() {
    grep -E '^\[name-scene-state\]' "$log" | tail -1 |
        sed -E 's/^.* main_proc=([0-9]+) sel_proc=([0-9]+) sel_menu=([0-9]+) cur_pos=([0-9]+) name_done=([0-9]+) .*$/\1 \2 \3 \4 \5/'
}

# The guest's own player record, parsed out of [player-scene-state]. This is the
# only observation point for the product's second half. Every earlier attempt
# read the player through a CPU hook at 0x80122D30, and that hook has never
# fired in any run, this project's own route included -- so "Link is under your
# control" has never actually been observed, and the one capture that claimed to
# show it (evidence/awake-action-capture-v4-20260901) fired at retrace 337, two
# retraces after the title screen, with the picture to match. The probe below
# reads the player pointer directly, so it needs no hook and cannot be skipped.
# Field order: demo_type demo_mode proc event_mode event msg pad_hold x y z
# This used to be one sed -E whose tenth field was written \10, and BSD sed
# reads that as group 1 followed by a literal 0: on any line whose demo_type is
# 1 the tenth field came back "10", and the position never parsed as three
# words. The clause below then compared two identically truncated reads. It is
# an awk field parse now, so the field count cannot depend on how a sed dialect
# counts capture groups.
# Which player-scene-state record a reader is allowed to measure. With no
# argument it is the last record of any kind, which is what every clause but the
# displacement one wants. With `control` it is the last record whose tuple is
# the tuple the route's own player trace defines as player control --
# demo_type=0 demo_mode=0 event_mode=0, with a live player pointer and a real
# position. The v19 table reads exactly that tuple as "player control", and the
# route oracle's `[player-control-admission] moved=1` ran inside it.
#
# This selector exists because the probe prints on *change*: a frozen state
# prints nothing at all, so a reader that just takes tail -1 measures whichever
# state was printed last, and v23 measured its W hold against
# `demo_type=2 demo_mode=6` -- a post-cutscene message sequence that pins the
# position -- while the stick reached the guest inside it. A reader has to name
# the state it is allowed to measure.
player_select() {
    if [ "${1:-any}" = control ]; then
        grep -E 'demo_type=0 demo_mode=0' |
            grep -E 'event_mode=0 ' |
            grep -vE 'player=0x0+ ' |
            grep -vE 'pos=00000000,00000000,00000000' |
            tail -1
    else
        tail -1
    fi
}

player_field() {
    grep -E '^\[player-scene-state\]' "$log" | player_select "${1:-any}" |
        awk '{
            for (i = 1; i <= NF; i++) {
                split($i, kv, "=")
                if (kv[1] == "demo_type") demo_type = kv[2]
                else if (kv[1] == "demo_mode") demo_mode = kv[2]
                else if (kv[1] == "proc") proc = kv[2]
                else if (kv[1] == "event_mode") event_mode = kv[2]
                else if (kv[1] == "event") event = kv[2]
                else if (kv[1] == "msg") msg = kv[2]
                else if (kv[1] == "pad_hold") pad_hold = kv[2]
                else if (kv[1] == "pos") split(kv[2], at, ",")
            }
            if (demo_type == "") next
            print demo_type, demo_mode, proc, event_mode, event, msg,
                  pad_hold, at[1], at[2], at[3]
        }'
}

# The guest's own decoded left stick (0x803A4DF0 for x, +4 for y, +8 for the
# magnitude), which is the value its player code steers with. pad_hold above is
# a *button* word, and a stick axis never appears in a button word, so a zero
# pad_hold is not evidence that a steering key missed the guest. Only these
# three words are.
player_stick() {
    grep -E '^\[player-scene-state\]' "$log" | player_select "${1:-any}" |
        awk '{
            for (i = 1; i <= NF; i++) {
                split($i, kv, "=")
                if (kv[1] == "stick") stick = kv[2]
            }
            if (stick != "") print stick
        }'
}

# Just the three position words the displacement clause compares.
player_pos() {
    player_field "${1:-any}" | awk 'NF == 10 { print $8 "," $9 "," $10 }'
}

# Is the guest's own record in the control tuple *right now*? The probe prints
# on change, so the newest record is the last line, and the record is in control
# exactly when that newest line is a control line. Matching the two selectors is
# what keeps a stale read from answering yes forever after one control line has
# ever been printed: as soon as any other tuple is printed, the two differ.
in_control() {
    ctl=$(player_field control)
    [ -n "$ctl" ] && [ "$ctl" = "$(player_field any)" ]
}

# The distance between two `pos=` records, in world units. The three words are
# IEEE-754 big-endian floats, so a raw word difference is not a distance: the
# same word delta means a different number of units at a different magnitude,
# and the cutscene's own idle bob moves the words by thousands while Link stands
# still (v23's record toggles y between 44CE4000 and 44CE7000 throughout). Units
# are also the number a "one press moves Link N units" clause wants.
pos_units() {
    python3 - "$1" "$2" <<'PY'
import struct, sys
a = sys.argv[1].split(",")
b = sys.argv[2].split(",")
if len(a) != 3 or len(b) != 3 or "" in a or "" in b:
    print("")
else:
    f = lambda h: struct.unpack(">f", bytes.fromhex(h))[0]
    print("%.4f" % (sum((f(x) - f(y)) ** 2 for x, y in zip(a, b)) ** 0.5))
PY
}

# The guest's own [frame-timing] stamps, read as the wall time from the host's
# first frame (the origin of the clock) to the first frame at or after $1, a
# retrace. Microseconds.
us_to_retrace() {
    awk -v r="$1" '
        /^\[frame-timing\]/ {
            n=$0; sub(/.*retrace=/, "", n); sub(/ .*/, "", n);
            u=$0; sub(/.*us=/, "", u); sub(/ .*/, "", u);
            if (f == 0) f = u + 0;
            if (n + 0 <= r + 0) t = u + 0;
        }
        END { printf "%d", (t == 0 ? 0 : t - f) }' "$log"
}

# Tap a real key until the guest's own state satisfies $1, an expression
# evaluated with main_proc sel_proc sel_menu cur_pos name_done bound, up to $6
# taps. Every tap still has to land, so a driver can never pass by pressing
# nothing; and it stops as soon as the guest reports name-input-complete or the
# new-game intro, because the second subsumes the first.
drive_name() {
    test_expr=$1; label=$2; code=$3; hold=$4; counter=$5; attempts=$6
    main_proc=x; sel_proc=x; sel_menu=x; cur_pos=x; name_done=x
    n=1
    while [ "$n" -le "$attempts" ]; do
        if seen 'boot-milestone. (name-input-complete|new-game-intro)'; then
            note "the guest reported the name accepted while driving [$label]"
            return 0
        fi
        f=$(name_field)
        if [ -n "$f" ]; then
            set -- $f
            main_proc=$1; sel_proc=$2; sel_menu=$3; cur_pos=$4; name_done=$5
            if eval "$test_expr"; then
                note "the guest's screen satisfied [$test_expr] after $((n - 1)) real press(es): $label"
                return 0
            fi
        fi
        tap "$label" "$code" "$hold" "$counter" || return 1
        j=0
        while [ "$j" -lt 6 ]; do
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.1
            j=$((j + 1))
        done
        n=$((n + 1))
    done
    note "the guest's screen never satisfied [$test_expr] after $attempts real presses ($label)"
    return 1
}

# The guest's decoded stick, held down in the background. The stick is what the
# route's own trace holds through this cutscene -- v20's recorded control line
# is pad_hold=0x08000000 with stick=00000000,3F800000,3F800000 -- and holding it
# means the stick is already down the instant the guest starts accepting input,
# instead of the measurement having to catch a control window that v23's own
# record shows can be two retraces wide.
#
# A hold is never killed part-way: a killed key-down could leave the guest's
# stick stuck on for the rest of the run, so each hold is allowed to expire and
# the next is armed only once the previous has exited.
stick_pid=""
stick_hold_off=""
stick_hold_keep() {
    [ -n "$stick_hold_off" ] && return 0
    if [ -n "$stick_pid" ] && kill -0 "$stick_pid" 2>/dev/null; then
        return 0
    fi
    "$focus" "$pid" 300 >>"$input_log" 2>&1
    "$key" "$pid" 13 4000 >>"$input_log" 2>&1 &
    stick_pid=$!
}

# Stop re-arming and let the last hold run out, so the stick is not down while
# the RETURN and arrow clauses are delivered.
stick_hold_stop() {
    stick_hold_off=1
    i=0
    while [ "$i" -lt 60 ] && [ -n "$stick_pid" ] &&
        kill -0 "$stick_pid" 2>/dev/null; do
        sleep 0.1
        i=$((i + 1))
    done
}

# The play scene opens on the authored Outset `awake` cutscene, and the
# cutscene is the second place a real key run has to drive something the route
# only ever drove with pulses. It is not a stall: `awake.stb` drives a separate
# fpcNm_MESG cutscene-text actor whose source-defined states 5 (`outwait`) and
# 10 (`closewait`) consume A/B, and every authored page suspends JStudio until
# it is confirmed. The route clears each page with an A pulse armed exactly
# while that actor is in one of those two states, and Link takes control back at
# `demo_mode` 4 - the same pair the player-ready capture is scheduled from. Its
# own run of this card confirms nine pages, the first at retrace 17,868, the
# last at 19,773, and control at 19,972.
#
# So this driver presses a real J (A) while the guest's own record is *not* in
# the control tuple, and stops the instant it is: the key is released between
# attempts so the cutscene actor sees a fresh trigger rather than a held button,
# and a press that does not land is still a failure.
#
# v23 priced the old stop condition. That run stopped feeding A the moment
# `control-admitted` fired, and the message sequence re-armed nine retraces
# later at retrace 20,173; the run then sat in `demo_type=2 demo_mode=6` for
# 2,817 retraces with the position pinned, so the 6 s W hold at 20,194 was
# measured against a sequence that does not move the player. Two things in
# v23's own record say which way to fix it. The messages do need A -- `msg`
# walked 16->17->18->19 while A was flowing in the sequence ending at 20,155 and
# then held at 16 for fifty-seven seconds once A stopped -- so the driver cannot
# simply stop. And the A press that caused the admission, at about 20,160, is
# followed by the re-arm at 20,173, so the driver must stop the moment the
# control tuple is read rather than one press later. This driver does both, and
# holds the stick throughout: what a person does at this cutscene is hold the
# stick and tap A only while the game is not taking input.
drive_play_control() {
    label=$1
    attempts=$2
    n=1
    landed=0
    misses=0
    while [ "$n" -le "$attempts" ]; do
        # The drive stops on the guest's own control milestone, not on a single
        # control line. in_control() is true whenever the newest player record is
        # a control line, which a brief flicker satisfies, while the gate below
        # waits for the milestone the probe emits after the control tuple has held
        # for eight retraces. Those are different tests, and the difference is
        # what produced the false failures: the drive returned "control" on a
        # flicker, and then wait_for control-admitted timed out because the latch
        # had never fired. Requiring the milestone here means the drive and the
        # gate agree by construction.
        if seen 'player-milestone. control-admitted'; then
            note "the guest admitted control after $landed real A press(es): $label"
            return 0
        fi
        if seen '\[run\] stopped'; then
            note "the run stopped while the awake cutscene was still waiting for input: $label"
            return 1
        fi
        stick_hold_keep
        # A press that does not land is retried rather than fatal. Aborting the
        # whole drive on one missed press is what made this clause look like an
        # input failure when the guest was fine: the v42 run reached control -
        # control-admitted at retrace 20,184, with control-tuple records through
        # 23,870 - and still reported "a real A never cleared the awake cutscene",
        # because the drive had already given up and the later phases inherited a
        # stuck cutscene. A person in that position presses again. The evidence
        # requirement is unchanged: only a press the guest's own record shows
        # counts, and twelve consecutive misses is still a failure.
        before_a=$(count_a)
        send 38 500
        sleep 0.2
        if [ "$(count_a)" -gt "$before_a" ]; then
            landed=$((landed + 1))
            misses=0
        else
            misses=$((misses + 1))
            if [ "$misses" -ge 12 ]; then
                bad "twelve consecutive real A presses never reached the guest controller record ($label)"
                return 1
            fi
        fi
        j=0
        while [ "$j" -lt 6 ]; do
            seen 'player-milestone. control-admitted' && break
            seen '\[run\] stopped' && break
            kill -0 "$pid" 2>/dev/null || break
            stick_hold_keep
            sleep 0.1
            j=$((j + 1))
        done
        n=$((n + 1))
    done
    if seen 'player-milestone. control-admitted'; then
        note "the guest admitted control after $landed real A press(es): $label"
        return 0
    fi
    note "the awake cutscene never left the control tuple measurable after $attempts real A presses ($label); last player record [$(player_field)]"
    return 1
}

# The displacement clause. The stick is already held by stick_hold_keep; this
# feeds A while the guest's own record is not in the control tuple and watches,
# in every control window the run offers, for a real displacement of the guest's
# own player position. It answers in units, so the cutscene's own idle bob --
# which moves the raw words by thousands while Link stands still -- cannot pass
# it. While the tuple is live it does not press A at all, because v23's record
# shows an A press at about retrace 20,160 followed by the message sequence
# re-arming at 20,173.
measure_stick_displacement() {
    label=$1
    deadline=$2
    w=1
    while [ "$(date +%s)" -lt "$deadline" ]; do
        if seen '\[run\] stopped'; then
            return 1
        fi
        if ! in_control; then
            stick_hold_keep
            tap "$label" 38 500 count_a || return 1
            j=0
            while [ "$j" -lt 6 ]; do
                in_control && break
                seen '\[run\] stopped' && break
                kill -0 "$pid" 2>/dev/null || break
                stick_hold_keep
                sleep 0.1
                j=$((j + 1))
            done
            continue
        fi
        # The record is in the control tuple. Read the position out of the
        # record itself and watch that same record for a displacement while the
        # tuple is live; the window closes as soon as the newest record is some
        # other tuple, so nothing sleeps over the seam that v23 stepped over.
        before_pos=$(player_pos any)
        note "control window $w open: pos=${before_pos:-none} stick=$(player_stick any) [$label]"
        j=0
        while [ "$j" -lt 20 ]; do
            stick_hold_keep
            sleep 0.15
            after_pos=$(player_pos any)
            units=$(pos_units "$before_pos" "$after_pos")
            if [ -n "$units" ] &&
                awk -v d="$units" 'BEGIN { exit !(d >= 4.0) }'; then
                note "a real held W moved Link in the guest's own player record: $before_pos -> $after_pos ($units units)"
                return 0
            fi
            in_control || break
            kill -0 "$pid" 2>/dev/null || break
            j=$((j + 1))
        done
        note "control window $w closed with no displacement; last record [$(player_field any)]"
        w=$((w + 1))
    done
    return 1
}

walk_new_file() {
    note "new-file walk, driven by the guest's own screen state"
    drive_name '[ "$main_proc" = 7 ] && [ "$sel_proc" = 0 ]' 'file menu: advance to the name-entry grid' 38 200 count_a 24 || {
        bad "the guest never put its name-entry screen on the letter grid; last state [$(name_field)]"
        return 1
    }
    note "the name-entry grid is up: $(name_field)"
    drive_name '[ "$cur_pos" -gt 0 ]' 'name entry: A types a character' 38 200 count_a 12 || {
        bad "no real A press put a character in the guest's name; last state [$(name_field)]"
        return 1
    }
    note "a character is in the guest's name: $(name_field)"
    drive_name '[ "$sel_menu" = 4 ]' 'name entry: RETURN moves to the END cell' 36 200 count_start 12 || {
        bad "no real RETURN moved the guest's own selection to the END cell; last state [$(name_field)]"
        return 1
    }
    note "the guest's own selection is the END cell: $(name_field)"
    drive_name '[ "$name_done" = 1 ]' 'name entry: A confirms the name' 38 200 count_a 12 || {
        bad "no real A press made the guest accept the name; last state [$(name_field)]"
        return 1
    }
    note "the guest accepted the real-key walk: $(name_field)"
    return 0
}

wait_for 'boot-milestone. title-ready' 2400 || bad "the title screen never reported"
note "the title is up: tapping a real J (A) at PRESS START until the guest reports the file menu"
drive 'boot-milestone. file-select' 'J/A at the title' 38 200 count_a 24

if seen 'boot-milestone. file-select'; then
    note "the file menu reported; walking the new game with real keys"
    walk_new_file
else
    bad "the file menu never reported after real J presses at the title"
fi

if [ "$mode" = game ]; then
    wait_for 'boot-milestone. new-game-intro' 1200 || bad "the new-game intro never reported"
    # V9's first lever is "skip the intro, if the game supports it", and the only
    # test on record pressed START once at retrace 2206 and watched for 36 s
    # before concluding the intro does not answer it. That is thin evidence for
    # the largest term in the route: the intro is 13,590 retraces, 226.5 s of the
    # 332 s content floor at authentic speed, so whether a human can skip it
    # decides whether the five minute clause is reachable at all. Try it properly
    # - several real START presses and several real A presses spread through the
    # intro - and let the play-scene retrace say whether any of them ended it.
    # If none does, the presses are harmless noise inside authored content and
    # the run proceeds exactly as before.
    note "intro skip: trying real START and A presses through the intro"
    skip_try=1
    while [ "$skip_try" -le 6 ]; do
        seen 'boot-milestone. play-scene' && break
        tap 'intro skip: START' 36 250 count_start || true
        sleep 1
        tap 'intro skip: A' 38 250 count_a || true
        sleep 1
        skip_try=$((skip_try + 1))
    done
    if seen 'boot-milestone. play-scene'; then
        note "the intro answered a real key press: play-scene follows"
    else
        note "the intro survived six real START and six real A presses"
    fi
    wait_for 'boot-milestone. play-scene' 6000 || bad "the play scene never reported"
    note "the play scene is up: the authored awake cutscene is waiting for A; driving it by the guest's own player record"
    if ! drive_play_control 'J/A through the awake cutscene' 1200; then
        bad "a real A never cleared the awake cutscene; the guest never handed control back"
    fi
    # The product gate for the second half of the run: the guest's own player
    # record reporting the state a stick is honoured in -- event_mode 0 with demo
    # playback off, held for several retraces -- and not the earlier demo_mode 4
    # gate. The route's own player trace shows demo_mode 4 arrives with
    # demo_type 1 and event_mode 2 and is the authored Outset awake cutscene still
    # running with its subtitle actor live, and a key held there is eaten by the
    # cutscene. v19 gated on demo_mode 4, stopped driving A the moment it
    # appeared, and left the run sitting in the cutscene for the remaining 2,300
    # retraces: its own record still read event_mode=2 at retrace 21,872 and the W
    # hold then moved nothing. The cutscene does not advance itself; it is fed by
    # the A presses this driver makes, so stopping early freezes the run.
    wait_for 'player-milestone. control-admitted' 1200 ||
        bad "the guest's own player record never reached control (event_mode 0 with demo playback off); a W hold measured here would have been measured against the awake cutscene, not against a controllable Link"
    wait_for 'frame-capture. awake-action scheduled' 600 ||
        bad "the player-ready capture never scheduled after the guest admitted control"
fi

if [ "$mode" = game ]; then
    # The player record is the guest's own, read by the PC-independent probe.
    # Every earlier attempt read a bare `pos=` grep, and that grep was matching
    # the collision-provenance line printed at the *title screen* (retrace 333),
    # so the clause compared the title's own camera words, not the player. Then
    # it measured a fixed 6 s window with two `tail -1` reads of the record, and
    # v23's log shows what that buys: its "before" and "after" reads are two
    # different lines of one `demo_type=2 demo_mode=6` message sequence, and the
    # line that proves the key arrived -- retrace 20,194,
    # `stick=00000000,3F800000,3F800000` -- is inside the hold but is neither of
    # the two lines the clause compared. A pinned position was compared with
    # itself twice while the key was reaching the guest. This measures inside
    # the control tuple the route's own trace defines as player control, and it
    # says which of the two it saw instead of calling either one "the keyboard".
    measure_deadline=$(( $(date +%s) + 60 ))
    note "the stick is held down and A is driven only while the guest's own record is not in the control tuple, up to 60 s"
    if measure_stick_displacement 'W held in every control window' "$measure_deadline"; then
        note "the displacement clause passed against the guest's own player record"
    else
        if grep -qE '^\[player-scene-state\].*stick=[0-9A-F]+,3F800000,3F800000' "$log"; then
            note "H-PLAYER-CONSUMES-STICK: this log shows the decoded stick at y=3F800000, so a real stick key did reach the guest's own steering value and the scene was not consuming it"
        else
            note "H-KEYBOARD-NEVER-ARRIVED: no record in this log shows the decoded stick at y=3F800000, so no stick key survived to the player layer"
        fi
        note "last player record [$(player_field any)]; last control-tuple record [$(player_field control)]"
        bad "a real held W never moved the guest's own player position in any control window"
    fi
    stick_hold_stop
    note "real RETURN at the play scene"
    tap 'play scene RETURN' 36 200 count_start || true
    sleep 2
    # P4 milestone 9 is save, quit and reload, and no run has ever pressed save:
    # the card copy has come back byte-identical every time, which is how we know
    # the game has never written one. The runtime's card path is real - CARDCheck,
    # Mount, Format, Create, Read, Write and SetStatus all dispatch to file-backed
    # HLE handlers - so the missing piece is the save flow being driven.
    #
    # The sequence comes from the game's own navigation table rather than a guess.
    # src/d/d_menu_collect.cpp holds item[current][direction] -> next, a 21x8
    # table, and the pause menu opens the save when mNowItem == 7. From item 1,
    # direction 4 (stick down) goes to 6; from item 6, direction 4 goes to 7. So
    # two stick-downs and then A. The cursor is stick-driven, not D-pad - the
    # table is indexed by stickDirection() - which is why the earlier attempt's
    # D-pad press moved nothing, and why only one stick press was tried.
    card_before_save=$(shasum -a 256 "$card" | awk '{print $1}')
    note "pause menu: two stick-downs to the Save entry, then A"
    send 1 600
    sleep 1
    send 1 600
    sleep 1
    tap 'pause menu: choose Save' 38 250 count_a || true
    sleep 2
    tap 'save question: confirm' 38 250 count_a || true
    sleep 3
    card_after_save=$(shasum -a 256 "$card" | awk '{print $1}')
    if [ "$card_after_save" != "$card_before_save" ]; then
        note "the game wrote a save: card copy $card_before_save -> $card_after_save"
    else
        bad "the game never wrote a save after the pause menu Save flow was driven with real presses"
    fi
    note "real left arrow at the play scene"
    send 123 800
    sleep 2
fi

note "awaiting the run's own stop at the retrace ceiling ($retraces)"
i=0
while [ "$i" -lt 9000 ]; do
    grep -qE '\[run\] stopped' "$log" && break
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.1
    i=$((i + 1))
done
if grep -qE '\[run\] stopped: normal' "$log"; then
    note "the run stopped normally"
else
    bad "the run did not report a normal stop"
fi
if grep -qE 'gx-core. shutdown: submitted=[0-9]+ rejected=0 failed=0' "$log"; then
    note "the graphics core shut down with zero rejects and zero failures"
else
    bad "no clean gx-core shutdown line; the run may not have exited cleanly"
fi
if grep -qE 'dsp-lle. summary dmas=[0-9]+ first_nonzero=1' "$log"; then
    note "the LLE DSP produced nonzero audio under LLE"
else
    bad "the DSP summary does not show nonzero audio under LLE"
fi

landed=$(count_any)
if [ "$landed" -ge 1 ]; then
    note "$landed real key presses landed in the guest's own controller record"
else
    bad "no real key press reached the guest controller record at all"
fi

if [ "$mode" = game ]; then
    # A written PPM is not automatically a picture. v24 wrote one whose pixels
    # were 99.9% black (nonblank=3257 of 1920x1440), and a clause that stopped at
    # "written=1" would have called that near-empty buffer "the picture of Outset
    # under control". The clause now also requires the frame to be a render: at
    # least a fifth of its pixels non-black. This is a strengthening, not a
    # substitute - a frame below the floor is reported with its own count, and a
    # run that writes no frame at all is still its own failure.
    frames_written=$(grep -cE 'frame-capture.*written=1' "$log" || true)
    if [ "${frames_written:-0}" -lt 1 ]; then
        bad "no frame was written; there is no picture of Outset under control"
    else
        best_frac=$(grep -E 'frame-capture.*written=1' "$log" | awk '
            {
                w = 0; h = 0; n = 0;
                for (i = 1; i <= NF; i++) {
                    if ($i ~ /^width=/) { sub(/width=/, "", $i); w = $i + 0 }
                    if ($i ~ /^height=/) { sub(/height=/, "", $i); h = $i + 0 }
                    if ($i ~ /^nonblank=/) { sub(/nonblank=/, "", $i); n = $i + 0 }
                }
                if (w > 0 && h > 0) { f = n / (w * h); if (f > best) best = f }
            }
            END { printf "%.4f", best + 0 }')
        note "$frames_written player-ready frame(s) written; best nonblank fraction $best_frac of the frame"
        if awk -v f="$best_frac" 'BEGIN { exit !(f >= 0.20) }'; then
            note "the player-ready frame is a render, not a near-empty buffer"
        else
            bad "the player-ready frame is a near-empty buffer (best nonblank fraction $best_frac); a PPM was written but it is not a picture of Outset under control"
        fi
    fi
    for want in title-ready file-select name-input-complete new-game-intro play-scene; do
        grep -qE "boot-milestone. $want" "$log" || bad "missing boot milestone: $want"
    done
    play_retrace=$(grep -oE 'boot-milestone. play-scene retrace=[0-9]+' "$log" | grep -oE '[0-9]+$' | head -1)
    # TTP-A: launch to the play scene itself. On a cold boot this is the whole
    # cost of walking the new-game intro by hand, and it is the number the
    # save-state work has to move. It is kept as its own clause so it can stay
    # red on its own without dragging the interactive number with it. The clock
    # is the host's own first [frame-timing] stamp, read through us_to_retrace.
    if [ -z "$play_retrace" ]; then
        bad "TTP-A: no play-scene retrace; time to the play scene is unmeasured"
    else
        ttp_a_us=$(us_to_retrace "$play_retrace")
        if [ "${ttp_a_us:-0}" -gt 0 ]; then
            ttp_a_s=$(awk -v u="$ttp_a_us" 'BEGIN { printf "%.2f", u / 1000000 }')
            note "TTP-A launch to play-scene: $ttp_a_s s (play-scene at retrace $play_retrace)"
            if awk -v u="$ttp_a_us" 'BEGIN { exit !(u < 300000000) }'; then
                note "TTP-A is under the five minute target"
            else
                bad "TTP-A launch to play-scene is $ttp_a_s s, over the five minute target"
            fi
        else
            bad "TTP-A: no frame-timing stamp at or before play-scene; time to the play scene is unmeasured"
        fi
    fi

    # TTP-B: launch to the first moment the player can act. From a cold new game
    # the play scene is that moment; a run that starts from a save state whose
    # intro is already cleared reaches control without replaying it, so the first
    # control-admitted or player-ready capture is the marker instead. The earliest
    # marker that exists answers, and the clause always names which one, so a
    # save-state run can never quietly pass itself off as the cold-boot number.
    interactive_retrace=""
    interactive_marker=""
    ctl_at=$(grep -oE 'player-milestone. control-admitted retrace=[0-9]+' "$log" | grep -oE '[0-9]+$' | head -1)
    ready_at=$(grep -oE 'frame-capture. awake-action scheduled retrace=[0-9]+' "$log" | grep -oE '[0-9]+$' | head -1)
    # The control markers are the only honest "a human can act now" evidence: the
    # guest's own record carrying event_mode 0 with demo playback off. On a cold
    # new game that sits past the play scene, on the far side of the awake
    # cutscene; a save-state run whose intro is already cleared reaches it without
    # ever trailing the play scene that way. So the FIRST control marker answers
    # and the play-scene milestone is only the fallback for a run with no control
    # marker at all. v24 is why an earliest-marker rule cannot be used: its
    # earliest marker was the play scene at retrace 14,020, which made TTP-B a
    # duplicate of TTP-A and hid the save-state win this clause exists to measure.
    if [ -n "$ctl_at" ]; then
        interactive_retrace=$ctl_at
        interactive_marker=control-admitted
    elif [ -n "$ready_at" ]; then
        interactive_retrace=$ready_at
        interactive_marker=player-ready
    elif [ -n "$play_retrace" ]; then
        interactive_retrace=$play_retrace
        interactive_marker=play-scene
    fi
    if [ -n "$interactive_retrace" ]; then
        ttp_b_us=$(us_to_retrace "$interactive_retrace")
        if [ "${ttp_b_us:-0}" -gt 0 ]; then
            ttp_b_s=$(awk -v u="$ttp_b_us" 'BEGIN { printf "%.2f", u / 1000000 }')
            note "TTP-B launch to interactive: $ttp_b_s s (marker \"$interactive_marker\" at retrace $interactive_retrace)"
            if awk -v u="$ttp_b_us" 'BEGIN { exit !(u < 300000000) }'; then
                note "TTP-B is under the five minute target"
            else
                bad "TTP-B launch to interactive is $ttp_b_s s (marker \"$interactive_marker\"), over the five minute target"
            fi
        else
            bad "TTP-B: no frame-timing stamp at or before the interactive marker (marker $interactive_marker retrace $interactive_retrace)"
        fi
    else
        bad "TTP-B: no play-scene or control marker at all; time to interactive is unmeasured"
    fi
else
    shots=$(grep -cE 'frame-capture.*written=1' "$log" || true)
    if [ "$shots" -ge 3 ]; then
        note "$shots frames were written for the menu walk"
    else
        bad "only $shots frames were written; the menu walk has no picture series"
    fi
fi

user_card_after=$(shasum -a 256 "$user_card" | awk '{print $1}')
if [ "$user_card_after" = "$user_card_before" ]; then
    note "the user's own save slot is byte-identical after the run"
else
    bad "the user's save slot changed during the run"
fi

printf '\n===== %s pass evidence =====\n' "$mode"
grep -E 'input-probe.*pressed=1' "$log" | head -2 || true
grep -E 'input-chain' "$log" | grep -E 'cpad_trig=0x(0100|0010)' | head -8 || true
grep -E '\[title-execute\]' "$log" | tail -2 || true
grep -E 'boot-milestone' "$log" | grep -E 'title-ready|file-select|name-input-complete|new-game-intro|opening-complete|play-scene' || true
grep -E '\[player-scene-state\]' "$log" | head -1 || true
grep -E '\[player-scene-state\]' "$log" | tail -2 || true
grep -E 'frame-capture.*written=1' "$log" | head -6 || true
grep -E '\[run\] stopped|gx-core. shutdown: submitted|dsp-lle. summary' "$log" | tail -3 || true
ls -l "$frames" || true

if [ "$failures" -ne 0 ]; then
    printf 'acceptance: FAILED %d clause(s); log %s\n' "$failures" "$log"
    exit 1
fi
printf 'acceptance: PASS; log %s frames %s\n' "$log" "$frames"
exit 0
