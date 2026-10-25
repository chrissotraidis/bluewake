# Route B BG Profile Execution - 2026-09-03

## Boundary

Execute the exact leading Room44 BG request through the unchanged original
profile and standard actor framework. Qualify archive lookup, J3D model
construction, optional material-animation ownership, collision registration,
room-ready publication, and one steady execute call without admitting the
other 50 persistent actor requests or phase 4.

This tier stops before `daBg_c::draw`, deletion, archive unload, functional
collision queries, or general J3D animation sampling. Constructed models are
therefore not yet evidence of visible Route B geometry.

## Result

The exact `/res/Stage/sea_T/Room44.arc` is 714,816 bytes and contains 26
entries. The unchanged original `g_profile_BG` now consumes its exact request
through standard process creation. It resolves and constructs three J3D model
instances from `model.bdl` (399,872 bytes), `model1.bdl` (111,296 bytes), and
`model3.bdl` (3,360 bytes); `model2` is absent, as expected.

The actor also resolves `model1.btk` (2,784 bytes), constructs one original
`mDoExt_btkAnm` owner, constructs one `dBgW`, invokes the bounded collision
`Set` and `Regist` owners, and publishes Room44 ready status `0x10`. No BRK is
present. One call through the profile execute method succeeds. The exact BTK
material name begins `SC_01`, selecting the actor's global-wave synchronized
frame path; a local `mDoExt_btkAnm::play` call is therefore correctly absent.

The principal failure was not in the actor. The reduced dRes tier returned raw
serialized `.btk` bytes where the original code requires a constructed
`J3DAnmTextureSRTKey`. The fix promotes the actual `J3DAnmLoader.cpp` owner for
this resource path and converts the reached big-endian metadata, offsets, and
name-table entries. Two vertex-color offset sites were made native-width so
the complete loader translation unit compiles on the host.

ASan then exposed a private-harness overflow: storage for `dComIfG_inf_c` used
the original 32-bit fixed size `0x1D1C8`. The probe now allocates
`sizeof(dComIfG_inf_c)`, matching the target-PC aggregate layout.

## Qualified Contract

- Exact Room44 archive and BG resource-name resolution.
- Three original J3D model-loader and model-create calls.
- One original BTK loader and wrapper construction, including material-name
  binding for the reached Room44 format.
- Original BG create completion, one profile execute call, and room-ready bit
  publication.
- One `dBgW` construction plus bounded collision `Set` and `Regist` calls.
- Missing optional `model2` and BRK resources remain valid absence paths.

## Remaining Fences

- `daBg_c::draw` is unentered. Graphics-list selection, model calculation,
  clipping, environment lighting, and `mDoExt_modelEntryDL` remain open.
- Collision calls are bounded host fences/counters. Raw DZB conversion and
  functional collision queries are not qualified.
- BTK metadata and material-name binding are qualified, but key tables and
  scale/rotation/translation samples remain serialized. Actual animation
  sampling is not qualified; the Room44 special path only chooses a global
  frame.
- Map-transform lookup remains a false-returning fence.
- Delete, unload, resource release, and vegetation cleanup are unentered.
- The private probe uses zero-initialized storage for the real game-info
  singleton; its full aggregate constructor is outside this tier.
- Other animation formats in `J3DAnmLoader.cpp` retain legacy 32-bit
  integer-to-pointer offset casts. The object target temporarily suppresses
  that diagnostic; only the reached BTK path was corrected.
- Duplicate `_OSPanic`/JUT assertion symbols remain a known static-archive
  linker warning and did not alter execution.

## Verification

- Debug: exact private BG probe passes; all 63 public CTest tests pass.
- Optimized Release: exact private BG probe passes; all 63 public tests pass.
- ASan/UBSan: exact private BG probe passes; all 63 public tests pass with
  `detect_leaks=0` because LeakSanitizer is unsupported on this macOS setup.
- Exact success marker:
  `bg-profile room=44 models=3 btk=1 brk=0 collision=1 execute=1 pass`.
- `scripts/prepare_route_b.sh --check` passes with patch 0123.

No BlueWake process or Simulator was launched for this boundary.

## Natural Stopping Point

BG creation and one steady execute call form a complete, reproducible
checkpoint. The next blocker is `ROUTE_B_BG_DRAW_SUBMISSION`: enter the
original draw method from this exact passing probe and qualify only the owners
needed to calculate, clip, light, and submit all three models while preserving
graphics-list state. Deletion/unload is a separate successor boundary.
