# BlueWake goal loop - v55 (2026-09-22): the route-B play-scene composition

**User-started; nothing is blocked by the user.** The loop chooses its own next
workstream here, as the user asked it to, and the choice follows from the
measurement the previous loop finished rather than from preference. The governing
specification is [PRD.md](PRD.md), the operating procedure is
[GOAL_LOOP.md](../GOAL_LOOP.md), the decision that this loop executes is
[status/ROUTE_DECISION_2026-09-22.md](status/ROUTE_DECISION_2026-09-22.md), and the
previous workstream page is [GOAL_PROMPT_V54_2026-09-22.md](GOAL_PROMPT_V54_2026-09-22.md).

## Why this is the next workstream, in one number

The target does not move: **60 retraces per second, 16.667 ms per retrace**, on the
certified route, headless and rendered. At the host's sustained ~14 G instructions a
second that is about **233 M instructions per play retrace**. The shipping route
measures **395.9 M** headless and **487.1 M** rendered, and the dossier's section 2
priced its remaining trims: the emitted bodies are 73.5 percent of the thread at
27.24 host instructions per guest instruction, the cycle accounting is 16 percent of
the body and cannot be removed without a second body (measured at +2.4 percent), and
every other A-side candidate is a few tenths of a percent. Its structural floor is
20-22 of those 27.24, against the ~13 authentic speed needs.

Route B is the only route whose cost per guest operation is not set by the
translator: the same hot function measured **0.91 host instructions per guest
instruction** against route A's 5.8 to 27.2. The barrier was never its speed. It is
the port's coverage, and section 8 recorded that as a claim. v55 is the loop that
turns the claim into a list and works down it.

## Where this loop enters

The LOGO-to-opening handoff on route B is now measured rather than assumed
(docs/archive/status/CURRENT.md, 2026-09-22). The scheduler diagnostic runs 600 original
scheduler frames, the LOGO's DVD-wait guard is fully open - queue drained, no object
resource outstanding, no reset - so `dComIfG_changeOpeningScene` really runs and
requests `fpcNm_OPENING_SCENE_e` with `fpcNm_OVERLAP0_e`. It cannot be answered
because the composition's static rel registry holds the LOGO scene alone. Registering
the opening scene moves the failure to the link, and the unresolved set is the play
scene's owner list.

One piece of that list is already landed: **`cDylPhs`**, the REL link phase
`d_s_play`'s `phase_6` and delete run, is answered from the static rel registry in
`route_b/src/native_dyl_phase.cpp`, with a test. It reports `cPhs_ERROR_e` for a
process name the composition does not carry rather than the console's `cPhs_INIT_e`,
which is the silent wait that parked the LOGO.

## The queue, ordered by what the link says is missing

1. **DONE - the width tranche, patches 0269 through 0272.** `JGadget/search.h`,
   `JGadget/vector.h` (twice: `size()` and then the `TVector<void*>` instantiation),
   `JStudio/functionvalue.h`, `JStudio/functionvalue.cpp`, `JStudio/jstudio-object.cpp`
   and `JStudio_JStage/object-actor.cpp`. The undefined set lost the `JStudio::TObject_*`
   and `TFunctionValue_*` families, `cDylPhs`, `TVector_pointer_void`, `dSnap_*`,
   `fopDwIt_*` and `JAIAnimeSound`'s vtable. The corrections the tranche earned:
   `d_stage.cpp` is not a width gap but a tier-partitioned unit, and `dStage_Create`
   exists as the stage-create *census* scaffold rather than as a play-scene owner.
2. **DONE - the stage runtime owners, patch 0273.** A new branch of `d_stage.cpp`'s outer
   chain carries the measured 25 functions no other tier provides, porting `dStage_ppntInfoInit`
   and `dStage_pathInfoInit` by analogy with their room-scoped siblings and taking
   `dStage_decodeSearchIkada`/`dStage_playerInitIkada` from `d_stage_player_ikada.inc`. With it
   in the composition, `dStage_Create`, `dStage_Delete` and `checkDrawArea` all leave the
   undefined set, and the tier shares zero non-weak symbols with the other 22.
3. **DONE - the message and save chains, patches 0275 and 0276.** `JGadget/linklist.h`'s
   unqualified call to a dependent base's `Erase` was the whole message chain: three units,
   and both `JMessage::TControl::setMessageCode_flush_()` and
   `JMessage::TResourceContainer::Get_groupID` resolve. `dComIfGs_initZone` is inline, so the
   room-control tier's declaration referred to a wrapper nothing defines; the include is now
   behind `BLUEWAKE_ROUTE_B_NATIVE_ROOM_CONTROL`, the port's native-header idiom, and with
   `d/d_save.cpp` - which compiles clean - in the composition the save side closes as one
   object.
4. **DONE - the console macro layer and the tribox actor.** `native_prelude.hpp` carries
   `DEG_TO_RAD`/`RAD_TO_DEG` with the decomp's own definitions (reaching the decomp's MSL
   `math.h` instead fails seven ways because it replaces the host's `<cmath>`), and
   `d/d_a_obj_tribox_static.cpp` joins `d/actor/d_a_obj_tribox.cpp`, which is where
   `daObjTribox::Act_c::reset()` lives.
5. **The two-item wall, both host capability rather than port work.** (a) `GXPeekARGB` and
   `GXPokeAlphaRead`, reached from `dSnap_packet::Judge()`: the host GX layer implements exactly
   one member of the CPU-to-EFB family (`GXPeekZ`), and the console implementation would not
   transfer (`*z = *(u32*)addr` against a raw MMIO address). Needs a decision, not a stub.
   (b) **DONE - the packet owners, patches 0277 and 0278.**
   `scripts/prepare_route_b_packet_assets.py` generates their 61 actor-local headers from the
   decomp's own converters, the material display list's two 32-bit assumptions are explicit
   (measured inert: Aurora stores the image3 register and never reads it back), and `d_wood`'s
   `cLib_checkBit` literals match the width the template deduces. All five owners compile clean.
5. **DONE - the colour readback, Aurora patch 0010, and the composition links.** `GXPeekARGB`
   answers from the most recent colour snapshot and requests the next, with the word layout derived
   from the guest's own `GXSetDstAlpha(col * 4)` plus `sp8 >> 26`; the module is a
   texture-to-buffer copy rather than a depth-peek clone, for the measured reason that depth needs a
   compute pass and colour does not. Aurora `gx_fifo_tests` is 207/207 with four new cases, and the
   route-B LOGO-to-opening composition now links with an empty undefined set.
6. **DONE - the composition links uniquely and runs.** The two room tiers' five duplicates were
   removed rather than tolerated: `init()` measured byte-identical in both copies, and then the
   runtime-owners tier absorbed the four symbols the pair existed for so the composition has one
   object per region, zero undefined and zero duplicate symbols. The composed binary now runs -
   `mDoGph_Create`, Metal, the scheduler's first frames, `mDoAud_Create` - and stops at
   `d_s_logo.cpp`'s toon-image setup, where `System.arc` reports `res nothing` for the toon texture
   and the ported assert fires.
7. **DONE - the toon-image failure was this campaign's own regression.** Neither hypothesis held
   (the same `full-logo.o` and the same resource control are in both compositions); a *control* did:
   the base cold-start composition, which has none of this campaign's additions, failed at the same
   assertion. Bisect named patch 0273, whose six new members in `dStage_stageDt_c` had moved the
   game-info layout out from under the port's prebuilt objects. The storage now lives in the tier's
   own file-scope table, the header is untouched, and both probes run - the composed one to its fence
   with LOGO actions advancing.
8. **DONE - the LOGO deletes, the opening scene creates, and the frontier is the play scene's
   draw.** The refused 408-byte allocation was the particle common heap, and the heap - not a
   function - was the blocker: profiled at the LOGO's delete it needs 1,627,296 bytes (425,296 of
   resident `common.jpc`, 71,280 of `JPAResourceManager`, 852,184 of `JPAEmitterManager(3000,150,200)`
   and 278,528 of 128-model pool) against the console budget's 1,501,184, so the host's wider pool
   entries are 126,112 bytes past a number the console fits. Patch 0279 makes the constructor ask for
   `bluewake::route_b::kParticleCommonHeapSize` (0x1c0000) on the host and keeps 0x16e800 on the
   console, with the measurement in `route_b/include/bluewake/route_b/particle_common_heap.hpp` and
   both numbers pinned by `route_b_particle_construction_census`; the private Always-model probe
   reports the heap it builds (`heap=1201984 used=1201984 free=0`) so the next widening aborts there.
   The composed probe now goes from `frame=247 scenes opening=0 overlap=1 logo=1` to `frame=287
   scenes opening=1 overlap=1 logo=0`, printing `[JAIZelBasic::load1stDynamicWave]` and
   `Start StageName:RoomNo [sea_T:44]`.
9. **DONE - the play scene's create completes, and the blocker was a console-legal null read.**
   `dScnPly_Draw` -> `dAttention_c::Draw` -> `dComIfGd_getViewRotMtx` read a null draw list view; on
   the console that is a read of physical address 0 (the view is cleared by the scene's own
   `phase_4` and only a camera sets it back, during the camera's draw), so the garbage matrix is
   discarded and harmless, while the host has no page there. Patch 0280 gives that one read the
   identity when there is no view. With it the probe runs `frame=247` overlap -> `frame=288` opening
   scene -> `frame=330` overlap retired -> its 600-frame fence, with no pending DVD and no reset, and
   the scene's create reaching `phase_4`'s `cPhs_COMPLEATE_e` after `phase_2` resolved the stage
   resource and called `dStage_infoCreate`. The probe also compiles `d_particle.cpp`,
   `d_attention.cpp` and `d_s_play.cpp` from the tree now instead of from the staging archive.
10. **DONE - the missing camera is the composition's own module table refusing it.** The stage file's
   node list carries `RCAM`, instrumented prints show `dStage_cameraInit` running (num=1) and
   `dStage_cameraCreate` running (idx=0), and `fopCamM_Create(0, fpcNm_CAMERA_e, ...)` returns process
   id 8 - but no camera process ever appears in the tree and `play.getCamera(0)` stays NULL. A create
   request loads its procedure before it allocates anything, and `cDylPhs::Link` answers
   `cPhs_ERROR_e` for a name the registry does not carry: measured straight from the probe as
   `camera=5 logo=4 opening=4 overlap=4`. The error deletes the request silently, so the draw list
   never gets a view.
11. **DONE - the camera's runtime owners are composed and the camera exists.** With the registry
   carrying `fpcNm_CAMERA_e` and `f_op_camera.cpp`/`f_op_view.cpp` in the composition - the units
   that define `g_fopCam_Method` and `g_fopVw_Method` - the create request no longer dies in its load
   phase: the camera's `init_phase1` runs and `play.getCamera(0)` is non-null from frame 300 on. The
   first build of that composition was refused by ASAN for an ODR violation on `cMl::Heap` (the
   composition's host-side `process_adjacent_services.cpp` and the dependency's `c_malloc.cpp`), so
   the composition's owner had to become the single owner first - two copies of the heap is two
   allocators.
12. **The player is two gaps rather than one.** The camera's create is parked in `init_phase2` because
   `get_player_actor(...)` is null, and the player is missing in two measured ways. First, the
   registry refuses it: `BW-REGISTRY camera=4 player=5` - the player profile gets the same
   `cPhs_ERROR_e` the camera got before it was added, so a player create request would die in its
   load phase. Second, the room's dispatch is not in the composition at all:
   `dStage_dt_c_roomLoader` and `dStage_playerInit` are absent from the linked binary while
   `dStage_actorInit` and `dStage_roomReadInit` are present, so a room file's `PLYR` node cannot be
   parsed. The camera that does exist came from the *stage* file's `RCAM` node through
   `dStage_cameraInit`, a different table that is composed.
13. **The room path runs; the gap is the room's player node and its profile.** Instrumented prints
   show both room handlers running as soon as the opening scene's stage is created:
   `BW-ROOMNODE actorInit num=2` and `BW-ROOMNODE roomReadInit num=50`, so the composition's room
   parsing works and its actor path reaches `fopAcM_Create`. What the running table does not carry is
   `PLYR`: `dStage_playerInit` and `dStage_dt_c_roomLoader` are absent symbols, and the module table
   still refuses `fpcNm_PLAYER_e` (`BW-REGISTRY camera=4 player=5`), so a dispatched player node could
   not become a process either.
14. **The gap is located in the table and in the tiering.** The compiled copy of
   `dStage_dt_c_stageLoader` (the SCOB_INFO region, `d_stage.cpp` 772) lists `MULT`, `RCAM`, `ACTR`,
   `RTBL`, `RARO`, `Pale`, `Colo`, `Virt`, `SCLS`, `RPPN`, `RPAT`, `SCOB`, `EVNT`, `EnvR` - and no
   `PLYR` - which is why `actorInit` and `roomReadInit` run while no player node is dispatched. And
   `dStage_playerInit`'s only definition (`d_stage.cpp` 1898) sits in a region no composed tier
   compiles: the symbol is absent from the linked binary even though three tables in the file
   reference it (410, 2652, 2677), and those tables are in uncomposed regions too, so the references
   never reach the linker.
15. **The player's integration surface, named by the linker.** Both halves compile: the composed
   loader table takes the `PLYR` entry (it needed only a forward declaration in its region), and the
   port's `ROOM_PLAYER_REQUEST` tier compiles in the composition, which is where the native
   `dStage_playerInit` lives (`d_stage_player_request.inc`). The link then named nine symbols the
   composition does not have: `dComIfGp_setShipId`, `dComIfGp_getStartStage`, `dComIfGp_setShipRoomId`
   (game info), `dComIfGs_getTurnRestartPos`, `dComIfGs_getTurnRestartParam`,
   `dComIfGs_getTurnRestartAngleY` (save), and the port's seams `bluewake_route_b_player_exists`,
   `bluewake_route_b_player_init_ikada`, `bluewake_route_b_stage_proc_name`.
16. **The player tier links, and the player's node is in a file the composition does not read.** With
   the port's `ROOM_PLAYER_REQUEST` tier compiled in its room-reloader configuration - which takes the
   game-info and save accessors inline rather than declaring them - and the loader table carrying
   `PLYR`, the link resolves after one real duplicate was removed: both that tier and the
   runtime-owners tier included `d_stage_player_ikada.inc`, and two definitions of
   `dStage_playerInitIkada` is a link error rather than a warning. Patch 0281 puts that include behind
   `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_COMPOSED`, a no-op for every existing composition and the
   enabler for one that composes both tiers. The run still has no player, and the node inventory says
   why: the file parsed carries `STAG`, `RTBL`, `EVNT`, `MULT`, `EnvR`, `Colo`, `Pale`, `Virt`, `RPAT`,
   `RPPN`, `ACTR`, `RCAM`, `RARO` and no `PLYR`. That is the stage file - which is why `actorInit` and
   `roomReadInit` run (`num=2`, `num=50`) - and the player's node is in the room's own file.
17. **The missing owner is the room scene.** The registry refuses it as it refused the camera -
   `BW-REGISTRY camera=4 player=5 roomscene=5 logo=4 opening=4 overlap=4` - and `g_profile_ROOM_SCENE`
   (`d_s_room.cpp`) sits in the dependency archive unreferenced, so it is never pulled. That unit is
   what reads the room's own file and dispatches its nodes, `PLYR` among them, and the play scene
   creates it through `fopScnM_CreateReq(fpcNm_ROOM_SCENE_e, ...)` (`d_stage_runtime.inc`,
   `d_stage.cpp`). The stage file the composition does read has no `PLYR` node at all.
18. **The room scene composes down to one symbol.** Composing it needed the `ROOM_NATIVE_VIEWS` tier -
   its loader table references `dStage_mapInfoInit`, `dStage_filiInfoInit`, `dStage_lbnkInfoInit` and
   `dStage_soundInfoInit`, all four defined by that one object - and the room-reloader composition flag
   on the ship-state tier, which declares the game-info and save accessors out of line in its own
   configuration (that took `dComIfGp_getShipId`, `dComIfGp_getShipRoomId`,
   `dComIfGs_getTurnRestartHasShip`, `dComIfGs_getTurnRestartShipPos`,
   `dComIfGs_setTurnRestartHasShip`, `dComIfGs_getTurnRestartShipAngleY` and
   `bluewake_route_b_find_ship` to none). What remains is `dStage_dt_c_roomReLoader`, referenced by
   `objectSetCheck(room_of_scene_class*)` in `d_s_room.o`: its only definition is at `d_stage.cpp` 53,
   inside the unit's runtime region, which no composed tier compiles - and neither
   `d_stage_runtime.inc` nor `d_stage_runtime_owners.inc` carries it.
19. **The reloader's tier is named and composes; two symbols are left.** `d_stage.cpp` 18 opens
   `#elif TARGET_PC && defined(BLUEWAKE_ROUTE_B_ROOM_RELOADER_TIER)`, and that region defines
   `bluewakeRoomLayerLoader`, `bluewakeRoomTreasureInit` and `dStage_dt_c_roomReLoader`. Naming the tier
   in the composition resolved `dStage_dt_c_roomReLoader` and exposed the two the tier itself needs:
   `dStage_roomDrtgInfoInit`, referenced from its table and declared at line 26, defined at 200 inside
   that same region, yet not defined by the compiled object - so a further guard excludes it - and
   `bluewake_route_b_room_layer_no`, the composition's seam for the room layer suffix, whose reference
   implementations are in `tests/route_b_room_aggregate_test.cpp`, `tests/route_b_room_lifecycle_test.cpp`
   and `tests/route_b_private_room_lifecycle_probe.cpp`.
20. **The last handler belongs to a tier that duplicates two others.** The region boundaries are not
   where the line numbers suggest: `d_stage.cpp` 19 opens the room-reloader tier, 68 guards
   `dStage_roomDt_c::init` behind `!ROOM_RELOADER_COMPOSITION`, and 109 opens `ROOM_SCALED_REQUEST` -
   the region that defines `dStage_roomDrtgInfoInit` (200) and its static helper
   `bluewakeRoomScaledInit`. Measured by `nm`, that tier's object is the handler's only owner, but
   composing the tier whole puts `dStage_tgscInfoInit` and `dStage_rpatInfoInit` in two objects at once
   (SCOB_INFO and runtime-owners already carry them) and the link reports both as duplicate symbols.
21. **The next step.** Share the handler and its helper the way patch 0281 shared the ikada helper: a
   shared include with the scaled-request region's copy behind a composition flag, plus a supplied
   `bluewake_route_b_room_layer_no` from the implementation the port's tests already carry. Then the
   room scene links, the room's own file is read, `PLYR` dispatches, and the camera's `init_phase2` -
   waiting on exactly that player - can finish and set the draw list's view. lldb breakpoints on
   these symbols proved unreliable in this composition (a breakpoint on `dStage_infoCreate` never
   fired while a print in `phase_2` showed the call), so the prints compiled into the object under
   test are the evidence.
3. **The remaining owners, all of which exist and need composing** - `dSnap_Create`,
   `dSnap_Execute`, `dSnap_Delete`, `dSnap_DebugDraw` (`d/d_snap.cpp`),
   `fopDwIt_Begin`/`fopDwIt_Next` (`f_op/f_op_draw_iter.cpp`), the grass/tree/wood/
   flower/magma packet owners, `daObjTribox::Act_c::reset` (`d/actor/d_a_obj_tribox.cpp`),
   `JStudio::data::ga8cSignature` (`jstudio-data.cpp`), the `JStudio_JStage/_JAudio/`
   `_JMessage/_JParticle` `TAdaptor_*` constructors and `JGadget::TVector_pointer_void`
   (`JGadget/std-vector.cpp`).
4. **Then the handoff itself**: the opening scene creates, the LOGO deletes, and the
   run reaches the next fence. Only at that point does route B have a scene the
   campaign can measure, and the play scene after it becomes the next target.

## The protocol, carried over

- **Quote instructions, not milliseconds.** Route A's counts stay digest-gated by
  `scripts/bench_instructions.sh`; route B's new work is gated by its own tests and
  by the scheduler diagnostic's stop point and frame inventory.
- **A count is not a price**, and a link failure is not a verdict: symbolize or read
  the undefined set before naming a cause.
- **A generated artifact must prove itself**, and **a route driver is a measurement,
  not the product** - the private probe measures, the committed port carries.
- **An iteration counts** when a measured number moved on a named path with the gate
  green, or when a new instrument changed a decision, or when a candidate was
  refuted with a number. Report to the user every three iterations.

## Fences

Inherited and not negotiable: no Nintendo data, original binaries, generated game
code, saves, captures, signing material or leaked source in the repository; one
BlueWake process at a time; no timing measurement during a build; `ref/` is a pinned
dependency whose uncommitted state *is* the patch series, so reverse an edit or
restore from `patches/` and never `git checkout` a file there; register every `ref/`
patch in `config/dependencies.lock.json`; do not weaken a clause to make a log pass;
do not re-run the refuted route-A candidates.

Kept from v54: **a crash is an attribution, not a verdict**, and **the target does
not move**. Route A remains the shipping route while route B accumulates; nothing on
this page changes that, and no route B work may regress the certified route.
