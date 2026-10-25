# Route B Authentic ARAM Archive Result

**Date:** 2026-09-02  
**Blocker:** `BW-P4-0083` / `ROUTE_B_TITLE_PRELOAD_CLOSURE`  
**Input alias:** `GZLE01-disc-image`  
**Platform:** Apple Silicon macOS

## Hypothesis

The original `JKRAramArchive` can retain exact 0x14 serialized RARC entries on
LP64 by storing cached native pointers in the existing `JKRArchive` side
table. The USA logo preload needs only MEM and ARAM archive modes, so DVD and
COMP factory modes can fail closed in this bounded tier.

## Result

Pass. Original `JKRArchive::mount` mounted authentic
`/res/Msg/itemres.arc` in Aurora ARAM. The archive is 10,368 bytes with 16
entries. Repeated original name lookup returned the same pointer for
`check_00.bti`; original buffered read returned the same 598 bytes, and
`sizeof(JKRArchive::SDIFileEntry)` remained `0x14`.

Strict ASan initially found `JKRHeap::copyMemory` rounding a 598-byte copy up
to 600 bytes. The target-PC implementation now uses exact-length `memcpy`;
the original word-rounded implementation remains unchanged on GameCube.

Patch 0028 changes 52 lines across 1,259 compiled owner lines, an adaptation
density of 4.13%. The three-mode 1,090-line census remains excluded from the
default build; unrequested DVD and COMP archive modes remain unadmitted.

## Verification

- Focused private probe: Debug, Release, and strict ASan/UBSan pass.
- Foundation suite: 10/10 in Debug, 10/10 in Release, and 10/10 under
  ASan/UBSan.
- Patch stack 0001-0028 replays cleanly.
- Repository audit still fails only because 20 files under ignored
  `local-research/` are tracked; disposition remains user-owned.

An aggregate GX-enabled all-target build still exposes the previously recorded
Aurora-global ownership debt in unrelated non-window private probes. Scoped GX
targets and the standard foundation matrices remain the accepted build shape.

## Successor

Link the reached ARAM archive owner into the original
`mDoDvdThd_mountXArchive_c` worker path and execute one retail async
`itemres.arc` command. Only after that command publishes completion may the
loop re-enable the complete phase-two preload request fanout.
