#!/bin/bash
set -u
DRV=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_android.py
log() { echo "[$(date +%H:%M:%S)] $*"; }
log "== lean module (arm64 -O2, untrained, --lean-memory --lean-deadlines) =="
python3 "$DRV" module || { log "MODULE FAILED"; exit 1; }
log "== package =="
python3 "$DRV" package || { log "PACKAGE FAILED"; exit 1; }
log "== ALL DONE =="
ls -la /home/james/bw-build-android/WindWakerRecomp.apk
