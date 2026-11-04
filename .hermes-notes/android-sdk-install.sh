#!/usr/bin/env bash
set -euo pipefail
SDK=/home/james/android-sdk
REPO=https://dl.google.com/android/repository
mkdir -p "$SDK"
cd "$SDK"
log() { echo "[$(date +%H:%M:%S)] $*"; }

# 1. commandline-tools
if [ ! -x "$SDK/cmdline-tools/latest/bin/sdkmanager" ]; then
  log "downloading cmdline-tools"
  curl -sSLo clt.zip "$REPO/commandlinetools-linux-16111833_latest.zip"
  rm -rf cmdline-tools
  mkdir -p cmdline-tools
  unzip -q clt.zip -d cmdline-tools
  mv cmdline-tools/cmdline-tools cmdline-tools/latest
  rm -f clt.zip
fi
log "cmdline-tools present"

# 2. NDK r30
if [ ! -d "$SDK/ndk/30.0.16248370" ]; then
  log "downloading NDK r30 (~1GB, takes a while)"
  curl -sSLo ndk.zip "$REPO/android-ndk-r30-linux.zip"
  mkdir -p ndk
  unzip -q ndk.zip -d ndk
  rm -f ndk.zip
fi
log "NDK present"

# 3. build-tools 35.0.1
if [ ! -d "$SDK/build-tools/35.0.1" ]; then
  log "downloading build-tools 35.0.1"
  curl -sSLo bt.zip "$REPO/build-tools_r35.0.1_linux.zip"
  mkdir -p build-tools
  unzip -q bt.zip -d build-tools
  rm -f bt.zip
fi
log "build-tools present"

# 4. platform-tools
if [ ! -x "$SDK/platform-tools/adb" ]; then
  log "downloading platform-tools"
  curl -sSLo pt.zip "$REPO/platform-tools_r37.0.1-linux.zip"
  unzip -q pt.zip
  rm -f pt.zip
fi
log "platform-tools present"

# 5. platform android-36 (android.jar) and android-35
for P in 35 36; do
  if [ ! -d "$SDK/platforms/android-$P" ]; then
    log "downloading platform android-$P"
    Z=$( [ "$P" = 35 ] && echo "platform-35_r02.zip" || echo "platform-36_r02.zip" )
    curl -sSLo p$P.zip "$REPO/$Z"
    mkdir -p platforms
    unzip -q p$P.zip -d platforms
    rm -f p$P.zip
  fi
done
log "platforms present"

log "ALL SDK COMPONENTS READY"
ls -la "$SDK"
