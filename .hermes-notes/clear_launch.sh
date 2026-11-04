#!/bin/bash
S=/home/james/android-sdk/platform-tools/adb
$S -s 192.168.86.234:44367 shell rm -f /sdcard/Android/data/dev.bluewake.BlueWake/files/launch.env
echo "launch.env removed"
$S -s 192.168.86.234:44367 shell am force-stop dev.bluewake.BlueWake
echo "app stopped"
