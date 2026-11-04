#!/bin/bash
while pgrep -f "ninja -j 8 -k 0" >/dev/null 2>&1; do sleep 60; done
MOD=/home/james/bw-build-android/composite-instr-o2/gGZLE01_recomp.so
if [ -f "$MOD" ]; then
  echo "INSTR_O2_DONE $MOD $(stat -c %s "$MOD")"
else
  echo "INSTR_O2_FAILED"
  tail -8 /home/james/bw-instr-o2-ninja.log
fi
