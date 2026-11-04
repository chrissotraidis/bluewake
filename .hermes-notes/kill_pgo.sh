#!/bin/bash
# Kill the PGO -O2 instrumented build (ninja + its clang children).
for pid in $(pgrep -x ninja); do kill -9 "$pid" 2>/dev/null; done
for pid in $(pgrep -x clang); do kill -9 "$pid" 2>/dev/null; done
sleep 1
echo "clang left: $(pgrep -c -x clang 2>/dev/null || echo 0)"
echo "ninja left: $(pgrep -c -x ninja 2>/dev/null || echo 0)"
