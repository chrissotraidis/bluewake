#!/bin/bash
S=/home/james/android-sdk/platform-tools/adb
echo "=== package ==="
$S -s 192.168.86.234:44367 shell dumpsys package dev.bluewake.BlueWake | grep -iE "versionName|lastUpdateTime|DEBUGGABLE" | head -3
echo "=== game files ==="
$S -s 192.168.86.234:44367 shell "ls /sdcard/Android/data/dev.bluewake.BlueWake/files/game/"
echo "=== launch.env (should be gone) ==="
$S -s 192.168.86.234:44367 shell "ls -la /sdcard/Android/data/dev.bluewake.BlueWake/files/launch.env 2>&1"
