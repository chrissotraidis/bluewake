#!/usr/bin/env bash
# Build BlueWake for an Apple TV from your own GZLE01 USA revision 0 disc.
# See docs/status/TVOS_BUILD.md for signing and install details.
exec "$(cd "$(dirname "$0")/../builder" && pwd)/build.sh" --game bluewake --platform tvos --no-mods "$@"
