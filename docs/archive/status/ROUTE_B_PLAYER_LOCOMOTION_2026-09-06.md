# Original input-driven PLAYER locomotion — 2026-09-06

Base `d766d29`; BW-P4-0126 remains open. This is a headless diagnostic
locomotion increment, not normal boot, PLAYER profile admission or gameplay
acceptance. Required checkpoint verification is complete.

## Measured behavior

The original PAD/JUTGamePad/CPAD path delivers main-stick and camera-stick
axes, A rising/held/release edges and analog-L transitions. Original PLAYER
updates consume the resulting stick value. With the actual Room44 PLYR
point 0 supplied through an actor append, 120 held-input updates produce:

- 119 changing right-foot matrices;
- 103.517 game units of net horizontal displacement;
- real ground/collision processing after each update;
- 60 further neutral-input updates ending at zero horizontal velocity.

The focused run reports the same displacement and passes assertions in
Debug, Release and strict ASan/UBSan. No position, speed, animation blend or
actor scale is assigned to force movement.

## Root cause of the stationary diagnostic

The previous phase-two-only diagnostic bypassed fopAc_Create and supplied no
actor append. Cleared actor storage therefore retained scale (0,0,0). Real
BCK frame values advanced and sampled rotations changed, but every foot
matrix collapsed to the actor's root position. Merely checking 42 matrices
for finite values did not detect this degeneracy.

The corrected diagnostic routes its process callback through original
fopAc_Create, whose actor-method callback still invokes phase two. Original
fopAcM_CreateAppend provides defaults including scale bytes (10,10,10), and
the original actor wrapper derives unit scale. Real PLYR position, yaw,
parameters and room are supplied in the append and consumed by that wrapper.
The previous direct actor-position/yaw and process-parameter assignments are
removed. Original draw-tag initialization is composed as well.

This remains a diagnostic actor profile, not the complete original PLAYER
creation chain, process/scene loop or full boot. Generic actor execute/draw
scheduling and profile teardown are not claimed.

## Input and native portability

- 0195 preserves full native addresses in the local GBA receive-buffer
  registration table, including its four AGB publications. Pointer/neighbor
  controls exercise the native record; GBA transfers and AGB behavior are
  unqualified and communication remains disabled by original initialization.
- 0196 matches native PADStatus's 16-byte Aurora ABI, with all offsets
  asserted. The console declaration remains unchanged.
- 0197 preserves retail fctiwz/halfword behavior at the forward-stick angle
  endpoint. All 65,536 signed-byte stick pairs pass strict finite/range and
  cardinal-angle controls; this is not a full retail interpolation oracle.
- 0199 preserves the two hat-phase cosine arguments' integer-then-halfword
  conversion, verified against retail 8011C9CC–8011C9D8 and
  8011CA40–8011CA4C. It fixes the reached update-17 sanitizer failure.

The Abseil failure was separately reproduced without game code. Route B now
builds Aurora's existing fallback version 20240722.0 from an archive-hashed
source pin, with matching instrumentation instead of unsanitized Homebrew
container objects. A standalone container test checks empty iteration,
mutation, copy/move and reuse. No sanitizer/generation/ODR checks are disabled.
The exact commit, archive hash and Apache-2.0 provenance are in the dependency
lock and `cmake/RouteBAbseil.cmake`.

## Audio composition and limits

Original animation scheduling and the qualified reconstructed registration
owners now compose in PLAYER. 0198 shares the original derived-sound factory,
stream initialization/status and default listener initialization. A retained
diagnostic JAI heap owns a real JaiInit sound table and original SE/stream
pool initialization; JAIZelSound has its original constructor and methods.

Unsupported sequence/stream/DSP branches remain uniquely logged aborts,
not recording fixtures or fabricated successful voices. The listener is the
original default listener, not a PLAYER-following camera. No audible output,
full audio startup/frame processing, or complete registration teardown is
claimed. Sound-distance culling and active registrations need integrated
PLAYER listener evidence before audio behavior is promoted.

The native PSMTXMultVec entry delegates to Aurora's actual scalar transform.
Nonidentity and in-place controls pass; arbitrary paired-single rounding
equivalence has not been established.

## Verification

- Focused locomotion: Debug, Release, strict sanitizers pass.
- Public CTest: 82/82 in each configuration.
- Retail animation parameter/emission comparisons: 11,076 / 12,880 cases pass
  in each configuration.
- Private accumulated matrix: all 20 targets and the additional PLAYER
  event-identity run pass in each configuration, including rebuilt pointer
  registration/neighbor controls.
- Production create / phase-three / runtime link censuses: expected build
  exit 2 with unchanged 4 / 41 / 52 unresolved symbols in all configurations.
- Both preparers pass; all 10 player asset tests pass. Both protected
  recompcore hashes are unchanged.
- Audit: byte-identical to the known 20 intentionally tracked research files.
- Diff whitespace check: pass.

The dependency lock intentionally adds the source-built Abseil pin; its final
SHA-256 is `35444a9868d3fd8d0b17b975bcd754b203d9558e12552fb819e0090a8f56c5e4`.
Protected CPU exception/HLE source hashes remain respectively
`5b8ebbc60520b8bc6ee2166c344d760a88a3d2f2419058eaeaa210f5ec07def6` and
`dcb97681b2d325cd9fbf23e7d359a6788aa3f05980174fc31fa299a7b3c4716a`.

Evidence is in ignored `/tmp/bluewake-player-locomotion-*`, the earlier
`/tmp/bluewake-player-gait-*` and `/tmp/bluewake-player-actor-init-*` logs.
Earlier failures and investigation details remain in
`ROUTE_B_PLAYER_INPUT_WIP_2026-09-06.md`.

## Next product boundary

After complete regression and a coherent checkpoint, compose a PLAYER camera,
real drawing and scene progression with these owners. The current camera
exercise still uses a non-PLAYER subject, one room request is not progressed,
and the probe exits without full profile teardown. The P4 normal-boot,
transition and save/reload requirements remain open. No GUI/Simulator was
launched and no user save or game data was modified.
