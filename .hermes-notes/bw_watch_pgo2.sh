#!/bin/bash
OUT=/home/james/bw-pgo2-milestones.log
MOD=/home/james/bw-build-android/composite-instr-o2pgo/gGZLE01_recomp.so
APK=/home/james/bw-build-android/training.apk
echo "[$(date +%H:%M:%S)] watching" > "$OUT"
# module
while [ ! -f "$MOD" ]; do sleep 60; done
while [ $(stat -c %Y "$MOD") -lt $(($(date +%s) - 30)) ]; do sleep 60; done
echo "[$(date +%H:%M:%S)] MODULE_DONE $(stat -c %s "$MOD")" >> "$OUT"
# training apk
while [ ! -f "$APK" ] || [ $(stat -c %Y "$APK") -lt $(($(date +%s) - 30)) ]; do sleep 60; done
echo "[$(date +%H:%M:%S)] TRAIN_APK_DONE $(stat -c %s "$APK")" >> "$OUT"
