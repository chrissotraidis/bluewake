#!/bin/bash
# Draw-fusion A/B on the Pixel 8a (DOL_GX_FUSE=1 vs 0), interleaved to rule
# out cold-start. Run AFTER the trained APK is installed and confirmed working.
# Reads: correctness, game FPS, GX worker busy%, and thermal (core freq).
set -u
S=/home/james/android-sdk/platform-tools/adb
SERIAL=100.106.76.98:39863
FILES=/sdcard/Android/data/dev.bluewake.BlueWake/files
LOG_DIR=/home/james/bw-fusion-ab

ab() {
  local label=$1 fuse=$2
  printf 'DOL_GX_FUSE=%s\n' "$fuse" > /tmp/fusion.env
  $S -s "$SERIAL" push /tmp/fusion.env "$FILES/launch.env" >/dev/null
  $S -s "$SERIAL" shell am force-stop dev.bluewake.BlueWake >/dev/null
  $S -s "$SERIAL" shell am start -n dev.bluewake.BlueWake/dev.bluewake.android.BlueWakeActivity --ez showWhenLocked true >/dev/null
  sleep 45
  local latest=$($S -s "$SERIAL" shell "ls -t $FILES/logs | head -1" | tr -d '\r')
  local lf="$LOG_DIR/$label.log"
  $S -s "$SERIAL" shell "cat $FILES/logs/$latest" > "$lf" 2>/dev/null
  echo "=== $label (fuse=$fuse) ==="
  grep -E "\[gx\] draw fusion|\[fps-dip\]|\[perf-summary\]|\[gx\]" "$lf" | tail -8
  # thermal
  local freqs=$($S -s "$SERIAL" shell "cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq 2>/dev/null" | tr '\n' ' ')
  echo "core freqs: $freqs"
  $S -s "$SERIAL" shell am force-stop dev.bluewake.BlueWake >/dev/null
}

mkdir -p "$LOG_DIR"
# Interleaved: on, off, on, off
ab on-1 1
ab off-1 0
ab on-2 1
ab off-2 0
$S -s "$SERIAL" shell rm -f "$FILES/launch.env"
echo "A/B done; logs in $LOG_DIR"
