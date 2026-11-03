# Route B ROOM_SCENE Async Archive Submission - 2026-09-06

## Boundary

The exact standard ROOM_SCENE lifecycle previously began with a privately
mounted and registered Room44 archive. Phase 1 acquired that existing dRes
owner, so the process did not prove the retail archive command's origin or
phase-2 wait behavior.

## Result

Patch 0132 adds `BLUEWAKE_ROUTE_B_ASYNC_STAGE_RESOURCE_TIER` as a default-off
extension of the bounded target-PC system-resource owner. When selected, the
original `dRes_info_c::set` builds the stage path and creates a real
`mDoDvdThd_mountArchive_c`. The original `setRes()` reports busy while the
command is pending, then takes its archive and heap, destroys the completed
command, and performs the already-qualified resource conversion. Existing
manual system-resource targets retain their prior behavior.

The exact private probe now starts with no Room44 dRes entry. Unchanged
ROOM_SCENE phase 1 submits `/res/Stage/sea_T/Room44.arc` into the real command
queue. To make the scheduling assertion deterministic in optimized builds, the
probe starts the real DVD worker only after phase 2 has observed the queued
command and returned busy. The worker then executes the original mount command
over Aurora DVD, publishes completion through the existing atomic command
contract, and phase 2 resumes to ready. This controls scheduling only: no read
hook, fake command, manual mount, or preloaded archive owner remains.

All later evidence is unchanged. The same standard process consumes the exact
714,816-byte archive and 10,880-byte DZR, publishes three initial plus 179
reload requests, owns the real map-128 set/delete pair, completes its standard
request, crosses delayed deletion, unmounts the archive, resets all native
owners, and empties the room heap.

## Evidence

- Patch 0132 SHA-256:
  `408c3033dadc93314e440d542e29d0b87e0a07f48e2d760edcc9069d9ea2dadf`.
- Debug private executable SHA-256:
  `8cd90ef83e883a42970212278e08033822be3c401be06ae49008a84ced1b4055`.
- Release private executable SHA-256:
  `788d5afd900db20c6ed507de0d3646bf3e6c9f7e848ad75bf715b5db5e06d6c5`.
- ASan/UBSan private executable SHA-256:
  `adf5e68a38b68e88141a62a62d55ede17e34b6ea80aa1b27430943ac78ba07fd`.
- Debug, optimized Release, and strict ASan/UBSan emit the identical result:
  `room-lifecycle room=44 archive=async-phase1 dzr=native requests=3+179 map=paired delete=delayed archive=unmounted owners=reset heap=reset pass`.
- All 65 registered public tests pass in those same three configurations.
- Patches 0001 through 0132 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- `scripts/audit_repo.sh` has only the known P0 failure for 20 intentionally
  tracked `local-research/` evidence files.

## Limitations

The scheduling control deliberately delays worker creation until the queued
busy state is visible; it does not measure production I/O latency or pacing.
The 3+179 requested children are still cancelled without profile execution.
Map drawing, functional collision, representative gameplay rendering, another
BlueWake process, and another Simulator remain outside this proof.

## Next Boundary

Execute only the already-qualified leading BG request as a real child of this
same exact ROOM_SCENE process. Prove its shared Room44 resource access,
three-model create/execute/draw publication, and deletion before the parent
unmounts dRes. Continue cancelling every other child and do not widen into
functional collision, map drawing, or representative gameplay presentation.
