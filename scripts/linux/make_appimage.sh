#!/usr/bin/env bash
# Package a built BlueWake Linux app folder as an AppImage.
#
#   scripts/linux/make_appimage.sh [BUILD_DIR] [OUT.appimage]
#
# BUILD_DIR is scripts/linux/build.py's --out (default build/linux); the app
# folder BUILD_DIR/BlueWake is expected to exist. OUT defaults to
# BUILD_DIR/BlueWake-x86_64.AppImage.
#
# The AppImage bundles the host, the translated game module, the DSP roms and
# every shared library the host links except the glibc/libstdc++ baseline. The
# player's own disc is NOT bundled: on first run the setup window asks for it,
# offers to install launchers, and prepares it into the data dir (a disc image
# and files extracted from it must never be distributed; the release gate
# rejects them). Bundling the disc would
# also bloat the image past 4 GB.
#
# Requirements on the build host: appimagetool, desktop-file-validate,
# zsyncmake and ImageMagick on PATH (pacman -S appimagetool desktop-file-utils
# zsync-curl imagemagick). APPIMAGETOOL may name another appimagetool
# executable. APPIMAGE_RUNTIME_FILE may name a pinned Type 2 runtime to embed
# instead of downloading one.
set -euo pipefail

for tool in "${APPIMAGETOOL:-appimagetool}" desktop-file-validate zsyncmake; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "make_appimage: $tool not found on PATH" >&2
        exit 1
    fi
done
if ! command -v magick >/dev/null 2>&1 && ! command -v convert >/dev/null 2>&1; then
    echo "make_appimage: ImageMagick (magick or convert) not found on PATH" >&2
    exit 1
fi

root=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(cd "${1:-$root/build/linux}" && pwd)
app="$build_dir/BlueWake"
out="${2:-$build_dir/BlueWake-x86_64.AppImage}"
out=$(cd "$(dirname "$out")" && pwd)/$(basename "$out")

if [ ! -x "$app/bluewake" ]; then
    echo "make_appimage: $app/bluewake not found; run scripts/linux/build.py first" >&2
    exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
appdir="$work/BlueWake.AppDir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/lib" "$appdir/usr/share/icons/hicolor/256x256/apps" \
         "$appdir/usr/share/applications"

# The host and its redistributable data files. main.dol, the RELs and the disc
# itself are deliberately absent: the launcher validates and prepares them from
# the player's own disc on first run.
cp "$app/bluewake" "$appdir/usr/bin/bluewake"
if [ -f "$app/initial_pipeline_cache.db" ]; then
    cp "$app/initial_pipeline_cache.db" "$appdir/usr/bin/"
fi
mkdir -p "$appdir/usr/bin/dsp"
cp "$app/dsp/dsp_rom.bin" "$app/dsp/dsp_coef.bin" "$appdir/usr/bin/dsp/"
cp "$app/gGZLE01_recomp.so" "$appdir/usr/bin/gGZLE01_recomp.so"

# Shared libraries the host links, minus the glibc/libstdc++ baseline. Bundling
# the baseline breaks the image on distros whose glibc differs from the build
# host's. absl/png/freetype/sqlite/zstd are not guaranteed on the target.
while IFS= read -r lib; do
    base=$(basename "$lib")
    case "$base" in
        libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|ld-linux*|libgcc_s.so*|libstdc++.so*)
            continue ;;
    esac
    cp "$lib" "$appdir/usr/lib/"
done < <(ldd "$app/bluewake" | awk '/=> \// {print $3}' | sort -u)

# AppRun: resolve the image's mount, set the library path, and exec the host.
# The launcher inside bluewake already handles first-run disc prep and paths.
cat > "$appdir/AppRun" <<'EOF'
#!/bin/sh
SELF=$(readlink -f "$0")
HERE=$(dirname "$SELF")
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
exec "$HERE/usr/bin/bluewake" "$@"
EOF
chmod +x "$appdir/AppRun"

# The .desktop entry, at the AppDir root (appimagetool looks there first) and
# copied into usr/share/applications for the desktop.
cat > "$appdir/BlueWake.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=BlueWake
Comment=The Legend of Zelda: The Wind Waker, statically recompiled
Exec=bluewake
Icon=BlueWake
Categories=Game;
Terminal=false
Actions=Setup;

[Desktop Action Setup]
Name=Configure BlueWake
Exec=bluewake --setup
EOF
cp "$appdir/BlueWake.desktop" "$appdir/usr/share/applications/BlueWake.desktop"

# The icon, from the Windows resource (ImageMagick converts the .ico).
icon="$appdir/usr/share/icons/hicolor/256x256/apps/BlueWake.png"
if [ ! -f "$root/windows/resources/BlueWake.ico" ]; then
    echo "make_appimage: windows/resources/BlueWake.ico not found" >&2
    exit 1
fi
# IMv7's `convert` is deprecated and a stub; `magick` is the real tool.
if command -v magick >/dev/null 2>&1; then
    magick "$root/windows/resources/BlueWake.ico[0]" -resize 256x256 "$icon"
else
    convert "$root/windows/resources/BlueWake.ico[0]" -resize 256x256 "$icon"
fi
cp "$icon" "$appdir/BlueWake.png"

# Symlinks so AppRun and the desktop entry both resolve the binary and icon.
ln -sf usr/bin/bluewake "$appdir/bluewake"
ln -sf usr/share/icons/hicolor/256x256/apps/BlueWake.png "$appdir/.DirIcon"

desktop-file-validate "$appdir/BlueWake.desktop"
desktop-file-validate "$appdir/usr/share/applications/BlueWake.desktop"

echo "make_appimage: squashing $out"
appimagetool=${APPIMAGETOOL:-appimagetool}
runtime_args=()
if [ -n "${APPIMAGE_RUNTIME_FILE:-}" ]; then
    runtime_args=(--runtime-file "$APPIMAGE_RUNTIME_FILE")
fi
ARCH=x86_64 "$appimagetool" "${runtime_args[@]}" "$appdir" "$out"

# zsync metadata for delta-capable update tools.
zsyncmake -u "$(basename "$out")" -o "$out.zsync" "$out"

echo "make_appimage: $out"
echo "make_appimage: $out.zsync"
echo "make_appimage: run with APPIMAGE_EXTRACT_AND_RUN=1 on a box without FUSE"
