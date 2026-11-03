# Route B Obj_Wood Profile Execution - 2026-09-03

## Boundary

Execute all nine exact Room44 Obj_Wood requests through the original profile
and standard actor framework. Qualify transient publication into the shared
wood packet: allocation, placement invocation, unit state, linked room
ownership, capacity, rejection, cleanup, and intentional proxy disposal.

The host placement owner does not implement the retail ground query, bush
animation, collision response, audio, packet update, or packet draw. Those
paths remain outside this tier and no visible-vegetation claim follows.

## Result

The unchanged original `g_profile_Obj_Wood` and `d_a_obj_wood.cpp` now run for
the exact five `woodb` and four `woodbx` title-room appends. The qualified
room reloader submits the requests, the static-REL registry resolves the
profile, standard actor construction invokes its original create method, and
each temporary actor returns the retail `cPhs_ERROR_e` after publication.
Every actor and create request is then intentionally gone; the shared packet
retains nine units.

The bounded owner preserves the original 200-entry free-slot scan, active
state, position copy, rotating selection among eight normal animation IDs,
per-room linked insertion, rejected-placement clearing, room deletion, and
play-state create/remove lifetime. The profile invokes placement exactly once
per exact request. A forced allocation failure publishes no packet, a rejected
ground result leaves no active unit, the 201st direct insertion reports full
capacity without mutation, and deleting Room 44 clears every linked unit.

The collision members embedded in the transient actor are construction-only
fences in this tier. They are never initialized or used by the original create
method before deliberate disposal. This does not qualify Obj_Wood execute,
collision, animation, audio, or rendering behavior.

During strict private qualification, ASan found that the first harness version
walked cancelled non-wood request records after cancellation. The replay now
retains a wood-only pointer set before cancellation; the corrected exact run
is clean in all modes.

## Verification

- Debug: 63/63 CTest tests pass; exact nine-request replay passes.
- Optimized Release: 63/63 CTest tests pass; exact replay passes.
- ASan/UBSan: 63/63 CTest tests pass; exact replay passes with
  `detect_leaks=0` because LeakSanitizer is unsupported on this macOS setup.
- Exact DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Protected recompcore files remain unchanged:
  `cpu_exception.c` = `5b8ebbc60520b8bc6ee2166c344d760a88a3d2f2419058eaeaa210f5ec07def6`;
  `hle_core.c` = `dcb97681b2d325cd9fbf23e7d359a6788aa3f05980174fc31fa299a7b3c4716a`.

No BlueWake process or Simulator was launched.

## Next Boundary

Promote the single leading original BG request. It is the first persistent
Room44 profile, compiles unchanged, and owns the room models, optional
material animations, `room.dzb` collision, room-ready status, and vegetation
cleanup lifetime. Remeasure its link frontier against the current qualified
runtime, then execute it without admitting the other 50 persistent requests
or phase 4.
