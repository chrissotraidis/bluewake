# Route B Asynchronous ARAM Archive Result

**Date:** 2026-09-02  
**Blocker:** `BW-P4-0083` / `ROUTE_B_TITLE_PRELOAD_CLOSURE`  
**Input alias:** `GZLE01-disc-image`  
**Platform:** Apple Silicon macOS

## Hypothesis

The promoted DVD worker can execute original
`mDoDvdThd_mountXArchive_c` for an authentic ARAM archive, publish command
completion, and preserve the direct-mount resource result without adding a
second archive owner.

## Result

Pass. One process starts original `mDoDvdThd`, submits authentic
`/res/Msg/itemres.arc` as `MOUNT_ARAM`, waits on the original command's
`sync()`, and receives a mounted archive with 16 files. Repeated original
lookup returns one stable pointer for the 598-byte `check_00.bti`; buffered
read returns identical bytes.

The first run crashed in `JKRAramStream::writeToAram`: retail
`mDoDvdThd::main` clears its current heap, and native worker scheduling does
not reproduce GameCube's thread-switch heap restoration. Patch 0029 gives the
stream transfer buffer the system heap on target PC.

Strict UBSan then exposed native-layout debt in `JKRExpHeap`: `CMemBlock`
contains native pointers and requires eight-byte alignment, while retail
splits rounded allocation sizes to four. Patch 0030 rounds target-PC splits
and resize to `alignof(CMemBlock)` and uses `sizeof(CMemBlock)` when resetting
the heap. The same matrix exposed empty process-tag lookup forming
`&null->mpNode`; patch 0031 returns explicit null on target PC. GameCube paths
remain unchanged.

## Verification

- Focused retail async probe passes Debug, Release, and strict ASan/UBSan.
- Intended foundation set passes 10/10 in Debug, Release, and strict
  ASan/UBSan.
- Patch stack 0001-0031 replays cleanly.
- Only one BlueWake/probe process ran at a time.
- Full CTest discovery in the reused non-GX Debug/Release trees still lists
  unrelated unavailable Aurora tests; the scoped ten-test matrix is the
  accepted build shape and this registration noise remains documented debt.

## Successor

Re-enable the complete original phase-two preload fanout in one bounded
private-disc process. Require all 26 archive commands, four raw loads, and
three dRes requests to satisfy the original `dvdWaitDraw` gate before
admitting title/J3D execution.
