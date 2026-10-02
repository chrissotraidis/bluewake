#!/usr/bin/env bash
# Add BlueWake's code mods (widescreen 16:9 and 16:10, Better Wind Waker's options,
# and each widescreen with them) to a device build's composite source, after
# scripts/ios/build_device.sh has built it.
#
#   scripts/mods/build_mods.sh BUILD_DIR [DISC.iso]
#
# BUILD_DIR is build_device.sh's --out (default build/device). Afterwards, run
# the composite build again (cmake BUILD_DIR/composite-ios && ninja -C
# BUILD_DIR/composite-ios); only the variant chunks and module_export.c compile.
# See docs/MODS.md.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
B=$(cd "${1:?usage: build_mods.sh BUILD_DIR [DISC.iso]}" && pwd)
M=$B/mods
mkdir -p "$M"
dolrecomp=${DOLRECOMP:-$B/dolrecomp/dolrecomp}
# Better Wind Waker's settings as game options (mods/betterww/options.txt): the
# translator emits both behaviours at each option site.
options=$root/mods/betterww/options.txt
python3 "$root/scripts/mods/game_options.py" sites "$options" > "$M/option-sites.txt"
sites=()

translate() {  # DOL OUT_DIR [RELS_DIR]; with the option sites while sites=(--option-sites FILE)
    rm -rf "$2"
    "$dolrecomp" --gamecube --backend c --cpu gekko --partition-instructions 4096 ${sites[@]+"${sites[@]}"} \
        "$1" "$2/dol" -j 8 >"$2.log" 2>&1
    if [ $# -gt 2 ]; then
        "$dolrecomp" --gamecube --backend c --cpu gekko --rel-base 0xC0400000 ${sites[@]+"${sites[@]}"} \
            "$3" "$2/rels" -j 8 >>"$2.log" 2>&1
    fi
}
composite() {  # DOL_DIR RELS_DIR RELS_BIN MAIN_DOL OUT
    python3 "$root/scripts/generate_composite.py" --dol-dir "$1" --rels-dir "$2" --rels-bin-dir "$3" \
        --main-dol "$4" --output-dir "$5" | tail -1
}

echo "==> widescreen"
mkdir -p "$M/widescreen"
python3 "$root/scripts/mods/gecko_apply.py" "$root/mods/widescreen/GZLE01.gecko" "$B/game/main.dol" \
    "$M/widescreen/main.dol" "$M/widescreen/runtime.json"
translate "$M/widescreen/main.dol" "$M/widescreen/translated"
composite "$M/widescreen/translated/dol/generated" "$B/translated/rels/generated/rels" "$B/game/rels" \
    "$M/widescreen/main.dol" "$M/widescreen/composite-src"

echo "==> widescreen 16:10"
# Derived from the 16:9 code; see scripts/mods/widescreen_aspect.py.
mkdir -p "$M/widescreen1610"
python3 "$root/scripts/mods/gecko_apply.py" "$root/mods/widescreen/GZLE01-16x10.gecko" "$B/game/main.dol" \
    "$M/widescreen1610/main.dol" "$M/widescreen1610/runtime.json"
translate "$M/widescreen1610/main.dol" "$M/widescreen1610/translated"
composite "$M/widescreen1610/translated/dol/generated" "$B/translated/rels/generated/rels" "$B/game/rels" \
    "$M/widescreen1610/main.dol" "$M/widescreen1610/composite-src"

echo "==> Better Wind Waker options"
# The game's own executable and modules, translated with the option sites; the
# variants are the chunks that hold a site (docs/MODS.md).
sites=(--option-sites "$M/option-sites.txt")
mkdir -p "$M/betterww"
translate "$B/game/main.dol" "$M/betterww/translated" "$B/game/rels"
composite "$M/betterww/translated/dol/generated" "$M/betterww/translated/rels/generated/rels" \
    "$B/game/rels" "$B/game/main.dol" "$M/betterww/composite-src"

echo "==> widescreen + Better Wind Waker options"
mkdir -p "$M/combo"
translate "$M/widescreen/main.dol" "$M/combo/translated"
composite "$M/combo/translated/dol/generated" "$M/betterww/translated/rels/generated/rels" \
    "$B/game/rels" "$M/widescreen/main.dol" "$M/combo/composite-src"

echo "==> widescreen 16:10 + Better Wind Waker options"
mkdir -p "$M/combo1610"
translate "$M/widescreen1610/main.dol" "$M/combo1610/translated"
composite "$M/combo1610/translated/dol/generated" "$M/betterww/translated/rels/generated/rels" \
    "$B/game/rels" "$M/widescreen1610/main.dol" "$M/combo1610/composite-src"
sites=()

echo "==> variants into $B/composite-src"
python3 "$root/scripts/generate_composite.py" --dol-dir "$B/translated/dol/generated" \
    --rels-dir "$B/translated/rels/generated/rels" --rels-bin-dir "$B/game/rels" --main-dol "$B/game/main.dol" \
    --output-dir "$M/composite-src.base" | tail -1
python3 "$root/scripts/mods/build_mod_variants.py" --composite-src "$M/composite-src.base" --base-dol "$B/game/main.dol" \
    --mod "widescreen:$M/widescreen/composite-src:$M/widescreen/main.dol:$M/widescreen/runtime.json" \
    --mod "betterww:$M/betterww/composite-src:$B/game/main.dol" \
    --mod "widescreen1610:$M/widescreen1610/composite-src:$M/widescreen1610/main.dol:$M/widescreen1610/runtime.json" \
    --combo "widescreen+betterww:$M/combo/composite-src" \
    --combo "widescreen1610+betterww:$M/combo1610/composite-src" \
    --exclusive widescreen,widescreen1610 \
    --options "$options" --rels-bin-dir "$B/game/rels"
if ! cmp -s "$M/composite-src.base/generated.h" "$B/composite-src/generated.h"; then
    echo "build_mods: $B/composite-src was generated from other inputs; rebuild it first" >&2
    exit 1
fi
python3 - "$M/composite-src.base" "$B/composite-src" <<'PYEOF'
import pathlib, shutil, sys
src, dst = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
for d in dst.glob("chunks_mod_*"):
    shutil.rmtree(d)
for d in src.glob("chunks_mod_*"):
    shutil.copytree(d, dst / d.name)
for f in ("generated_composite.h", "mod_variants.inc"):
    shutil.copy2(src / f, dst / f)
PYEOF
echo "build_mods: done; rebuild the composite: cmake $B/composite-ios && ninja -C $B/composite-ios"
