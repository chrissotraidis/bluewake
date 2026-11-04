#!/usr/bin/env bash
set -uo pipefail
DRV=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
MODULE=/home/james/bw-build-android/composite/libgGZLE01_recomp.so
log() { echo "[$(date +%H:%M:%S)] $*"; }

log "waiting for the game module to finish compiling"
while true; do
  if [ -f "$MODULE" ] && ! pgrep -f "bw_android.py module" >/dev/null; then
    log "module ready: $(ls -la $MODULE | awk '{print $5}') bytes"
    break
  fi
  if ! pgrep -f "bw_android.py module" >/dev/null && [ ! -f "$MODULE" ]; then
    log "ERROR: module build died without producing $MODULE"
    exit 1
  fi
  sleep 30
done

log "== building libmain.so =="
python3 "$DRV" app || { log "app build failed"; exit 1; }

log "== packaging the APK =="
python3 "$DRV" package || { log "package failed"; exit 1; }

log "== installing on Waydroid =="
python3 "$DRV" install || { log "install failed"; exit 1; }

log "== ALL DONE =="
