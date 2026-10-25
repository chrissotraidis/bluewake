# Route B Room Scaled Requests - 2026-09-02

## Boundary

Room44's remaining reload records are five TGDR doors and one SCOB object.
They share the retail 0x24-byte scaled request shape, but the published TGDR
table must retain independent storage after SCOB is decoded.

## Result

Patch 0119 ports one shared decoder/request mechanism with distinct room-owned
TGDR and SCOB buffers. Base fields and byte scales propagate unchanged into
room-44 append requests; KNOB00D/KNOB03D map to KNOB00 and `ky_tag1` maps to
KYTAG01. Requests are unconditional, matching retail scaled-object behavior.

`dStage_roomDrtgInfoInit` publishes the five-entry native door owner through
`setDrTg`. A subsequent SCOB decode uses separate storage, and the contract
proves that the published fifth door remains KNOB03D. Allocation failure,
unknown-name cleanup, and reset lifetime also pass. No requested profile runs.

## Evidence

- Patch SHA-256:
  `4d919890dcf82dbe421d7e1c44512a627104918e8f3a21a60cbcaf213b52814e`.
- Debug scaled-request object SHA-256:
  `acb7ebab96f4a955e262139eac83b032de2d6692f3a39012ff866025cb89bb6c`.
- Debug focused executable SHA-256:
  `f088aa8ad1be06550321e3a39fd33af7d061a998ccded95a322cc4ae9792ff22`.
- Release focused executable SHA-256:
  `87a366a16552a628d520307d0ac2749c9d0a8012cae36448ae797dca5d5a065e`.
- ASan/UBSan focused executable SHA-256:
  `93743a73c0505067bded599275725bc34c4369de3833809599f7547e6ffb6e82`.
- Patches 0001 through 0119 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 58 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan.

No BlueWake process or Simulator was launched.

## Next Boundary

Compose the qualified ACTR, TGDR, and SCOB mechanisms through the original
seven-slot `dStage_dt_c_roomReLoader` against the exact private Room44 DZR.
Prove source-order dispatch, 178 requests, independent native owners, TGDR
publication, zero layered requests, and reset lifetime. Keep leading BG,
`objectSetCheck`, requested-profile execution, phase 4, BlueWake, and Simulator
closed.
