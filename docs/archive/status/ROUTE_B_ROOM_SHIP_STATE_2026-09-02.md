# Route B Room Ship State - 2026-09-02

## Boundary

The initial `sea_T/Room44` loader contains one `SHIP` record. Unlike the six
previous room views, its handler reads global ship and restart state, may move
an already-existing ship actor, and conditionally clears saved IDs. The
serialized record is big-endian and cannot remain a native 64-bit overlay.

## Result

Patch 0113 adds a serialized 0x10-byte SHIP type, decodes it into room-owned
native storage, publishes the native counted view, and releases that storage
with `dStage_roomDt_c`. The original `dStage_shipInfoInit` decision tree is
retained. A single target-PC adapter supplies the existing ship actor without
admitting actor-list traversal or actor construction into this tier.

The public contract verifies exact coordinate, angle, and ID conversion. It
also covers the title route's empty saved IDs, a matching saved ship and room,
a mismatched ship ID, turn-restart placement, GanonM state clearing, Windfall's
special `0x80` placement, and room reset. PLYR and all requested profiles stay
fenced.

## Evidence

- Patch SHA-256:
  `ae1f3bf0d8d4056d91999dbe094ee490a6b5dd01ade78a4b0fbcbb77f8a131c4`.
- Debug SHIP object SHA-256:
  `227785e81700d242003cb2cdde2c8a3f007a63b252b77883e5d1512e56e9210a`.
- Debug focused executable SHA-256:
  `92379dd12e2b9e6ef5895eab40b59fc84218be0f44a17cea6fcb68864faf5e8e`.
- Release focused executable SHA-256:
  `ff4fa13f87e9329d1c56758a7c78c1b90bcf2bf8d7e237ddc61bda6696588816`.
- ASan/UBSan focused executable SHA-256:
  `35f3b9c0a92ce84132f28b6d2930792002a4703b356cdca57c9d1f06e910b217`.
- Patches 0001 through 0113 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 54 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan.

The patch changes 133 lines across the 4,921-line header/source boundary, a
2.70% partition-and-adaptation density. No PLYR request, profile execution,
aggregate room phase, BlueWake process, or Simulator was admitted.

## Next Boundary

Qualify the one present PLYR record as a room-owned native actor view and run
the retail player initializer only through process-request submission. Prove
point selection, request order and payloads, restart/start-stage state, ship
IDs, and reset lifetime while fencing aggregate phase 2, CAMERA resume,
profile execution, room reload, another BlueWake process, and another
Simulator.
