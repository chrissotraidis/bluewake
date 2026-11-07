# BlueWake game profile for scripts/builder/build.sh: The Legend of Zelda:
# The Wind Waker, GameCube USA (GZLE01, revision 0).
#
# A profile is sourced by the generic pipeline. It sets the variables below and
# defines the hook functions the pipeline calls in order. Everything specific to
# this game, its translator and its app lives here; the pipeline only
# orchestrates, logs, embeds, signs, packages and checks. A new port adds its own
# profile file with the same variables and hooks (docs/BUILDER.md).
#
# Hooks run with the pipeline's helpers (run LOG cmd..., die, step) and its
# variables (root, out, logs, jobs, iso, mods, opt_level, accept_new, ...):
#   profile_check_tools   extra tools this game needs
#   profile_dependencies  fetch pinned runtime and translator sources
#   profile_extract       disc image -> $out/game (verifies the disc)
#   profile_translate     $out/game -> translated C
#   profile_generate      translated C -> $out/composite-src, verified by digest
#   profile_mods          optional code mods into the composite source
#   profile_compile       composite source -> sets module=<path to PROFILE_MODULE>
#   profile_build_app     the app bundle -> sets app=<path to .app>

PROFILE_NAME=bluewake
PROFILE_TITLE="The Legend of Zelda: The Wind Waker (GameCube USA GZLE01 rev 0)"
PROFILE_APP_NAME=BlueWake
PROFILE_BUNDLE_ID=dev.bluewake.BlueWake
PROFILE_MODULE=gGZLE01_recomp.dylib
PROFILE_DEFAULT_OUT=build/device
PROFILE_HAS_MODS=1
# Bundled optimization profiles, trained on the macOS host: the runtime's
# dispatch and memory code, and the app's host code. Neither names or contains
# any game code (both pass release_gate.py). About 5 percent faster on the
# iPad than none; docs/BUILDER.md.
PROFILE_COMPOSITE_PGO=scripts/builder/profiles/bluewake/composite-rt.profdata
PROFILE_HOST_PGO=scripts/builder/profiles/bluewake/host.profdata

# RecompCore: chrissotraidis/RecompCore candidate branch codex/bluewake-mem1-alias-parity, which is
# elliotttate/RecompCore's windows-release 9618e9d (chrissotraidis 2d60636 plus
# patches/recompcore/0098-0113) plus BlueWake stability patches 0114-0130, reconciliation patches 0131-0139, logging patches 0140 and 0142, pacing patch 0141 and Elliott Tate's post-0.4.0 runtime 0143-0150, the FPS overlay fix 0151, and Elliott Tate's October 3 runtime 0152-0155, module display list textures 0156, the hardware's texgen and TEV stage counts 0157, the Linux port's shutdown fix 0158 (James Koehler-Killeen), the GameCube stick scale 0159, Aurora's user folder 0160, the window's position at creation 0161 and the FPS overlay's position 0162 (saulob), the renderer's name 0163, Smooth Motion on 8+ threads 0164, and Elliott Tate's October 4 draw fusion and GPU profiler 0165-0168, and the Wind Waker HD look's bloom, far blur, ambient occlusion, cloud shadows, sun shadows, edge light and torch light 0169-0175, and a cache flush keeping the GPU's EFB copies 0176, the Picto Box's actor ids from the current EFB 0177 (jslay88), and a resized Vulkan window keeping its surface 0178 (pdale-boop), and the copy crop for pictures taken through a widened camera 0180 (pdale-boop), and the HD look's cloud shadows from the sea's plane with dithered shades 0182, and Smooth Motion without the flicker in the HD look, clouds, wave crests and a swimming Link's ripples 0179 (pdale-boop), and sun shadows from a shadow map 0183 (pdale-boop). DolRecomp: elliotttate's copy of
# chrissotraidis 5c91d6e plus patches/dolrecomp/0019.
# Dawn source/package compatibility: patches/recompcore/0184-0185 (RecompCore #44).
# The HD look's ambient occlusion blurred and upscaled along each surface and faded with distance, its bloom screened, the far blur and lightning with the sky left sharp, its effects before the motion blur's copy of the frame, its occlusion only from what is within reach in the scene, and the sun shadow map's bias near the camera as far off and its distances read with the camera's far plane: patches/recompcore/0186 (pdale-boop).
# A texture pack's optional/ folders, loaded over it only when asked (HD's sky clouds): patches/recompcore/0187 (pdale-boop).
# The sun shadow map's shadows while the game draws its sun disk and the moon's from 25 degrees, and the moon's at dawn down to 8 degrees, as in Wind Waker HD: patches/recompcore/0188 (pdale-boop).
# The sea takes sun shadows but casts none, with no slope or normal bias, and the gap fill counts only what the sun shadow map holds: patches/recompcore/0189 (pdale-boop).
# The sun shadow map's sun held at 24 degrees with its shadows thinning out from there, long shadows soft toward their tips, two debug views, its shadow a colour lit by the sky, as in Wind Waker HD, three cascades, a darkness the player chooses and less acne far off: patches/recompcore/0190 (pdale-boop).
# A quad draw's three left-over vertices drawn as a triangle, as on the hardware (the stars), and the stars steady in Smooth Motion: patches/recompcore/0191 (pdale-boop).
# The scene's brightness from the game, where there is depth and not the sea (Wind Waker HD's sunlight for the hour; the sea found by a mask from its textures): patches/recompcore/0192 (pdale-boop).
# The guest-alias range query and its registry generation (RecompCore #45), and the guest alias lookup in constant time, the last hit first and then a page index (RecompCore #48): patches/recompcore/0193-0196 (irregularthanis).
# 0.10.0 pins the bluewake-next commit that merges #45, #48 and 0186-0192.
# The HD look's sun shadows, occlusion and cloud shadows leave the effects drawn over the scene alone (the spin attack's rings, the Master Sword's glow), as Wind Waker HD does: patches/recompcore/0206 (pdale-boop).
# Wind Waker HD's facing term in the sun shadows, and its light for the hour in their shade (aurora_set_sun_palette); the gap fill off; the slope term as far off near the camera: patches/recompcore/0207 (pdale-boop).
# The facing term from Wind Waker HD's own normals, by the GameCube's vertex positions (test switch): patches/recompcore/0208 (pdale-boop).
# Wind Waker HD's shadow bias, terrain shadow mix and light rule (aurora_set_sun_hd_light), and its normals for the stage in play from the host (aurora_set_hd_normals): patches/recompcore/0209 (pdale-boop).
# Wind Waker HD's bloom, SSAO, rim light and cloud shadows (with its cloud texture from the host), and its own shadow pass as the "hd" shadow quality: patches/recompcore/0210 (pdale-boop).
# The shadow pre-pass blended with the last frames' by distance (test switch): patches/recompcore/0211 (pdale-boop).
# The palms shaded and lit as Wind Waker HD's (skinned scenery as scenery, smooth normals, HD's vertex colours and light; test switches): patches/recompcore/0212 (pdale-boop).
# Wind Waker HD's shadow cascade splits from the game's camera each frame (aurora_set_sun_cascade_splits): patches/recompcore/0213 (pdale-boop).
# Wind Waker HD's normals and colours for the objects (aurora_set_hd_object_tables), and its light on the scenery (test switch): patches/recompcore/0214 (pdale-boop).
# Wind Waker HD's actor light (its toon shader for the characters: ramp switch times the sun shadow, hemisphere shade, rim; aurora_set_actor_light_hd; test switch): patches/recompcore/0215 (pdale-boop).
# The sea as Wind Waker HD's: the sea rooms' square of sea in the sea mask at the size it is drawn, foliage in front clearing the marks behind it, and HD's own sea term on its light: patches/recompcore/0216 (pdale-boop).
# Wind Waker HD's clouds on the characters' light, and its cloud floor from the game's clock (aurora_set_cloud_hd_floor): patches/recompcore/0217 (pdale-boop).
# Wind Waker HD's cascades that follow the camera, each fitted to its slice of the view (DOL_AURORA_SUN_SHADOW_CASCADES_FOLLOW=0 keeps the old ones): patches/recompcore/0218 (pdale-boop).
# Alpha-tested casters of more than one texture cut out by the texture their alpha is from, and the game's blended, alpha-tested grass with them: patches/recompcore/0219 (pdale-boop).
# Wind Waker HD's light on the Mirror Shield, the sword's grip, ice, Chus, bombs and chests (three more toon patterns), with the GameCube's effect light on it: patches/recompcore/0220 (pdale-boop).
# Wind Waker HD's far blur: its linear ramp from 25,000 to 50,000 and its level for the hour from the host (aurora_set_far_blur_level): patches/recompcore/0221 (pdale-boop).
# Wind Waker HD's light buffer: the game's point and effect lights on the characters' light (x 1.75) and the lit scenery's (x 1.25), the shade leaving the lamps' light, no torch glow or effect-light stage with it: patches/recompcore/0222 (pdale-boop).
# Each character's own palettes on Wind Waker HD's actor light, its toon registers against the GameCube's global actor C0 and K0 (aurora_set_actor_light_hd_own): patches/recompcore/0223 (pdale-boop).
# Wind Waker HD's caster culling in its lit rooms (aurora_set_sun_caster_cull), and its lava glow there from the pack's version 2 room colours: patches/recompcore/0224 (pdale-boop).
# Wind Waker HD's monotone at the game's grey composite (Frozen Hyrule's sepia; aurora_set_monotone_hd): patches/recompcore/0225 (pdale-boop).
# Wind Waker HD's fog curve, the square of the GameCube's linear fog while the host has HD's distances in place (aurora_set_fog_hd), and HD's fog after the shade, from HD's start where the game clamps it at 0 (aurora_set_fog_hd_starts): patches/recompcore/0226 (pdale-boop).
# Wind Waker HD's hemisphere per vertex on all smooth scenery, from its own normals blended across each triangle, not renormalized: patches/recompcore/0227 (pdale-boop).
# Wind Waker HD's fog start before the camera on the draws' own fog too (characters, plants, objects): patches/recompcore/0228 (pdale-boop).
# A texture pack's optional/ folders loaded or dropped while running (dol_aurora_set_texture_pack_optional), with the decode cache cleared when the replacements change: patches/recompcore/0229 (pdale-boop).
# Dragon Roost Cavern's lava glow, HD's albedo x lava K x 4 x (1 - the vertex alpha) squared, on the GameCube's three-stage lava materials: patches/recompcore/0230 (pdale-boop).
# The Tower of the Gods' water floor summed in linear light, as Wind Waker HD's shader for it (ShaderKey::hd_water): patches/recompcore/0231 (pdale-boop).
# Wind Waker HD's fog mixed in linear light, on the draws and in the sun pass, from a copy of the scene taken just before it: patches/recompcore/0232 (pdale-boop).
# Stable sun-shadow cascades as a switch (DOL_AURORA_SUN_SHADOW_CASCADES_STABLE): each cascade in the sun's frame, its box the square about the sphere round its slice of the view: patches/recompcore/0233 (pdale-boop).
# Effects that write depth left out of the HD look's shading too: the game's particles and glow models (Dragon Roost Cavern's lava geysers, a door's dust): patches/recompcore/0234 (pdale-boop).
# The heat haze in a monotone frame (Xboss0's sepia replays) samples the frame with Wind Waker HD's monotone in: patches/recompcore/0235 (pdale-boop).
# Wind Waker HD's own cascade fit as a test switch (DOL_AURORA_SUN_SHADOW_CASCADES_HD=1 or 2, off), and the pre-pass shadow in the temporal debug view: patches/recompcore/0236 (pdale-boop).
# The stable sun-shadow cascades by default (DOL_AURORA_SUN_SHADOW_CASCADES_STABLE=0 gives back HD's frame that turns with the camera): patches/recompcore/0237 (pdale-boop).
# The lava geysers' core and a door's dust left unshaded too: glow models may take the rasterized colour when their textures only cut the shape, effects at their own depth keep their share near or far, and a blended effect's share by its opacity: patches/recompcore/0238 (pdale-boop).
# Past the second sun-shadow cascade only the stage's BG lists cast, as in Wind Waker HD, from the host's mark in BP register 0x7A (DOL_AURORA_SUN_SHADOW_FAR_CASTERS=all keeps every caster): patches/recompcore/0239 (pdale-boop).
# The sun shadows' smooth normals and HD colours kept per draw from frame to frame, compared in full (DOL_AURORA_SUN_SHADOW_NORMAL_CACHE=0 makes them every frame): patches/recompcore/0240 (pdale-boop).
RECOMPCORE_URL=https://github.com/chrissotraidis/RecompCore.git
RECOMPCORE_SHA=89fb4b8590c19f007eb5613043a56badbcd1bd9a
DOLRECOMP_SHA=b8b534591cba8ca7cd43943a655ee6e2591cf5de
DAWN_URL=https://github.com/encounter/dawn/releases/download/v20260618.032059/dawn-ios-arm64.tar.gz
DAWN_SHA256=ada0bafc173152d80eba7c3b2f9609a71185d5809cbd5dd3251b91a0803a7ae2
# Digest of the generated composite source (scripts/ios/composite_manifest.py)
# for GZLE01 USA rev 0 with the translator above: 754 files (the mod-ready
# dispatcher and an empty mod_variants.inc since 2026-09-26; docs/MODS.md).
COMPOSITE_DIGEST=54f54434c3f9c899d43a96373dc0b4c1aed0e50db8b820b9698dfa76571a770a

profile_check_tools() {
    # The mods need only python3's standard library: Better Wind Waker's
    # settings are game options built from mods/betterww/options.txt, not its
    # patcher (which needed PyYAML and Pillow).
    :
}

profile_dependencies() {
    recompcore=$root/ref/recompcore
    local fresh_clone=0
    if [ ! -e "$recompcore/.git" ]; then
        if [ -e "$recompcore" ] && [ -n "$(ls -A "$recompcore")" ]; then
            die "ref/recompcore exists but is not a git checkout: move it aside and rerun"
        fi
        mkdir -p "$recompcore"
        git -C "$recompcore" init -q
        fresh_clone=1
    fi
    if [ "$(git -C "$recompcore" rev-parse HEAD 2>/dev/null || true)" != "$RECOMPCORE_SHA" ]; then
        if [ -n "$(git -C "$recompcore" status --porcelain --untracked-files=no 2>/dev/null || true)" ]; then
            die "ref/recompcore has local changes and is not at $RECOMPCORE_SHA: move it aside and rerun"
        fi
        echo "fetching RecompCore $RECOMPCORE_SHA (GXRuntime, vendored Aurora, DolRecomp pointer)"
        git -C "$recompcore" remote remove bluewake >/dev/null 2>&1 || true
        git -C "$recompcore" remote add bluewake "$RECOMPCORE_URL"
        if [ "$fresh_clone" -eq 1 ]; then
            run recompcore-fetch git -C "$recompcore" fetch --recurse-submodules=no --depth 1 bluewake "$RECOMPCORE_SHA"
        else
            run recompcore-fetch git -C "$recompcore" fetch --recurse-submodules=no bluewake "$RECOMPCORE_SHA"
        fi
        git -C "$recompcore" checkout -q --detach FETCH_HEAD
    fi
    [ "$(git -C "$recompcore" rev-parse HEAD)" = "$RECOMPCORE_SHA" ] || die "ref/recompcore is not at $RECOMPCORE_SHA"
    git -C "$recompcore" submodule sync -q -- DolRecomp
    if [ "$(git -C "$recompcore/DolRecomp" rev-parse HEAD 2>/dev/null || true)" != "$DOLRECOMP_SHA" ]; then
        run dolrecomp-fetch git -C "$recompcore" submodule update --init --depth 1 -- DolRecomp
    fi
    [ "$(git -C "$recompcore/DolRecomp" rev-parse HEAD)" = "$DOLRECOMP_SHA" ] || die "ref/recompcore/DolRecomp is not at $DOLRECOMP_SHA"
    if [ -n "$(git -C "$recompcore" status --porcelain --untracked-files=no)" ] ||
       [ -n "$(git -C "$recompcore/DolRecomp" status --porcelain --untracked-files=no)" ]; then
        die "ref/recompcore has local changes; the build must use the pinned source exactly"
    fi
    echo "RecompCore $RECOMPCORE_SHA, DolRecomp $DOLRECOMP_SHA"

    deps=$root/build/deps
    mkdir -p "$deps"
    # Desktop Aurora resolves its pinned macOS Dawn package itself.
    [ "$platform" != macos ] || return 0
    local dawn_tar=$deps/dawn-ios-arm64.tar.gz
    if [ ! -f "$dawn_tar" ] || [ "$(shasum -a 256 "$dawn_tar" | awk '{print $1}')" != "$DAWN_SHA256" ]; then
        run dawn-download curl -fL -o "$dawn_tar" "$DAWN_URL"
    fi
    [ "$(shasum -a 256 "$dawn_tar" | awk '{print $1}')" = "$DAWN_SHA256" ] || die "Dawn package checksum mismatch"
    if [ ! -f "$deps/dawn-ios/lib/cmake/Dawn/DawnConfig.cmake" ]; then
        rm -rf "$deps/dawn-ios" && mkdir -p "$deps/dawn-ios"
        tar xzf "$dawn_tar" -C "$deps/dawn-ios"
    fi
    echo "Dawn iOS package $DAWN_SHA256"
    if [ "$platform" = tvos ]; then
        local dawn_tvos=$deps/dawn-tvos
        if [ ! -f "$dawn_tvos/lib/cmake/Dawn/DawnConfig.cmake" ] ||
           [ "$(cat "$dawn_tvos/retagged-from" 2>/dev/null || true)" != "$DAWN_SHA256" ]; then
            [ ! -e "$dawn_tvos" ] || die "existing Dawn tvOS cache does not match the pin; move it aside and rerun"
            ditto "$deps/dawn-ios" "$dawn_tvos"
            python3 "$root/scripts/ios/retag_macho_platform.py" --platform tvos \
                "$dawn_tvos/lib/libwebgpu_dawn.a" "$dawn_tvos/lib/libwebgpu_dawn.a"
            ranlib "$dawn_tvos/lib/libwebgpu_dawn.a"
            printf '%s\n' "$DAWN_SHA256" > "$dawn_tvos/retagged-from"
        fi
        echo "Dawn arm64 archive prepared for tvOS from the pinned iOS package"
    fi
}

profile_extract() {
    mkdir -p "$out/tools"
    run disc-extract-build clang -O2 -o "$out/tools/disc_extract" scripts/ios/disc_extract.c \
        apple/ios/src/disc_import.c -Iapple/ios/src
    # disc_extract checks the disc ID (GZLE01) and the executable's hash
    # (revision 0) and refuses anything else.
    run disc-extract "$out/tools/disc_extract" "$iso" "$out/game"
    [ "$(ls "$out/game/rels" | wc -l | tr -d ' ')" = 415 ] || die "expected 415 RELs in $out/game/rels"
    echo "main.dol and 415 RELs in $out/game"
}

profile_translate() {
    run dolrecomp-configure cmake -S "$recompcore/DolRecomp" -B "$out/dolrecomp" -G Ninja -DCMAKE_BUILD_TYPE=Release
    run dolrecomp-build cmake --build "$out/dolrecomp" --target dolrecomp -j "$jobs"
    local dolrecomp=$out/dolrecomp/dolrecomp
    rm -rf "$out/translated.new" && mkdir -p "$out/translated.new"
    run translate-dol "$dolrecomp" --gamecube --backend c --cpu gekko --partition-instructions 4096 \
        "$out/game/main.dol" "$out/translated.new/dol" -j "$jobs"
    run translate-rels "$dolrecomp" --gamecube --backend c --cpu gekko --rel-base 0xC0400000 \
        "$out/game/rels" "$out/translated.new/rels" -j "$jobs"
    rm -rf "$out/translated" && mv "$out/translated.new" "$out/translated"
    echo "translated: $(ls "$out/translated/dol/generated/chunks" | wc -l | tr -d ' ') DOL chunks, $(ls "$out/translated/rels/generated/rels" | wc -l | tr -d ' ') RELs"
}

profile_generate() {
    rm -rf "$out/composite-src.new"
    run composite-generate python3 scripts/generate_composite.py \
        --dol-dir "$out/translated/dol/generated" --rels-dir "$out/translated/rels/generated/rels" \
        --rels-bin-dir "$out/game/rels" --main-dol "$out/game/main.dol" --output-dir "$out/composite-src.new"
    tail -1 "$logs/composite-generate.log"
    local digest
    digest=$(python3 scripts/ios/composite_manifest.py "$out/composite-src.new" | awk '{print $1}')
    if [ "$digest" = "$COMPOSITE_DIGEST" ]; then
        echo "composite source digest $digest: the verified tree"
    elif [ "$accept_new" -eq 1 ]; then
        echo "composite source digest $digest differs from the verified $COMPOSITE_DIGEST (accepted)"
    else
        die "composite source digest $digest differs from the verified $COMPOSITE_DIGEST (wrong disc revision or translator?); --accept-new-composite overrides"
    fi
    # Reuse only a verified tree made by the same generators and mod selection.
    # Checking the base digest alone used to retain mod variants after --no-mods.
    local inputs current saved optimization_recipe
    optimization_recipe=$(python3 "$root/scripts/builder/module_optimizations.py" fingerprint "$module_optimizations" ${lean_arg:-})
    inputs=$( { printf '%s\n' "$digest" "$mods" "$optimization_recipe"; shasum -a 256 \
        "$root/scripts/mods/"*.py "$root/scripts/mods/"*.sh \
        "$root/mods/widescreen/"*.gecko "$root/mods/betterww/options.txt"; } | shasum -a 256 | awk '{print $1}')
    current=""
    if [ -d "$out/composite-src" ]; then
        current=$(python3 scripts/ios/composite_manifest.py "$out/composite-src" | awk '{print $1}')
    fi
    saved=$(cat "$out/composite-final.digest" 2>/dev/null || true)
    if [ -n "$current" ] && [ "$current" = "$saved" ] && \
       [ "$(cat "$out/composite-inputs.digest" 2>/dev/null || true)" = "$inputs" ]; then
        rm -rf "$out/composite-src.new"
    else
        if [ -d "$out/composite-src" ]; then
            local previous
            previous=$(mktemp -d "$out/composite-previous.XXXXXX")
            mv "$out/composite-src" "$previous/source"
            echo "previous generated source preserved at $previous/source"
        fi
        mv "$out/composite-src.new" "$out/composite-src"
        echo "$digest" > "$out/composite-src.digest"
        echo "$inputs" > "$out/composite-inputs.digest"
        echo "$digest" > "$out/composite-final.digest"
        printf '%s\n' pending > "$out/mods.done"
    fi
}

profile_mods() {
    # Widescreen, Better Wind Waker's options and both together, as variants
    # compiled into the same module (docs/MODS.md). Done once per composite source.
    if [ "$(cat "$out/mods.done" 2>/dev/null || true)" = complete ]; then
        echo "mods already in $out/composite-src"
        return
    fi
    run mods scripts/mods/build_mods.sh "$out" "$iso"
    python3 scripts/ios/composite_manifest.py "$out/composite-src" | awk '{print $1}' > "$out/composite-final.digest"
    printf '%s\n' complete > "$out/mods.done"
    echo "widescreen and Better Wind Waker variants added"
}

profile_train() {
    local args=(--disc "$iso" --out "$out" --jobs "$jobs" --module-optimizations "$module_optimizations")
    [ -z "$training_save" ] || args+=(--save "$training_save")
    [ "${direct_gate:-0}" -eq 0 ] || args+=(--direct-gate)
    # Lean memory makes the instrumented module too large to link at -O0.
    case " ${lean_arg:-} " in *" --lean-memory "*) args+=(--instrumented-opt-level 1) ;; esac
    run local-training python3 "$root/scripts/builder/train_local_pgo.py" "${args[@]}"
    [ -s "$out/pgo-local/composite.profdata" ] || die "local training produced no game profile"
    composite_pgo+=("$out/pgo-local/composite.profdata")
    # Headless boot training does not exercise the renderer; retain the
    # existing host profile until rendered local training is validated.
}

profile_compile() {
    local cmake_system=iOS sdk=iphoneos build_dir="$out/composite-ios" deployment=17.0
    if [ "$platform" = macos ]; then
        cmake_system=Darwin sdk=macosx build_dir="$out/composite-macos" deployment=14.0
    fi
    if [ "$platform" = tvos ]; then
        cmake_system=tvOS sdk=appletvos build_dir="$out/composite-tvos"
    fi
    local flags="-mcpu=$device_cpu" feature_flags feature
    local optimization_flags=()
    feature_flags=$(python3 "$root/scripts/builder/module_optimizations.py" flags "$module_optimizations")
    while IFS= read -r feature; do optimization_flags+=("$feature"); done <<< "$feature_flags"
    if [ ${#composite_pgo[@]} -gt 0 ]; then
        run composite-pgo-merge xcrun llvm-profdata merge -o "$out/composite.profdata" "${composite_pgo[@]}"
        # The profile is a compiler input but not a C header dependency. Put
        # its hash in the flag so Ninja recompiles when the counters change.
        local profile_hash profile_path
        profile_hash=$(shasum -a 256 "$out/composite.profdata" | awk '{print $1}')
        mkdir -p "$out/profiles"
        profile_path=$out/profiles/composite-$profile_hash.profdata
        cp "$out/composite.profdata" "$profile_path"
        flags="$flags $(pgo_flags "$profile_path")"
        echo "with the composite profile(s): ${composite_pgo[*]}"
    fi
    # Tiered, as on Windows and Linux: the chunks the profile never ran (504 of 813
    # for 0.7.0, a third of the compile) at -O1, the rest at -O2. Empty clears it.
    local cold_file=""
    if [ "$tiered" -eq 1 ] && [ "$opt_level" = 2 ] && [ ${#composite_pgo[@]} -gt 0 ]; then
        cold_file=$out/composite-cold-sources.txt
        run composite-cold-sources python3 "$root/scripts/builder/cold_sources.py" \
            "$out/composite.profdata" "$out/composite-src" "$cold_file"
        tail -1 "$logs/composite-cold-sources.log"
    fi
    run "composite-configure-$platform" cmake -S cmake/composite -B "$build_dir" -G Ninja \
        "-DCMAKE_SYSTEM_NAME=$cmake_system" "-DCMAKE_OSX_SYSROOT=$sdk" -DCMAKE_OSX_ARCHITECTURES=arm64 \
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_C_FLAGS=$flags" \
        -DCOMPOSITE_OPTIMIZATION_LEVEL="$opt_level" "-DCOMPOSITE_COLD_SOURCES_FILE=$cold_file" "${optimization_flags[@]}" \
        -DCOMPOSITE_DIR="$out/composite-src" -DGXRUNTIME_DIR="$recompcore/GXRuntime" \
        -DABI_DIR="$recompcore/Source/Core/Core/PowerPC/StaticRecomp"
    run "composite-build-$platform" cmake --build "$build_dir" -j "$jobs"
    module=$build_dir/$PROFILE_MODULE
}

profile_build_app() {
    local cmake_system=iOS sdk=iphoneos app_build="$out/app" dawn_dir="$deps/dawn-ios"
    local tvos_flag=OFF
    if [ "$platform" = tvos ]; then
        cmake_system=tvOS sdk=appletvos app_build="$out/app-tvos"
        dawn_dir="$deps/dawn-tvos"
        tvos_flag=ON
    fi
    local host_flags=""
    if [ -n "$host_pgo" ]; then
        local profile_hash profile_path
        profile_hash=$(shasum -a 256 "$host_pgo" | awk '{print $1}')
        mkdir -p "$out/profiles"
        profile_path=$out/profiles/host-$profile_hash.profdata
        cp "$host_pgo" "$profile_path"
        host_flags=$(pgo_flags "$profile_path")
        echo "with the host profile $host_pgo"
    fi
    if [ "$platform" = macos ]; then
        app_build="$out/app-macos"
        run app-configure-macos cmake -S scripts/builder/training -B "$app_build" -G Ninja \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF \
            -DAURORA_DAWN_PROVIDER=package -DAURORA_DAWN_LINKAGE=static \
            -DAURORA_SDL3_PROVIDER=vendor -DAURORA_SDL3_LINKAGE=static \
            '-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local' -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON \
            "-DCMAKE_C_FLAGS=$host_flags" "-DCMAKE_CXX_FLAGS=$host_flags"
        run app-build-macos cmake --build "$app_build" --target bluewake_host -j "$jobs"
        app=$app_build/host/BlueWake.app
        return
    fi
    run "app-configure-$platform" cmake -S apple/ios -B "$app_build" -G Ninja \
        "-DCMAKE_SYSTEM_NAME=$cmake_system" "-DCMAKE_OSX_SYSROOT=$sdk" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DPNG_SHARED=OFF \
        "-DBLUEWAKE_TVOS=$tvos_flag" \
        -DAURORA_DAWN_PROVIDER=system -DDawn_DIR="$dawn_dir/lib/cmake/Dawn" \
        -DAURORA_SDL3_PROVIDER=vendor -DAURORA_SDL3_LINKAGE=static -DAURORA_DAWN_LINKAGE=static \
        "-DCMAKE_C_FLAGS=$host_flags" "-DCMAKE_CXX_FLAGS=$host_flags" \
        '-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local' -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON
    run "app-build-$platform" cmake --build "$app_build" --target BlueWake -j "$jobs"
    app=$app_build/BlueWake.app
}

# Assemble a movable personal desktop app; never install over player data.
profile_package_mac() {
    local args=(--app "$app" --output "$out/packaged/BlueWake.app"
        --runtime "$recompcore" --source-commit "$source_commit"
        --identity "${identity:--}" --module-optimizations "$module_optimizations")
    if [ "$app_only" -eq 0 ]; then
        args+=(--module "$module" --game "$out/game" --disc "$iso")
    fi
    if [ "$source_modified" != false ] || [ "$source_commit" != "$(git rev-parse HEAD)" ] ||
       [ -n "$(git status --porcelain)" ]; then
        args+=(--source-modified)
    fi
    run package-macos python3 "$root/scripts/builder/package_macos.py" "${args[@]}"
    app=$out/packaged/BlueWake.app
}
