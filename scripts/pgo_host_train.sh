#!/usr/bin/env bash
# Generate private game/host optimization profiles on an Apple Silicon Mac.
# Usage: scripts/pgo_host_train.sh DISC.iso BUILD_DIR [--jobs N] [--save YOUR.card]
# BUILD_DIR must contain game/ and composite-src/ from the source build stages.
# The optional card is copied. No downloaded or repository save is required.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
if [ "$#" -lt 2 ]; then
    echo "usage: scripts/pgo_host_train.sh DISC.iso BUILD_DIR [--jobs N] [--save YOUR.card]" >&2
    echo "For the complete build, use scripts/ios/build_device.sh DISC.iso --train-pgo --ipa OUT.ipa" >&2
    exit 2
fi
training_disc=$1
training_out=$2
shift 2
exec python3 "$root/scripts/builder/train_local_pgo.py" --disc "$training_disc" --out "$training_out" "$@"
