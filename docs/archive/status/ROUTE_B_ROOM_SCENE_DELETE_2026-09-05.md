# Route B Room-Scene Delete - 2026-09-05

## Boundary

Qualify the exact Room44 archive owner at `dScnRoom_Delete`, after the BG
actor-owned lifetime has already ended. Prove real stage-resource removal,
room status/data reset, optional room-particle cleanup, salvage reset, and
room-heap teardown without admitting room phase 4, map creation, unrelated
profiles, another BlueWake process, or another Simulator.

This is a target-PC partition of the retail delete method. It preserves the
reached source order and real resource/reset owners. Next-stage event bits,
start-stage special cases, NPC sea-talk state, and the map-delete branch are
not reached by the exact Room44 fixture and remain outside this claim.

## Result

The private probe opens the authorized GZLE01 disc, reads the exact 714,816-byte
`/res/Stage/sea_T/Room44.arc`, mounts it through `JKRMemArchive`, and publishes
it through the actual `dRes_control_c` stage-resource table. Both `model.bdl`
and `room.dzb` resolve before deletion.

The bounded diagnostic entry then enters `dScnRoom_Delete` for room 44. The
original inline `dComIfG_deleteStageRes` reaches real
`dRes_control_c::deleteRes`; the resource count reaches zero, the real
`dRes_info_c` destructor releases converted resources, and the fixed archive
unmounts. Subsequent lookups prove the resource owner and both exact resources
are unavailable.

In the same source order, the room's status flags clear, real
`dStage_roomDt_c::init` removes the deliberately dirty file-list publication,
the present room-particle branch fires exactly once, salvage reset receives
room 44 exactly once, and real `JKRHeap::freeAll` returns the room heap to zero
used bytes. The fixture has `mbSetMap == false`, so map deletion correctly
remains uncalled.

## Qualified Contract

- Room44's exact archive remains available through both BG actor lifetimes and
  becomes unavailable only when the room-scene owner deletes it.
- Real stage-resource teardown invalidates `model.bdl` and `room.dzb`.
- Room 44 status flags and room-data publication return to initialized state.
- Optional room-particle cleanup and salvage reset execute once with the exact
  room identity.
- The real room heap is emptied by `freeAll`.
- The map-delete seam is not invoked when the source-owned `mbSetMap` flag is
  false.

## Remaining Fences

- Map image set/delete ownership is not qualified. The exact scene has not yet
  passed room phase 4, which is the source path that first sets `mbSetMap`.
- Next-stage event-bit handling, special start-stage state, and NPC sea-talk
  cleanup are excluded from this exact Room44 partition.
- Room-particle removal and salvage reset terminate at narrow observers; their
  broad singleton/profile owners are not promoted here.
- Functional collision, the other 50 persistent Room44 requests, another
  BlueWake process, and another Simulator remain closed.

## Verification

- Exact private Room44 probe passes in Debug and optimized Release.
- Strict ASan/UBSan passes with halt-on-error settings and `detect_leaks=0`.
- All 63 public CTest tests pass after full builds in Debug, optimized Release,
  and strict sanitizer configurations.
- Exact success marker:
  `room-delete room=44 archive=unmounted status=reset data=reset particle=1 heap=reset salvage=1 pass`.
- `scripts/prepare_route_b.sh --check` replays patches 0001-0127 cleanly.

## Natural Stopping Point

Room44 now has a qualified archive/resource/reset teardown owner. The next
source-local lifecycle gap is `ROOM_SCENE_PHASE4_MAP_LIFETIME`: qualify exact
room phase 4's player-ready barrier and map-image publication, then prove the
already-qualified delete path removes that same map image. Pearl, Dragon
Roost, and unrelated stage branches must remain closed for the exact Outset
fixture.
