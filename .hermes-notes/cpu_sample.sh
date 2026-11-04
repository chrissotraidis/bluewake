#!/bin/bash
S=/home/james/android-sdk/platform-tools/adb
PID=25649
read U1 S1 < <($S -s 192.168.86.234:44367 shell "cat /proc/$PID/stat" | awk '{print $14, $15}')
sleep 4
read U2 S2 < <($S -s 192.168.86.234:44367 shell "cat /proc/$PID/stat" | awk '{print $14, $15}')
echo "utime: $U1 -> $U2"
echo "stime: $S1 -> $S2"
