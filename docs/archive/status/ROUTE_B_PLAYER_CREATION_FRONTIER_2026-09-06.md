# PLAYER creation frontier and full model services — 2026-09-06

Base `b0da8c6`; BW-P4-0126 remains active. No PLAYER creation, execute,
visible control or P4 promotion. This checkpoint separates creation's static
dependencies from the much larger continuous-update closure.

## Measured dependency sets

The optional `bluewake_route_b_player_runtime_link_census` retains original
phase_2 and daPy_lk_c::execute against the combined camera/resource services.
The initial strict-sanitizer link exposed 196 unresolved symbols. Composing
original dCcD, Acch, water/special-group checks and csXyz reduced this to 165.
Full m_Do_ext composition plus the corresponding J3D model/cluster owners
reduces the final creation/update census to 156. No missing behavior is given
a fake successful implementation to obtain these counts.

The separate `bluewake_route_b_player_create_link_census` retains phase_2
without execute. It exposes 19 unresolved symbols in the final composition:

- Particle/actor callbacks: fopKyM_create, JPAGetXYZRotateMtx,
  JPASetRMtxTVecfromMtx, JPABaseEmitter::deleteAllParticle, dPa_control_c::mStatus,
  smoke/follow callback constructors, ripple/cut-turn callback vtables.
- Graphics: GFSetBlendModeEtc, mirror packet init and vtable.
- Save/event queries: dSv_info_c::isSwitch/onSwitch, dSv_event_c::isEventBit.
- Audio: linkVoiceStart, getLinkVoiceVowel, checkPlayingSubBgmFlag and
  checkPlayingMainBgmFlag.

These are retained static dependencies, not an executed call trace or 19 small
tasks. The constructor retains callback vtables; some listed methods need not
execute during startup. For example, fopKyM_create is referenced by the water
drop particle callback, while mirror init is referenced directly by playerInit.
The probe retains existing diagnostic audio/cutscene/room boundaries from the
camera setup, so this is not a production-complete executable link check.

Original phase_1 publishes the player and attention pose; phase_2 waits for
the camera, placement-constructs the real daPy_lk_c and calls playerInit.
playerInit begins with the original solid-heap createHeap callback. That heap
initializes Link models, BCK/BTK/BRK resources, skinning, old-frame blending,
animation buffers and HIO. phase_3 then waits for real ground collision.
None of these PLAYER phases is claimed executed here.

## Full model/animation owner repair

The optional `bluewake_route_b_player_ext_census_objects` compiles the FULL
m_Do_ext.cpp rather than selecting only model-create, model-draw or McaMorf
fragments. Its initial compile exposes missing JKRExpHeap declarations,
archive pointer-difference narrowing and ungenerated display-list includes.

Patch 0159 includes the complete native heap type and checks the archive entry
index before explicit conversion to the original s32 return type. Existing
valid lookups preserve their result; missing/unrepresentable indices return -1.
The runtime test covers actual System IDs and an actually absent ID, not a
fabricated gigantic archive or unrelated-pointer subtraction.

The private PLAYER asset generator now emits the existing sight display list
plus five exact m_Do_ext display-list includes. Pinned symbols/config specify:

| Symbol | DOL address | Bytes | Linkage |
|---|---|---|---|
| l_invisibleMat | 0x803717C0 | 133 | local |
| l_matDL | 0x80371860 | 141 | local |
| l_toonMatDL | 0x80371900 | 156 | global |
| l_mat1DL | 0x803719A0 | 150 | global |
| l_toonMat1DL | 0x80371A40 | 165 | global |

All arrays retain 32-byte alignment and original local/global linkage. DOL
identity and bounded, unique section mapping are checked before any output is
written. Outputs remain under ignored generated/route_b_player/assets; no
game bytes are tracked. Eight synthetic generator tests cover both asset
families, all descriptors, identity/range/mapping errors and determinism.

The PLAYER targets remove overlapping partial m_Do_ext object groups and
retain their non-overlapping d_lib, machine, actor-heap, J3DModel and cluster
sources separately. There is one full model/animation implementation owner,
not an allow-duplicate-symbol workaround.

## Executed combined service checks

`bluewake_route_b_private_player_model_probe` takes the validated disc as its
sole argument. It uses the full model/animation owner with the prior real
System/Always camera setup, without linking or substituting a PLAYER object.

It checks original archive-ID conversion, 32 forward and eight reverse frame
updates, allocation inside the intended solid heap and exact parent-heap
recovery, and an eight-step old-frame morph countdown against independent
expected ratios. Zero-duration initialization, joint range and native array
identity are checked too. The same executable then performs the 120 original
camera updates, yielding center x 100 -> 340. These are service contracts,
not Link skinning, decoded Link animations or rendered display-list proof.

## Verification and next action

Public 72/72, full PLAYER and full m_Do_ext compilation, the new combined
service probe and all existing private/camera regressions pass Debug, Release
and strict ASan/UBSan. Creation/update and creation-only censuses deliberately
remain link failures, with their measured counts recorded above. Both preparers,
eight generator tests, regeneration, shell syntax and whitespace checks pass.
Protected runtime hashes and dependency lock are unchanged; audit retains
only the known 20 tracked local-research files. No app window or Simulator is
launched. Private evidence is ignored under
`local-research/evidence/route-b-player-creation-20260906`.

Next: compose the original particle-callback/mirror construction owners and
classify the remaining phase-two requirements, then execute actual PLAYER
initialization with real Link resources and the already-running camera.
Do not let the 156-symbol continuous-update census replace that first runtime
milestone, and do not equate 19 link symbols with a small amount of missing
behavior. Unavailable diagnostic paths must still visibly fail on use.
