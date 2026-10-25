# Route B Room Player Request - 2026-09-02

## Boundary

PLYR is the only process-bearing handler in the initial Room44 loader. Its
serialized actor overlay is invalid on the native host, and its initializer
selects the spawn point, updates restart/start-stage and ship state, then
submits the player and UI process requests. Requested profiles remain outside
this boundary.

## Result

Patch 0114 decodes PLYR into a room-owned native actor array and preserves the
retail initializer through request submission. Exact `Link` name adjudication
maps to `fpcNm_PLAYER_e`; PLAY_SCENE submits PLAYER, METER, and AGB, while the
non-play branch inserts TITLE before the two UI requests. Scene-name lookup,
existing-player observation, and the raft-only point `-2` helper use narrow
target-PC adapters; the current title route does not invoke the raft helper.

The private census establishes one exact title record: parameters
`0x00FF002C`, position `(-192700.796875, 550, 318904)`, angles
`(0, 30765, 0)`, and set ID `0xFFFF`. The public contract reproduces that
cardinality and payload, then checks alternate point selection and parameter
merge, room and turn restart state, exact request order and payloads,
pre-existing-player no-op, and room reset lifetime.

## Evidence

- Patch SHA-256:
  `b8e29e454827923f364d2eebc836b7b0818c675b17b4cfc554dfdac024339a65`.
- Debug PLYR object SHA-256:
  `9e10460cdc34b983625269735efbedc4ee469986fb4da79cc4f90a9f4a82c956`.
- Debug focused executable SHA-256:
  `9c6efad43478ecb2c845f9caf697018861b743c8e860870361cea7e238b60db3`.
- Release focused executable SHA-256:
  `5192a2748c9e159a3b1c48637c90f64dbb850b34d95d8ca18c0d2a46afbdfbfa`.
- ASan/UBSan focused executable SHA-256:
  `a694e1d980273f2638337df3969d8838fd7c36e2f92bd5e5c79384919635ccd0`.
- Patches 0001 through 0114 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 55 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan.

The patch changes 139 lines across the 5,058-line header/source boundary, a
2.75% partition-and-adaptation density. No requested profile, aggregate room
loader, surrounding phase 2, room reload, BlueWake process, or Simulator was
admitted.

## Next Boundary

Compose all eight qualified initial-room handlers through
`dStage_dt_c_roomLoader` against the exact Room44 DZR. Prove source order,
complete native ownership, exact PLAYER/METER/AGB requests, start/ship state,
and reset lifetime while fencing surrounding phase-2 sync, zone, demo-bank,
particle work, profile execution, room reload, another BlueWake process, and
another Simulator.
