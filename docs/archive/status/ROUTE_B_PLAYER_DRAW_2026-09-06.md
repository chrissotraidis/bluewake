# Original PLAYER camera and draw loop — 2026-09-06

Verified increment from `c7ac69a`. BW-P4-0126 remains active; P4 has not passed.
This is headless execution and packet submission, not a playable application.

## Result

Debug, optimized Release and strict ASan/UBSan pass 180 original PLAYER frame
iterations: controller read, PLAYER execute, collision Move, original camera
execute/draw, and original PLAYER draw. The first 120 hold the stick; the next
60 release it. Observed displacement is 105.843, foot matrices change on 119
held-frame comparisons, and released horizontal speed ends at zero. The camera
follows the actual published Link, with center displacement 331.581.

Every frame checks valid camera projection, view/inverse agreement, listener
pointer identity, empty initial 3D draw buffers, actual Link-model packets and
restoration of the original default draw buffers. Frame reset occurs before
camera drawing, preserving its map/2D enqueue work. First/last frames contain
26 queued packets and 21 body-shape references. Link identity is established by
the queued shape model's joint-zero matrix pointer matching PLAYER's original
joint accessor, not by a diagnostic replacement model.

## Source ownership and repairs

- Original scene/node registration and play phase one publish the stage;
  original PLAYER phase one publishes Link before phase two/three.
- 0200 shares original BMT conversion/material setup. Actual ICE MAT3 names,
  counts and original BTK material bindings are checked against retained bytes.
- 0201 exposes original camera execute/draw; 0202 composes real STAG publication
  with actual common-game stage/save helpers (near 1, far 160000).
- 0203–0204 share original render/map/list and camera audio state. Full original
  map/AGB sources are composed. Unsupported scene/wave audio transitions retain
  explicit aborts; they are not reached by this replay. Logging-only upstream
  transition stubs are not accepted as behavior.
- Complete original d_drawlist.cpp replaces the PLAYER mirror/window/camera/
  2D/insertion slices. Original construction precedes init/reset, establishing
  embedded polymorphic shadow receivers as well as allocated draw buffers.
  Real Always simple-shadow texture binding and original shadow collision run.
- The private asset preparer now emits 33 pinned-DOL headers, including original
  draw-list bytes and native-endian vertex arrays. No game assets are tracked.
- Original KANKYO profile creation runs through fpcBs_Create/SubCreate. Real sea
  EnvR/Colo/Pale/Virt metadata and Room44 FILI use existing native loaders;
  environment pointers match the real stage records. Room selection checks
  establish start/stay/actor/lighting room 44.
- 0205 shares original scene countdown and menu pause storage/query/setter.
- 0206 reconstructs the three audio position resets from the locked retail DOL:
  each clears its corresponding 32-bit member (sea 1b80, river 1dd0, window
  1ec0). The old empty-source-body tests were not retail behavior evidence.
  Updated tests require exact field resets, preservation of every other byte,
  singleton identity and idempotence.

## Verification

- Public CTest: 82/82 in Debug, Release and strict ASan/UBSan.
- All 20 private/composed probes plus separate PLAYER event identity: all modes.
- Retail animation comparison corpora: 11,076 parameter and 12,880 emission
  cases in all modes; these do not qualify sound backend output.
- Production PLAYER create/phase-three/runtime link censuses: expected exit 2,
  unchanged 4/41/52 missing symbols in all modes. Production remains incomplete.
- Private asset-preparation unit tests: 11 passing.
- Both source preparers and git diff whitespace check pass.
- Protected CPU exception/HLE hashes and dependency lock hash are unchanged.
- Repository audit reports only the known 20 intentionally tracked evidence
  files under local-research; no new audit failure.

Evidence: `/tmp/bluewake-player-draw-loop-{sanitize,debug,release}-run.log`,
`/tmp/bluewake-draw-matrix-*`, `/tmp/bluewake-player-draw-public-*-ctest.log`,
`/tmp/bluewake-player-draw-*-{parameter,emission}-oracle.log`,
`/tmp/bluewake-draw-census-*`, and `/tmp/bluewake-player-draw-audit.log`.
Investigation history is in `ROUTE_B_PLAYER_CAMERA_ADMISSION_WIP_2026-09-06.md`.

## Limits and next work

The diagnostic still uses raw game-info backing with selected original
subobject construction, selected scene/PLAYER phases, retained archives/heaps,
virtual controller input and an explicit frame loop. A room request remains
pending. Full scene/environment scheduling, normal boot, GPU consumption,
visible correctness, live interaction, audio output and teardown are not
qualified. ASan leak detection remains disabled for these retained-lifetime
diagnostics; `_Exit` is not normal shutdown acceptance.

Next compose the visible native frame and live input with actual PLAYER,
camera and Room44 geometry, then scene progression. Do not claim playability
until the app is launched, observed and measured. No external blocker.
