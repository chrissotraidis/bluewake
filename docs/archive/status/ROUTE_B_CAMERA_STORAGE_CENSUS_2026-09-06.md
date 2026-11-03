# Route B camera storage and constructor census — 2026-09-06

**Historical checkpoint:** the constructor link census described here is
superseded and retired by the executable construction probe in patch 0147.
Current status: `ROUTE_B_CAMERA_CONSTRUCTION_2026-09-06.md`.

## Result and limits

State: `RETRY_NEW_HYPOTHESIS`, P4, BW-P4-0126 remains active. Base checkpoint
`d76bf93`, locked `GZLE01-disc-image`, Apple Silicon macOS 26.6.2. No dependency
repin, app launch, gameplay measurement, or user action required.

Patch 0146 closes full PLAYER compilation and compiles six original camera
units in Debug, optimized Release, and strict ASan/UBSan. The native process
now reserves the actual controller type rather than a shortened phase-only
object. This is storage/compile acceptance, not construction or gameplay.
The explicit constructor/destructor link census fails with 24 unresolved
symbols in Debug/sanitizers and 22 in Release. No missing owner is stubbed.

## Implementation and experiments

- Restore `d_camera.h`, the original pre-phase integer, phase request, and
  `dCamera_c` storage in `camera_process_class`. Native typed union storage
  defers construction to original `init_phase2` placement new. Original
  `camera_delete` explicitly destroys the controller. The wrapper constructor
  initializes only base/phase state; its destructor does not double-destroy
  the manually managed controller. Retail layout is unchanged.
- Existing phase-one runtime remains fenced at phase two, execute, draw, and
  delete. No fence was removed and no new controller implementation was added.
- Full PLAYER target now compiles its three original units and gameplay
  includes; it is still excluded from default build/runtime registration.
- Camera source census compiles `d_camera`, `d_cam_param`, `d_cam_type`,
  `d_cam_type2`, `d_cam_style`, and `d_ev_camera` without a runtime-tier macro.
  The first three-unit attempt exposed 13 unsigned-long mask template errors
  in the seagull header and one native-long narrowing in `Run`. Explicit u32
  masks and an s32 mode local retain the original fixed-width semantics.
- The first link attempt also lacked camera style/type definitions; their
  original source units are now included, not reconstructed constant tables.
- The enlarged header graph exposed missing asset/MSL includes in earlier
  foundation tests and an MSL/libc++ collision in the profile-registry test.
  Target-local include paths and the existing native standard-library shim
  resolve those errors. No global warning suppression was added.
- Extend the existing camera-base test to assert storage extent, alignment,
  phase/body non-overlap, zero phase initialization, and unchanged base view
  access. Merely forming the inactive union member's address does not invoke
  its constructor. Existing phase-one process allocation/publication tests
  are replayed with the new profile size.

## Constructor closure: classified, not yet composed

`bluewake_route_b_camera_constructor_link_census` retains a function that
calls the real placement constructor and destructor. Its volatile function
pointer prevents the linker from discarding that reference. It never calls
the function: if linked, main prints its diagnostic-only status and returns 2.
It has no synthetic player, captured state, substitute camera, or owner fences.
Dead stripping isolates this lifecycle reference; it is not a full game link
or proof that every retained branch executes during Outset creation.

| Owner group | Debug/sanitizer symbols | Source boundary |
|---|---:|---|
| Process and game/input globals | 4 | fopCamM_GetParam, actor stopStatus, gameInfo, CPad storage |
| Room camera/arrow lookup | 2 | Original d_com_inf_game accessors; absent from optimized unresolved set |
| Vector, angle, globe, random | 10 | c_xyz, c_angle, c_math, plus cM_rnd_c::init in d_drawlist |
| Ground-query base | 3 | cBgS_GndChk constructor, cBgS_Chk destructor/typeinfo |
| Spline initialization | 1 | d2DBSplinePath::Init in d_spline_path |
| Window and focus-line drawing | 3 | setScissor, setViewPort, dDlst_effectLine_c vtable |
| Ship offset | 1 | Original daObjPirateship::getShipOffsetY in d_com_static |

Counts classify the raw closure against the six camera units, not a claim
that 24 implementations are missing. Many owners already have qualified
source or bounded runtime tiers elsewhere. Release's two fewer references
are not stronger runtime evidence. Full camera update dependencies and full
player linkage are still unmeasured.

## Verification

- Full default builds and 67/67 public CTests pass in all three modes.
- Both full PLAYER and six-unit camera census builds pass in all three modes.
- Explicit constructor link fails as recorded, excluded from default/CTest.
- Five synthetic player asset-preparer tests pass; private header stays ignored.
- Private Toripost heap, McaMorf, BG, Stone2, and Room44 parent lifecycle probes
  pass in Debug, Release, and strict sanitizers after rebuilding the shared ABI.
- Strict runtime options: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1`. This does not certify process-wide leak freedom.
- Patch preparation through 0146 and whitespace checks pass. Protected
  cpu_exception.c/hle_core.c hashes and dependency lock match the previous
  checkpoint. Audit reports exactly the known 20 tracked local-research files;
  no new private data is added.
- Private logs: ignored `local-research/evidence/route-b-camera-storage-20260906/`.

Reproduce with the player preparation instructions in `route_b/README.md`,
then build the two census object targets and the explicit constructor link
target in `build/route-b-{debug,release,sanitize}`. Public tests use CTest;
private probes use corresponding `build/route-b-aurora-gx-*` trees and the
validated disc alias. No performance or visual claim follows from these runs.

## Next action

Compose the measured original constructor owners, using existing qualified
implementations where their contracts fit. Qualify real controller creation
against genuine player/game-state ownership, then update/delete and full
PLAYER link/create/execute/draw. Preserve original stage-dependent behavior,
including ship offsets and focus-line ownership; do not replace it with a
fixed camera. The organizing milestone remains continuous controllable
Outset, followed by transition/save/reload and normal-boot acceptance.
This checkpoint does not complete BW-P4-0126 or the durable product goal.
