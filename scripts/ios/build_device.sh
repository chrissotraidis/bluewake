#!/usr/bin/env bash
# Build BlueWake for iPad/iPhone from your own GZLE01 disc image.
# Kept for existing instructions: this runs scripts/builder/build.sh with the
# BlueWake profile, and takes the same options (--help lists them).
#
#   scripts/ios/build_device.sh "/path/to/The Legend Of Zelda The Wind Waker.iso"
exec "$(cd "$(dirname "$0")/../builder" && pwd)/build.sh" --game bluewake "$@"
