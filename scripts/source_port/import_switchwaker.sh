#!/usr/bin/env bash
# Import the SwitchWaker source base into source-port/ at the pinned commit.
#
# Copies, group by group, only the files listed below (PLAN_IMPORT.md groups G1-G9) from a
# read-only clone of centollOS/SwitchWaker, then writes source-port/IMPORT_MANIFEST.sha256
# (sha256sum format, sorted, destination-relative). It never touches the clone, never copies
# switch/, native/data, native/tools/dksh_cache, the Switch Aurora patches or anything outside
# the list, and refuses a clone that is not at the pin or not clean.
#
#   scripts/source_port/import_switchwaker.sh [--clone DIR] [--dest DIR] [--dry-run]
#
# Defaults: --clone /home/fred/projects/ilp32-census-work/switchwaker
#           --dest  <repository>/source-port
#
# The destination mirrors the upstream repository root for the imported trees: game/ and
# native/ keep their names, so the port's own CMakeLists.txt, cmake/ and tools/ keep working
# unchanged (COS_ROOT is ../game from native/, gen_assets.sh reads ../../game/UPSTREAM).
#
# The manifest records the current file hashes (destination-relative, LC_ALL=C sorted). One
# imported file carries documented local changes (native/cmake/GameConfig.cmake: three GCC flag
# gates, see source-port/PROVENANCE.md), so its manifest hash is not the upstream one.

set -euo pipefail

pin=04372277d03f934e1e563ae302bba9f565c5cb5d
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
clone=/home/fred/projects/ilp32-census-work/switchwaker
dest="$repo/source-port"
dry_run=0

usage() {
    sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed '$d' | sed 's/^# \{0,1\}//'
    exit 2
}

while [ $# -gt 0 ]; do
    case "$1" in
        --clone) clone="$2"; shift 2 ;;
        --dest) dest="$2"; shift 2 ;;
        --dry-run) dry_run=1; shift ;;
        -h|--help) usage ;;
        *) echo "import_switchwaker: unknown option $1" >&2; usage ;;
    esac
done

die() { echo "import_switchwaker: $*" >&2; exit 1; }

# --- the clone, read-only, at the pin and clean ------------------------------------------------
[ -d "$clone/.git" ] || die "$clone is not a git checkout"
head="$(git -C "$clone" rev-parse HEAD)"
[ "$head" = "$pin" ] || die "$clone is at ${head:0:12}, not the pin ${pin:0:12}"
status="$(git -C "$clone" status --porcelain)"
[ -z "$status" ] || die "$clone has local changes; refusing to import"

# --- the file list (groups G1-G9 of PLAN_IMPORT.md) ---------------------------------------------
groups=(
    game
    native/CMakeLists.txt
    native/README.md
    native/cmake
    native/sdk
    native/include/sdk
    native/include/helpers
    native/include/pc
    native/src/helpers
    native/src/pc
    native/check
    native/tools
    native/dsp_hle
    native/patches/aurora
)
# Files copied under a different name: "upstream path:destination path".
mappings=(
    "LICENSE:native/LICENSE"
)
list="$(mktemp)"
trap 'rm -f "$list"' EXIT
for g in "${groups[@]}"; do
    git -C "$clone" ls-files -- "$g"
done | grep -v '^native/tools/dksh_cache/' | LC_ALL=C sort -u > "$list"

[ -s "$list" ] || die "empty file list"
if grep -Eq '^(switch/|native/data/|native/tools/dksh_cache/|docs/|scripts/|mods/)' "$list"; then
    die "the file list contains an excluded group"
fi
bad_patches="$(grep -E '^native/patches/' "$list" | grep -v '^native/patches/aurora/' || true)"
[ -z "$bad_patches" ] || die "the file list contains a patch outside native/patches/aurora/"
if grep -Eq '\.(iso|gcm|rvz|dol|rel|gci|nro|nsp|elf|bin)$' "$list"; then
    die "the file list contains a binary or disc-derived file"
fi
for m in "${mappings[@]}"; do
    src="${m%%:*}"
    git -C "$clone" ls-files --error-unmatch "$src" >/dev/null 2>&1 || die "$src is not tracked upstream"
done

files="$(wc -l < "$list")"
mapped="${#mappings[@]}"
bytes="$(while IFS= read -r p; do stat -c %s "$clone/$p"; done < "$list" | awk '{s+=$1} END {print s}')"
for m in "${mappings[@]}"; do
    bytes=$((bytes + $(stat -c %s "$clone/${m%%:*}") ))
done
echo "import_switchwaker: ${files} files + ${mapped} mapped, ${bytes} bytes, clone ${head:0:12}, dest $dest"
if [ "$dry_run" = 1 ]; then
    echo "import_switchwaker: dry run; nothing copied"
    exit 0
fi

[ -e "$dest" ] && die "$dest already exists; remove it first (this script never merges)"

# --- copy, group by group, and write the manifest ------------------------------------------------
mkdir -p "$dest"
manifest="$dest/IMPORT_MANIFEST.sha256"
: > "$manifest"
while IFS= read -r p; do
    mkdir -p "$dest/$(dirname "$p")"
    cp -p "$clone/$p" "$dest/$p"
    printf '%s  %s\n' "$(sha256sum "$clone/$p" | cut -d' ' -f1)" "$p" >> "$manifest"
done < "$list"
for m in "${mappings[@]}"; do
    src="${m%%:*}"
    dst="${m##*:}"
    mkdir -p "$dest/$(dirname "$dst")"
    cp -p "$clone/$src" "$dest/$dst"
    printf '%s  %s\n' "$(sha256sum "$clone/$src" | cut -d' ' -f1)" "$dst" >> "$manifest"
done
LC_ALL=C sort -k2 -o "$manifest" "$manifest"
chmod 644 "$manifest"

copied="$(find "$dest" -type f ! -name IMPORT_MANIFEST.sha256 | wc -l)"
[ "$copied" = "$((files + mapped))" ] || die "copied $copied files, expected $((files + mapped))"
echo "import_switchwaker: copied ${copied} files; manifest $(sha256sum "$manifest" | cut -d' ' -f1)"
