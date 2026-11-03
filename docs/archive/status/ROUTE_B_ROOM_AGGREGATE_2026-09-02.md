# Route B Room Aggregate - 2026-09-02

## Boundary

All eight initial Room44 handlers passed separately, but
`dStage_dt_c_roomLoader` first resets the shared room owner and dispatches a
22-slot table in source order. Aggregate ownership, interaction, request order,
and lifecycle therefore needed qualification against the exact title payload
before widening into ROOM_SCENE phase 2.

## Result

Patch 0115 restores the bounded room-loader composition. The private harness
uses the exact 10,880-byte `sea_T/Room44` DZR exported by the existing census.
It asserts all 11 node headers, admits only the eight initial-loader tags, and
aborts if any absent handler is called. FILI, 2DMA, LBNK, SHIP, PLYR, RPAT,
RPPN, and SOND all publish their room-owned native views in one invocation.

The exact `Link` record submits PLAYER, METER, and AGB in order, publishes
point 0 / room 44 / layer 2, and derives ship ID 0 with ship room 44. Reset
then releases and clears every aggregate owner. The later ACTR, TGDR, and SCOB
records are not consumed by this loader.

## Evidence

- Exact private DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Patch SHA-256:
  `bac9eb36e949089cf24fc8940c6c7df185201d69e863a22f4b80633c8a37f427`.
- Debug aggregate object SHA-256:
  `144447abfa745d1d6a8b79605452d99a10fc7c6d4ffa1e65b02adc6dce086e80`.
- Debug private executable SHA-256:
  `0d6d26bbde930843e3638ae3f7ed6315d36855cc505a1a1940b094b9862acb44`.
- Release private executable SHA-256:
  `7a886df945bd5ab34598fe9d1300a9580038a671ad3126758923b81cca94872d`.
- ASan/UBSan private executable SHA-256:
  `218719a2c977c585803751a995f6334a51d0e0f879ea71d936392980f0aa8b23`.
- Patches 0001 through 0115 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The exact private aggregate passes in Debug, optimized Release, and strict
  ASan/UBSan; all 55 registered public tests pass in those same modes.

The patch changes 47 lines across the 3,713-line source boundary, a 1.27%
partition-and-adaptation density. No surrounding phase-2 resource work,
requested profile, room reload, BlueWake process, or Simulator was admitted.

## Next Boundary

Promote ROOM_SCENE phase 2 around the qualified loader. Prove resource
busy/error/ready behavior, existing and newly-created zone publication, exact
DZR retrieval, demo-bank decisions, FILI particle number, scene-command
ownership, and the final phase result while fencing phase 3, requested-profile
execution, room reload, another BlueWake process, and another Simulator.
