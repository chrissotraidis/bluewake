# PLAYER original common-game and stage runtime — 2026-09-06

Base: `9d67bdc`. Blocker: **BW-P4-0126 / ROUTE_B_PLAYER_CAMERA_RUNTIME**.
P4 remains open. No external blocker; macOS first, mobile deferred.

## Change and causal scope

Original `phase_3` / `makeBgWait` requires common-game, item, message,
geometry, stage-selection and restart services. Full `d_item.cpp`,
`f_op_msg_mng.cpp`, `c_m2d.cpp`, `d_lib.cpp` and `d_com_inf_game.cpp` now
compose with PLAYER. This is dependency integration, not proof that every
function in those units executes correctly.

- Patch 0174 makes the existing day-count quotient's narrowing to `u32`
  explicit. Full d_lib replaces the animation-only fragment and provides
  original STControl construction required by the message manager.
- Patch 0175 retains player-recollection buffer addresses as byte pointers,
  preserving native field-offset arithmetic without 32-bit pointer truncation.
  An explicit external-storage option excludes only the common-game singleton
  definition for the existing raw-singleton diagnostic. Original methods and
  colors replace PLAYER's color/accessor fragments. Normal singleton
  construction and recollection execution remain unqualified; the upstream
  recollection code also documents a fifth equipment-array access requiring
  investigation before admission.
- Full d_stage syntax admission still encounters fourteen legacy serialized
  offset/pointer errors. Patch 0176 shares eight original runtime bodies in
  `d_stage_runtime.inc`, included by both the full source and a native-runtime
  composition. All eight bodies were compared byte-for-byte with the prior
  patched source. No behavioral substitute or disk-offset-to-host-pointer cast
  was introduced. The existing native room-read view owner supplies reverb.
  Runtime functions operate on the established decoded resource interfaces.

The moved functions are start-stage set, next-stage set, player-ID query,
ocean X/Z decode, change-scene request, room restart and turn restart. Their
presence does not qualify a completed scene transition.

## Executed evidence

The private PLAYER init probe exhaustively verifies all 256 packed signed
ocean-coordinate values. It verifies next-stage request latching, preservation
of every field when a second request is ignored, explicit release and reuse.
These are synthetic service oracles, not gameplay acceptance.

Actual Link phase two then returns NEXT using the original process allocator
and diagnostic phase-two callback. ID 37, parameters, layer/tags and event
identity remain correct. Existing real model/animation/skin checks also pass.
This combined probe passes Debug, optimized Release and strict ASan/UBSan.

All 74 public CTest tests pass in all three configurations. Accumulated
private model, camera-run/event/constructor/matrix/mass, sea, attention, DZB,
Toripost heap, morph, BG, Stone2 and room lifecycle probes pass in all three.
Player asset Python tests pass 10/10. TWW preparer through 0176 and Aurora
preparer checks pass; shell syntax and diff whitespace checks pass.

Both protected recompcore hashes and dependency-lock hash remain unchanged.
Repository audit still fails only for the known 20 tracked local-research
files. No additional private artifacts are staged. No GUI or Simulator was
launched; no performance, visible gameplay or input claim is made.

Private logs: ignored `local-research/evidence/route-b-player-stage-runtime-20260906`.
Builds use the existing pinned TWW/Aurora dependencies, private GZLE01 disc,
Apple Silicon macOS and the three established CMake configurations.

## Remaining frontier

All-mode phase-two/three linkage has 58 unresolved symbols, compared with 76 at
the base checkpoint. Intermediate counts were 70, 67, 64 and 59. These are
static dependency counts, not executed calls, remaining task counts or a
percentage of completion. Original turn-restart/day processing exposes letter
delivery owners; audio, other actors, JPA and JStudio remain in the closure.
Phase-two-only and phase-two/execute censuses retain four and 71 symbols in
every mode. All three censuses intentionally fail at the linker, exit 2.

Next: compose the original letter/day-processing dependency, then resolve the
remaining behavior owners with source or independent reconstruction evidence.
Continue toward actual makeBgWait with real Room44 collision and initial model
calculation, then PLAYER execute. Do not skip phase three or inject a fake
initial procedure. Full PLAYER admission/deletion, continuous update/draw/input,
normal boot, audio, saves and full-game acceptance remain unproven.
