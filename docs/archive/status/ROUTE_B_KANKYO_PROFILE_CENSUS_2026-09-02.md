# Route B KANKYO Profile/Create Census - 2026-09-02

## Result

Patch 0093 isolates the unchanged KANKYO lifecycle wrappers, original
`dKy_Create`, method table, and `g_profile_KANKYO` from the 3,721-line retail
`d_kankyo.cpp`. The tier compiles with 13 imports instead of the complete
unit's 81-import census. This is a closure measurement only: no process was
linked or created, and no callback was entered.

The imports divide at the lifecycle boundary. Original creation requires
`envcolor_init`, `dKy_setLight_init`, `dKy_wave_chan_init`,
`dKy_event_init`, `dKy_Sound_init`, `dKyw_wind_set`, and `g_env_light`.
Delete, execute, and draw additionally retain `plight_init`,
`dKy_event_proc`, `dScnKy_env_light_c::exeKankyo`, and
`dScnKy_env_light_c::drawKankyo`. The remaining two imports are the qualified
generic process tables `g_fpcLf_Method` and `g_fopKy_Method`.

## Evidence

- Census object SHA-256:
  `15f67e9cffd3aeec5715aa1757f1b91ba8be1a8ef83706b656d7ec96463a1b5f`.
- Patch replay passes through patch 0093.
- The isolated object builds under the normal strict Route B compile policy.
- Patch 0093 adds 38 partition/declaration lines to the original 3,721-line
  source, or 1.02%, with no retail function-body edits.
- No BlueWake process or Simulator was launched.

## Next Boundary

Retain and measure the original seven-owner creation closure. Require a real
standard-create request and direct dirty-to-retail assertions before claiming
KANKYO creation. Keep delete, execute, draw, the other environment profiles,
aggregate `dStage_Create`, another BlueWake process, and another Simulator
closed.
