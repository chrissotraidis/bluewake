#!/bin/bash
# Watch the full untrained build and log each milestone to a single file.
OUT=/home/james/bw-full-milestones.log
DRV=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
MOD=/home/james/bw-build-android/composite/gGZLE01_recomp.so
LIB=/home/james/bw-build-android/app/libmain.so
APK=/home/james/bw-build-android/WindWakerRecomp.apk

echo "[$(date +%H:%M:%S)] watching" >> "$OUT"

# module
while pgrep -f "bw_android.py module" >/dev/null 2>&1 || pgrep -f "bw_full_untrained" >/dev/null 2>&1; do
  if [ -f "$MOD" ] && ! pgrep -f "ninja" >/dev/null 2>&1; then
    echo "[$(date +%H:%M:%S)] MODULE_DONE $(stat -c %s "$MOD" 2>/dev/null)" >> "$OUT"
    break
  fi
  sleep 60
done

# app
while [ ! -f "$LIB" ]; do sleep 30; done
echo "[$(date +%H:%M:%S)] APP_DONE $(stat -c %s "$LIB" 2>/dev/null)" >> "$OUT"

# package
while [ ! -f "$APK" ]; do sleep 30; done
# ensure it's the fresh one (mtime recent)
while [ $(stat -c %Y "$APK") -lt $(date +%s -d '1 minute ago') ]; do sleep 30; done
echo "[$(date +%H:%M:%S)] PACKAGE_DONE $(stat -c %s "$APK")" >> "$OUT"
echo "[$(date +%H:%M:%S)] ALL_DONE" >> "$OUT"
