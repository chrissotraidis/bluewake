#!/bin/bash
# Unattended PGO chain. Waits for the -O2 instrumented module, packages it into
# a training APK, installs, then runs the plain playback as the Houdini test.
set -u
MOD=/home/james/bw-build-android/composite-instr-o2/gGZLE01_recomp.so
NINJA=/home/james/bw-instr-o2-ninja.log
DRV=/home/james/nfs/Projects/bluewake/.hermes-notes/bw_pgo_push.py
log() { echo "[$(date +%H:%M:%S)] $*"; }

log "waiting for -O2 instrumented build to finish"
while pgrep -f "ninja -j 8 -k 0" >/dev/null 2>&1; do sleep 60; done
if [ ! -f "$MOD" ]; then
  log "BUILD FAILED"
  tail -20 "$NINJA"
  exit 1
fi
log "instrumented -O2 module ready: $(stat -c %s "$MOD") bytes"

# Package the instrumented module into a training APK (reuse package_final)
log "packaging -O2 instrumented training APK"
python3 - "$MOD" <<'PYEOF'
import sys, subprocess
mod = sys.argv[1]
sys.path.insert(0, "/home/james/nfs/Projects/bluewake/.hermes-notes")
import bw_pgo_push as B
from pathlib import Path
apk = B.package_final(Path(mod))
print("PACKAGED", apk)
PYEOF
# package_final wrote WindWakerRecomp.apk; rename to training-o2.apk
cp /home/james/bw-build-android/WindWakerRecomp.apk /home/james/bw-build-android/training-o2.apk
log "installing training-o2.apk"
/home/james/android-sdk/platform-tools/adb install -r -g /home/james/bw-build-android/training-o2.apk || { log "install failed"; exit 1; }

log "running plain playback (Houdini survival test)"
python3 "$DRV" playback || { log "PLAYBACK FAILED (Houdini crash or no profile)"; exit 1; }

log "PGO PLAYBACK SUCCEEDED - profile written"
