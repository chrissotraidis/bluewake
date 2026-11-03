# Route B Room Phase 2 - 2026-09-02

## Boundary

ROOM_SCENE phases 0-1 already published the process and submitted the Room44
archive. Phase 2 owns archive synchronization, save-zone assignment, DZR
retrieval, the qualified room loader, optional demo-bank selection, and room
particle submission. These interactions had not previously executed together.

## Result

Patch 0116 promotes the retail phase-2 control flow. Narrow target-PC adapters
bind the unpromoted resource manager, save-zone allocator, room-status lookup,
layer query, and particle controller. Room-control state and
`dStage_dt_c_roomLoader` remain direct native owners.

The private exact-payload contract proves sync busy and error outcomes, escape
restart on failure, an existing zone with no DZR, and the ready path with a new
zone. The ready path retrieves the exact title DZR, publishes every native room
view, submits PLAYER/METER/AGB, requests LBNK-selected `Demo51`, derives FILI
particle number `0xFF`, and publishes the returned scene command. A second
ready pass proves failed demo submission clears `mDemoArcName` while particle
ownership still completes.

## Evidence

- Exact private DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Patch SHA-256:
  `aa4277d7a6e3efffdf8e6cd60e8eeba8813e1ff1907381f03b293101a80e76b3`.
- Debug phase-2 object SHA-256:
  `c32c9e9813d19847510041934050448960ecbf0cbfbed6ec2abd6fc392d6732f`.
- Debug private executable SHA-256:
  `af551c89c9cab3f01a886a27b757b26892b76b695afc33dcfd5ba7b6bd4e9c01`.
- Release private executable SHA-256:
  `16f3717aad4bd8e15a8b6c1d8d36c76cf4cbb5e34a55331d00c5439dcc394904`.
- ASan/UBSan private executable SHA-256:
  `efa869b2bce0d17408d9ba9134bb1c2e90248562fa1550791a7a1bc1c93654b3`.
- Patches 0001 through 0116 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The exact private phase passes in Debug, optimized Release, and strict
  ASan/UBSan; all 55 registered public tests pass in those same modes.

The patch changes 65 lines across the 467-line source boundary, a 13.92%
partition-and-adaptation density. No phase-3 command completion, room reload,
requested profile, phase 4, BlueWake process, or Simulator was admitted.

## Next Boundary

Qualify ROOM_SCENE phase 3 only through demo-resource and particle-command
completion, room-particle publication, and command deletion. Stop at an
explicit `objectSetCheck` fence while keeping room reload, requested-profile
execution, phase 4, another BlueWake process, and another Simulator closed.
