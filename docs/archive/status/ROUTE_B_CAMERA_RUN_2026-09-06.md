# Original camera updates execute — 2026-09-06

Base `ab3bbd0`; BW-P4-0126 remains active. This is an integrated headless
diagnostic, not PLAYER runtime, visible gameplay, or P4 acceptance.

## Execution and corrected fixture contracts

The new optional `bluewake_route_b_private_camera_run_probe` mounts actual
System and Always resources, constructs original attention and background/
dynamic-collision services, event control, demo manager and camera, then calls
the original camera Run method 120 times while moving a typed non-PLAYER
subject two units per update. It requires every update to return true, finite
eye/center coordinates on every frame, and a changed final center. No audio
or unqualified cutscene actor/factory boundary may return success.

The first execution stopped in UBSan at a null demo-manager lookup. Adding
the original manager construction resolved that fixture omission. The full
d_demo source remains its owner; the construction-only version is not linked
over it. Original JStage base interfaces and JStudio construction services
are composed, while playback factories remain explicitly unqualified.

Repeated updates then exposed uninitialized camera shake state. Debug also
exposed divergent coordinates from uninitialized base-actor state. Both stack
fixtures had omitted the clearing performed by original fpcBs_Create through
sBs_ClearArea before phase construction. Aligned, zeroed raw storage followed
by the original constructors reproduces that process-allocation contract.
No camera algorithm, bounds guard, or fake successful collision was added to
hide these failures. A passing first update alone was insufficient evidence.

## Production owners and build composition

`camera_actor_globals.cpp` owns the original zero-initialized Medli flight/
mirror, Makar flight, possessed-gull and cannon pointer symbols with types
from their actor headers. The corresponding GZLE01 .sbss symbols are
803F6B5F/60, 803F6E61, 803F6B7E and 803F6B70. Original actor methods retain
responsibility for subsequent writes; storage does not implement their lives.

Native JUT confirmation reports a false condition to stderr without aborting;
true conditions remain silent. This intentionally uses host reporting rather
than the retail screen/device-mask routing. The public camera-services test
checks the actor-state accessors and exact false/true reporting behavior.

The production Run link census has only the two unimplemented audio exports
remaining: JAIZelBasic::seStart and zel_basic. That census does NOT include the
private diagnostic audio boundary and still fails linking as expected.

The combined private target removes duplicate JMath, J3DSys/J3DTevs, packet,
draw-buffer and matrix owners from overlapping object groups. Original
graphics/resource owners remain, and Aurora owns matrix operations. No linker
allow-duplicates workaround is used.

## Explicit limitations

- Subject motion is supplied by the diagnostic, not input or Link execution.
- The background world is empty. No real room collision response is proved.
- Stage information is typed synthetic input named sea, not normal boot.
- Demo manager construction is real; cutscene actor admission and JStudio
  factory execution abort if reached. Construction of audio/particle factory
  objects with null service pointers does not qualify those services.
- The private audio entry aborts on use; its interface is null, so strict
  sanitizers may stop a null member call before the entry reports its ID.
- Room-map/error boundaries remain aborting. Two room camera/arrow getters
  reproduce the original accessors because the fixture uses raw singleton
  storage rather than linking the global constructor unit.
- No controller input, actual PLAYER, event progression, draw, sound output,
  normal boot, transition/save/reload or global teardown is accepted. The
  diagnostic exits with _Exit after its assertions, retaining prior teardown
  regressions as separate evidence rather than claiming this fixture cleans up.

## Reproduction and next action

Configure an existing Aurora-GX/DVD build, build target
`bluewake_route_b_private_camera_run_probe`, and run it with the validated
GZLE01 disc path as its sole argument. Strict runs use
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
The public target is `bluewake_route_b_camera_services_test`.

The next action is actual PLAYER create/update composition with this original
camera/service closure, followed by real collision and visible control. Measure
the reached player dependencies; do not return to unrelated actor enumeration
or assume all audio reconstruction must precede a diagnostic update. Any reached
unqualified behavior remains a real frontier, not permission for a fake success.

## Verification

Debug, Release and strict ASan/UBSan all complete 120 updates with center x
100.000 -> 340.000. Public 72/72 CTests, full PLAYER compilation and existing
private event/sea/attention/DZB/Toripost/McaMorf/BG/Stone2/Room44 and camera
constructor/matrix/mass regressions pass all three modes. The separate Run
census retains exactly the two audio exports in all modes.

Both preparers pass (TWW through 0158, Aurora 0001), as do five asset-generator
tests, locked private asset regeneration, shell syntax and whitespace checks.
Both protected runtime hashes are unchanged. Dependency lock SHA256 remains
b9e74b327368a9bc6e3c994b6f8794dea92e06bf822512774fb42bf32a240aba.
Audit retains only the known 20 tracked local-research files. No BlueWake
window or Simulator was launched. Private replay logs are ignored under
`local-research/evidence/route-b-camera-run-20260906`.
