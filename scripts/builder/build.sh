#!/usr/bin/env bash
# BlueWake Builder: turn your own game disc into your own app, on your Mac.
#
#   scripts/builder/build.sh DISC.iso [--ipa OUT.ipa] [options]
#   scripts/builder/build.sh --app-only --ipa OUT.ipa   (the published app, no game code)
#
# The pipeline is generic; everything game-specific (disc checks, translator
# settings, the app target, mods) lives in a profile, scripts/builder/profiles/
# NAME.sh (default: bluewake). docs/BUILDER.md explains the split and what a new
# port's profile provides.
#
# Steps, each logged under OUT/logs:
#   1 tools, 2 dependencies, 3 extract from the disc, 4 translate,
#   5 generate the composite source, 6 mods, 7 compile (the long step),
#   8 build the app, embed the game module, sign, 9 optional IPA and install
#
# Options:
#   --app FILE.ipa            add the game module to this published app (made with
#                             --app-only) instead of building the app here
#   --app-only                build only the app, with no disc and no game code: the
#                             IPA a release publishes; players add the module
#   --ipa FILE                also write an unsigned IPA for sideloading (AltStore,
#                             SideStore, Sideloadly, Xcode). It contains the game
#                             code translated from YOUR disc: it is for you only,
#                             never share or upload it
#   --no-mods                 skip the Widescreen and Better Wind Waker variants
#   --out DIR                 build directory (default build/device, or build/macos)
#   --jobs N                  parallel compile jobs (default: all cores)
#   --game NAME               profile to use (default bluewake)
#   --platform ios|tvos|macos Apple target (default ios)
#   --combined-optimizations opt into the measured Mac optimization candidate
#   --identity NAME           codesign identity, e.g. "Apple Development: You (TEAMID)"
#   --profile FILE            device provisioning profile (with --identity)
#   --install DEVICE          install with devicectl after signing (needs --identity)
#   --no-train                skip local optimization training: about 20 minutes of
#                             training and a Mac test build saved, but slower in game
#                             (about 27.5 instead of 30 FPS measured on an iPad Pro M2)
#   --no-pgo                  skip all optimization profiles (also skips training)
#   --train-pgo               train even when it would otherwise be skipped (default on)
#   --training-save FILE      optional personal memory card for training (copied)
#   --composite-pgo FILE      LLVM .profdata for the game module (repeatable; replaces
#                             the profile's bundled one)
#   --host-pgo FILE           LLVM .profdata for the app's host code (replaces the bundled one)
#   --device-cpu CPU          -mcpu for the game module (default apple-a13)
#   --accept-new-composite    continue if the generated source differs from the verified one
#   --source-only             stop after step 5: checks tools, disc and translation in
#                             minutes, before the long compile
#
# The disc, the extracted files, the translated code and the app stay in the
# build directory, which git ignores. Nothing is uploaded.
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"

iso="" game=bluewake platform=ios out="" ipa="" published_app="" app_only=0
jobs=$(sysctl -n hw.ncpu)
identity="" profile="" install_device="" host_pgo=""
train_pgo=auto training_save=""
module_optimizations=none
composite_pgo=()
# -O2 always: -O1 compiled in 47 min instead of 80 but held only 26 FPS
# on an iPad Pro (M2) at Outset (docs/BUILDER.md), so there is no quick option.
device_cpu=apple-a13 opt_level=2 mods=1 accept_new=0 source_only=0 use_pgo=1

die() { echo "builder: $*" >&2; exit 1; }
step() { echo; echo "==> $*"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --app|--ipa|--out|--jobs|--game|--platform|--identity|--profile|--install|--composite-pgo|--host-pgo|--device-cpu|--training-save)
            [ $# -ge 2 ] && [ -n "$2" ] && [[ "$2" != --* ]] || die "$1 needs a value" ;;
    esac
    case "$1" in
        --app) published_app=$2; shift 2 ;;
        --app-only) app_only=1; shift ;;
        --ipa) ipa=$2; shift 2 ;;
        --no-mods) mods=0; shift ;;
        --out) out=$2; shift 2 ;;
        --jobs) jobs=$2; shift 2 ;;
        --game) game=$2; shift 2 ;;
        --platform) platform=$2; shift 2 ;;
        --combined-optimizations) module_optimizations=combined-v1; shift ;;
        --identity) identity=$2; shift 2 ;;
        --profile) profile=$2; shift 2 ;;
        --install) install_device=$2; shift 2 ;;
        --composite-pgo) composite_pgo+=("$2"); shift 2 ;;
        --host-pgo) host_pgo=$2; shift 2 ;;
        --no-pgo) use_pgo=0; shift ;;
        --train-pgo) train_pgo=1; shift ;;
        --no-train) train_pgo=0; shift ;;
        --training-save) training_save=$2; shift 2 ;;
        --device-cpu) device_cpu=$2; shift 2 ;;
        --accept-new-composite) accept_new=1; shift ;;
        --source-only) source_only=1; shift ;;
        -h|--help) awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"; exit 0 ;;
        -*) die "unknown option $1" ;;
        *) [ -z "$iso" ] || die "one disc image only"; iso=$1; shift ;;
    esac
done

[[ "$game" =~ ^[a-z][a-z0-9_-]*$ ]] || die "invalid game profile name: $game"
[[ "$platform" = ios || "$platform" = tvos || "$platform" = macos ]] || die "--platform must be ios, tvos or macos"
if [ "$module_optimizations" != none ]; then
    [ "$platform" = macos ] && [ "$app_only" -eq 0 ] || die "--combined-optimizations needs a personal --platform macos build"
fi
if [ "$platform" = macos ]; then
    [ -z "$ipa$published_app$profile$install_device" ] || die "macOS produces a local .app; --ipa, --app, --profile and --install are device-only"
fi
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || die "--jobs must be a positive integer"
[[ "$device_cpu" =~ ^[a-zA-Z0-9_-]+$ ]] || die "invalid --device-cpu"
[ "$train_pgo" != 1 ] || [ "$use_pgo" -eq 1 ] || die "--train-pgo conflicts with --no-pgo"
# Local training is the default: it replaces the developer's private game
# profile, which is what the measured 30 FPS depends on (docs/BUILDER.md).
[ "$train_pgo" != auto ] || train_pgo=$use_pgo
[ -z "$training_save" ] || [ "$train_pgo" -eq 1 ] || die "--training-save needs training (remove --no-train/--no-pgo)"
profile_file=$root/scripts/builder/profiles/$game.sh
[ -f "$profile_file" ] || die "no profile $profile_file"
# shellcheck source=profiles/bluewake.sh
. "$profile_file"
[ "$PROFILE_HAS_MODS" = 1 ] || mods=0
# Profile-guided optimization profiles that ship with the game profile (they
# cover only the port's own code; docs/BUILDER.md). Explicit files replace them.
if [ "$use_pgo" -eq 1 ]; then
    if [ ${#composite_pgo[@]} -eq 0 ] && [ -n "${PROFILE_COMPOSITE_PGO:-}" ]; then
        composite_pgo=("$root/$PROFILE_COMPOSITE_PGO")
    fi
    if [ -z "$host_pgo" ] && [ -n "${PROFILE_HOST_PGO:-}" ]; then
        host_pgo=$root/$PROFILE_HOST_PGO
    fi
fi

if [ "$app_only" -eq 1 ]; then
    [ -z "$iso" ] && [ -z "$published_app" ] || die "--app-only takes no disc and no --app"
    [ "$platform" = macos ] || [ -n "$ipa" ] || die "--app-only needs --ipa OUT.ipa"
    [ -z "$identity" ] || die "--app-only makes the unsigned published IPA; remove --identity"
else
    [ -n "$iso" ] || die "usage: scripts/builder/build.sh DISC.iso [--ipa OUT.ipa] [options] (--help)"
    [ -f "$iso" ] || die "disc image not found: $iso"
    iso=$(cd "$(dirname "$iso")" && pwd)/$(basename "$iso")
fi
if [ "$platform" = macos ]; then out=${out:-$root/build/macos}; fi
out=${out:-$root/$PROFILE_DEFAULT_OUT}
mkdir -p "$out"
out=$(cd "$out" && pwd)
case "$out" in
    "$root") die "--out must not be the source checkout itself; use build/device" ;;
    "$root"/*) git check-ignore -q "$out/" || die "--out inside this checkout must be git-ignored; use build/device" ;;
    *) echo "builder: using external private build directory $out" ;;
esac
if [ "$platform" != macos ] && [ -n "$identity" ] && [ -z "$profile" ]; then die "--identity needs --profile"; fi
if [ -n "$install_device" ] && [ -z "$identity" ]; then die "--install needs --identity and --profile"; fi
for f in ${composite_pgo[@]+"${composite_pgo[@]}"} "$host_pgo" "$profile" "$training_save" "$published_app"; do
    [ -z "$f" ] || [ -f "$f" ] || die "file not found: $f"
done
abspath() { echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"; }
[ -z "$host_pgo" ] || host_pgo=$(abspath "$host_pgo")
[ -z "$published_app" ] || published_app=$(abspath "$published_app")
if [ -n "$ipa" ]; then
    case "$ipa" in *.ipa) ;; *) die "--ipa needs a file name ending in .ipa" ;; esac
    mkdir -p "$(dirname "$ipa")"
    ipa=$(abspath "$ipa")
    # The IPA holds game code: keep it out of anything git could commit.
    case "$ipa" in "$root"/*)
        git check-ignore -q "$ipa" || die "$ipa is inside the repository but not ignored; write it under build/ or outside the repository" ;;
    esac
fi
pgo_flags() {
    python3 - "$1" <<'PY_FLAGS'
import shlex, sys
print(shlex.quote('-fprofile-instr-use=' + sys.argv[1]),
      '-Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date -Wno-backend-plugin '
      '-mllvm -enable-machine-outliner=never')
PY_FLAGS
}
logs=$out/logs
mkdir -p "$logs"
source_commit=$(git rev-parse HEAD)
source_modified=false
[ -z "$(git status --porcelain)" ] || source_modified=true
run() {  # run LOGNAME command...: periodic progress plus complete file log
    local log=$logs/$1.log; shift
    if ! python3 "$root/scripts/builder/run_stage.py" --log "$log" -- "$@"; then
        die "failed: $* (full log $log)"
    fi
}

if [ "$app_only" -eq 1 ]; then
    echo "Building the $PROFILE_APP_NAME app without game code"
else
    echo "Building $PROFILE_TITLE from $iso"
fi

step "1/9 tools"
for tool in xcrun cmake ninja python3 git curl shasum clang codesign ditto; do
    command -v "$tool" >/dev/null || die "missing $tool (Xcode, CMake 3.25+ and Ninja are required; brew install cmake ninja)"
done
case "$platform" in macos) sdk=macosx ;; tvos) sdk=appletvos ;; *) sdk=iphoneos ;; esac
xcrun --sdk "$sdk" --show-sdk-path >/dev/null 2>&1 || die "the $sdk SDK is missing: install Xcode and run sudo xcode-select -s /Applications/Xcode.app"
cmake_version=$(cmake --version | awk 'NR == 1 { print $3 }')
python3 - "$cmake_version" <<'EOF' || die "CMake 3.25 or newer is required"
import sys
v = tuple(int(x) for x in sys.argv[1].split('.')[:2])
sys.exit(0 if v >= (3, 25) else 1)
EOF
profile_check_tools
echo "xcode $(xcodebuild -version | awk 'NR == 1 { print $2 }'), $sdk SDK, cmake $cmake_version, $jobs jobs"

step "2/9 dependencies"
profile_dependencies

if [ "$app_only" -eq 1 ]; then
step "app only: build $PROFILE_APP_NAME without game code (steps 3 to 8 need a disc)"
app=""
profile_build_app
[ -d "$app" ] || die "the app was not produced"
[ ! -e "$app/Frameworks/$PROFILE_MODULE" ] || die "the published app must not contain $PROFILE_MODULE"
signed="later by the player's sideloading tool"
if [ "$platform" = macos ]; then
    profile_package_mac
    signed="ad hoc"
fi
else
step "3/9 extract the game from the disc"
profile_extract

step "4/9 translate"
profile_translate

step "5/9 generate the composite source"
profile_generate
if [ "$source_only" -eq 1 ]; then
    echo
    echo "source check passed: $out/composite-src. Rerun without --source-only to compile and build the app."
    exit 0
fi

step "6/9 mods"
if [ "$mods" -eq 1 ]; then profile_mods; else echo "skipped"; fi
if [ "$module_optimizations" != none ]; then
    run module-prepare python3 "$root/scripts/builder/module_optimizations.py" prepare "$module_optimizations" --out "$out"
fi

if [ "$train_pgo" -eq 1 ]; then
    step "local optimization training (first run adds a Mac test build and about 20 minutes of playback)"
    declare -F profile_train >/dev/null || die "$game does not support local training"
    profile_train
else
    step "local optimization training"
    echo "skipped: performance must be checked for this untrained build"
fi

step "7/9 compile the game module (-O$opt_level, $device_cpu; this is the long step)"
start=$(date +%s)
module=""
profile_compile
[ -f "$module" ] || die "the game module was not produced"
echo "game module built in $(( ($(date +%s) - start) / 60 )) min: $module"

step "8/9 build, embed and sign the app"
app=""
if [ -n "$published_app" ]; then
    # The release's app, unchanged: only the player's game module is added.
    rm -rf "$out/published-app"
    mkdir -p "$out/published-app"
    ditto -x -k "$published_app" "$out/published-app"
    app=$(find "$out/published-app/Payload" -maxdepth 1 -name '*.app' -type d | head -1)
    [ -n "$app" ] || die "no app inside $published_app"
    [ ! -e "$app/Frameworks/$PROFILE_MODULE" ] || die "$published_app already contains a game module"
    echo "using the published app $published_app"
else
    profile_build_app
fi
[ -d "$app" ] || die "the app was not produced"
if [ "$platform" = macos ]; then
    profile_package_mac
    signed="${identity:-ad hoc}"
else
mkdir -p "$app/Frameworks"
cp "$module" "$app/Frameworks/$PROFILE_MODULE"
if [ -n "$identity" ]; then
    cp "$profile" "$app/embedded.mobileprovision"
    security cms -D -i "$profile" > "$out/profile.plist"
    if ! python3 "$root/scripts/builder/signing_entitlements.py" \
        "$out/profile.plist" "$out/entitlements.plist" "$PROFILE_BUNDLE_ID" "$platform"; then
        die "the provisioning profile does not authorize $PROFILE_BUNDLE_ID"
    fi
    run sign-module codesign -f -s "$identity" "$app/Frameworks/$PROFILE_MODULE"
    run sign-app codesign -f -s "$identity" --entitlements "$out/entitlements.plist" \
        --generate-entitlement-der "$app"
    signed="with $identity"
else
    rm -f "$app/embedded.mobileprovision"
    run sign-module codesign -f -s - "$app/Frameworks/$PROFILE_MODULE"
    run sign-app codesign -f -s - "$app"
    signed="ad hoc"
fi
run sign-verify codesign -v --strict "$app"
fi # device signing
fi

step "9/9 package"
if [ -n "$ipa" ]; then
    stage=$(mktemp -d "$out/ipa-stage.XXXXXX")
    mkdir -p "$stage/Payload"
    staged=$stage/Payload/$(basename "$app")
    ditto "$app" "$staged"
    # Unsigned: the sideloading tool signs it with the player's own Apple ID.
    rm -f "$staged/embedded.mobileprovision"
    find "$staged" -name _CodeSignature -type d -prune -exec rm -rf {} +
    while IFS= read -r -d '' f; do
        case "$(file -b "$f")" in *Mach-O*) codesign --remove-signature "$f" ;; esac
    done < <(find "$staged" -type f -print0)
    # Provenance, for bug reports: what this build was made from.
    cat > "$staged/BuilderProvenance.json" <<EOF
{
  "profile": "$PROFILE_NAME",
  "containsTranslatedGameCode": $([ "$app_only" -eq 1 ] && echo false || echo true),
  "source_commit": "$source_commit",
  "packaging_commit": "$(git rev-parse HEAD)",
  "source_modified": $([ "$source_modified" = false ] && [ "$source_commit" = "$(git rev-parse HEAD)" ] && [ -z "$(git status --porcelain)" ] && echo false || echo true),
  "composite_digest": "$(cat "$out/composite-src.digest" 2>/dev/null)",
  "mods": $([ "$mods" -eq 1 ] && [ "$app_only" -eq 0 ] && echo true || echo false),
  "local_training": $([ "$train_pgo" -eq 1 ] && [ "$app_only" -eq 0 ] && echo true || echo false),
  "composite_profile_sha256": "$([ ${#composite_pgo[@]} -eq 0 ] || [ "$app_only" -eq 1 ] || shasum -a 256 "$out/composite.profdata" | awk '{print $1}')",
  "module_sha256": "$([ "$app_only" -eq 1 ] || shasum -a 256 "$staged/Frameworks/$PROFILE_MODULE" | awk '{print $1}')",
  "built": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
}
EOF
    # Audit: the IPA holds the app and the translated module, never the disc,
    # saves or signing material.
    bad=$(find "$staged" \( -iname '*.iso' -o -iname '*.gcm' -o -iname '*.rvz' -o -iname '*.wbfs' \
        -o -iname '*.wia' -o -iname '*.ciso' -o -iname '*.gcz' -o -iname '*.nfs' -o -iname '*.dol' \
        -o -iname '*.rel' -o -iname '*.card' -o -iname '*.gci' -o -iname '*.sav' -o -iname '*.raw' \
        -o -name embedded.mobileprovision -o -name _CodeSignature -o -name '*.p12' \) -print)
    [ -z "$bad" ] || die "refusing to package private files: $bad"
    if [ "$app_only" -eq 1 ]; then
        [ ! -e "$staged/Frameworks/$PROFILE_MODULE" ] || die "the published app must not contain $PROFILE_MODULE"
    else
        [ -f "$staged/Frameworks/$PROFILE_MODULE" ] || die "the staged app has no $PROFILE_MODULE"
    fi
    pending_ipa=$(mktemp "${ipa}.pending.XXXXXX")
    (cd "$stage" && ditto -c -k --norsrc --keepParent Payload "$pending_ipa")
    # No "| grep -q" here: under pipefail, grep exiting early can kill unzip
    # with SIGPIPE and fail a good IPA. unzip itself fails if the member is missing.
    unzip -l "$pending_ipa" "Payload/$(basename "$app")/Info.plist" >/dev/null 2>&1 || die "the IPA has no Info.plist"
    mv "$pending_ipa" "$ipa"
    rm -rf "$stage"
    echo "IPA: $ipa ($(du -h "$ipa" | awk '{print $1}'), unsigned)"
    if [ "$app_only" -eq 1 ]; then
        echo "     The app without game code: players add their own module with --app."
    else
        echo "     It contains game code translated from your disc: keep it for yourself."
    fi
else
    echo "no IPA requested (--ipa FILE)"
fi
if [ -n "$install_device" ]; then
    run install xcrun devicectl device install app --device "$install_device" "$app"
    echo "installed on $install_device"
    if [ "$platform" = tvos ]; then
        run tvos-disc-transfer xcrun devicectl device copy to --device "$install_device" \
            --domain-type appDataContainer --domain-identifier "$PROFILE_BUNDLE_ID" \
            --source "$iso" --destination "Library/Caches/BlueWake/GZLE01.iso"
        echo "disc image copied into $PROFILE_BUNDLE_ID/Library/Caches/BlueWake/GZLE01.iso"
        run tvos-first-launch xcrun devicectl device process launch --device "$install_device" "$PROFILE_BUNDLE_ID"
    fi
fi

echo
echo "$PROFILE_APP_NAME.app: $app ($(du -sh "$app" | awk '{print $1}'), signed $signed)"
if [ "$app_only" -eq 0 ]; then
    if [ "$platform" = macos ]; then
        echo "Open $app or copy it to Applications. Saves and preferences stay in your Library/Application Support/BlueWake."
        echo "This personal app contains your disc and translated game code. Never share or upload it."
        exit 0
    fi
    echo "game module: $(shasum -a 256 "$app/Frameworks/$PROFILE_MODULE" | awk '{print $1}')"
    if [ "$platform" = tvos ]; then
        if [ -n "$install_device" ]; then
            echo "The app prepares the copied disc image automatically."
        else
            echo "Copy GZLE01.iso into Library/Caches/BlueWake with devicectl; the app prepares it automatically."
        fi
    else
        echo "On first launch the app asks for the disc image; copy it to the device with Finder or the Files app."
    fi
fi
