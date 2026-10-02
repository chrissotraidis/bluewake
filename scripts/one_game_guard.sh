#!/usr/bin/env bash
# Refuse to start a game process while another one is running on this Mac.
#
# BlueWake's loop runs exactly one Wind Waker at a time: the iOS simulator's
# app, the macOS host, or the Dolphin reference, never two (docs/GOAL_PROMPT_V56).
# Two at once starve each other, which ruins timing and has overloaded the machine.
#
# usage: scripts/one_game_guard.sh [--ignore-sim]   exits 1 and names the process
set -euo pipefail
ignore_sim=0
[ "${1:-}" = --ignore-sim ] && ignore_sim=1
busy=""
if pgrep -x Dolphin >/dev/null 2>&1; then busy="$busy Dolphin"; fi
if pgrep -x bluewake_host >/dev/null 2>&1; then busy="$busy bluewake_host"; fi
if pgrep -f 'BlueWake\.app/Contents/MacOS/BlueWake([[:space:]]|$)' >/dev/null 2>&1; then
    busy="$busy BlueWake.app(macOS)"
fi
if [ "$ignore_sim" -eq 0 ] && pgrep -f 'Bundle/Application/.*/BlueWake.app/BlueWake' >/dev/null 2>&1; then
    busy="$busy BlueWake(simulator)"
fi
if [ -n "$busy" ]; then
    echo "one_game_guard: already running:$busy. Stop it first; one game at a time." >&2
    exit 1
fi
