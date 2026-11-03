# Route B KANKYO Wind Publication - 2026-09-02

## Result

Patch 0097 promotes unchanged
`dStage_roomControl_c::getStatusRoomDt`, the bounds-checked accessor directly
after the already-qualified room initializer. This closes the final room
lookup edge in unchanged `dKyw_wind_set` without admitting room heaps, maps,
or scene behavior. The patch changes four lines in `d_stage.cpp`; no retail
body is edited.

`bluewake_route_b_kankyo_wind_set_test` composes the original wind body with:

- authentic `g_dComIfG_gameInfo` static construction from patch 0096;
- the real room-status owner and accessor;
- original JMath sine-table initialization;
- original `cLib_addCalc`, `cM_atan2s`, `G_CM3D_F_ABS_MIN`, and Dolphin
  vector-square magnitude; and
- the qualified `g_env_light` owner.

The test proves no-room ambient wind, room file-list wind level 2, explicit
vector override, custom-power clamping, event-forced zero strength, and the
non-initial interpolation path. It also proves both invalid room indices
return null. Shutdown-only dependencies remain fail-closed and `_Exit` keeps
them unentered.

All 39 public tests pass in Debug, optimized Release, and strict ASan/UBSan.
Debug SHA-256 values are:

- room owner object:
  `f17310dbe39553265e58ae477d8c41c73ed37f1e46a0270948fda07ebcbcb972`
- unchanged wind object:
  `c835282bdb242b153a1b4a73caf4a86272fb88b360dc6fc946028c9351520614`
- composed wind test:
  `6c0f707a5f78a381086f560076ce1cfaead20a0f25d21214e091bf500bd67640`

## KANKYO Re-Link

A single dead-stripped re-link exported only retail `g_profile_KANKYO`. After
adding the qualified data, wave, wind, math, room, environment, and aggregate
game owners, the unqualified creation-state frontier is now:

- `JAIZelBasic::zel_basic` and its three position-reset methods;
- `g_regHIO`;
- `dSv_event_c::isEventBit`; and
- `dSv_player_collect_c::isSymbol`.

The other retained imports are already-qualified construction/framework
support, diagnostics, fail-closed shutdown, or the deliberately unentered
KANKYO execute/draw/event lifecycle. The probe SHA-256 is
`f6c901dbb32501cf4b3434f837c39d4bd8e97b5652c599b74f210a39dd9d6639`.
No further guard-window or broad-link iteration is warranted.

## Next Boundary

Qualify the unchanged JAudio position-reset owner as the next source-ordered
creation dependency. Then retain the two read-only save queries and real HIO
storage before attempting one completed KANKYO standard request.
