# Route B Room Phase-4 Map Lifetime - 2026-09-05

## Boundary

Qualify exact Room44 phase 4 from its player-ready barrier through the map
set call, then compose that state with the already-qualified room-scene delete
path and prove a single paired map-delete call. Keep the real `dMap` room-info
implementation, pearl/Dragon-Roost mutations, functional collision, unrelated
profiles, another BlueWake process, and another Simulator closed.

## Result

Patch 0128 adds a bounded, source-ordered target-PC partition of `phase_4`
and `setMapImage`. With no player, the function returns `cPhs_INIT_e` before
consulting room or item state. With a player but no valid stay room, it
completes without publishing a map. With exact room/stay 44, it passes the
player's 321.5 Y coordinate to the map boundary, sets `mbSetMap`, and returns
`cPhs_COMPLEATE_e`. A repeated call does not insert the map again.

The exact private-disc process composes that phase with the prior delete
probe. The authorized 714,816-byte Room44 archive and both exact resources
remain live through the null-player wait and map publication. Scene deletion
observes `mbSetMap`, removes the same room-44 map call exactly once, then
performs the real archive/resource/status/data/heap teardown already qualified
by patch 0127.

The Outset fixture reports no Din pearl, has start-stage name `sea`, and is
room 44, so the Farore/Omori and Dragon Roost branches are structurally
unreachable. Their observers abort if the exact process escapes its declared
boundary.

## Qualified Contract

- Missing player returns `cPhs_INIT_e` without map or item work.
- A missing stay room does not set `mbSetMap` or call the map boundary.
- Exact room/stay 44 publishes one call with player Y 321.5 and completes.
- Re-entry is idempotent at the map-insertion boundary.
- The same scene object reaches one paired room-44 map-delete call.
- Exact archive resources remain live until deletion and are unavailable
  afterward.

## Remaining Fences

- Map set/delete are narrow ordered observers. `dMap_c::setImage`,
  `dMap_RoomInfoCtrl_c`, exact map texture/data selection, and
  `dMap_c::deleteImage` are not yet qualified.
- The false Din predicate is qualified only for the exact Outset fixture;
  pearl mutation, the `Omori` Farore branch, and Dragon Roost event writes are
  outside this boundary.
- This is direct phase qualification, not yet a standard ROOM_SCENE process
  creation sequence through all five phases.
- Functional collision, the other 50 persistent Room44 profiles, another
  BlueWake process, and another Simulator remain closed.

## Verification

- The new public phase-4 test passes in Debug, optimized Release, and strict
  ASan/UBSan.
- The exact private composed probe passes in the same three configurations.
- All 64 public CTest tests pass after full builds in all three configurations.
- Exact private marker:
  `room-phase4-delete room=44 player=ready map=paired archive=unmounted status=reset data=reset particle=1 heap=reset salvage=1 pass`.
- `scripts/prepare_route_b.sh --check` replays patches 0001-0128 cleanly.

## Natural Stopping Point

The room scene now owns an exact, paired phase-4/delete map call contract. The
next source owner is `ROUTE_B_MAP_ROOMINFO_OWNER`: census and qualify only the
real `dMap_c::setImage`/`deleteImage` room-info lifetime reached by Room44,
using exact floor/map resources while leaving map drawing and AGB behavior
closed.
