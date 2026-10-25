# Original Link phase three on real Room44 ground — 2026-09-06

Base `0961835`; patch 0188. BW-P4-0126 remains active; no P4 promotion.

## Integrated result

The new `bluewake_route_b_private_player_phase_three_probe` composes the
existing real Link archive/model initialization and original phase two with
original `phase_3` / `makeBgWait`. It first executes the no-ground wait and
requires `cPhs_INIT_e`. It then registers actual decoded Room44 DZB geometry
in the existing original dBgS world and places Link on a queried triangle.
The original phase-three method returns `cPhs_NEXT_e` (2).

Debug, optimized Release and strict ASan/UBSan all report:

- Room 44, ground height 2200; placement on real triangle 0.
- All 42 Link joint matrices contain finite values after initialization.
- Original item-model work publishes sword `(2,2)`, shield `(1,2)`, boots 0
  to the constructed game-facing audio interface.
- No unsupported-service fence is reached in the accepted replay.

This advances actual Link initialization, not just an isolated audio test.
It does **not** prove normal spawning, continuous player updates, controller
input, visible geometry, player-following camera, sound output or gameplay.

## Reached evidence changed the plan

The prior production link census had 41 missing phase-three symbols. Rather
than reconstructing every referenced branch before execution, the diagnostic
retains explicit abort-on-use methods for unavailable actor/audio services.
It is a separate EXCLUDE_FROM_ALL target, never production admission.

Actual execution first passed the no-ground wait, then stopped in
`dStage_roomRead_dt_c_GetReverbStage` because the old diagnostic stage had no
room table. The probe now mounts real `/res/Stage/sea_T/Stage.arc`, validates
RTBL node/table/record/room-array bounds, and calls the existing original
room-table publication owner. Fifty room entries publish; Room44 reverb is 0.
The archive stays alive while its borrowed room bytes are used.

The next reached boundary was `JAIZelBasic::setLinkSwordType` from
`setItemModel`, near the end of `makeBgWait`, after its animation/model work.
Its receiver was absent in the old fixture. No DSP dependency had been
reached. Connecting actual construction and original equipment setters was
sufficient for this initialization path to finish.

## Source change

0188 exposes original JAIBasic construction separately from unrelated
methods. It moves the unchanged original JAIZelBasic constructor and three
equipment setters to a shared conditional section, preserving their full
normal-build definitions and making them available to the player-interface
tier. There are no replacement successful setter bodies in the test.

Both original constructors execute over explicitly cleared, aligned storage.
The diagnostic checks singleton publication and initial equipment/BGM state,
then restores prior singleton/heap pointers after its bounded use. Audio
driver/interface initialization is absent; JASDram is null and unavailable
virtual factories abort. This is not full JAIZelSound or audio startup.

Room collision arrays are allocated on a dedicated JKRSolidHeap, as required
by cBgW, not the exp heap. The world unregisters and its object is destroyed
before that heap is retired. No allocator warning is suppressed.

## Diagnostic limitations

- Link is allocated through the original process allocator and phase-two
  callback; phase three is called directly on that correctly adjusted actor.
  The complete PLAYER profile lifecycle is not admitted.
- Placement is explicitly chosen on real geometry, not read from normal PLYR
  spawning. Save/story state, STAG and the camera setup remain diagnostic.
  Only the reached RTBL metadata is newly loaded from the real stage.
- Existing raw game singleton and headless model/resource fixtures remain.
  Finite matrices are not a visual rendering check.
- Instrument, salvage and race globals have explicit diagnostic storage;
  unavailable methods abort. Their full owners are not qualified. The
  instrument constructor values in the fixture do not constitute production
  instrument admission.
- No sound is emitted. The preceding registered-BAS replay remains an
  independent regression, not a JAI frame backend installed in PLAYER.
- The process exits without full PLAYER deletion or normal scene teardown.
  No continuing frame is run after retiring this diagnostic ground/audio.

## Verification

- New real-ground probe passes Debug/Release/strict ASan+UBSan, with
  `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1` for strict checks.
- Public CTest: 81/81 in all three configurations.
- Animation parameter/emission oracle replays: 11,076 / 12,880 cases in each.
- Rebuilt private regressions pass in all three modes: registered SE,
  animation playback, player init and event identity, player model, camera
  run/event/constructor/matrix/mass, sea, attention, DZB, toripost heap,
  morph audio, BG profile, stone2 and room lifecycle.
- Python player-asset tests: 10/10.
- Production link censuses remain 4 / 41 / 52 (phase two / phase three /
  execute) in every mode, with expected linker exit 2. This diagnostic
  completion does not silently remove the production missing-service gate.
- Both source preparers pass. Protected recompcore files and dependency-lock
  SHA-256 hashes match the previous checkpoint. Repository audit output is
  byte-identical to its known 20 tracked local-research files failure.
- No app window or Simulator was launched; no user assets, dependency pins
  or protected files were changed.

Logs: ignored `local-research/evidence/route-b-player-real-ground-20260906`;
working logs `/tmp/bluewake-phase-three-*`. Early failing logs are diagnostic
history, not the final passing replay.

## Next required action

Keep the real ground, resources and constructed interface alive and execute
original Link update behavior. Measure the first reached missing service,
then connect authentic controller state and camera updates toward a continuous
Link/ground/input/camera/draw session. Preserve fail-on-use boundaries and
distinguish diagnostic placement from normal profile/spawn admission. Do not
return to unneeded backend completion or isolated symbol counts as the main
plan. Full product requirements, transitions and saves remain required.
