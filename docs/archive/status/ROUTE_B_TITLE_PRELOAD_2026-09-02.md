# Route B Complete Logo Preload Result

**Date:** 2026-09-02  
**Resolved blocker:** `BW-P4-0083` / `ROUTE_B_TITLE_PRELOAD_CLOSURE`  
**Successor:** `BW-P4-0084` / `ROUTE_B_OPENING_SCENE_ADMISSION`  
**Input alias:** `GZLE01-disc-image`  
**Platform:** Apple Silicon macOS

## Hypothesis

The complete original logo phase-two request block can reuse the qualified
DVD, JKR MEM/ARAM, raw-file, and native object-resource owners. Once all
requests complete, unchanged original `dvdWaitDraw` should request the retail
opening-scene transition without admitting title J3D or DSP policy.

## Result

Pass. One private-disc process executes original phases 0, 1, and 2. Phase 2
issues 26 archive commands, four raw-file commands, and three dRes requests.
All 26 archives are mounted, all four raw buffers are nonempty, and the
existing qualified native object-resource owner validates `Always`, `Link`,
and `Agb`. Unchanged original `dvdWaitDraw` observes the complete set, sees
reset clear, and emits exactly one opening-scene transition.

Patch 0032 exposes only the request block and `dvdWaitDraw` in the bounded
composition tier. Code after request issuance remains excluded so display/
reset/audio policy and downstream title J3D are not satisfied by probe stubs.
The pre-existing composition control still emits the 992-byte Nintendo quad.

## Verification

- Complete preload probe passes Debug and Release.
- Complete preload probe passes strict ASan/UBSan.
- Original bounded composition control passes after the source partition.
- Patch stack 0001-0032 replays cleanly.
- Only one BlueWake/probe process ran at a time.
- Repository audit still fails only on the same 20 tracked files under ignored
  `local-research/`; disposition remains user-owned.

## Adaptation Stop

Patch 0032 changes 35 lines in the logo owner. Cumulative logo adaptation is
139 changed lines across 1,198 compiled source/header lines (11.60%), above the
10% stop condition. Further `d_s_logo` adaptation is closed. The next owner is
the original opening scene selected by the authentic transition.

## Successor

Run one deterministic source/link census over original `d_s_open.cpp` and its
reached JStudio/J2D/process dependencies. Select the first executable opening
boundary from observed missing owners; do not begin broad J3D or actor work.
