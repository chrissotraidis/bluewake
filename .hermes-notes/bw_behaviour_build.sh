#!/bin/bash
# Behaviour-timing untrained build: -O2 module (no profile) from prepared-behaviour,
# host app rebuild, package (variant "behaviour-timing"), install.
set -u
AND=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
log() { echo "[$(date +%H:%M:%S)] $*"; }

log "== module (arm64 -O2, untrained, behaviour-timing) =="
python3 "$AND" module || { log "MODULE FAILED"; exit 1; }

log "== app rebuild =="
python3 "$AND" app || { log "APP FAILED"; exit 1; }

log "== package (variant: behaviour-timing) =="
export BLUEWAKE_APK_VARIANT=behaviour-timing
python3 "$AND" package || { log "PACKAGE FAILED"; exit 1; }

log "== install =="
python3 "$AND" install || { log "INSTALL FAILED"; exit 1; }

log "== ALL DONE =="
ls -la /home/james/bw-build-android/WindWakerRecomp-behaviour-timing-*.apk | tail -1
