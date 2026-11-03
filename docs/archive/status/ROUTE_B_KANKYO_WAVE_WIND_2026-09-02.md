# Route B KANKYO Wave and Wind Initialization - 2026-09-02

## Result

Patch 0095 isolates unchanged `dKy_wave_chan_init` from `d_kankyo_rain.cpp`
and unchanged `dKyw_wind_set` from `d_kankyo_wether.cpp`. The wave owner
imports only `g_env_light`. A public dirty-state test proves all 13 fields
written by the original function and confirms that untouched
`mWaveFlatInter` is preserved.

The wind owner compiles with ten imports: `g_env_light`, real aggregate game
state, the existing room-control stay/status owners, three JMath sine-table
symbols, vector-square magnitude, `cM_atan2s`, and `cLib_addCalc`. It has no
weather movement or rendering imports. Wind behavior is not claimed because
even its override branch links the inline aggregate game-state lookup; a fake
`g_dComIfG_gameInfo` would invalidate acceptance evidence.

Adding both original objects to the KANKYO profile-rooted probe removes the
wave and wind symbols. The remaining product frontier is now JAudio position
reset, real game/save and room state, HIO storage, and math support. Three
lifecycle symbols remain intentionally unresolved.

## Evidence

- Wave initializer object SHA-256:
  `5482e06bd3a05bc49dfea33c0fe431ade03183ed0a64a8be61ede9ca0da5689b`.
- Wind-set object SHA-256:
  `c835282bdb242b153a1b4a73caf4a86272fb88b360dc6fc946028c9351520614`.
- Patch replay passes through patch 0095.
- Patch 0095 adds 23 partition-control lines across 5,105 original source
  lines, or 0.45%, with no retail function-body edits.
- No BlueWake process or Simulator was launched.

## Next Boundary

Measure and promote the authentic aggregate game/save context required by
both `envcolor_init` and `dKyw_wind_set`, reusing the qualified room-control
owner. Do not define a test-only game singleton, enter lifecycle callbacks,
admit weather movement, or compose aggregate stage creation.

Follow-up: patches 0096-0097 now qualify that authentic aggregate and execute
all reached wind branches. See
`docs/archive/status/ROUTE_B_KANKYO_WIND_SET_2026-09-02.md` for the successor frontier.
