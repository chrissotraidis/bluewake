# Route B Map Room-Info Owner - 2026-09-05

## Boundary

Promote the smallest original `d_map.cpp` owner beneath the already-qualified
Room44 phase-4 map-set/delete call pair. The admitted behavior is limited to
floor selection, exact room map resources, `dMap_RoomInfo_c`,
`dMap_RoomInfoCtrl_c`, `dMap_c::setImage`, and `dMap_c::deleteImage`.

Map drawing, AGB communication, cursor/icon state, unrelated static map
objects, other room profiles, functional collision, and pixel presentation
remain outside this checkpoint.

## Source Partition

Patch 0129 compiles the original map translation unit with a target-PC
room-info tier. It retains only the room-info/controller static storage needed
by the reached set/delete owner. The two virtual draw methods required by the
retained room-info vtables terminate at an explicit aborting fence; unrelated
map draw and AGB methods are dead-stripped.

The prepared 2,211-line source receives a 78-line source delta (3.53%),
primarily guards and narrow host seams. Exact resource lookup is routed to the
already-qualified stage dRes table because Room44 is mounted there. Stage type
and map kind are supplied as bounded read-only facts for the exact Outset
branch.

Two source/host repairs were required and remain target-PC-only:

- the checked-in nonmatching `getRoomImage` loop otherwise performs zero work
  when its caller disables downward searching; the tier always attempts the
  requested floor once, then searches lower floors only when requested;
- embedded Aurora `GXTexObj` and `GXTlutObj` storage is aligned to eight bytes,
  as required by their native pointer-bearing implementations. PowerPC layout
  is unchanged.

When room-local `FLOR` data is absent, the bounded tier returns the same
no-floor result needed by Room44 rather than dereferencing the not-yet-promoted
global stage owner.

## Exact Runtime Proof

The private GZLE01 probe mounts the exact 714,816-byte Room44 archive and uses
the original native-view `dStage_mapInfoInit` owner to decode its single
56-byte `2DMA` record. That record publishes map ID 128. Original map code then:

1. waits until the phase-4 player exists;
2. looks up exactly `s128.bti`, `m128.bti`, and `m128.amp` from Room44;
3. initializes both original GX texture wrappers;
4. creates one controller entry with `DSP_ENABLE_BOTH_SIZE`;
5. publishes floor 128 and the current room-info pointer;
6. suppresses a repeated phase-4 set call; and
7. removes the controller entry and clears the current pointer before real
   archive, room-data, and heap teardown.

The final marker is:

```text
room-phase4-delete room=44 player=ready map=paired map-resources=3 archive=unmounted status=reset data=reset particle=1 heap=reset salvage=1 pass
```

## Verification

- Patch replay through 0129 passes.
- The exact private probe passes in Debug and optimized Release.
- The same probe passes with
  `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- All 64 public tests pass in Debug, optimized Release, and sanitizer builds.
- Repository diffs are whitespace-clean.

## Natural Stopping Point

`BW-P4-0110` is resolved. The next source-local vertical checkpoint is
`BW-P4-0111`: compose the already-qualified room phases 0 through 4 and delete
through the standard ROOM_SCENE process lifecycle over one exact Room44
archive. Do not broaden into the other 50 persistent profiles or map drawing.
