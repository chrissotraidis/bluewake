# Aurora Assessment (encounter/aurora @ 1d10fa1, 2026-08-06)

Condensed from full source inspection (2026-08-09). Aurora is a source-level
GameCube/Wii SDK compatibility layer (MIT, by Luke Street / Metaforce team),
and is the graphics+SDK substrate Dusklight shipped on ("Powered by Aurora").

## What it provides (verified in source)

- **GX:** not an API shim — a FIFO-accurate reimplementation. GX calls encode
  real big-endian GP commands; a 1,962-line command processor decodes
  BP/CP/XF loads. Display lists fully supported (+ an optimizer that
  pre-indexes triangles). TEV complete: all 16 stages, konst, swap tables,
  fog, alpha compare, ZTexture, **indirect texturing** (65 ind-tex call sites
  in tww game code — covered, incl. GXSetTevIndTile/IndWarp/IndBump*).
- **EFB copies: implemented properly** (resolve render pass → cached texture
  keyed by dest pointer; later GXInitTexObj on that address binds the resolved
  texture). Conversion shaders for I4/I8/IA4/IA8/RGB565/RGBA8/Z8/Z16 with
  correct BT.601 luma. Cross-checked against tww call sites:
  - m_Do_graphic.cpp:657 Z16 depth copy — covered
  - m_Do_graphic.cpp:238 I8 grayscale framebuffer — covered
  - d_drawlist.cpp:1533 I4 128×128 — covered
  - d_ovlp_fade3/4 RGBA8, d_gameover/d_msg RGB565 — covered
  - **Gap: `GXPeekARGB` (d_snap.cpp:1952, Picto Box) unimplemented.** Aurora's
    483-line `depth_peek.cpp` (async snapshot for GXPeekZ) is the template for
    a color equivalent. Well-scoped; can ship v1 degraded.
- **Backend:** WebGPU via Google Dawn; WGSL shader gen; on Apple forces
  `DAWN_ENABLE_METAL=ON`; surface via SDL_MetalView→CAMetalLayer. **Prebuilt
  Dawn binaries pinned for `darwin-arm64` AND `ios-arm64`** (v20260618).
  SQLite+zstd persistent pipeline cache (shader-stutter mitigation). Tracy
  GPU profiling. Dolphin-format HD texture pack support (1,919 lines).
- **Input:** SDL3 (release-3.4.10) gamepad → GameController.framework on
  Apple; 1,833-line extended PAD (rebinding, gyro, rumble, GC adapter);
  **iOS CoreHaptics rumble in `lib/device_ios.mm`**.
- **DVD:** full (1,478 lines) backed by nod — reads ISO/GCM/RVZ directly.
- **CARD:** full, Dolphin .gci/.raw-compatible (kabufuda-derived).
- **MTX/GD/SI/PAD:** full. **VI:** thin stub (window control); retrace loop
  left to the game project.
- **OS:** arena/heap/time/report implemented; **OSThread/OSMutex/OSMessage/
  OSAlarm/OSInterrupt/OSContext/OSCache NOT implemented** (headers only) —
  Dusklight supplies these (~1,190-line stubs.cpp + OSThread.cpp etc.).
- **Audio: ZERO.** No AI/DSP/AX/THP implementation, no SDL audio init. ARAM
  emulation only. Dusklight's `src/dusk/audio/` (1,060 lines incl. 779-line
  software DSP) is the blueprint; WW additionally needs THP audio/video (cutscene-
  less WW uses THP rarely — verify usage).

## Platform reality

- macOS ARM64: first-class, CI-covered (macos-latest runners are arm64).
- iOS: builds supported (prebuilt Dawn ios-arm64, device_ios.mm, SDL iOS
  path) and **proven by Dusklight shipping an IPA**, but Aurora has no iOS
  CI and no app-shell (Info.plist/touch UI are the game project's job).
- Quality signals: 89 TODOs total in 35.5k lines, zero in gfx core; 238/238
  tests pass locally on Apple Silicon (E6). ~440 of 524 commits in last 6
  months (post-Dusk revival). Bus factor: 58% single author.

## Verdict for BlueWake

Aurora is the correct GX/SDK layer for a Wind Waker native port. The
Wind-Waker-specific deltas are: GXPeekARGB (Picto Box), audio (JAudio2 via
adapted DuskDsp), OS threads/VI (copy from Dusklight), ~200 lines of trivial
GX stubs (GXSetMisc/GXAbortFrame/GXColor4x8/perf counters). None are
research-grade unknowns.
