# Route B BG Delete - 2026-09-05

## Boundary

Delete the exact Room44 BG actor through the standard process-deletion queue,
unchanged actor framework, and unchanged BG profile methods. Prove the actor's
collision-release call, room publication/status cleanup, solid-heap teardown,
and repeatable create-execute-draw-delete lifetime.

This boundary stops before room-scene deletion and archive unload, functional
collision queries, presented pixels, the other 50 persistent Room44 profiles,
or play phase 4.

## Result

The exact actor is submitted to `fpcDt_Delete`. The probe proves the retail
delete tag's one-handler-tick delay, then observes the original
`fpcLf_Delete` -> `fopAc_Delete` -> BG delete chain complete. The unchanged BG
destructor releases the same collision pointer admitted at creation, clears
`dStage_roomControl_c::getBgW(44)`, and turns off ready flag `0x10`. The actor
framework then reaches solid-heap destruction and removes the process.

The probe repeats the entire lifecycle using the same exact mounted
`/res/Stage/sea_T/Room44.arc`: a second BG actor creates all three models,
executes with wave frame 38, calculates and submits packets for every model,
restores shared draw state, and completes the same delayed delete sequence.
Both cycles finish with no published BG pointer or ready flag.

Runtime assertions deliberately prove `model.bdl` and `room.dzb` remain
available after both deletions. This is source-correct: `daBg_c::~daBg_c` owns
actor state but calls no resource-delete API. Retail `dScnRoom_Delete` owns
`dComIfG_deleteStageRes`, room-data/status reset, optional particle removal,
and room-memory reset.

Strict UBSan exposed that the zero-backed game-info probe lacked the demo
owner used by generic actor cleanup. Patch 0126 adds a diagnostic-only setter;
the probe provides minimal demo and audio owners whose reached methods remain
observers.

## Qualified Contract

- Standard delayed process deletion enters the unchanged leaf, actor, and BG
  delete methods.
- The exact registered collision pointer is released once per actor.
- The Room44 BG publication and ready bit are cleared once per actor.
- Actor solid-heap destruction is reached once per actor.
- Process lookup no longer finds either actor after deletion is queued.
- A second create-execute-draw-delete cycle reconstructs and submits all three
  exact models, then returns to the same clean actor-owned state.
- The mounted Room44 archive remains available across actor lifetimes.

## Remaining Fences

- Collision `Set`, `Regist`, and `Release` are bounded stateful call-contract
  implementations. No DZB query or collision response is claimed.
- The heap-delete helper executes the retail operation—destroy a non-null
  solid heap and clear the actor pointer—but its broad manager translation
  unit is not promoted.
- Generic actor demo lookup and audio object deletion terminate at minimal
  probe owners.
- Vegetation packet cleanup branches are not reached because those shared
  owners are null in this exact BG probe; their room cleanup was qualified in
  the GRASS and Obj_Wood checkpoints.
- Archive deletion/unload is not a BG fence. It belongs to the room-scene
  delete method and remains the next blocker.

## Verification

- The exact private BG probe passes in Debug and optimized Release.
- Strict ASan/UBSan passes with halt-on-error settings and
  `detect_leaks=0`, required by this macOS setup.
- All 63 public CTest tests pass after full rebuilds in Debug, optimized
  Release, and sanitizer configurations.
- Exact success marker:
  `bg-profile room=44 models=3 btk=1 brk=0 collision=1 execute=1 draw=1 calc=3 clip=3 light=3 submit=3 restore=1 delete=2 release=2 heap=2 recreate=1 archive=mounted pass`.
- `scripts/prepare_route_b.sh --check` replays patches 0001-0126 cleanly.

## Natural Stopping Point

BG's actor-owned lifecycle is now repeatable and source-native through its
profile/framework methods. The next blocker is
`ROUTE_B_ROOM_SCENE_DELETE_UNLOAD`: qualify the actual Room44 archive owner
through exact scene deletion without opening phase 4 or additional profiles.
