# Route B ROOM_SCENE BG Child Execution - 2026-09-06

## Boundary

The exact standard ROOM_SCENE lifecycle published the leading Room44 BG
request but cancelled it with the other 178 phase-3 children. BG
create/execute/draw/delete had been qualified independently, so the remaining
gap was authentic parent/child composition and shared archive ownership.

## Result

The private lifecycle registry now admits `g_profile_BG` beside
`g_profile_ROOM_SCENE`. The exact native reloader still publishes all 179
phase-3 requests; the probe retains only the leading BG request and cancels
the other 178. Standard creation runs the unchanged BG profile while its
ROOM_SCENE parent remains in phase 4 waiting for the player.

The child consumes the parent-owned Room44 resources. It constructs models
0, 1, and 3 from the exact BDLs, binds the present BTK, registers the exact
room DZB with the bounded collision owner, and publishes Room44 BG/status
state. One execute at wave frame 37 advances the special BTK contract. The
unchanged draw method then submits material packets from all three models
through the four real J3D draw buffers and preserves the qualified twelve-event
clip/light/list ordering.

Standard delayed deletion completes before the parent is allowed to leave
phase 4. The child releases the same collision owner, destroys its actor heap,
and clears Room44 BG/status publication while BDL, BTK, DZB, and dRes remain
available to the parent. The parent then completes its real map-128 lifetime,
crosses delayed deletion, unmounts Room44, resets all native owners, and empties
the room heap.

This composition exposed a real lifetime defect in the asynchronous dRes tier:
converted BDL/BTK owners were allocated from the selected current heap but had
no resource-local teardown owner. Patch 0133 creates a dedicated solid data
heap beneath the Room44 parent, converts all resources within it, restores the
prior current heap, adjusts the data heap, and destroys it before archive
unmount. The target-PC `JKRSolidHeap::create(-1, ...)` path also leaves the
host expansion heap 0x10 bytes for its free-block bookkeeping when the largest
block begins aligned. PowerPC behavior and non-selected dRes tiers are
unchanged.

## Evidence

- Patch 0133 SHA-256:
  `1b4a9db092f6dd807ee2574d3f3a807765576b599f22f65f1f1b1b15aa0dfe54`.
- Debug private executable SHA-256:
  `5b3b4dcbf88facf86da2ba690eb10c71992600a7c268a5eb3aadc40d3f8f8adc`.
- Release private executable SHA-256:
  `2d09e37219e61c8fa06e1ffadde6e26d6f5b18218d6b1fbaa42f426b7697857d`.
- ASan/UBSan private executable SHA-256:
  `f0406250481757997acd14f665eb1eacc7cbdaab768e4ed808baf6edcf00440a`.
- Debug, optimized Release, and strict ASan/UBSan emit the identical result:
  `room-lifecycle room=44 archive=async-phase1 dzr=native requests=3+179 bg=child-create-execute-draw-delete map=paired delete=delayed archive=unmounted owners=reset heap=reset pass`.
- All 65 registered public tests pass in those same three configurations.
- The standalone BG profile probe still passes after the target-PC solid-heap
  correction.
- Patches 0001 through 0133 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- `scripts/audit_repo.sh` has only the known P0 failure for 20 intentionally
  tracked `local-research/` evidence files.

## Limitations

The collision seam proves exact pointer registration/release but not functional
collision queries. Clip and environment-light calls remain bounded observers,
and material-packet submission is not a presented-pixel claim. The probe waits
for worker completion before selecting the Room44 heap for phase-2 resource
conversion because the target-PC DVD worker does not preserve the calling
thread's JKR current-heap selection. The other 178 phase-3 children are still
cancelled. Map drawing, representative gameplay rendering, another BlueWake
process, and another Simulator remain outside this proof.

## Next Boundary

Retain the one already-qualified KYTAG01 request in the same exact parent
lifetime alongside BG. Execute its authentic create/execute/draw/delete
contract, prove both children finish before parent archive teardown, and
continue cancelling the other 177 phase-3 requests. Do not widen into grass,
wood, map drawing, functional collision, or representative gameplay
presentation.
