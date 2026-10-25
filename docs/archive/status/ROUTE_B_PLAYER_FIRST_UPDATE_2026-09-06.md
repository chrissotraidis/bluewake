# Original Link first update — 2026-09-06

Base `3688ca3`; patches 0189–0192. BW-P4-0126 remains active; P4 is not passed.

## Integrated evidence

The new private player-execute probe retains real Link resources, Room44 DZB,
stage RTBL, collision and audio owners through original phase three and one
original `daPy_lk_c::execute` call. It reads Room44 PLYR point 0 parameters,
coordinates and yaw instead of choosing triangle 0. Placement is still a
diagnostic operation, not normal PLAYER profile admission or original boot.

Debug, Release and strict ASan/UBSan complete the first update. Ground height
is 550 in room 44. All 42 joint matrices remain finite. Three collision
objects retain the correct native actor identity (zero attack, three target,
one contact registrations). Original room streaming queues one Room0 scene
request; requested is set and loaded remains clear. Original audio health
and off-board state agree with the diagnostic game state.

This proves one integrated update, not movement under input, contact
resolution, completed room loading, player-following camera, drawing or a
sustained session. The existing 120 camera updates precede Link initialization
and still use a non-PLAYER subject. Save/story/STAG state remains diagnostic.

## Reached failures and source owners

- Triangle 0 entered an exit-area branch. The probe now uses real PLYR point
  0 rather than qualifying an exit caused by arbitrary placement.
- 0189 shares unchanged original room-streaming and stay/time-pass methods
  between full stage source and the native stage runtime. Original scene,
  overlap and node-request owners queue the request; no loaded flag is faked.
- 0190 shares original player environment/audio state methods.
- 0191 composes original detection, TagLight constants and collision
  registration. The actor-queue callback uses its actual typed actor payload.
  Collision registration is not collision resolution.
- 0192 reconstructs the empty health/audio feedback method from GZLE01 DOL
  instructions at 0x802AB8B0–0x802AB9F4. It preserves signed inputs, separately
  rounded float conversion/division, upper-only ratio clamp, sub-BGM selection
  and ordered tempo arithmetic. Floating-point contraction is disabled.
  A request observer checks 2,662 health/BGM cases and 400 equipment cases;
  it is not sequence playback. This is behavioral reconstruction, not a
  byte-matched object or an instruction-execution oracle.
- Strict sanitizers rejected a null particle-controller receiver in original
  foot-effect checks, although Debug/Release returned successfully. The probe
  now calls original Zelda-heap creation, game particle creation and particle
  construction, checks the solid heap's parent, and restores root allocation
  context. Original Zelda heap definitions replace overlapping diagnostic
  definitions only in this target. The intermediate current-heap mistake
  produced a solid-heap individual-free warning; the corrected replay does
  not. No emitter resources/backend are initialized or claimed.

Unsupported exit, path, boomerang and audio backend branches abort on use.
No successful substitute is admitted to production. Full PLAYER deletion,
global frame scheduling, scene-request progression and particle teardown are
not covered; the private diagnostic exits without normal global teardown.

## Verification state

- First-update probe passes Debug, Release and strict ASan/UBSan with
  `detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`.
- Public CTest passes 82/82 in each configuration.
- Python player-asset tests pass 10/10.
- Both source preparers pass; protected recompcore hashes and dependency-lock
  hash are unchanged. Audit output is byte-identical to the known 20 tracked
  local-research-files failure. No game data or dependency pin was changed.
- Rebuilt private regressions pass in all modes: phase three, SE registration,
  animation playback, player init/event identity/model, camera run/event/
  constructor/matrix/mass, sea, attention, DZB, toripost heap, morph audio,
  BG profile, stone2 and room lifecycle.
- Callback parameter/emission oracles pass 11,076 / 12,880 comparisons per
  configuration. Production creation/phase-three/runtime link censuses remain
  four/41/52 missing symbols with expected build exit 2 in each mode; private
  fences do not silently qualify the production profile.

Evidence: ignored `local-research/evidence/route-b-player-first-update-20260906`;
working `/tmp/bluewake-player-execute-*` logs. Earlier failed
attempts are described above; successful exit alone was not used to dismiss
the allocator warning. No app or Simulator was launched.

## Next action

Compose original per-frame collision and process/scene progression with real
input and PLAYER-following camera. Original `dScnPly_Draw` calls collision
`Move`, which performs area/contact checks and clears registration counts;
this is the next source-owned boundary, not a manual counter reset.
Do not loop `execute` alone over accumulating registrations and pending room
requests or treat that as a sustained gameplay session. The target remains
visible, controllable Link with collision and camera, then all remaining PRD
requirements. No external blocker exists.
