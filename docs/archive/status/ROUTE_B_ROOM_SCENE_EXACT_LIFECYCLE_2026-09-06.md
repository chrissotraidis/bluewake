# Route B Exact Room44 Scene Lifecycle - 2026-09-06

## Boundary

The public ROOM_SCENE fixture proved standard process ordering through phases
0-4 and delayed deletion, but its content owners were observers. Exact Room44
archive, DZR, request, map, and teardown evidence still existed in separate
executables.

## Result

One private standard ROOM_SCENE process now consumes the exact 714,816-byte
`sea_T/Room44.arc` ownership established by the private-disc harness, waits on
the real room heap, and crosses the source-ordered phase table. Phase 1 acquires
the expected second dRes reference; the preload-handoff adapter releases the
harness reference before the real phase-2 sync so the process becomes the sole
archive owner.

The same process then decodes the exact 10,880-byte native DZR, publishes all
eight present initial views, and submits PLAYER, METER, and AGB in order. The
explicit title start layer is 2, so the exact LBNK entry selects no demo archive
in this integrated lifetime; the earlier `Demo51` result remains a separately
qualified layer-0 phase-2 scenario. FILI still selects particle number `0xff`.

After the particle barrier, original visible-room and reload ownership submit
one BG request plus all 178 exact ACTR/TGDR/SCOB requests. The harness cancels
those unqualified child processes without executing their profiles while
asserting the 172/5/1 native owner counts and representative profile totals.
Phase 4 waits for the player, installs the real map-128 room-info/controller
entry from the exact `2DMA` view and three resources, and completes the standard
create request.

Standard delayed deletion then enters the original room delete owner, removes
the map entry, unmounts Room44 through real dRes destruction, resets all initial
and reload native owners, reaches particle and salvage cleanup, empties the room
heap, and removes the process. Patch 0131 gives the composition one
`dStage_roomDt_c::init` owner while preserving every standalone partition.

## Evidence

- Patch 0131 SHA-256:
  `01a03bbc49cbdffc71d3d8fe5f2dd5fc33f0ab2b1386882ce274145615ef75e5`.
- Debug private executable SHA-256:
  `74aaa322b9882c73facc84f6163caab8dd67e55bce0b9753771789fb0255423e`.
- Release private executable SHA-256:
  `2fc7dfc89234a3bf399aa28b8426d500af04c71b8b7bee75a9556371070271d3`.
- ASan/UBSan private executable SHA-256:
  `441637dc8e68118d3f2bb72a6be1078b5ed5a4671f785847eedd03e0c4f8931d`.
- The exact private lifecycle passes in Debug, optimized Release, and strict
  ASan/UBSan with identical `3+179` request, paired-map, delayed-delete,
  archive-unmount, owner-reset, and heap-reset results.
- All 65 registered public tests pass in those same three configurations.
- Patches 0001 through 0131 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The two protected recompcore files retain their required SHA-256 values.
- `scripts/audit_repo.sh` has only the known P0 failure: 20 intentionally
  tracked `local-research/` evidence files.

## Limitations

This is exact content and ownership composition, but not yet exact archive
submission timing. The private harness mounts and registers Room44 before the
ROOM_SCENE request; phase 1 acquires that owner rather than creating and waiting
for the asynchronous DVD mount command itself. Requested child profiles remain
unexecuted, and map drawing, functional collision, representative gameplay
rendering, pacing, another BlueWake process, and another Simulator remain
outside this proof.

## Next Boundary

Remove the preload handoff. Start with no Room44 dRes entry, let unchanged phase
1 submit `/res/Stage/sea_T/Room44.arc` through the real asynchronous
`mDoDvdThd_mountArchive`/Aurora DVD path, prove phase 2 waits and then resumes,
and retain the exact lifecycle and teardown assertions. Do not widen into child
profile execution, map drawing, or functional collision.
