#!/usr/bin/env bash
# Convert BlueWake's existing iOS artwork into the macOS bundle icon.
set -euo pipefail
[ "$#" -eq 2 ] || { echo "usage: make_icon.sh SOURCE.png OUTPUT.icns" >&2; exit 2; }
source_png=$1
output_icon=$2
iconset="${output_icon%.icns}.iconset"
mkdir -p "$iconset"
for size in 16 32 128 256 512; do
    sips -z "$size" "$size" "$source_png" --out "$iconset/icon_${size}x${size}.png" >/dev/null
    retina_size=$((size * 2))
    sips -z "$retina_size" "$retina_size" "$source_png" --out "$iconset/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$iconset" -o "$output_icon"
