# Dusk/Dusklight Assessment (TwilitRealm/dusk @ 7434a0f8, 2026-08-08)

Condensed from full source inspection (2026-08-09). Dusklight = shipped native
TP port (Win/macOS/Linux/Android/iOS/tvOS-wip), CC0 license, 6,776 commits.
This is the proven instance of the architecture BlueWake would use.

## Architecture facts (verified)

- **Hard fork of zeldaret/tp** (shares first commit; subtree merges from
  upstream). NOT decomp-as-submodule. The decomp itself was edited:
  1,020/1,139 src files modified, **~62k changed lines** total across
  src/include/libs. The port is "decomp + systematic portability pass,"
  not "decomp + shim."
- **100% recompiled C/C++.** Zero PPC translation/interpretation/JIT
  anywhere. Only 2 asm files in tree (MetroTRK, never compiled).
  `TARGET_PC` (1,804 uses) + `IF_DUSK`/`DUSK_IF_ELSE` macros gate divergences.
- **All 757 RELs compiled statically into one executable**; dynamic linker
  compiled out (`c_dylink.cpp` link/unlink → no-ops; profile list statically
  linked via `g_fpcPf_ProfileList_p`). RELS.arc still mounted at runtime as a
  DATA source (see dvd_asset below).
- **Graphics = Aurora GX → Dawn/WebGPU → Metal** on Apple (no MoltenVK).
  Ships `initial_pipeline_cache.db` to kill first-run shader stutter.
- **Audio =** decompiled JAudio2 + Z2AudioLib compiled directly, on top of
  `DuskDsp` — a ~25 KB host-float reimplementation of DSP *semantics*
  (ADPCM, resample, biquad, volume ramps, freeverb reverb; optional HRTF),
  NOT DSP instruction emulation. SDL3 output 32 kHz f32.
  `DspStub.cpp`: "We do not directly emulate the DSP."
- **Assets: user-supplied disc image** (ISO/GCM/RVZ/WIA/WBFS/CISO/GCZ) read
  live via nod (Rust). xxh3-128 disc validation against accepted hashes.
- **dvd_asset.cpp escape hatch:** parses the DOL/REL from the user's disc and
  memcpy's not-yet-decompiled data tables at runtime by (version, address)
  — 27 call sites. Means incomplete DATA decomp is not a blocker.

## Portability pass taxonomy (the transferable recipe — BlueWake's core cost)

1. `BE<T>` endian wrapper template — **1,058 uses** (on-disc struct fields).
2. `OffsetPtr` (32-bit stored offsets vs 64-bit host pointers) — 238 uses in
   stage/collision/path data headers.
3. `AVOID_UB` fixes — 63 uses (tp upstream had 23; **tww has 0 today** —
   BlueWake builds this surface from scratch).
4. Heap/arena growth for 64-bit: MEM1=256 MB, MEM2=24 MB, gameHeapSize×20
   ("maybe do a better fix later") — flagged as the **#1 iOS memory risk**
   (jetsam on 4 GB iPhones).
5. ABI details: `-fsigned-char` on ARM, `MULTI_CHAR()`, MWCC intrinsic
   shims (`__cntlzw`, `__rlwimi`, ...), `_SDA_BASE_` stubs, std::iterator
   deprecation suppression.
6. JSystem split into 21 static libs with GNU-ld RESCAN groups (cyclic refs).

## iOS reality (proven, inspectable)

- CMake preset `ios-default`: leetal ios.toolchain.cmake, PLATFORM=OS64,
  DEPLOYMENT_TARGET=14.0, BUILD_SHARED_LIBS=OFF, static Dawn, Rust target
  aarch64-apple-ios (nod is Rust — iOS Rust toolchain is a hard dep).
- **Live iOS CI job** (macos-latest, uploads Dusklight.app). tvOS commented out.
- Distribution: sideload IPA (AltStore/SideStore, free Apple ID), Developer
  Mode, UIFileSharingEnabled to drop disc image in Files.app. Landscape-only,
  iPhone+iPad UIDeviceFamily, CADisableMinimumFrameDurationOnPhone (ProMotion
  >60fps), LSSupportsGameMode.
- Touch controls: 80 KB RmlUi-based system w/ layout editor, safe-area,
  virtual sticks → injected via Aurora `PADSetVirtualStatus` (game-agnostic
  API). CoreHaptics rumble (aurora device_ios.mm). SDL3 owns UIKit lifecycle
  and AVAudioSession (never touched directly).

## Reusable for BlueWake

- ~100%: aurora (MIT), borealis (encounter/borealis — app shell: disc
  verify, data dirs, file picker, crash/Sentry, updater), OSThread/OSMutex/
  OSContext/OSReport (+90% of stubs.cpp), DuskDsp audio stack, helpers
  (BE/OffsetPtr), build system/CI/platform shells (rename+rebrand),
  touch-controls scaffold.
- 0% (game code, replaced by tww): DOLZEL files, RELs, SSystem, Z2AudioLib.
  **Audio caveat: TP=JAudio2, TWW=JAudio1 (different library, 66 files in
  tww/src/JSystem/JAudio + 8 in src/JAZelAudio)** — DuskDsp's hardware-level
  model reusable; the library layer must come from tww's decomp.
- JSystem: 15/17 subsystems overlap by name but TWW's is an earlier revision
  (+JRenderer unique to TWW) — dusk's ~12k lines of libs/ fixes are a
  template, not a patch.
- No Wind Waker plans anywhere in dusk repo (exhaustive grep: zero hits).

## Conclusion

Dusk empirically validates every layer BlueWake needs on Apple platforms and
is legally reusable (CC0). The delta to Wind Waker = (a) finish/await tww
decomp matching, (b) redo the ~50-60k-line portability pass over tww's tree,
(c) JAudio1 vs JAudio2 audio work, (d) GXPeekARGB (Picto Box), (e) rebrand
shells. Of these, only (a) is not ordinary engineering.
