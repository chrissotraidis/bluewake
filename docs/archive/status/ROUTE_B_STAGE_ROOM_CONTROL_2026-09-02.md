# Route B Stage Room Control - 2026-09-02

## Result

Patch 0088 partitions the unchanged `dStage_roomControl_c::init` and
`dStage_roomDt_c::init` bodies with their real static owner. Both start-point
branches pass over fully dirtied state:

- a nonnegative start point calls `dComIfGs_initZone`, resets all 64 zone IDs
  to `-1`, and does not clear room switches;
- a negative start point preserves each zone ID and clears switches only for
  nonnegative zones, in room order; and
- both branches clear every reached room-data pointer, light-vector count,
  room flag, draw flag, memory-block ID, background pointer, stay/old-stay ID,
  dark ratio, and all 16 memory-block pointers.

The test records 65 start-point reads per call, matching the unchanged owner:
one before the loop and one for each room.

## Closure

The retained object imports three behavior owners:
`dComIfGp_getStartStagePoint`, `dComIfGs_initZone`, and
`dComIfGs_clearRoomSwitch`. Out-of-line room-map virtuals and diagnostics are
aborting fences. The real static room array constructs its embedded lighting
state, so the focused fixture supplies the exact retail
`j3dDefaultLightInfo`; it does not replace entered room-control behavior.

Patch 0088 changes 39 lines (33 additions and six deletions) across the 3,693
line `d_stage.cpp`/`d_stage.h` source surface, a 1.06% adaptation density.
Stage decoding, room-scene creation, maps, event management, environment
processes, and aggregate `dStage_Create` remain unentered.

## Verification

- Debug: 32/32 tests passed.
- Optimized Release: 32/32 tests passed.
- Strict ASan+UBSan: 32/32 tests passed with leak detection disabled on macOS
  and halting address/undefined-behavior checks enabled.
- Patch replay passes through patch 0088.
- Repository audit has only the known `local-research` provenance failure;
  dependency-lock validation passes.
- No BlueWake process or Simulator was launched.

## Next Boundary

`BW-P4-0105` remains active. Measure the source-ordered
`dKankyo_create` environment-process closure once. Its four authentic process
requests require a real process/profile ownership plan; a callback recorder is
ordering evidence only and cannot qualify the owner.
