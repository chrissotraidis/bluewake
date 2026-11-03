# Route B Aggregate Game-Info Owner - 2026-09-02

## Question

Can Route B publish the authentic retail `g_dComIfG_gameInfo` singleton needed
by KANKYO wind initialization without defining a test-only global or admitting
the unrelated save, resource, and gameplay runtime?

## Census

One whole-unit compile of `d_com_inf_game.cpp` reached only two late,
unrelated save-buffer pointer-to-`u32` narrowings. That horizontal shape is
closed after one attempt. A constructor-retained object reduced the real
closure to play state, draw-list state, background/collision state, seven
small event/detection/vibration owners, `dRes_info_c` construction, and
`dSv_memory_c::init`.

All but the final two were already independently qualified. Patch 0096 adds
target-PC-only partitions for:

- the real global and unchanged two-statement `dComIfG_inf_c::ct`;
- unchanged `dRes_info_c` construction; and
- unchanged `dSv_memBit_c::init` plus `dSv_memory_c::init`.

The default PowerPC source route is unchanged. Patch 0096 changes 34 of 4,734
lines across the three touched original units (0.72%). No retained retail body
is edited.

## Contract

`bluewake_route_b_game_info_owner_test` enters through static construction of
the real `g_dComIfG_gameInfo`. It proves:

- brightness is initialized to `0xFF` by the retail aggregate constructor;
- the embedded play owner has the qualified camera, player, ship, demo,
  particle, and window defaults; and
- all 16 saved-stage memory records plus the active memory record execute the
  original memory-bit reset and publish zero key counts.

Shutdown-only resource, draw-list, attention, and vibration destructors remain
explicit aborting fences. The test uses `_Exit` after its assertions, so none
is entered or claimed.

## Evidence

Debug object SHA-256 values:

- `d_com_inf_game.cpp.o`:
  `2a15d0a762873b299af51b89c979da8f6e24122d593ba16d3ec1e6b8783d31dc`
- `d_resorce.cpp.o`:
  `10096816256c4705e5d442c4962ad0ae631b719102562df458e511de5821d513`
- `d_save.cpp.o`:
  `f000286cffb4efc3f815010191ae76bd60dcec77036eb7bca1bfc5821b1c5da1`

All 38 public tests pass in Debug, optimized Release, and strict ASan/UBSan.
Patch replay passes from pinned TWW commit
`03d27aa14389e648f51df2bc055ee3d53a3b67f2`.

## Boundary

This qualifies construction and publication of the aggregate singleton, not
save loading, resource destruction, or general gameplay use. The next action
is to execute unchanged `dKyw_wind_set` against this real singleton and the
qualified room owner. Only after that passes should the KANKYO creation link
be re-observed.
