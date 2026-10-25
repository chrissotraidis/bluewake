# Route B KANKYO Initializer Closure - 2026-09-02

## Result

Patch 0094 retains the complete original `d_kankyo.cpp` implementation while
excluding only its already-separate environment singleton, profile band, and
the three lifecycle bodies for execute/event processing and draw. A
dead-stripped unresolved link rooted at patch 0093's retail profile proves
that creation retains original `envcolor_init`, `plight_init`, `plight_set`,
`dKy_plight_set`, `dKy_event_init`, `dKy_Sound_init`, and
`dKy_setLight_init`.

The remaining product frontier is now explicit: seven weather-data accessors,
original wave and wind initialization, three JAudio position resets and its
owner, real game/save state, HIO register storage, and `g_env_light`.
`OSReport_Warning` is a diagnostic edge and `memcpy` is libc. The profile's
three lifecycle imports remain deliberately unresolved and can be fenced in
the eventual creation test.

This is retained-link evidence only. It does not yet qualify KANKYO process
creation or any lifecycle callback.

## Evidence

- Initializer-tier object SHA-256:
  `fcfbd708de5013aedc2b8b7aa84de28582aada57eaa9a0b1b76ac6c3ba3ae187`.
- Profile-tier object SHA-256 remains
  `15f67e9cffd3aeec5715aa1757f1b91ba8be1a8ef83706b656d7ec96463a1b5f`.
- Patch replay passes through patch 0094.
- The strict initializer object compiles with the same source include policy
  as the complete KANKYO census.
- Patch 0094 adds 20 partition-control/declaration lines to the 3,721-line
  original, or 0.54%, with no retail function-body edits. The final seven
  lines keep color-helper ownership in patch 0091's environment tier.
- No BlueWake process or Simulator was launched.

## Next Boundary

Promote unchanged `d_kankyo_data.cpp` as the smallest cohesive owner of the
seven weather-data imports and prove its pointers/range contract. Then retain
the original wave/wind functions. Keep fabricated game state, lifecycle
execution, other profiles, aggregate stage creation, and concurrent processes
closed.
