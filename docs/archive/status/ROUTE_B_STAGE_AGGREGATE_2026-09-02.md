# Route B Stage Aggregate Composition - 2026-09-02

## Boundary

Outset `stage.dzs` contains 15 chunks. Retail consumes `STAG` earlier through
`dStage_dt_c_stageInitLoader`; `dStage_dt_c_stageLoader` then searches for the
remaining 14 present tags in a fixed table order. Every handler had passed in
isolation, but their shared stage lifetime and combined process effects had
not been qualified.

## Result

Patch 0109 composes exactly those 14 qualified handlers in their original
stage-loader order. It links the established handler objects rather than
copying their implementations. The only helper change gives SCOB's bounded
LOD lookup private names so it can coexist with ACTR's retail-name lookup.

The exact preserved 0x30A0-byte DZS passes as one input. All native owners
publish with authentic counts: MULT 50, RCAM 1, ACTR 4, RTBL 50, RARO 1,
Pale 57, Colo 10, Virt 37, SCLS 212, RPPN 40, RPAT 4, SCOB 50, EVNT 57,
and EnvR 52. The process queue contains exactly 55 requests in loader order:
one CAMERA, Ship/sea/Md1/Cb1, then 50 LODBG requests. No profile executes.
`STAG` remains untouched, proving that the earlier init-loader ownership was
not accidentally folded into this tier.

The patch changes 47 lines in the 3,117-line translated unit, a measured 1.51%
adaptation density.

## Evidence

- Preserved DZS SHA-256:
  `5f5a29bd46e5132b7446cff9085939cf2373eb471eaafadbfecb2748be3be113`.
- Aggregate object SHA-256:
  `f5f30c8dd1c61ece1d2c8f9998641d738e122aeb7815b8fcbbae4171a7208dde`.
- Debug aggregate executable SHA-256:
  `589d2156574e22059e8e5a1200461ec74ce90a7b3b53b2c88afd32cbd36180e3`.
- Patch SHA-256:
  `54d13f18bf55988b0e52cb21355224f12cc41e6758dea34809ffb72a848ad134`.
- Patches 0001 through 0109 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The exact private-input aggregate passes in Debug, optimized Release, and
  strict ASan/UBSan. The public suite remains 50/50 green in all three modes;
  the retail DZS itself is not committed or advertised as public evidence.

No room scene, queued profile, BlueWake app, or Simulator was launched.

## Next Boundary

Qualify `dStage_roomInit` through creation of the initial room-scene request.
Its payload currently crosses `fopScnM_CreateReq` through `u32`, truncating the
allocated pointer on 64-bit macOS. Promote that scene-request user payload to
native width, then prove status flag 2, exact start-room payload ownership,
request identity, and stay-room publication without executing the ROOM_SCENE
profile.
