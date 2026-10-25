# Original camera capsule/mass execution — 2026-09-06

- Base: `ddc95ac`; BW-P4-0126 / P4 remains active.
- State: ADVANCE within camera update dependencies; no Run acceptance.
- No upstream source edits; ordered patches remain through 0151.

## Executed behavior

Full original c_cc_d.cpp and d_cc_mass_s.cpp now compile and execute alongside
original geometry and SDK math. The new game-data-free camera_mass_probe is
part of default CTest. It checks 60 horizontal capsule/vertical-cylinder cases
against the analytic radii-sum boundary, plus six clear hit/miss cases with
vertical, sloped, and zero-length capsules. It also checks category result
bits, accumulation, independently nearest top/bottom endpoints, and reset
semantics. SetCam resets result bits but retains the old top position until
Prepare resets it; this is intentional original frame-to-frame behavior.

The optional camera_mass_composed_probe reuses the original camera constructor
fixture. Across sixteen lifetimes and four stage/yaw variants, its original
center/eye poses feed original dCamera_c::lineCollisionCheckBush. Original
dCcS initialization, manager Prepare/Chk and cylinder/capsule collision supply
the result. The camera maps mass bits 2/4/8 to 1/2/4, publishes the next capsule
with radius 30, and retains the prior top position for the separate accessor.
A translated next capsule rejects the prior obstacle. No collision answer or
result bit is supplied by a replacement function.

This remains a synthetic obstacle/base-actor fixture, not live vegetation
producers, actual PLAYER, camera Run, actor-to-actor correction, damage,
mass-object/area callbacks, full cCcS::Move, rendering, or normal boot.
The full shape source compiles, but unexercised shape-pair methods are not
individually runtime-qualified by these cases.

## Composition and measured failures

The camera-specific collision root retains original cCcS/dCcS root bodies
while replacing its root-only mass and cylinder objects with full originals.
The existing test shape-abort definitions are excluded only for this explicit
composition; historical consumers retain their existing configuration.
Higher-level collision correction/damage fences remain closed.

The first standalone build needed the test's initializer_list include, then
the existing resource runtime's host assertion/panic support. The first
composed link exposed four cylinder methods still omitted by CC_ROOT_TIER;
using the full original cylinder unit resolved them. No game behavior or
source was patched to satisfy these links.

Camera Run now has the same 22 unresolved symbols in all modes, down from 25:
sea/wind (3), actor shared state (5), audio (2), assertion confirmation (1),
vibration (1), graphics state (5), event/demo (5). Run remains unexecuted.

## Verification

- Default builds/CTest: 69/69 PASS Debug, Release, strict ASan/UBSan.
- Composed camera mass probe: PASS all modes, sixteen camera lifetimes each.
- Camera constructor/matrix probes: PASS all modes.
- Actual attention, private DZB, Toripost heap, McaMorf, original BG collision,
  historical standalone Stone2, and Room44 lifecycle: PASS all modes.
- Camera Run census: expected diagnostic FAIL, same 22 symbols all modes.
- Player asset generator: five PASS; preparation through 0151 and diff checks
  PASS; protected recompcore hashes and dependency lock unchanged.
- Audit retains exactly the known 20 tracked local-research files.

The default mass probe takes no arguments. Build the optional
bluewake_route_b_camera_mass_composed_probe in an Aurora-GX build tree; it also
takes no disc argument. Strict options remain detect_leaks=0:halt_on_error=1
for ASAN_OPTIONS and halt_on_error=1 for UBSAN_OPTIONS. No app window or
Simulator was launched. Private logs are retained under
local-research/evidence/route-b-camera-mass-runtime-20260906/.

## Next action

Measure and compose original sea/wind services required by camera Run, and
qualify their real state/query behavior. Then close the remaining graphics,
event/demo, audio, vibration and actor-state dependencies before camera and
attention updates with actual PLAYER. Preserve real attention and room
collision regressions. Controllable Outset, transition/save/reload, normal
boot and the complete PRD remain required and incomplete.
