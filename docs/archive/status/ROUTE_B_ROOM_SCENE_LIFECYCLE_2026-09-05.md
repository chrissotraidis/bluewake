# Route B ROOM_SCENE Lifecycle Skeleton - 2026-09-05

## Boundary

Compose the already-qualified Room44 phase bands in their original
`dScnRoom_Create` table and drive one `ROOM_SCENE` instance through the
standard creation and delayed-deletion queues. This checkpoint proves process
ordering and shared room-state handoff; it does not claim exact archive bytes,
native DZR/reloader execution, real map resources, or requested-profile
execution in this same test.

## Source Composition

Patch 0130 adds the source-ordered phase-4 entry to the target-PC room phase
table when `BLUEWAKE_ROUTE_B_ROOM_PHASE4_TIER` is selected. The composed
object enables phases 0-4 and the original delete method together. The phase
bodies are unchanged; the patch is three conditional lines in the 642-line
prepared source.

The public fixture uses standard `dStage_roomInit(44)`, scene request
management, profile lookup, process allocation, repeated `fpcCt_Handler`
creation ticks, `fpcDt_Delete`, and the two-tick deletion queue. It deliberately
bounds the content-heavy owners behind contract seams already qualified by
focused exact-data probes.

## Executable Contract

The test proves this single ordered lifetime:

1. an occupied room heap holds phase 1 before any archive request;
2. freeing it issues exactly one `Room44` request on that heap;
3. the phase-2 resource barrier waits, then publishes zone and room data and
   starts the room-particle command;
4. phase 3 waits for that command, publishes its particle payload, releases
   the command, and invokes object-set ownership once;
5. phase 4 waits for a player, then publishes room/stay 44 and Y 321.5 once;
6. the standard create request completes and releases its user payload;
7. delayed process deletion removes the map image, stage resource, particle
   state, and salvage state, resets room data/status, empties the room heap,
   notifies the scene owner, and frees the process.

The lightweight Route B host uses separate allocators for `JKRAlloc` and
`cMl`. After phase 0 consumes the scene payload, the fixture rehomes that
payload into `cMl` storage so the unchanged `fpcBs_DeleteAppend` release path
is exercised without adding JKR link dependencies to every lightweight
process test.

## Verification

- Patch replay through 0130 passes.
- All 65 public tests pass in Debug.
- All 65 public tests pass in optimized Release.
- All 65 public tests pass under the strict sanitizer configuration.
- Repository diffs are whitespace-clean.

## Natural Stopping Point

This is a qualified standard-process skeleton, not resolution of
`BW-P4-0111`. The next smallest action is to replace its phase-2/3/delete
content seams with the exact Room44 archive, native room loader and reloader,
and the already-qualified real map owner in one private standard-process
composition. The other 50 persistent profiles, map drawing, functional
collision, and another BlueWake process remain closed.
