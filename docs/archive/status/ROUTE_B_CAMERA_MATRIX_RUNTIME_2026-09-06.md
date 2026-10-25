# Original camera pose-to-viewport execution — 2026-09-06

- Gate: P4 native player/camera composition; BW-P4-0126 remains active.
- State: ADVANCE within camera prerequisites; no camera Run acceptance.
- Base: `908f98b`; Apple Silicon macOS, Debug/Release/strict ASan/UBSan.
- Source pins/patches unchanged through 0149. No new upstream source edits.

## Executed result

The optional camera_matrix_probe reuses the original camera-construction probe
and its qualified game-info/base-actor owners. During each of sixteen camera
lifetimes it feeds the original controller's eye/center into original mDoMtx
look-at, original perspective creation, projection/view concatenation, and
mDoLib_project. Synthetic viewport/FOV inputs remain explicitly diagnostic.

Executable checks prove:

- the camera eye maps to the view origin and its center to depth -200;
- the center projects to the 640x480 viewport center with expected GX depth;
- a point offset along camera-right/up agrees with an independent trigonometric
  projection calculation;
- nonzero viewport origin follows the original viewport-offset behavior;
- the original null-view branch returns its defined zero result;
- X/Y/Z set and multiply rotations match analytic cardinal-angle basis vectors;
- translation, scale, rotation, and local translation compose in the expected
  order; and
- prior drawlist view/viewport pointers and matrix-stack state are restored.

Every mode passes all sixteen constructor/destructor cycles with the added
matrix checks. This is original pose-to-viewport execution, not GPU rendering,
camera updates, a real PLAYER instance, or normal boot.

## Source/link composition

Full original m_Do_mtx.cpp, m_Do_lib.cpp, J3DUClipper.cpp, d_attention.cpp, and
d_att_dist.cpp compile without portability edits. The camera Run target now
retains those sources plus original actor iteration, actor-tag queue ownership,
and process-ID search. The intermediate census exposed the missing attention
distance table, then the actor queue; adding their original defining units
resolved those ownership gaps. No synthetic table or replacement search was
introduced.

Camera Run still fails linking in all three modes, now with the same 25
symbols rather than 40. Remaining groups are:

| Owner | Symbols |
|---|---:|
| Dynamic collision mass/camera state | 3 |
| Sea/wind | 3 |
| Actor shared state | 5 |
| Event/demo | 5 |
| Graphics blur/state | 5 |
| Audio | 2 |
| Vibration | 1 |
| Assertion reporting | 1 |

Attention's methods are link-composed, not runtime-qualified. Its real
constructor requires Always model/BCK/BPK resources and a solid heap; two
ordinary allocation sites in that constructor warrant an ownership reproducer
before promotion. Those observations do not justify synthetic attention state,
omitting arrow resources, or claiming targeting works. Previously retained
fences likewise remain unqualified if a later runtime probe reaches them.

## Verification

- Original camera matrix/constructor probes: PASS in all three modes.
- Public default builds/CTest: 68/68 PASS in all three modes.
- Private real-DZB, original BG collision, Room44 lifecycle, Toripost heap,
  McaMorf, and historical standalone Stone2 regressions: PASS in all modes.
- Camera Run census: expected diagnostic FAIL, 25 unresolved symbols per mode.
- Player asset generator: five tests PASS.
- Preparation through 0149, whitespace check: PASS.
- Both protected recompcore and dependency-lock SHA-256s unchanged.
- Audit retains only the known 20 tracked local-research files. No new private
  input or generated game data tracked. No app/Simulator launched.

Reproduce by building/running optional `bluewake_route_b_camera_matrix_probe`
in an Aurora-GX build tree; it takes no disc argument. Separately build
`bluewake_route_b_camera_run_link_census` to reproduce the remaining failure.
Strict environment: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`.
Ignored logs: `local-research/evidence/route-b-camera-matrix-runtime-20260906/`.

## Next action

Qualify original attention construction with actual Always resources and
heap ownership, and the dynamic collision camera service required by Run.
Then close the remaining measured update dependencies and execute camera
updates with actual PLAYER integration. Preserve the real actor/room collision
route as regression. The full PRD, normal boot, controllable Outset,
transition/save/reload, and all later gates remain required and incomplete.
