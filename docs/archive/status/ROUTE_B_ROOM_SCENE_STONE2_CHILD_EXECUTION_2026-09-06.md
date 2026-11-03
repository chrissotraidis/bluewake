# Route B ROOM_SCENE Stone2 Child Execution - 2026-09-06

## Boundary

The exact private Room44 parent retained BG, KYTAG01, all nine Obj_Wood
requests, and all 119 GRASS requests while cancelling the remaining 49. The
single exact ACTR-144 `Stone2`/`Ekao` request was independently qualified, but
its object-resource, move-BG, draw, and delayed-delete lifetimes had not been
composed with the parent and its existing children.

## Result

The native reloader still publishes all 179 phase-3 requests. The private
registry now also admits original `g_profile_Stone2` and cancels only the
remaining 48. The fixture mounts the exact 16,640-byte `Ekao` archive as one
object-resource owner before starting the standard ROOM_SCENE process. The
exact request preserves parameters `0xf37f6c3f`, position
`(-208620.0, 2300.0, 317730.0)`, Y angle `14381`, Z parameter `255`, room 44,
and set ID `0xffff`.

Standard child creation enters the already-qualified original Stone2 wait
path. It increments the parent fixture's `Ekao` owner count from one to two,
constructs the resource-index-4 model and resource-index-7 move-BG collision,
and completes beside the existing BG collision owner. Two original executes
decrement the wait counter from two to zero, lock the move-BG object, and
publish two collision updates. Original draw submits one material packet and
observes its reached light and shadow contracts.

Standard delayed deletion runs Stone2 before BG cleanup. It releases the exact
Stone2 collision pointer, destroys the child heap, and decrements `Ekao` back
to the fixture's single owner while its resources remain readable. BG then
releases its distinct collision and heap, clears the Room44 vegetation and
wood packets, and the parent completes map, dRes, archive, native-owner, and
room-heap teardown. The fixture finally releases its own `Ekao` owner.

The shared move-BG test owner now tracks up to four distinct registrations so
BG and Stone2 can coexist and be released by exact pointer. No new TWW source
patch was required; patches 0001 through 0135 remain the complete source-port
series.

## Exact evidence

- Debug private executable SHA-256:
  `b5cb063d2407278f648ef87bbda46aefabae174ee7aee9e22efd42fc6af797d8`.
- Release private executable SHA-256:
  `209c734c27c619f72d50a53d9bbd926d569105e8d4ee5be05be6c3bc336f1e8f`.
- ASan/UBSan private executable SHA-256:
  `0ebff50160ea4aa79bb85201d127cc43e1ae2b3d9fc8779d75aa49983e1439e3`.
- Debug, optimized Release, and strict ASan/UBSan emit the identical result:
  `room-lifecycle room=44 archive=async-phase1 dzr=native requests=3+179 children=bg+kytag01+stone2-deleted remaining=48 grass=982+61+130-published-disposed-cleared wood9=published-disposed-cleared map=paired delete=delayed archive=unmounted owners=reset heap=reset pass`.
- The exact standalone Stone2 and BG create/execute/draw/delete probes pass in
  all three configurations after the shared collision owner change.
- All 65 registered public tests pass in Debug, optimized Release, and strict
  ASan/UBSan.
- Patches 0001 through 0135 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- The repository audit remains failed only by the known 20 intentionally
  tracked `local-research/` evidence files; this checkpoint adds none.

## Limitations

Stone2 retains the independent wait-tier fences: carry/drop/damage, event,
particle, audio, item, camera, functional collision, and raster presentation
remain outside the proof. Native C++ places the polymorphic actor's vptr over
the standard manager's retail-layout process-ID word when the actor is not
process ID 1. The composed probe therefore locates the uniquely registered
Stone2 process by profile/name after standard request creation; this is an
explicit target-PC ABI debt, not a claim that generic ID lookup is correct for
polymorphic actors. The other 48 Room44 requests remain cancelled.

## Next Boundary

Qualify original `OBJ_TORIPOST`, the smallest remaining one-request Room44
cohort. First close only its two strict target-PC source-portability errors:
the unsigned collision-mask aggregate and native-width J3D model user area.
Require the complete 1,056-line translation unit to compile in Debug,
optimized Release, and strict ASan/UBSan before measuring its link closure.
Keep parent composition, mailbox event/talk behavior, collision, animation,
item delivery, another BlueWake process, and another Simulator closed.
