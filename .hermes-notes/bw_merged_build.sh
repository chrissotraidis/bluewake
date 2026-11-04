#!/bin/bash
# Merged-tree trained build: -O2 + profile module (new direct-skip), host app
# rebuild, package, install. Uses the existing prepared-lean source (already
# lean-memory+lean-deadlines, matching the merged builders' new defaults) and
# the profile collected from the pre-merge playbacks (game chunks unchanged).
set -u
PGO=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_pgo_phone.py
AND=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
log() { echo "[$(date +%H:%M:%S)] $*"; }

log "== optimized module (-O2 -fprofile-instr-use, merged tree) =="
python3 "$PGO" optimized || { log "MODULE FAILED"; exit 1; }

log "== copy trained module into composite/ for packaging =="
cp /home/james/bw-build-android/composite-pgo/gGZLE01_recomp.so \
   /home/james/bw-build-android/composite/gGZLE01_recomp.so

log "== app rebuild (host changes from merge) =="
python3 "$AND" app || { log "APP FAILED"; exit 1; }

log "== package (variant: pgo) =="
export BLUEWAKE_APK_VARIANT=pgo
python3 "$AND" package || { log "PACKAGE FAILED"; exit 1; }

log "== install over adb =="
python3 "$AND" install || { log "INSTALL FAILED"; exit 1; }

log "== ALL DONE =="
ls -la /home/james/bw-build-android/WindWakerRecomp-untrained-*.apk | tail -1
