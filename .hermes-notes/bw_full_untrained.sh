#!/bin/bash
# Full untrained build against the freshly-merged-and-prepared source.
set -u
DRV=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
log() { echo "[$(date +%H:%M:%S)] $*"; }

log "== module (arm64 -O2, untrained) =="
python3 "$DRV" module || { log "MODULE FAILED"; exit 1; }

log "== app (libmain.so with merged host) =="
python3 "$DRV" app || { log "APP FAILED"; exit 1; }

log "== package =="
python3 "$DRV" package || { log "PACKAGE FAILED"; exit 1; }

log "== ALL DONE =="
ls -la /home/james/bw-build-android/WindWakerRecomp.apk
