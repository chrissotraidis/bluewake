# BlueWake Autonomous Goal-Based Implementation Loop

## Current operating loop — migration acceptance, reoriented October 3, 2026

Make BlueWake the maintained home for the approved consolidation with Elliott.
The [reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md) owns the
feature inventory, platform matrix and evidence. Source is substantially
consolidated; replacement of the donor project is not yet accepted. Finish the
player paths and gameplay qualification before recommending cutover. Historical
v56/v55/Route B campaigns below do not control this goal.

### Preserve the completed work

- Cumulative draft [PR #37](https://github.com/chrissotraidis/bluewake/pull/37)
  contains the integration ancestry, including 26 Elliott-authored commits and
  13 Elliott co-author trailers in BlueWake. Runtime adaptations carry their own
  attribution. Continue here; do not create another import stack.
- Current maintained runtime is `0568fedd`; its product code matches
  `2218107d` (the follow-up only fixes a Windows test output path). Translator
  stays `b8b5345`. Windows host linking and all 58 source regressions pass.
- The frozen clean Mac player build is BlueWake `3392854` / runtime `18ba3b64`.
  Its owned-disc translation, local training, O2 compilation, signed relocatable
  package, bounded restart/resume, game save/separate reload, settings, states,
  local upgrade and bounded progression remain accepted within their scope.
- The personal Pictobox candidate overlays host `9706637` / runtime `2218107d`
  on that module. Regular/Deluxe photos, repeat/cancel, actual card saves,
  separate album reload and legacy-state capture pass on Mac. Queued camera
  pitch and water/wall collision also pass. This overlay is not a new clean build.

Do not rerun these accepted checks without a relevant source change, failure or
identified coverage gap. Documentation-only changes do not invalidate binaries.

### Critical path and exit evidence

| Priority / work package | Next action and completion evidence | Current dependency |
| --- | --- | --- |
| 1. Source consolidation and candidate identity | Keep one cumulative review, verify source/dependency/patch agreement and authorship, and remove stale current-status claims. Plan runtime integration into `bluewake-next` and BlueWake integration through #37 into `main`. Record the exact final package source, host, module and compatibility; distinguish clean-build provenance from a diagnostic host overlay. | Current PRs are mergeable drafts. Source review and documentation can proceed independently of hardware; merging source is not migration acceptance. |
| 2. Complete the player build paths | Resume the retained `3392854` PadMint workspace without repinning; verify reused objects/profiles, final assembly and provenance. Then qualify the maintained candidate's reproducible app/update path and determine whether changed module inputs require a rebuild. Finish signing, device run/save/reload and in-place data-preserving upgrade. Do not count the older workspace as a clean build of the newer candidate. | Two previous attempts stopped for low disk. Retain 593 objects. Resume only with stable 30 GiB free or a verified suitable external target. Physical-device coordination remains pending. |
| 3. Finish local Mac gameplay coverage | Use isolated copied saves and the existing identified app for the remaining option/climbing checks. Establish the relevant gameplay action before an off/on comparison. Complete real mouse/controller input, audible intro/scripted music, and a representative 30-minute gameplay route with actual progression, settings, save/reload and scene transitions. | Small functional checks can proceed. Real controller/audio acceptance needs the relevant input/output observation. Sustained performance needs an uncontended host. |
| 4. Native Windows player acceptance | On confirmed x64 hardware, build the owned-disc O2 module and run native Direct3D. Cover disc import/recovery, fullscreen/restart, settings, controls/haptics, saves/states/upgrade, Pictobox, startup and scripted-music reports. Record app/module/source identities and distinguish reproduction from a claimed fix. | Hardware/controller availability unconfirmed. CI is green but cannot close these checks. Do not restart Parallels or take a shared device. |
| 5. Performance and migration decision | Use matched original-30-Hz configurations and scenes, compare correctness plus frame-time tails/stalls, and complete sustained play on each claimed target. Review the single platform matrix, tested instructions, issue dispositions and proposed donor notice against those results. | Quiet hardware and completed player candidates required. Historical 22.8% Outset gain is bounded, not final performance parity. No redirect, donor closure or public release. |

Select the highest-priority **available** missing result. Unavailable hardware
or storage is a dependency, not a reason to repeat completed tests or add
optimization families. Do not fetch new donor features into this fixed campaign
unless they address an established migration blocker; track later changes apart.

### Iteration contract

1. Read live repository status and the ledger's current summary. Name one
   missing acceptance result and the observation that would close it.
2. Check prerequisites before launching. For gameplay, first prove the action
   occurs (for example, an actual dialogue for instant text). A clean exit,
   enabled-option log, idle screenshots or a compiled fixture does not suffice.
3. Run the smallest discriminating check. If automation misses the action,
   record a setup failure and correct the route before spending on an A/B pair.
   After three materially identical failures, change the experiment. Do not
   extend small input probes indefinitely while a larger gate is actionable.
4. Change source only for an established defect. Run relevant regressions and
   affected gameplay checks; preserve failed evidence and original player data.
5. Record exact identities, result, limits and the next gate. Checkpoint validated
   source-only progress in #37 and update the private continuation. Summarize
   closed gates and external dependencies rather than counting probes or PRs.

The October 3 instant-text pair `instant-text-ae5ceqfa` completed but never
opened dialogue: both cases show idle Link and have identical captured frames.
The enabled case patches message data, which is implementation evidence only.
Its gameplay result is **inconclusive due to setup**, not a pass or game defect.
Do not repeat that unchanged route.

Storage is volatile: this reorientation observed 28 GiB free, up from the prior
sub-2-GiB state, still below the resume threshold. Two unrelated simulator test
processes each consume approximately 200% CPU. Recheck prerequisites when a
relevant work package becomes actionable; do not start a large build or present
this as a quiet performance window. Do not terminate other tasks to create one.

### Retained state and completion boundary

Primary checkout: `codex/fork-consolidated`; nested runtime:
`codex/bluewake-pe-token`. The `bluewake-cpu-contract` worktree stays clean at
`3392854`, runtime `18ba3b64`, for exact PadMint resume. Keep the other evidence
worktrees, failed baseline, personal modules, profiles, captures, saves and
signing material. Do not clean up unique artifacts to make a build fit.

The goal stays active until required parity, player-build and gameplay checks
have current evidence. Original 30 Hz simulation, Smooth Motion Off and
experimental 60 Hz Off remain defaults; preserve explicit preferences.
Public releases additionally require the private Clear audit and artifact gate.
Publishing personal builds or donor redirects/closures is outside this goal.

---

**Historical — v56, 2026-09-23. The iPadOS loop.** The user redirected the project:
make the game actually work on iPadOS, tested in the iOS simulators one at a
time, with no hardware iPad yet. Route A is hosted on iPadOS by `apple/ios` and
already reaches controllable Outset gameplay in the simulator; the loop now
works down what stops a person from playing on an iPad. The operating document
is [GOAL_PROMPT_V56_2026-09-23.md](GOAL_PROMPT_V56_2026-09-23.md) and the
decision record is
[status/IPADOS_REORIENTATION_2026-09-23.md](status/IPADOS_REORIENTATION_2026-09-23.md).
The v55 Route B campaign below continues as a background track.

**Superseded by v56 — v50, 2026-09-21. Finish the project.** The product half of macOS is
met: the signed app boots the retail disc through the original flow into
controllable Outset gameplay with real keys, real video, real audio, the game's
own save and a normal stop, and Route A is the shipping route. What is left is
everything after "it runs": authentic play-scene speed (the only large item),
the renderer's cost and its p99 tail, graphics correctness, physical
controller, the compatibility campaign from new game to ending, a composite
that regenerates from the tree, and then iPhone and iPad. The full operating
document is [GOAL_PROMPT_V50_2026-09-21.md](GOAL_PROMPT_V50_2026-09-21.md),
and its first act is to resolve the two different instruction counts the ledger
carries for the same play window before any further performance work is
planned.

**September 9 stopping point (historical):** TWW 0267–0268 repaired null heap
exchange and native DVD/audio command handoff. The private original
scheduler/Painter experiment ran 600 frame cycles with original audio startup and
reached the opening request; transition/profile and archive handoff were left
unfinished, and its deliberate end fence was not boot success. See the
[session handoff](status/SESSION_HANDOFF_2026-09-09.md) for that evidence.

**Original cold-start integration / TWW 0265–0266:** a strict headless
experiment now completes original `mDoGph_Create` and the complete LOGO profile's
Create method, drains its queued DVD preloads, and exits zero. Full resource
ownership replaces the path-rejecting diagnostic loader. Native JKR heap context
is now per-thread, and host OS registries survive game-global destruction; both
bugs have failing-before/passing-after regressions. All 24 regression runs pass,
including the existing world replay and unchanged paced cursor PCM fingerprint.
Source replay and private-data audits pass. Earlier failed-link claims of “no
duplicate definitions” were premature: executable closure exposed conflicting
diagnostic owners, removed from this experiment. Normal LOGO drawing/deletion,
archive handoff, opening transition and METER admission remain next; audio startup,
reset recovery and legacy MAT2/BLS formats are not qualified here. Solid-heap
individual-free/size warnings remain recorded. No product gate advances; full PRD
goal active. Recovery: `local-research/checkpoints/reorientation-20260909-logo-cold-start/`.
See [world integration evidence](status/ROUTE_B_WORLD_INTEGRATION_2026-09-09.md).

**Governing requirements:** [PRD.md](PRD.md)

**Purpose:** operating procedure for a long-running implementation agent

**Prepared:** 2026-08-21

**Terminal condition:** every PRD definition-of-done item passes with current
evidence

## Current continuation loop — 2026-09-22 (v55: the route-B play-scene composition)

**USER-STARTED / NOTHING IS BLOCKED BY THE USER, and the loop assigns its own next
workstream here.** The full definition is
[GOAL_PROMPT_V55_2026-09-22.md](GOAL_PROMPT_V55_2026-09-22.md). The target does not
move: 60 retraces per second, 16.667 ms per retrace, on the certified route.

**Why route B, in one number.** At ~14 G instructions a second the target is about 233 M
instructions per retrace; the shipping route measures 395.9 M headless and 487.1 M
rendered, its structural floor is 20-22 of the 27.24 host instructions it spends per
guest instruction, and authentic speed needs about 13. Route B's same hot function
measured **0.91**. Its barrier was never speed - it is the port's coverage, which the
dossier recorded as a claim and this loop turns into a list.

**Where the loop enters.** The LOGO-to-opening handoff is measured rather than assumed
([status/CURRENT.md](status/CURRENT.md), 2026-09-22): the scheduler diagnostic runs 600
original scheduler frames, the LOGO's DVD-wait guard is fully open (queue drained, no
object resource outstanding, no reset), so `dComIfG_changeOpeningScene` really runs and
requests `fpcNm_OPENING_SCENE_e` with `fpcNm_OVERLAP0_e`. It cannot be answered because
the composition's static rel registry holds the LOGO scene alone; registering the
opening scene moves the failure to the link, and the unresolved set is the play scene's
owner list. One entry of that list is already landed: **`cDylPhs`**, the REL link phase,
answered from the static rel registry in `route_b/src/native_dyl_phase.cpp` with a test,
reporting `cPhs_ERROR_e` for a name the composition does not carry instead of waiting
forever on a REL that is not coming - the silent wait that parked the LOGO.

**The queue, ordered by what the link says is missing.** (1) **DONE** - the width tranche,
patches 0269-0272, which took the `TObject_*`, `TFunctionValue_*`, `cDylPhs`,
`TVector_pointer_void`, `dSnap_*`, `fopDwIt_*` and `JAIAnimeSound` symbols out of the undefined
set. (2) **DONE** - the stage runtime owners, patch 0273: the measured 25 functions no other tier
provides, including `dStage_Create`, `dStage_Delete` and `checkDrawArea`, which all leave the
undefined set. (3) **DONE** - the message and save chains, patches 0275 and 0276: the JMessage
system was three units and one unqualified call to a dependent base, and the save side closed as one
object because `dComIfGs_initZone` is inline and `d/d_save.cpp` compiles clean. (4) **DONE** - the
console macro layer (`DEG_TO_RAD`/`RAD_TO_DEG` in the prelude) and `d/d_a_obj_tribox_static.cpp`,
which is where `daObjTribox::Act_c::reset()` lives. (5) The two-item wall, both host capability
no symbols at all. (a) **DONE** - the packet owners (patches 0277, 0278), `GXPokeAlphaRead` (Aurora
0009), and the colour readback (Aurora 0010): `GXPeekARGB` answers from the most recent colour
snapshot, its layout is derived from the guest's own `GXSetDstAlpha(col * 4)` and `sp8 >> 26`, and
Aurora's `gx_fifo_tests` is 207/207 with four new cases. **The route-B LOGO-to-opening composition
links end to end, with zero undefined and zero duplicates** - the duplicates the `ar`-by-basename
staging archive had been hiding, and then the five between the two room tiers, removed by absorbing
the four symbols that pair existed for (patches 0273 regenerated). (6) **The composed binary runs:**
heaps, `mDoGph_Create`, Metal, the scheduler's first frames, `mDoAud_Create`, and then the LOGO's
toon-image setup. (7) **DONE** - that failure was this campaign's own regression, found by a control
(the base cold-start composition fails the same way), bisected to patch 0273, and explained: its six
new members in `dStage_stageDt_c` moved the game-info layout out from under the port's prebuilt
objects. The storage now lives in the tier's own file-scope table, the header is untouched, and
**the composed probe runs the original scheduler to its fence with the full composition linked.**
(8) **DONE** - the failing 408-byte allocation was the particle common heap, and the heap turned out
to be the first console budget the host outgrows: profiled at the LOGO's delete it needs 1,627,296
bytes against the console's 1,501,184, and patch 0279 sizes it for the host (0x1c0000) with the
measurement recorded in `route_b/include/bluewake/route_b/particle_common_heap.hpp`. **The transition
happens:** the composed probe runs from `logo=1 overlap=1` at frame 247 to `opening=1 overlap=1
logo=0` at frame 287, printing `[JAIZelBasic::load1stDynamicWave]` and `Start StageName:RoomNo
[sea_T:44]`. (9) **DONE** - the play scene's draw stopped in `dAttention_c::Draw` ->
`dComIfGd_getViewRotMtx` because the draw list's view is null, and that null read is console-legal:
`phase_4` clears the view itself and only a camera sets it back, so on the GameCube the inverse reads
physical address 0 and its garbage is discarded because nothing is drawn in that window. Patch 0280
gives that one read the identity when there is no view. **The play scene's create now completes** -
the probe goes `frame=247` overlap, `frame=288` opening scene, `frame=330` overlap retired, and runs to
its 600-frame fence with no pending DVD and no reset, the scene's phase machine left with a null
handler table at index 6 (`phase_4` returning `cPhs_COMPLEATE_e`) after `phase_2` resolved the stage
resource and called `dStage_infoCreate`. **(10) DONE** - the missing camera is the composition's own
module table refusing it: the stage's `RCAM` node dispatches, `dStage_cameraInit`/`dStage_cameraCreate`
run and `fopCamM_Create(0, fpcNm_CAMERA_e, ...)` returns process id 8, but no camera process ever
appears, because a create request loads its procedure first and `cDylPhs::Link` answers
`cPhs_ERROR_e` for a name the registry does not carry - measured directly as `camera=5 logo=4
opening=4 overlap=4`. The error deletes the request silently, so `play.getCamera(0)` stays NULL and
the draw list never gets a view. **(11) DONE** - the camera's runtime owners are composed and the
camera exists: with the registry carrying it and `f_op_camera.cpp`/`f_op_view.cpp` in the composition,
the create request no longer dies in its load phase and `init_phase1` publishes `play.getCamera(0)`
(non-null from frame 300 on). The composition's host-side cMl had to become the single owner first -
ASAN found `cMl::Heap` in both `process_adjacent_services.cpp` and the dependency's `c_malloc.cpp`, an
ODR violation and two allocators. **(12) The next step:** the camera's create is parked in
`init_phase2`, which returns `cPhs_INIT_e` while `get_player_actor(...)` is null, and it is null -
`dComIfGp_getPlayer(0)` reads null at frames 300 through 540 - so the camera process is not yet in a
layer, its draw never runs and the draw list's view stays unset. The player actor
(12) **DONE, and the player is two gaps rather than one.** The registry reports `camera=4 player=5`:
the player profile is refused exactly as the camera's was, so a player create request would die in its
load phase. And the room's dispatch is absent from the composition altogether - `dStage_dt_c_roomLoader`
and `dStage_playerInit` are not in the linked binary while `dStage_actorInit` and `dStage_roomReadInit`
are - so the room file's `PLYR` node cannot be parsed at all. The camera that exists came from the
*stage* file's `RCAM` node through `dStage_cameraInit`, a different table that is composed. **(13) The
next step:** compose the room loader and the room-level handlers it dispatches to, and register the
player profile with its owners, following the shape the port already has in
`tests/route_b_player_init_services.cpp`. **(13) DONE, and the gap is narrower than it looked.** The
room path is alive: instrumented prints show `BW-ROOMNODE actorInit num=2` and
`BW-ROOMNODE roomReadInit num=50` as soon as the opening scene's stage is created, so rooms are parsed
and the actor path reaches `fopAcM_Create`. What the running room table does not carry is `PLYR` -
`dStage_playerInit` and `dStage_dt_c_roomLoader` are still absent symbols - and the module table still
refuses `fpcNm_PLAYER_e` (`camera=4 player=5`), so a player node that was dispatched could not become a
process either. **(14) The next step:** dispatch `PLYR` in the composition's room loader and register
the player profile with its owners, following `tests/route_b_player_init_services.cpp`; the camera's
`init_phase2` is waiting on exactly that player, and the camera's draw is what sets the draw list's
view. **(14) DONE - the gap is located in the table and in the tiering.** The composed copy of
`dStage_dt_c_stageLoader` (the SCOB_INFO region, `d_stage.cpp` 772) lists `MULT`, `RCAM`, `ACTR`, `RTBL`,
`RARO`, `Pale`, `Colo`, `Virt`, `SCLS`, `RPPN`, `RPAT`, `SCOB`, `EVNT`, `EnvR` and no `PLYR` - which is
why `actorInit` and `roomReadInit` run while no player node is dispatched - and `dStage_playerInit`'s
only definition (`d_stage.cpp` 1898) is in a region no composed tier compiles, so the symbol is absent
from the binary even though three tables in the file reference it (410, 2652, 2677); those tables are
in uncomposed regions too, so the references never reach the linker. **(15) The next step:** add the
`PLYR` entry to the composed table and provide `dStage_playerInit` with `dStage_playerInitIkada` in a
composed region (the runtime-owners include patch 0273 introduced is the natural home), and stop the
composition refusing the player profile. **(15) DONE - the surface is named by the linker.** Both
halves compile - the composed loader table takes the `PLYR` entry and the port's `ROOM_PLAYER_REQUEST`
tier (native `dStage_playerInit` in `d_stage_player_request.inc`) compiles in the composition - and the
link then named nine missing owners: `dComIfGp_setShipId`, `dComIfGp_getStartStage`,
`dComIfGp_setShipRoomId`, `dComIfGs_getTurnRestartPos`, `dComIfGs_getTurnRestartParam`,
`dComIfGs_getTurnRestartAngleY`, and the port's seams `bluewake_route_b_player_exists`,
`bluewake_route_b_player_init_ikada`, `bluewake_route_b_stage_proc_name`. The entry was not landed
alone because it references `dStage_playerInit` and would break any composition that compiles the
aggregate tier without the player-request object; the edit was reverted and the series check is green
at 280 patches. **(16) The next step:** land the `PLYR` entry and those nine owners together - the
game-info and save accessors already exist in the tree and the three seams are the port's own hooks, so
it is composition rather than authoring. **(16) DONE - the tier links, and the player node is in a
file the composition does not read.** The player tier composes and links once its room-reloader
configuration takes the game-info and save accessors inline and the ikada helper is included by only
one tier; patch 0281 puts that include behind `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_COMPOSED`, a no-op
for existing compositions and the enabler for one that composes both tiers. But the run still has no
player, and the node inventory says why: the stage file carries `STAG`, `RTBL`, `EVNT`, `MULT`, `EnvR`,
`Colo`, `Pale`, `Virt`, `RPAT`, `RPPN`, `ACTR`, `RCAM`, `RARO` and no `PLYR`, so the player's node is in
the room's own file, which this composition does not read. The `PLYR` entry is necessary and not
sufficient, and it was measured but not landed, since it references `dStage_playerInit` and belongs in
the same change as the room file read that can dispatch it. **(17) The next step:** read the room's own
file (its node dispatch is where `PLYR` lives) and land the `PLYR` entry with it. **(17) DONE - the
missing owner is the room scene.** The registry refuses it like the others - `camera=4 player=5
roomscene=5` - and `g_profile_ROOM_SCENE` (`d_s_room.cpp`) sits in the archive unreferenced, so it is
never pulled. That unit is what reads the room's own file and dispatches its nodes, `PLYR` among them;
the play scene creates it through `fopScnM_CreateReq(fpcNm_ROOM_SCENE_e, ...)` (`d_stage_runtime.inc`,
`d_stage.cpp`). **(18) The next step:** compose `d_s_room.cpp`'s room-scene profile and register
`fpcNm_ROOM_SCENE_e`, land the `PLYR` entry with it, and the camera's `init_phase2` - waiting on exactly
that player - can finish and set the draw list's view. **(18) DONE - the room scene composes down to
one symbol.** Composing it needed the `ROOM_NATIVE_VIEWS` tier (four handlers its loader table
references) and the room-reloader composition flag on the ship-state tier (seven more, since that tier
declares the game-info and save accessors out of line in its own configuration). What is left is
`dStage_dt_c_roomReLoader`, referenced by `objectSetCheck(room_of_scene_class*)` in `d_s_room.o`, whose
only definition sits at `d_stage.cpp` 53 in the unit's runtime region - a region no composed tier
compiles, the same tiering shape that hid `dStage_playerInit`, and neither runtime include carries it.
**(19) The next step:** put `dStage_dt_c_roomReLoader` and the static `bluewakeRoomLayerLoader` beside it
in a composed region (the runtime-owners include is the natural home), then the room scene links, the
room's file is read, `PLYR` dispatches, and the camera's `init_phase2` has the player it waits for.
**(19) DONE - the reloader's tier is named and composes.** `d_stage.cpp` 18 opens
`BLUEWAKE_ROUTE_B_ROOM_RELOADER_TIER`, the region that defines `bluewakeRoomLayerLoader`,
`bluewakeRoomTreasureInit` and `dStage_dt_c_roomReLoader`; naming it resolved the symbol and exposed the
two the tier itself needs - `dStage_roomDrtgInfoInit` (declared at 26, defined at 200 inside that same
region, yet the compiled object does not define it, so a further guard excludes it) and
`bluewake_route_b_room_layer_no`, the port's seam for the room layer suffix, whose reference
implementations are in `tests/route_b_room_aggregate_test.cpp`, `tests/route_b_room_lifecycle_test.cpp`
and `tests/route_b_private_room_lifecycle_probe.cpp`. **(20) The next step:** read the guard that
excludes `dStage_roomDrtgInfoInit` from the compiled region, supply `bluewake_route_b_room_layer_no` from
the shape the tests already carry, and the room scene links. **(20) DONE - the handler belongs to a tier
that duplicates two others.** The regions are not where the line numbers suggest: 19 opens the
room-reloader tier, 68 guards `dStage_roomDt_c::init`, and 109 opens `ROOM_SCALED_REQUEST`, the region
that defines `dStage_roomDrtgInfoInit` (200) with its static helper `bluewakeRoomScaledInit`. Measured
by `nm`, that object is the handler's only owner - but composing the tier whole puts
`dStage_tgscInfoInit` and `dStage_rpatInfoInit` in two objects at once (SCOB_INFO and runtime-owners
already carry them) and the link reports both as duplicates. **(21) The next step:** share the handler
and its helper the way patch 0281 shared the ikada helper - a shared include with the scaled-request
copy behind a composition flag - and supply `bluewake_route_b_room_layer_no` from the implementation the
port's tests already carry. Then the room scene links, the room's file is read, `PLYR` dispatches, and
the camera's `init_phase2` has the player it waits for. **(21) DONE - the room's TGDR handler is
shared.** `dStage_roomDrtgInfoInit` and its static helper `bluewakeRoomScaledInit` moved into
`ref/tww/src/d/d_stage_room_drtg.inc`, which the scaled-request region includes as before and the
runtime-owners tier includes only when the composition asks for it
(`BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_CARRIES_ROOM_DRTG`); patch 0282 lands it and the port's own
`room_scaled_request`/`room_reloader_scaled` targets compile with it. The composition's remaining
duplicates are the room tiers' overlap again - `room_control_init`, `room_native_views` and
`stage_runtime_owners` define `dStage_floorInfoInit`, `dStage_plightInfoInit`, `dStage_rpatInfoInit` and
`dStage_roomDt_c`'s typeinfo twice, the same overlap patch 0273 resolved by dropping `room_native_views`
and absorbing its four symbols. **(22) The next step:** absorb the four handlers `room_native_views` still
exists for (`dStage_mapInfoInit`, `dStage_filiInfoInit`, `dStage_lbnkInfoInit`, `dStage_soundInfoInit`)
the same way, and drop the tier again. **(22) DONE, and one of the two ways is refuted.** Dropping
`room_control_init` instead of `room_native_views` is refused by the link - `dStage_roomControl_c`'s
statics (`mDarkRatio`, `mOldStayNo` and the rest) are undefined without it - so the handlers are what
moves. `dStage_mapInfoInit` is already carried by the runtime-owners tier, leaving
`dStage_filiInfoInit`, `dStage_soundInfoInit` and `dStage_lbnkInfoInit`; extracting those three into
`d_stage_room_handlers.inc` and including it behind `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_CARRIES_ROOM_VIEWS`
was refused by the compiler, because `resizeRoomEntries` is a region-local helper the handlers call - so
the move is a two-function chain, one helper deeper, exactly like the ikada and drtg moves. The
experiment was reverted and the series check is green at 282 patches. **(23) The next step:** extract
`resizeRoomEntries` together with the three handlers, have the native-views region include the result,
and drop `room_native_views` from the composition. **(23) DONE - the overlap is now a per-symbol map.**
`dStage_roomDt_c::init` is defined in the room-reloader region (guarded), the scaled-request region, the
actor-request region, the room-aggregate region and the native-views region (511);
`dStage_rpatInfoInit` in the native-views region (442), the path-graph region (822) and the
runtime-owners region (2180); `dStage_plightInfoInit` and `dStage_floorInfoInit` in the runtime-owners
region (2037, 2206). The composition composes the native-views and runtime-owners tiers at once, which
is why the link reports exactly that set. **(24) The next step:** apply the rule the last three patches
established, one symbol at a time - one owner per symbol with every other definition behind a
composition flag - starting with `dStage_rpatInfoInit` (runtime-owners keeps it; native-views and
path-graph give theirs up) and `dStage_roomDt_c::init` (which also stops a second typeinfo record), while
the four native-views-only handlers (`dStage_mapInfoInit`, `dStage_filiInfoInit`, `dStage_soundInfoInit`,
`dStage_lbnkInfoInit`) stay with their tier or move with `resizeRoomEntries`. **(24) DONE - the rule is
applied and its first two pairs are gone.** `nm` on the composed tier objects showed
`dStage_roomDt_c::init` and the class's typeinfo in both `room_control_init` and `room_native_views`, and
`getMapInfo2`/`getMapInfoBase` in both `room_native_views` and `stage_runtime_owners` - the pair patch
0273 had absorbed. Patch 0283 puts the native-views copies behind
`BLUEWAKE_ROUTE_B_ROOM_CONTROL_INIT_COMPOSED` and `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_COMPOSED`,
undefined in every existing composition, and the rebuild confirms both pairs disappear from the link.
Sixteen duplicates remain, all the same kind: `dStage_floorInfoInit`, `dStage_plightInfoInit`,
`dStage_rpatInfoInit`, `dStage_lgtvInfoInit` and the rest are carried by the runtime-owners tier and
defined again in the room tiers this composition composes. **(25) The next step:** guard those symbols the
same way, using the region-and-line map from the previous iteration. **(25) DONE - the remaining pairs
name one second owner each.** The link reports `dStage_floorInfoInit`, `dStage_plightInfoInit`,
`dStage_rpatInfoInit` and `dStage_lgtvInfoInit` in `d_stage_room_native_views.o` **and**
`d_stage_stage_runtime_owners.o` (with `rpat` also in `d_stage_stage_path_graph.o`). The native-views
region does not define them in its own body: it gets them from `d_stage_room_metadata.inc`, which it
includes, while the runtime-owners tier's copies come from the region that includes its file, at 2037
(`plight`), 2180 (`rpat`), 2206 (`floor`) - a region whose guard is not identified yet. **(26) The next
step:** find that region's guard, and stand its copies down rather than the native-views ones. Nothing
was changed this iteration; the working tree is clean and the series check is green at 283 patches.
**(26) DONE - the second owner is an include, and two more pairs stand down.** The copies at 2037/2053/
2180/2206 are inside a `#if !TARGET_PC` block, so the host link never sees them; the real second owner is
`d_stage_room_metadata.inc`, which both the native-views region (352) and the runtime-owners region
(1230) include. Patch 0284 puts the native-views include behind
`BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_COMPOSED`, undefined in every existing composition, and the
rebuild takes the sixteen duplicates down to twelve, with `floor`, `plight` and `lgtv` no longer
duplicates at all. Guarding the runtime-owners side too is refuted by the link: those three become
undefined, because that tier is the only remaining owner once the native-views copy is gone - so the
partition is asymmetric, the runtime-owners region keeps the include and the native-views region gives
it up. **(27) The next step:** guard the twelve that remain (`rppn`, `rpat` and the rest) inside
`d_stage_runtime_owners.inc`, where the tier defines them in its own body while the metadata include
defines them again. **(27) DONE - the room scene links and the run reaches actor creation.**
`dStage_rppnInfoInit` and `dStage_rpatInfoInit` are defined in the native-views region's own body and
again in the runtime-owners `.inc`, which patch 0273 gave them deliberately; patch 0285 puts those
definitions behind `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_OWNERS_COMPOSED` in each region that carries them, so
a composition that composes the runtime-owners tier takes them from it and one that does not is
unchanged. The link then has no duplicate and no undefined symbol - **the room scene and the player are
in the composition** - and `BW-REGISTRY` reports `roomscene=4`. The run went one level deeper: it reaches
the opening scene's stage read and the actor creation that follows, and stops with a SEGV inside
`dStage_actorInit` (`d_stage.cpp` 1125) under the address sanitizer. **(28) The next step:** measure which
pointer is null in `dStage_actorInit`'s actor-request half - the frontier is now a port defect inside a
composed handler rather than a missing unit. **(28) DONE - the room scene runs, and its reload path is
handed an uninitialized stage object.** The sanitizer's backtrace names the chain: `dScnRoom_Create`
(`d_s_room.cpp` 619) -> `phase_3` (578) -> `objectSetCheck` (398) -> `dStage_dt_c_roomReLoader`
(`d_stage.cpp` 64) -> `dStage_dt_c_decode` (2474) -> `dStage_actorInit` (1129) -> `realloc`. Instrumented
prints show the run's two calls: the first sane (`num=2`, entries inside the file), the second with
`num=172`, a `stage` pointer in a different address region from the file, and a capacity of `-1`.
`objectSetCheck` passes `i_this->mpRoomDt` as that stage, and the capacity it reads is
`stage->mHostActorCapacity`, which the port only ever sets to 0 in the room's initializer - so
`mpRoomDt` is not a stage object this port initialized. **(29) The next step:** print `mpRoomDt` and the
node's raw and swapped `m_entryNum`/`m_offset` at the reload path's entry, which distinguishes "the room
scene holds the wrong object" from "the room's file was not endian-converted on this path". **(29) DONE
- the stage object is real and the node fields are unconverted.** The reloader's entry print shows
`i_stage` with capacity 271 and entries on the sanitizer heap, a host-initialized object rather than the
one the previous run's `-1` suggested. The node's fields have the shape of an unconverted big-endian
record: `m_offset` read `16777216` (`0x01000000`, swapped 1) and the actor count came out `172`
(`0xAC`, swapped small count). `d_stage.h` declares the file header's fields as `dStageFileS32` and the
like, the port's endian-aware accessors, while `dStage_nodeHeader` carries plain fields - so the stage
path converts through the typed accessors and the room path reads raw, and `dStage_dt_c_decode` is
shared by both. **(30) The next step:** find the stage path's conversion step - in the resource runtime,
the stage-resource seam (`bluewake_route_b_get_stage_res`) or `dStage_dt_c_offsetToPtr`'s host branch -
and give the room's `room.dzr` the same treatment. The instrument was reverted; the series check is green
at 285 patches. **(30) DONE - the composition compiles the runtime branch, not the seam.** `nm` on the
linked probe shows no `bluewake_route_b_get_stage_res` symbol at all, and the seam appears only in the
port's tests, so the room scene's `mpRoomData` comes from the other branch of `d_s_room.cpp`,
`dComIfG_getStageRes(arcName, "room.dzr")`. The stage file's nodes read correctly in this composition -
the earlier node inventory printed `STAG`, `RTBL` and the rest with sane counts - so `stage.dzs` is
converted and `room.dzr`, going through the same runtime accessor, comes back big-endian: hence the
reloader's `m_offset = 0x01000000` and count `0xAC`. **(31) The next step:** find where the `stage.dzs`
conversion lives - the resource runtime's host tier or `d_resorce.cpp` - and extend it to `room.dzr`, or
compose the seam branch that converts. Two earlier conclusions are corrected: the "uninitialized stage
object" reading was a consequence of this, and the capacity `271` measured since says `mpRoomDt` is a
host-initialized object after all. Note
also that lldb
breakpoints on these symbols proved
unreliable here (a breakpoint on `dStage_infoCreate` never fired while a print in `phase_2` showed the
call), so prints compiled into the object under test are the evidence.

**Protocol.** Route A's counts stay digest-gated by `scripts/bench_instructions.sh`;
route B's new work is gated by its own tests and by the scheduler diagnostic's stop
point and frame inventory. An iteration counts when a measured number moved on a named
path with the gate green, or a new instrument changed a decision, or a candidate was
refuted with a number. Fences are v54's, with the standing addition that no route B work
may regress the certified route.

## Current continuation loop — 2026-09-22 (v54: the rendered crash is the worker's frame transition)

**USER-STARTED / NOTHING IS BLOCKED BY THE USER.** The full definition is
[GOAL_PROMPT_V54_2026-09-22.md](GOAL_PROMPT_V54_2026-09-22.md). The target does not
move: 60 retraces per second, 16.667 ms per retrace, on the certified route.

**State.** v53 landed the zero-charge tolerance (+2.67 percent of the play window,
398.3 M against 409.2 M per play retrace, digest 92dd816c... unchanged) and its
rendered half crashed. The crash is now a mechanism rather than a suspicion: the
newest report faults at address 0x28 on the FIFO translation worker's stack, inside
aurora::gfx::get_render_target_size, which reads a null g_recordingFrame - the packet
pointer Aurora clears in end_frame and restores in begin_frame, both inside
aurora_backend_present(). The worker was recording a batch across that window.
Headless never opens a recording frame, which is why only the rendered path died.

**The queue, as it stands on 2026-09-22 evening.** Items 1 and 2 are done: the
worker's batch is serialized against the present transition and the worker is stopped
and joined (patches 0051 and 0052), and the rendered path is re-established at 14,700
at the certified pc. Item 4 is closed both ways: the dispatch cache landed at -1.43
percent, and the emitter-side zero-charge alternative was built with its mid-block
fix and refuted at **+2.4 percent**, because the case that makes a second body
necessary is the case the per-instruction charge exists to serve. Item 3 is
therefore the head, and it is the largest unpriced lever in the ledger: **the host's
per-turn cycle credit**, where half of all turns credit nothing. Then item 5: the
rendered level and its p99 tail, the overlap guard, and the ref/ patch registration
debt. All five have since been measured or closed: the rendered level and p99 are on
record (487.1 M a retrace, median 25.0 fps, p99 58.4 ms), the overlap guard's memo
measured a null, and the registration debt is closed - 344 patch files on disk, 344
named in the lock, checked by scripts/audit_repo.sh.

**And on 2026-09-22 the queue ran out of body-side candidates, so section 8 was
invoked.** The last one, a block-local `downcount`, is refuted by the emitter's own
shape: the computed-goto table makes every instruction an entry point, so a local
would have to be reloaded per instruction. With that, the emitted bodies - 73.5
percent of the main thread at 27.24 host instructions per guest cycle - have no
remaining measured candidate inside route A's emit shape, and the next step is a
decision rather than a trim. The dossier section 8.1 requires is at
[status/ROUTE_DECISION_2026-09-22.md](status/ROUTE_DECISION_2026-09-22.md): the
measured state, the exhausted fixes, the affected surface, the routes the ground
rules leave open (a runtime JIT is not one of them) and a falsifiable spike - one hot
function compiled from the tww source, measured in host instructions per guest
operation - that decides which route to fund.

**Protocol and fences** are v53's, with one addition this loop earned: **a crash is an
attribution, not a verdict** - symbolize it and read the fault address before naming a
cause.

## Current continuation loop — 2026-09-22 (v52: the per-block constant is closed, the split is the question)

**USER-STARTED / NOTHING IS BLOCKED BY THE USER.** The full definition is
[GOAL_PROMPT_V52_2026-09-22.md](GOAL_PROMPT_V52_2026-09-22.md), which restates the
PRD requirements this loop is measured against and the protocol it uses. The target
does not move: 60 retraces per second, 16.667 ms per retrace, on the certified route.

**State.** The per-block constant workstream is closed by measurement. Three of its
increments landed in one session - the boundary predicate inlined into the chassis
(0.78 percent), the interrupt-source publish path inlined into the refresh (2.29),
and the recomputation made conditional on the device events that can change it
(**7.33**, the largest of the workstream) - and the per-term mask that followed was
refuted at +0.15 percent and reverted. The bench window now measures **412.3 M
instructions per play retrace**, digest `83d2590d...` unchanged throughout, against
roughly 465 M when the session opened. At the host's sustained ~14 G instructions per
second that is about 29.5 ms a retrace against the 16.667 ms target: **the gap is
1.77x**, and closing it needs about 233 M.

**The split is measured and it moved the queue.** `sample` on a headless play window,
parsed by the sanctioned tool, gives: guest execution **71.9 percent** (emitted chunk
bodies at least 43.8 of it, the chassis edge service 3.95), the per-turn device
service **10.2 percent** of which **10.06 is the donor DSP LLE interpreter**, the rest
of `main` about 18, and the dispatch and lookup 4.3. The guest body a retrace is
8.1 M guest cycles at 26.9 host instructions per cycle, about 218 M of the 412.3 M.

**Item 2 was priced, built, and refuted on 2026-09-22 - do not rebuild it.** The
block-entry precharge decision keeps 9.8 percent of the emitted body, but the second
body that reaches it cannot be made correct for less than it saves: the prepaid copy
with the mid-block-entry fix (patch 0053, its leader bitmap and its per-observation
reconcile) measures **409.5 M against the control's 399.9 M, +2.4 percent**, digest
unchanged, turn counts at the recorded reference. The prize is the per-instruction
charge test and the one case that makes a second body necessary - a mid-block entry -
is the case that test exists to serve, so the entry guard and the hand-off cost more
than the test they replace. The code growth the screening loop was built to price is
not the problem: ten chunks with both bodies grow the artifact 0.19 percent, because
the prepaid body compiles smaller than the precise one. See
[status/CURRENT.md](status/CURRENT.md) for the pair; 0053 stays in
`patches/recompcore/` as the record of the shape and of why it fails. The emitter is
`ref/recompcore/DolRecomp/src/backend/emitter.c`, not the `ref/DolRecomp` checkout
beside it, which never contained the cycle emission.

**The DSP interpreter has been run, and its dispatch was not where its cost is.** The
denominator is now measured rather than quoted - the delivery-safety census's
`cpu_cycles / 6` differenced over the bench window gives **1,349,988 DSP instructions
per play retrace** - and removing the predecoded table's per-instruction
initialization guard and call is worth **-0.37 percent** on the route: nine
instructions in the object, about 1.1 per emulated instruction. The handler bodies are
the rest of the interpreter's cost and they are the DSP's semantics rather than
overhead, so this item closes; see [status/CURRENT.md](status/CURRENT.md). It spends
about **31 host instructions per emulated DSP instruction** (41.6 M host instructions
a retrace against the 1.35 M measured above), and the ledger's estimate that the
dispatch and bookkeeping were ten to fifteen of those was a static count that the
route did not charge: nine in the object priced at 1.1 on the route, because a test of
a run-invariant flag inside the DSP's loop is what a compiler hoists and a predictor
makes free.

**The queue after that, ordered by what is known:** the GX FIFO path off the main
thread, whose first question is the fact of what pins translation to the main thread;
the save-continue path; and, if the split says otherwise, the crumbs that are left in
the per-block path - the overlap guard, the device-predicate frames, the dispatch
cache's five unexplained instructions. Refuted with numbers and not to be re-run:
the per-term mask, the redundant intercept re-probes (1.53 percent hit rate), the
access path, the pc-store, branch hints on the hot guards, the register file, the
edge service's per-boundary refresh call (landed, -0.25), the DSP's dispatch guard
(landed, -0.37, patch 0054), the alias-state revalidation (landed, -0.42, patch
0055), and three nulls measured the same way: forcing `Interpreter::Step` inline,
moving the edge service's cold block out of line (the emitted prologue is
byte-identical), the DSP's `std::function` memory callbacks (395.7 against 395.9 M),
and memoising the overlap guard's monotone conjunction (395.6 against 395.9 M, and
it trades a semantics corner for the 0.08 percent it might be worth).

## Superseded continuation loop — 2026-09-22 (v51: the per-block constant, now counted)

**USER-STARTED / NOTHING IS BLOCKED BY THE USER.** The full definition is
[GOAL_PROMPT_V51_2026-09-21.md](GOAL_PROMPT_V51_2026-09-21.md). The two
decisions v50 left with the user are now the loop's own and are decided there:
the target stays 60 retraces per second, and time-to-playable is read against
continuing from a save the game itself wrote. What v51 adds is a working model
rather than a plan: the edge fast-reject landed at -4.42 percent by removing
about twenty host instructions from the miss path at every generated-block
dispatch, which puts the per-block dispatch count at ~1.07 M per play retrace
and prices one host instruction on that path at **0.22 percent** of the play
window - twenty seconds to build, four minutes to measure. The queue is ordered
by that price: the overlap-phase observation's three guest reads per block
boundary, the dispatch entry itself, the cycle-accounting design (10-15 percent
of the emitted body, priced with the two-minute ablation harness first), the GX
FIFO path off the main thread, and the save-continue path. The instrument now
refuses to report a pair whose stop pc and turn count do not match the recorded
reference for the ceiling, because a broken host change stops "normally" too and
reads as a spectacular win.

**2026-09-22: the price list is 2.7 times smaller than v51 inferred, and the
second item of its queue has landed.** The boundary census
(`BLUEWAKE_BOUNDARY_CENSUS`, printed as `turns=` in the stop line) counts the calls
the chassis makes to `host_chassis_edge_service`: a play retrace runs **391,432
boundaries**, not the ~1.07 M the fast-reject's 4.42 percent implied, so one host
instruction removed per boundary is worth **0.080 percent** of the steady window
(0.083 in the certified 13,800-14,100 window the 1.81x figure is quoted in, 0.090
in early play, 0.040 in the opening cutscene, per the table in
[status/CURRENT.md](status/CURRENT.md)) and
a candidate has to remove about fifty instructions a boundary to weigh what the
fast-reject weighed. The dispatch entry is the second increment, and it is the
dispatch cache rather than the loop that carries it: an exact pc-keyed 4096-entry
probe (eleven instructions where the window probe spent twenty) plus a split
dispatcher whose cold paths live in a noinline `dolrecomp_call_slow` (a
two-register frame where the frame held six) is worth **-1.43 percent** - 462.4 M
to 455.8 M instructions per play retrace on frozen copies measured back to back,
digest `83d2590d...` unchanged, artifact `3b7ea820...`. The boundary loop's
inlining was attributed that pair and is a null on its own: the committed sources
relinked with the original dispatch header measure 462.3 M, because the loop does
inline and its call does devirtualise but `selected_dispatch` is 908 bytes and
stays out of line. The artifact is reproducible byte for byte from the generator
with `scripts/splice_composite_dispatch.py`, which is the check that settles
which change the hash belongs to; the entry in
[status/CURRENT.md](status/CURRENT.md) has the evidence and the corrections.
The per-block constant has since lost its call frame: `bluewake_edge_requires_host`
is inlined into the chassis, worth **-0.78 percent** (459.0 M to 455.4 M on a frozen
copy of the composite, digest `83d2590d...` unchanged), which the corrected list
predicted at about half of what it measured - a static count screens a candidate,
it does not price one.
The edge-service body has since been decomposed by `BLUEWAKE_EDGE_CENSUS` - compile
time, off in the shipping build - and in the play window the overlap observation's
guard is true at **100.00 percent** of boundaries while its object is never a valid
pointer and its phase never changes, the intercept predicate answers true on **1.53
percent** of boundaries, which refutes the redundant-probe candidate at under 0.02
percent, and the interrupt-source refresh is what is left: it rebuilds three device
predicates 1.305 times a boundary and changes its answer 17 times a retrace.
The refresh's publish path has since inlined - the delivery-safety census body moved
out of line and the comparison inlined, worth **-2.29 percent** (455.4 M to 444.9 M,
digest `83d2590d...` unchanged), which is the dead-body effect the ledger measured at
2.1 percent for six instructions in 2026-09-18 and not the ten-instruction frame.

**What is left in the per-block constant, in the order the decomposition prices
it:** the recomputation itself, which now runs only on the device events that can
change it - **-7.33 percent**, 444.9 M to 412.3 M, digest unchanged, with the
census's `published` count identical at 261,281 over the route while `publishes`
falls 65 percent, so the gate skipped 607 M recomputations without losing one - and
which is about 85 instructions on the paths that still run it:
`dol_di_interrupt_pending` and `dol_si_interrupt_pending` are separate functions in
`ref/recompcore/GXRuntime` (17 and 8 instructions plus a frame each) and want a
registered patch to move into headers. A per-term mask over those sources was tried
and **refuted** at +0.15 percent with a page table and +0.32 with a containment
chain: the mmio census shows 99.65 percent of the play window's device writes are
GX FIFO bytes, but every access still steps the cursors whose terms are one of the
three lines, so the recomputation is owed whatever the address. The question it
exposed is the cursor advance itself - 48,742 accesses a retrace, and the reason the
recomputation cannot be skipped - and that needs its own measurement first. The
overlap
observation's guard and cached slot read,
about a dozen instructions a boundary and nearly irreducible, nil in output during
play; the 18 instructions that remain in the now-inlined predicate, whose extra
probes on a hit are refuted at a 1.53 percent hit rate; the five
instructions a boundary that the dispatch cache's -1.43 percent implies but its
static count does not account for. Two explanations are refuted rather than open: a
four times larger pc cache measures a null (395.7 against 395.9 M), so the probe is
not missing often, and the probe is not outlined - the built object has no
`dolrecomp_find_original` symbol, only its tables, so the probe is inlined into
`chassis_dispatch` (the outlining that cost +0.53 percent was specific to the
two-arity form, which is reverted). What remains is a counter on probe hits, misses
and search steps - the split of "cheaper hit" from "fewer misses"; the
cycle-accounting design at 10-15
percent of the emitted body, priced first with `scripts/ablate_chunk.py`; the GX FIFO
path off the main thread; and the save-continue path.

## Superseded continuation loop — 2026-09-21 (v50: finish the project from the certified number)

**USER-STARTED / FINISH THE PROJECT.** The full definition is
[GOAL_PROMPT_V50_2026-09-21.md](GOAL_PROMPT_V50_2026-09-21.md). v49 closed its
own workstream: the emitter-trim hunt ended with two landed changes worth 0.79
percent between them, the cycle accounting priced at 10-15 percent of the emitted
body, and the register-file, access-path and pc-store candidates at or near zero.
What v49 left unresolved is the number the whole decision rests on - the ledger
carries 302.3 M instructions per play retrace for the certified window and 491.7
M for the benchmark window, both called "the play window", and the two give
different gap ratios. v50's first gated item is one headless pair at the
certified ceiling that reports instructions, milliseconds and achieved
instructions per second from the same runs, so the gap is stated once, in the
benchmark's units, before a program is built on it. The ordered program after
that is P0 truth and hygiene, P1 authentic speed, P2 the renderer and its tail,
P3 graphics correctness, P4 input, P5 the compatibility campaign, P6
reproducibility, P7 iOS and iPadOS, with the user-owned decisions named in P8.

## Superseded continuation loop — 2026-09-18 (v49: stop trimming clauses, measure the translation roofline)

**USER-STARTED / FINISH AND TEST THE APP.** The full definition is
[GOAL_PROMPT_V49_2026-09-18.md](GOAL_PROMPT_V49_2026-09-18.md). The product half is met: the
signed app boots the retail disc to controllable Outset gameplay with real keys, real video, real audio,
the game's own save and a normal stop. What is not met is throughput - the play scene runs at about 51
percent of authentic speed headless and 41 percent rendered, and authentic speed needs 1.81x and 2.47x.

V49 enters on the previous stretch's eleven refutations, all at or under one percent, and reads them as a
statement about *shape* rather than about clauses: at 302.3 M host instructions for 8.1 M guest cycles the
host runs about 37 instructions per guest cycle against roughly 20 for authentic speed, and no clause in a
37-instruction path is worth 45 percent. So this loop stops trimming. It builds the microbenchmark that
three consecutive ledger entries asked for and never got, uses it to measure the roofline for a hand-
optimised body, and then builds whatever that number justifies - with the GX FIFO path off the main thread
as the one large candidate that does not depend on the answer. The hypotheses, protocol and fences are in
the prompt; the refuted candidates are listed there so they are not re-run.

## Superseded continuation loop — 2026-09-14 (v14: the cap sets delivery granularity, not a timestamp)

**USER-STARTED / FINISH AND TEST THE APP.** The full definition is
[GOAL_PROMPT_V14_2026-09-14.md](GOAL_PROMPT_V14_2026-09-14.md). V14 narrows V13's H-DSP-DEADLINE by reading it to the end: the
coupling between the cycle cap and the route digest is not a stale timestamp, it is the turn boundary itself.
The recorded cycle is already the exact guest cycle - the end-of-turn flush (`main.c:10814`) runs before the
delivery guard (`main.c:4813`) is evaluated - and that guard, the sole call site of `deliver_external_interrupt`,
is evaluated once per turn, at the top of it. The cap therefore sets how often the guest is asked to accept an
interrupt, and the deadline terms are a bystander: the DSP term is `12600 - elapsed`, which cannot bind inside
any cap this project allows. V14's hypothesis is H-DELIVERY-IDENTITY - the route invariant is which interrupt, on
which PC, into which context, in which order - and if that sequence matches under both caps with a bounded
per-delivery cycle shift, then the cap buys turns at a bounded interrupt-latency cost and nothing about the
guest's behaviour has changed. The instrument is built (`[delivery-hash] no_cycle=...`,
`BLUEWAKE_DELIVERY_TRACE=LO:HI`) and the pair of runs is what this loop owes. v13 and earlier are kept below
as the record.

## Superseded continuation loop — 2026-09-14 (v13: the cap is real, and it is not free)

**USER-STARTED / FINISH AND TEST THE APP.** The full definition is
[GOAL_PROMPT_V13_2026-09-14.md](GOAL_PROMPT_V13_2026-09-14.md). V13 takes the three measurements V12 asked
for. The report's first clause is retired: a double-click with no synthetic pulse reached File Selection on
real key presses, with frames. H-INTERRUPT-CHURN is falsified before it was built - the deadline census
puts 98% of the dispatch budget on the 256-cycle cap and only 0.009% on one-cycle churn, so the lever is the
number of host turns, not the value the deadline returns. And the cap experiment itself is done: dynamic
cap cuts host turns 820,034,526 -> 521,121,740 (-36.5%) on the identical route with an identical cycle
trajectory, and moves exactly one of 1,050 digest records - the external-delivery history hash, over
146,755 deliveries dominated by DSP interrupts. So the turn length is observable as interrupt latency and
the cap was never free. The loop's next hypothesis is H-DSP-DEADLINE: make the device deadline terms bind
inside the cap, and the 36.5% becomes free. v12 and earlier are kept below as the record.

**Terminal condition:** from a double-click with no exported variables, a human reaches controllable Outset
gameplay with correct video and audio, drives it with the keyboard for a bounded session without a crash,
time-to-playable is under five minutes, and the five governing numbers hold on this host.

**The metric this loop adds - time-to-playable:** double-click to controllable Outset, no exported
variables, measured on the product path or not measured at all. It is the one number here that is
simultaneously a performance result and a playability result.

**The finding that sets the work:** the new-game intro is the wall. new-game-intro fires at retrace 773 and
opening-complete does not fire until 13850 - 13,077 guest frames, about 3.6 minutes of authored cutscene.
It renders correctly, it does not answer START or A, and on this host it costs about 11.5 minutes of the
roughly 20-minute double-click-to-Outset journey. It is M2 (performance) expressed as a number a player
feels, and it now outranks Link's hair and audio quality as the obstacle between this project and a person
playing.

**Immediate queue, in order:**

1. H-INTRO-SKIP is answered and not supported: START and A both reached the guest during the intro and it
   still ran to retrace 13850. The intro is authored content that runs to its end.
2. Reach controllable Outset by keyboard, photographing each screen and recording the press that caused it.
   The scripted route already reaches Outset and renders it (Room44 mounts, the Outset Island banner and
   fly-in are captured); what is unproven is the keyboard getting there from the Name Entry grid. This
   settles whether game logic advances past the file menu under real input.
3. Cut the intro's cost instead of skipping it. Its rate measured 10-13 fps, so the cost is the 13,077
   authored frames rather than a path-specific slowdown. The question is whether those frames can execute
   faster without moving the digest.
4. M1c, the one press by a human hand on a physical keyboard on a double-clicked window.
5. M2/M3 and H1/H2/H3 as recorded in v4.

**Methodological rule, carried from v8:** before instrumenting anything, launch the product the way a
player does - open BlueWake.app, no environment, no arguments - and watch it. A measurement taken from a
launch path a human never uses is not a measurement of the product.

## Superseded continuation loop — 2026-09-14 (v8)

**USER-STARTED / FINISH AND TEST THE APP:** The organizing principle of this loop is that the artifact
under test is the app a human double-clicks, not the binary a script can conveniently launch. On
2026-09-14 the double-click path was found to hard-stop at retrace 502 on a 50M-turn budget default that
every script had silently overridden. With that removed, `open build/runtime-host-dsp/BlueWake.app` boots
the game, takes the foreground, plays the opening cutscene, and one J press opens File Selection. Input is
no longer the blocker.

**Terminal condition for this loop:** from a double-click with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the keyboard for a bounded
session without a crash, and the five governing numbers in
[GOAL_PROMPT_V8_2026-09-14.md](GOAL_PROMPT_V8_2026-09-14.md) hold on this host.

**Immediate queue, in order:**

1. Play the app with the keyboard from the title through File Selection into the naming screen, the new
   game intro and Outset, capturing a frame at each transition. This is the first time the forward
   progression can be driven by real input rather than a synthetic pad pulse; use it.
2. Photograph each player-visible screen and record the press that caused it. Any transition that does not
   answer a press is a defect and gets an iteration.
3. Measure the playback rate a player actually experiences on this path. The title sequence runs near 6
   fps, which is an M2 problem rather than an input problem, but it is a player-visible number.
4. Ask the user for the one human press on a physical keyboard (M1c). No synthesized event substitutes.
5. Then M2/M3 and H1/H2/H3 as recorded in v4.

**The methodological rule this loop adds:** before instrumenting anything, launch the product the way a
player does (`open BlueWake.app`, no environment, no arguments) and watch it. A measurement taken from a
launch path a human never uses is not a measurement of the product.

## Superseded continuation loop — 2026-09-09

**USER-RESUMED / WHOLE-UNIT INTEGRATION:** Follow
`status/REORIENTATION_2026-09-09.md`. The user explicitly started a new goal loop.
First repair coherent stage ownership and patch reproduction, then actual J3D
presentation/live world input, then original scheduler/Painter and normal boot.
Prefer full original owners; source tiers and test counts are supporting evidence,
not the organizing milestone. The pause and next-action paragraphs below are
historical and superseded by this directive. Full PRD scope remains mandatory.

**Active frontier after stable `ab8a9e9` (committed/pushed):** the expanded
world links with complete METER, message, instrument, SDK and resident owners;
strict scripted 180-frame world replay passes. Original `--scheduler` reaches
METER Create and fails UBSan at a missing J2DPicture. Menu/message/font/item-icon/
action-icon archives are all null: the diagnostic bootstrap bypasses the original
logo preload and deletion handoff. Complete logo compiles without a logo tier;
the current cold-start experiment completes whole LOGO Create with original
graphics/resource owners. Continue original LOGO draw/wait/delete and its complete
archive/particle handoff before room admission; cold-start setup is still diagnostic. Do not skip HUD panes or inject one archive pointer.

The third link-only attempt left the empty pinned `JAIBasic::startSoundVec`.
The loop changed to retail instruction comparison and a real sound-registration
regression; 0224 restores the wrapper and strict tests pass. 0223 shares original
restart/heart state. Source replay through 0224 passes. No new broad qualification
or checkpoint. 0216's absence claim was wrong: pinned d_com_static.cpp already
owns salvage/Medli; 0221 reconciles native duplicates. Full stage still has 11
relocation/path compile errors. Held-key movement unverified, window closed,
full goal active; no external blocker.

**Current experiment:** stage ownership/MULT and native material texture repairs
reach strict 180-frame actual Metal submission. Seven host matrix routines pass
numerical tests and typed DRW1 indices fix an 8→2048 endian error. Actual textured
terrain and Link are now visible. 0214 restores 120 weighted skeleton matrices;
0215 fixes the original camera smoothing operand, verified against retail
instructions. Link remains in view after settling. Combined all-mode public/private/oracle
regression passes; production gaps remain 4/41/52. Next continuous live control
and original scene/scheduler/Painter ownership. Follow
`status/ROUTE_B_WORLD_INTEGRATION_2026-09-09.md`, then qualify visible/live world
and retire diagnostic phase ownership through full scene/scheduler composition.

## Historical snapshots — superseded by the active continuation above

**2026-09-09 review adoption overrides historical next actions below.**
Implementation remains paused. On resume, use
`research/INDEPENDENT_DEEP_DIVE_ADOPTION_2026-09-09.md`: reconcile patch drift
and restore the stage-runtime owner, measure the world replay, then observe
real J3D consumption/presentation and live input before more diagnostic tiers.
Prefer coherent full-unit ownership where compile evidence supports it; record
concrete compile/link barriers and the runtime milestone for any retained slice.
The report's whole-stream `dl::Reader` proposal does not support the native
Aurora commands in that stream; use the corrected experiment. Apply actual
session launch constraints at resume rather than historical Simulator PIDs or
an inferred universal GUI-approval rule. No new gate or goal completion claim.

- Latest verified increment from `512470d`: 0208 active-array binding, exact
  native command records and real GX initialization pass all modes with full
  public/private/event/oracle regression and unchanged 4/41/52 production gaps.
  Experimental `--visible` builds but is unrun; do not close Simulator without
  the pending user choice. Next actual renderer observation and original Room44
  BG/live-input integration. Use BG's original collision owner instead of
  duplicate diagnostic ground registration. Headless integration remains possible;
  this is not an external block on the full goal, a P4 pass or gameplay acceptance.
  Evidence: `status/ROUTE_B_ACTIVE_VERTEX_BINDING_2026-09-06.md`.

- Current stable checkpoint: `512470d`, committed/pushed with all-mode original
  PLAYER command emission, bounded matrix-array bindings and GX state controls.
  Active 0208 WIP also passes all-mode focused 180-frame replay with exact active
  array records and real GX initialization. Main-body packets use resource arrays;
  transformed-pointer coverage is a separate restored metadata-only control.
  Public/private regressions are running before another checkpoint. An opt-in
  `--visible` Metal submission path builds but is unrun; leave Simulator untouched
  until the user resolves the pending choice. Next actual renderer observation,
  then Room44 world/live-input integration and scene progression. Do not promote
  command capture or a scripted Link-only presentation to playable/normal-boot
  acceptance. BW-P4-0126 and the full PRD goal remain open.

- Verified all-mode 180-frame input/update/collision/camera/PLAYER draw loop;
  original Link packets, real lighting/FILI and constructed shadow owners.
  Public 82/82, private matrix/event identity, animation oracles and unchanged
  production 4/41/52 link censuses pass. Checkpoint from c7ac69a is documented
  in ROUTE_B_PLAYER_DRAW_2026-09-06.md. Continue BW-P4-0126 with visible native
  frame/live input and actual Link/camera/Room44 geometry, then scene progression.
  No visible gameplay, full boot or P4 acceptance; no external blocker.

- Original KANKYO creation, real sea lighting and Room44 FILI now enable Link
  drawing; original draw-list construction fixes shadow vtable lifetime.
  Strict 180 input/update/collision/camera/draw iterations pass per-frame
  actual-Link packet identity and list-restoration controls. Reset occurs
  before camera/map enqueue. 0205 pause owners and 0206 retail position resets
  are WIP; reset test passes all modes. Finish all-mode replay/full regressions,
  checkpoint only when qualified, then visible live-input/scene integration.
  No display/normal-boot/P4 claim; stable c7ac69a, no external blocker.

- Full original draw-list/shadow source compiles and replaces diagnostic
  slices in PLAYER. Original init/reset and Always shadow texture binding run;
  actual PLAYER draw now stops in lighting at null mpSchejule after strict
  180 camera updates. Next original KANKYO initialization (schedule/palettes/
  lights), then PLAYER submissions and visible integration. No isolated pointer
  assignment, no completed draw claim. WIP uncommitted, stable c7ac69a.

- Strict camera execute/draw and projection/inverse/listener controls pass;
  logging-only audio stubs excluded, transition fences not reached. Final
  PLAYER draw temporarily withheld for that replay is restored and exposes
  five shadow/depth-sort links. Next original shadow/list ownership/init,
  PLAYER submissions and visible integration. No isolated symbol checkpoint;
  stable `c7ac69a`, WIP link failure, no external blocker.

- 0203–0204 share original render/map/list and camera-audio state. Strict
  link reaches four original demo/scene/dynamic-wave services, preserved by
  group-change logic. Next compose these whole owners, then camera_draw and
  PLAYER draw with real projection/listener/list lifetime. No isolated symbol
  checkpoint or fake success; stable `c7ac69a`, WIP uncommitted.

- Whole map/AGB source review: both compile unchanged; composition resolves
  map drawing and two AGB notification methods, reaching original static
  mFlags excluded by current tier. Remaining seven links span camera audio,
  draw-list set, render mode and flags. Compose those original owners together
  with scene/wave side effects intact, then camera/PLAYER draw. No isolated
  symbol checkpoint; WIP link failure after `c7ac69a`, no external blocker.

- Real STAG via original stage/save initialization fixes view validity;
  strict 180 original camera_execute updates pass. Actual camera_draw now
  exposes seven connected audio/list/render/map links. Next compose those
  whole owners then camera/PLAYER draw, without priority bypass or fake
  successful methods. Current link fails, stable `c7ac69a`; no external blocker.

- Camera view WIP: body-camera all modes pass; strict ICE MAT3/name and BTK
  controls pass. 0201 original camera_execute publishes view but new checks
  reject zero STAG near plane. Next real STAG via original init with stage/save
  effects, then valid view/camera draw/listener and PLAYER draw. Current strict
  exit 134 is uncommitted WIP after `c7ac69a`; no external blocker.

- Actual PLAYER camera WIP: retained non-root game heap fixes scene context;
  original player publication replaces manual assignment. Strict 180 camera
  body updates pass with moving Link (105.843) and changing center (331.581).
  Debug/Release focused pipeline 18282 running. Next ICE controls and camera
  profile/view/draw with PLAYER drawing; not visible acceptance. `c7ac69a`
  remains stable; full regression and checkpoint pending.

- 0200 BMT conversion lets scene and PLAYER phases one/two return in strict
  replay. Ground allocation exposes diagnostic root-current heap context,
  not exhaustion: original child construction changes current heap and
  material create restores that now-full child. Next proper retained non-root
  scene context and explicit ICE controls, then PLAYER camera/draw. No heap
  enlargement/allocator workaround; stable `c7ac69a`, WIP not checkpointed.

- Native scene WIP: full-layout bootstrap, original node/scene wrappers,
  materials and async resource/DVD owners compile together. Original play
  phase one reaches ICE material creation; strict run exits 134 because raw
  BMT bytes bypassed native conversion. Next source-owned BMT/material loading
  and real ICE validation, then complete PLAYER/camera/scene integration.
  No downstream name-pointer workaround or stable checkpoint yet; `c7ac69a`.

- Scene admission WIP after `c7ac69a`: original PLAYER phase one aborts at
  missing registered play scene. The existing camera follows a placeholder,
  not the actual moving Link. Compose original scene/node ownership and play
  initialization before PLAYER/camera; do not substitute fake stage lookup
  success. Strict build passes, run exits 134 at reached assertion. This is
  an integration frontier, not a stable checkpoint or external blocker.
  See `status/ROUTE_B_PLAYER_CAMERA_ADMISSION_WIP_2026-09-06.md`.

- Verified locomotion: source audit found diagnostic actor scale zero despite
  advancing real BCK samples. Original fopAc_Create/append initialization
  replaces the bypass and direct actor placement. All modes pass 120 held
  updates (103.517 displacement, 119 changing foot matrices) and 60 released
  updates ending at zero horizontal speed. Full public/private regression
  pipelines pass; production link censuses remain 4/41/52 in all modes. Next
  original PLAYER camera/draw and scene composition. Verified increment from
  `d766d29`, no P4 or external blocker.

- 0198–0199 WIP: original sound allocation/stream state/default listener and
  real table/SE pools compose; retail-confirmed hat angle wrap fixes update
  17 overflow. Strict 120 input-fed updates/Move pass but no displacement.
  LLDB confirms MOVE, normal speed 4.57064438, old-frame flag true, m3598=1.
  Next audit real animation/joint-to-foot locomotion, not forced positions
  or another input shim. Focused audio tests pass, broad matrix pending;
  stable `d766d29`, no external blocker/P4 promotion.

- PLAYER audio composition WIP: qualified playback/registration objects and
  original JAIConst now link in PLAYER. Six strict updates/Move consume real
  input (CPAD and Link stick both 0.518519), no position/speed change; seventh
  reaches checkStreamPlaying fence. Next full original initialization owners
  for stream state, derived sounds, real table/category pools and listener.
  Do not reuse simulated backend fixtures. Stable `d766d29`, no full matrix,
  locomotion, scene/camera/draw or P4 claim; no external blocker.

- Input WIP advances: matching source-built Abseil fixes strict polling;
  original axes/button/trigger transport passes. 0197 preserves retail
  fctiwz/sth angle wrap; all 65,536 byte pairs pass strict checks. Forward
  input reaches original PLAYER update and existing setAnimSound fence.
  Next compose qualified animation playback/registration into PLAYER with
  genuine lifetime, then complete input-driven updates and camera/scene/draw.
  No broad regression or stable checkpoint yet; `d766d29` remains stable.
  No external blocker; `status/ROUTE_B_PLAYER_INPUT_WIP_2026-09-06.md`.

- WIP after `d766d29`: original input polling reaches Aurora PADRead after
  native local GBA-address and PADStatus ABI fixes (0195–0196). Strict empty
  controller-map iteration fails; standalone Abseil empty-map repro proves
  a sanitizer/external-library configuration mismatch. Next pinned-source
  Abseil with consistent instrumentation, then neutral/axis/edge/release
  transport and actual Link input. Do not disable generation checks. No
  controller acceptance, stable commit or external blocker yet;
  `status/ROUTE_B_PLAYER_INPUT_WIP_2026-09-06.md`.

- Verified increment from `a7d00f1`: original complete in-memory save init and earlier audio
  lifetime fix the zero-health death branch. 0193–0194 compose original mode/
  identity owners and preserve the retail selected-item/equipment alias.
  Two-update/Move replay, public 82/82, private matrix and oracles pass all
  modes. Next actual JUTGamePad/mDoCPd input, scene
  progression and PLAYER camera/draw. No normal boot, full frame or gameplay
  claim; valid room paths remain unqualified. No external blocker;
  `status/ROUTE_B_PLAYER_SAVE_INIT_2026-09-06.md`.

- WIP after `a7d00f1`: full original collision managers compile unchanged;
  strict first update/Move retires registrations. Second update enters death
  voice from zero-health diagnostic save, confirmed by LLDB. Next original
  complete in-memory save initialization before phase two and earlier audio
  lifetime/output-mode owners. Do not hand-set life or silence voice. Current
  focused probe fails; no stable checkpoint or P4 claim. Then two updates,
  real input/scene/camera/draw. No external blocker;
  `status/ROUTE_B_PLAYER_COLLISION_WIP_2026-09-06.md`.

- Original PLAYER first update completes at real Room44 PLYR point 0 in all
  modes with original particle/heap owners, three collision registrations,
  42 finite matrices and one pending Room0 request. Public 82/82, private
  matrix and callback oracles pass. Next original collision Move (called by
  dScnPly_Draw) and process/scene progression, then genuine input/PLAYER
  camera/draw. Do not simulate a frame by manually clearing queues. No
  normal boot, sustained gameplay or P4 promotion; no external blocker.
  Base `3688ca3`; `status/ROUTE_B_PLAYER_FIRST_UPDATE_2026-09-06.md`.

- Integrated original PLAYER phase three now completes on real Room44
  collision in all three modes. Actual execution exposed missing RTBL then
  game-facing equipment audio state; patch 0188 reuses original constructors
  and setters. NEXT, room 44/ground 2200, 42 finite matrices; public 81/81 and
  private regressions pass. No normal spawn/control/render/P4 claim. Next
  retain these owners and execute original Link updates, measuring reached
  services toward continuous authentic input/camera/draw. No external blocker.
  Base `0961835`; `status/ROUTE_B_PLAYER_REAL_GROUND_2026-09-06.md`.

- Verified whole ownership increment from `39962e1`: 0182–0187 connect real
  sound-table/category initialization and start/stop/setters to original BAS
  scheduling. All 2,202 SE entries, 91 BAS resources, 4,686 frames and 6,242
  live-slot observations pass, with public 81/81 and private regressions all
  modes. Stored-state only, not real JAI frame/DSP/PLAYER acceptance. Next
  compose the game-facing audio interface into actual PLAYER phase three,
  measuring reached missing services toward real-ground makeBgWait; do not
  assume every backend branch must be rebuilt before diagnostic execution.
  Preserve fail-on-use services and full product scope. No external blocker;
  `status/ROUTE_B_REGISTERED_ANIMATION_2026-09-06.md`.

- Uncommitted 0184–0185 extend the pool WIP into request arbitration,
  eviction/rollback, ordered retirement and real stop/fade/mute transitions.
  Public 81/81, prior callback oracles and private regressions pass all modes.
  Next actual table/category allocation, basic start dispatch and
  remaining parameter owners, then replace real-BAS voice observers and
  continue to real-ground makeBgWait. No isolated pool/symbol checkpoint,
  PLAYER or P4 promotion. Last commit `39962e1`; no external blocker.
  `status/ROUTE_B_SE_REGISTRATION_WIP_2026-09-06.md`.

- Uncommitted WIP from `39962e1`: 0182 reconstructs sound/SE pool operations
  and native initParameter. Focused strict ownership/heap/pointer controls
  and 0182's full matrix pass (public 80/80). Follow-on 0183 types active-sound
  entries as native pointers; focused strict pass, full replay pending.
  Continue whole SE registration
  and typed active-sound storage, required real dispatch/parameter services,
  then replace frame-playback voice observers and reach real-ground makeBgWait.
  No isolated pool/symbol milestone; `status/ROUTE_B_SOUND_POOL_WIP_2026-09-06.md`.

- Verified original frame playback from `25966ce`: patch 0181 composes original
  scheduler/init/stop with reconstructed callbacks; 91 real BAS resources,
  4,686 frame calls, 1,205 starts/stops and 16,023 parameter requests pass all
  modes. Public 79/79, callback oracles and private matrix pass. Test voice
  responses are not PLAYER/backend acceptance. Next sound/SE pool registration
  lifetime and native handle ownership as a coherent subsystem, then real
  start/stop/parameter services and runtime closure to real-ground makeBgWait.
  No P4 change; `status/ROUTE_B_ANIMATION_FRAME_PLAYBACK_2026-09-06.md`.

- Verified reconstruction from `22dda48`: patches 0179–0180 implement the
  parameter/emission callbacks with independent retail instruction oracles.
  Public 78/78, extended 11,076/12,880 comparisons and rebuilt private matrix
  pass all modes. PLAYER keeps its fences; censuses four/41/52 and phase three
  remain unchanged. Next original animation frame scheduling/eight-slot
  lifecycle with real BAS and reconstructed callbacks, then backend/runtime
  owners to actual makeBgWait on real ground. No gameplay or P4 promotion;
  `status/ROUTE_B_ANIMATION_REQUEST_PROTOCOL_2026-09-06.md`.

- Current uncommitted WIP from `22dda48`: patch 0179 reconstructs animation
  speed parameter requests, deliberately excluded from PLAYER until real
  emission owners are ready. Public 77/77 and 11,076 request-oracle/control
  cases pass all three modes; full private WIP matrix remains pending.
  Next reconstruct startAnimSound's complete ordered effects and qualify
  request/slot ownership, then remaining owners to actual real-ground
  makeBgWait. This is subsystem work, not another isolated symbol checkpoint
  or gameplay acceptance. See `status/ROUTE_B_ANIMATION_REQUESTS_WIP_2026-09-06.md`.

- Latest verified increment from `8fb2f50`: patch 0178 establishes BAS serialized
  layout in Link's owned copy, structural record bounds, original animation
  init and verified-DOL position reconstruction. Ninety-one real resources,
  186 records and 1,662 comparisons pass with actual Link phase two/identity.
  Public 76/76 and private rebuilt matrix pass all modes, including the five
  unchanged shared actor units identified by the owner review. Censuses
  four/41/52. No phase-three, active-voice teardown or sound-output acceptance.
  Next continue the selected animation-audio owner: reconstruct missing
  emission methods and qualify request/slot ownership, then remaining owners
  toward actual makeBgWait with real ground. Keep integrated endpoint and
  full PRD scope intact. See `status/ROUTE_B_ANIMATION_AUDIO_STATE_2026-09-06.md`.

- Previous review/WIP after `8fb2f50`: mandatory subsystem review tested 23 full units
  (14 compile, 9 fail) and found original main-DOL static actor services own
  many apparent REL dependencies. Five unchanged units now compose together;
  strict Link phase two/identity passes, phase-three census is 42 (30 audio,
  12 other). This group is uncommitted and not fully regression-qualified.
  Review identified empty JAIZelAnime methods despite clean compilation.
  Verified private DOL evidence bounds setPlayPosition behavior; native BAS
  layout/ownership must accompany reconstruction. Next implement that coherent
  animation-audio data/state owner while preserving the grouped actor work,
  then remaining audio/runtime owners toward actual real-ground makeBgWait.
  Do not resume one-symbol milestone checkpoints. See
  `status/ROUTE_B_PLAYER_OWNER_REVIEW_2026-09-06.md`.

- Latest verified increment from `8161af3`: patch 0177 admits original JStudio
  data parsing with native pointer arithmetic and byte-order/alignment-safe
  reads. Independent encoded cases pass; public 75/75, actual phase two and
  accumulated private rebuilt probes pass all modes. No phase-three execution.
  Anti-stall review is now mandatory after stage/day/parser support increments:
  classify remaining whole owners (30 audio, 25 other actor/runtime symbols)
  and choose a coherent plan terminating in real-ground makeBgWait execution.
  Do not continue isolated symbol/test-count checkpoints or inject an initial
  procedure. Full scope remains intact; no external blocker. See
  `status/ROUTE_B_JSTUDIO_PARSER_2026-09-06.md`.

- Latest verified increment from `b7526f0`: unchanged full d_letter.cpp now
  composes; four-state letter and representative original dKy_DayProc checks
  pass with actual Link phase two/identity. Synthetic state is restored.
  Public 74/74 and accumulated private replays pass all modes. Censuses
  four/56/69. Next: original JStudio demo parser's three pointer-width compile
  failures and adjacent endian/alignment reads, with independent encoded
  parser oracles. Then continue makeBgWait closure and real-ground execution.
  No phase-three, normal save init, mail campaign or P4 acceptance. See
  `status/ROUTE_B_PLAYER_DAY_PROCESSING_2026-09-06.md`.

- Verified follow-on from `9d67bdc`: patches 0174–0176 compose full common-game,
  d_lib, item, message, circle/line and eight unchanged stage runtime bodies.
  Existing native resource views remain owners; full-stage serialized pointer
  fixups are not admitted. All 256 ocean-coordinate values and next-stage
  latch/release checks pass with actual phase two/identity in all modes.
  Public 74/74 and accumulated private regressions pass all modes. Censuses
  are four/58/71; no actual phase-three execution or P4 promotion. Next compose
  original letter/day processing, then remaining makeBgWait behavior owners
  and execute with real Room44 ground/model state. No skipped initial
  procedure. See `status/ROUTE_B_PLAYER_STAGE_RUNTIME_2026-09-06.md`.
- Gate: P4+ native Route B gameplay composition.
- State: `RETRY_NEW_HYPOTHESIS`; `BW-P4-0126` is the sole active blocker. Toripost lifecycle
  `BW-P4-0124` is deferred; shared allocation repair remains accepted.
- Previous result: patches 0172–0173 establish canonical native actor API/process
  layout at the exercised frontier. Actual Link uses original allocation and
  a phase-two diagnostic callback; ID 37, parameters and layer/tags survive.
  Identity passes all modes. Four paired same-profile synthetic lifetimes
  prove original event partner lookup, stale-ID rejection during identical
  address reuse, callbacks and exact heap recovery. Stone2/Room44 pass after
  removing the old field mirror and unique-profile lookup. Public 74/74 and
  accumulated private regressions pass all modes. Production censuses remain
  four/76/89. Next: compose original phase_3/makeBgWait with real Room44 ground
  and initial model state, then PLAYER execute. Full profile/Link teardown,
  other actor layout qualification, normal boot and P4 remain open. See
  `status/ROUTE_B_NATIVE_ACTOR_LAYOUT_2026-09-06.md`.
- Previous result: full original event control compiles unchanged; stable queue
  ordering/capacity/reset and typed-base ID checks pass with actual phase two.
  Public 73/73 and prior private regressions pass all modes. New
  --event-identity fails exit 2 in every mode: derived Link pointer erasure
  loses its native base adjustment; three ID and parameter sentinels each
  mismatch while adjusted-base controls pass. Reframe at the native process/
  actor ABI owner before makeBgWait: canonical typed/erased boundaries,
  allocation/construction, callbacks, multiple same-profile IDs and deletion.
  No fixed-offset or unique-profile workaround. Phase-two/three census 76;
  phase-two/execute 89; phase-two-only four. No identity/P4 acceptance.
  See `status/ROUTE_B_PLAYER_NATIVE_IDENTITY_2026-09-06.md`.
- Previous result: full original actor manager/scene lookup and shared 825-entry
  actor-name table replace PLAYER fragments/fences. Patches 0170–0171 fix a
  missing heap header and share original lookup owners. Actor append transport/
  recovery and name lookup tests pass; actual phase two, public 73/73 and
  private regressions pass all modes. Phase-two/three census 104 -> 82;
  phase-two/execute 118 -> 95; phase-two-only remains four audio symbols.
  Next: original event-control order/PID/compulsory owners, then phase three
  with real Room44 collision. No phase-three execution or P4 promotion.
  See `status/ROUTE_B_PLAYER_ACTOR_MANAGER_2026-09-06.md`.
- Previous result: patches 0168–0169 fix BDLC admission and BTP representation.
  Original PLAYER phase_2 returns NEXT in all modes; four real models match
  16 raw counts and five face tracks match 205 exact samples/material binding.
  Public 72/72 and private regressions pass. Next: compose original phase_3 /
  makeBgWait with real Room44 collision and initial behavior/model calculation,
  then PLAYER updates with camera. Do not skip phase three by injecting a
  procedure or treating phase two as complete actor creation. Normal boot,
  draw/input and P4 remain open. Production phase-two linking retains four
  audio symbols; phase-two/execute 118. See
  `status/ROUTE_B_PLAYER_PHASE_TWO_2026-09-06.md`.
- Previous result: patch 0167 fixes original skin packed-index reads and JKR
  ownership. Real shield setup covers 452 vertices, 155 position/217 normal
  mappings and exact heap recovery in all modes. Public 72/72 and private
  regressions pass. Actual init advances to heavy-boots allocation failure;
  next verify counts/endian/sizing/ownership before increasing heap estimates.
  No completed init/deformation/P4. See
  `status/ROUTE_B_PLAYER_SKIN_INDICES_2026-09-06.md`.
- Previous result: patch 0166 admits original BRK objects and endian-aware data.
  Eleven Link resources, 24 names, 10,928 independent channel samples and
  original sword-grip binding pass all modes. Public 72/72 and private
  regressions pass. Actual init advances to J3DCluster.cpp:311 packed-index
  alignment failure under strict sanitizers. Next: original skin-deform
  display-list decoding, then replay actual init. No P4 promotion.
  See `status/ROUTE_B_PLAYER_BRK_2026-09-06.md`.
- Previous result: the new optional init probe enters actual phase_2/playerInit/
  createHeap with real Link/Lkanm, then aborts on resource-86 BRK name binding
  in every mode. Patch 0165 repairs earlier signed J3D LOD UB; eight exact
  commands, prior combined tests, public 72/72 and private regressions pass.
  Next: BRK material-name resource ownership/binding, then rerun actual init.
  Audio remains fail-on-use diagnostic only. No completed init/P4 promotion.
  See `status/ROUTE_B_PLAYER_INIT_ENTRY_2026-09-06.md`.
- Previous result: original actor water lookup passes empty-world/outside and 32
  real-sea compositions; original heap ownership/recovery and native static
  registry lookup pass all modes. Patch 0164 composes the water owner.
  Public 72/72 and private regressions pass. Creation-only retains four audio
  symbols; creation/update 118. Next: original BGM queries and missing Link
  voice methods toward actual phase-two execution. No PLAYER/P4 promotion.
  See `status/ROUTE_B_PLAYER_WATER_PROCESS_2026-09-06.md`.
- Previous result: original in-memory save owners pass all 240 switch routes and
  2,048 event bits; original JPA field dispatch/drag/recycling pass all modes.
  Patch 0163 excludes unqualified native card serialization without substitutes.
  Public 72/72 and private regressions pass. Creation-only linking retains six
  symbols, creation/update 121. Next: original water/environment creation and
  remaining audio requirements toward actual Link initialization. No P4
  promotion. See `status/ROUTE_B_PLAYER_SAVE_FIELDS_2026-09-06.md`.
- Previous result: original mirror initialization passes eight real-resource
  lifetimes; exact native vertex-array pointer/extent/stride/endian commands
  pass all modes. Patch 0162 composes mirror owners. Public 72/72 and private
  regressions pass. Creation-only linking retains 10 symbols; creation/update
  139. Next: original save-switch/event-bit and JPA field owners, then remaining
  actual Link initialization dependencies. No draw/PLAYER/P4 promotion.
  See `status/ROUTE_B_PLAYER_MIRROR_2026-09-06.md`.
- Previous result: original follow callbacks execute 256 updates, smoke setup/end,
  lighting vectors and exact GF bytes pass alongside model/camera checks.
  Patches 0160–0161 bind native GF transport and compose original helpers.
  Public 72/72 and private regressions pass all modes. PLAYER creation-only
  linking retains 12 symbols; creation/update 141. Next: original mirror
  construction and graphics/resource owners, then remaining phase-two
  requirements toward actual Link initialization. No PLAYER or P4 promotion.
  See `status/ROUTE_B_PLAYER_PARTICLE_SERVICES_2026-09-06.md`.
- Previous result: PLAYER phase-two-only linking has 19 unresolved symbols versus
  156 for creation plus execute, after original collision/full model-service
  composition. Patch 0159 and private display-list preparation enable full
  m_Do_ext. Original frame/morph/resource-ID/heap checks execute alongside
  120 camera updates; public 72/72 and private regressions pass all modes.
  Next: original particle callback/mirror construction owners, then actual
  PLAYER initialization with real Link resources and the working camera.
  No PLAYER runtime or P4 promotion. See
  `status/ROUTE_B_PLAYER_CREATION_FRONTIER_2026-09-06.md`.
- Previous result: 120 original camera Run calls complete in all three modes with
  real System/Always resources and a moving non-PLAYER subject. Original demo
  construction and process-allocation clearing are composed. Public 72/72 and
  private regressions pass; production Run still lacks two audio exports.
  No input, actual PLAYER, draw or P4 acceptance. Next: measure/compose actual
  PLAYER creation/updates with this camera/service closure, then real collision
  and visible control. See `status/ROUTE_B_CAMERA_RUN_2026-09-06.md`.
- Previous result: patch 0158 repairs native audio Actor identity width from two
  verified retail wrappers. Public 71/71 and private regressions pass all
  modes. Run still has eight symbols; seStart and lower dispatch/frame bodies
  require reconstruction, not merely linkage. The full audio census retains
  two JAIBasic path-length narrowing errors. See
  `status/ROUTE_B_AUDIO_REFRAME_2026-09-06.md`.
- Previous result: patch 0157 admits actual sea event data through bounded native
  resource ownership. Original setData and nonempty substance reads pass four
  loads; two malformed copies reject and clean up. All native owners and
  archive heap space retire correctly. No event progression or PLAYER/camera
  Run promotion. See `status/ROUTE_B_NATIVE_EVENTS_2026-09-06.md`.
- Previous result: original typed event queries/reset and eight demo-camera
  lifetimes pass all modes. Real sea event_list.dat reproduces the endian
  admission frontier (252 events versus raw native 4,227,858,432); expected
  diagnostic exit 2, original setData not called. Public 69/69 and PLAYER
  compilation pass all modes. Run still fails with eight symbols. Patch 0156
  preserves native strlen width. See `status/ROUTE_B_CAMERA_EVENTS_2026-09-06.md`.
- Previous result: original sea create/updates/queries pass four real-resource heap
  lifetimes and 88,000 query comparisons per mode. Reproduced LOD conversion,
  height-grid ownership and indexed FIFO defects are repaired. Public 69/69
  and private/camera regressions pass all modes; Run still fails with 16
  unresolved symbols. TWW through 0155 and Aurora patch 0001 are required.
  See `status/ROUTE_B_SEA_CAMERA_SERVICES_2026-09-06.md`.
- Previous result: original camera mass geometry passes 66 cases, and original
  camera bush-collision consumes/publishes capsules across sixteen controller
  lifetimes in all modes. Public 69/69 and private regressions pass. Run now
  has 22 unresolved symbols, still unexecuted. No source edits beyond 0151.
  See `status/ROUTE_B_CAMERA_MASS_RUNTIME_2026-09-06.md`.
- Previous result: patches 0150–0151 decode real arrow BPKs and fix both reproduced
  attention heap escapes. Independent color samples/material binding and eight
  original constructor/destructor lifetimes pass all modes. Public 68/68 and
  private regressions remain green. No actual PLAYER or targeting updates.
  See `status/ROUTE_B_ATTENTION_CONSTRUCTION_2026-09-06.md`.
- Previous result: actual Always/System admission resolves the arrow model/BCK,
  but all five attention BPKs remain serialized. The explicit diagnostic exits
  2 in all modes without attempting attention construction. Public replay is
  68/68 all modes; no production behavior changed. Stable base `3df6aee`;
  `status/ROUTE_B_ATTENTION_RESOURCE_FRONTIER_2026-09-06.md`.
- Previous result: original camera pose-to-viewport and matrix composition checks
  pass over sixteen constructor lifetimes in all modes. Attention source/table
  and original actor queue/search are link-composed, not runtime-qualified.
  Camera Run now has 25 unresolved symbols. Public 68/68 and private regressions
  remain green. See `status/ROUTE_B_CAMERA_MATRIX_RUNTIME_2026-09-06.md`.
- Previous result: original collision construction/registration/query/release now
  executes in BG and Room44 actor lifetimes. BG poisoned-heap recreation and
  independent BG/Stone2 deletion pass in all modes. Public 68/68 and private
  regressions remain green. Camera Run now has 40 unresolved symbols after
  functional background/geometry composition; update execution remains open.
  See `status/ROUTE_B_BG_FUNCTIONAL_COLLISION_2026-09-06.md`.
- Previous result: patch 0149 decodes real DZB into bounded native resource owners.
  Real Room44 original queries agree with an independent barycentric oracle
  over four loads per mode, with malformed-load cleanup and independent room/
  object lifetimes proven. Public 68/68 and private/background/camera probes
  pass all three modes. See `status/ROUTE_B_NATIVE_DZB_2026-09-06.md`.
- Previous result: patch 0148 executes original ground/line queries, camera/Link
  polygon filters, moving geometry, and release on typed synthetic geometry.
  Six original collision allocations now obey solid-heap ownership; eight
  lifetimes and exhausted-heap cleanup pass in all three modes. Public 67/67
  and five private regressions plus camera construction remain green.
  Real DZB loading and playable-scene integration remain open. See
  `status/ROUTE_B_BACKGROUND_QUERIES_2026-09-06.md`.
- Previous result: patch 0147 verifies 16 original camera constructor/destructor
  cycles per mode against typed synthetic actor/stage inputs. A negative
  float-to-u32 UB defect is repaired using verified retail conversion behavior.
  Full PLAYER/camera compilation, public 67/67, and five private regressions
  remain green. Run still fails linking with 60 symbols in all three modes.
  This is not normal boot, camera process phase two, or PLAYER runtime. See
  `status/ROUTE_B_CAMERA_CONSTRUCTION_2026-09-06.md`.
- Next loop: compose remaining original actor/assertion owners and measure
  actual camera/PLAYER execution requirements. Diagnostic-only unqualified
  audio boundaries must fail visibly on use, not fabricate success. Do not
  assume every audio branch is required before the first diagnostic update;
  use actual reached calls to prioritize reconstruction. Real event resource
  admission and original substance reads remain in regression.
  Preserve original sea, camera mass, real attention and room collision tests.
  Close remaining audio/actor/assertion dependencies and execute updates with
  actual PLAYER. Audio seStart is incomplete source, not merely unlinked.
  Keep real actor/room collision lifetimes in regression; do not return to an
  unrelated prop queue or count attention linking as runtime acceptance.
  Do not make each prop the organizing milestone. Evidence and critical path:
  `status/PLAYABILITY_CRITICAL_PATH_2026-09-06.md`.
- Organizing integration milestone: one continuous controllable Outset session
  with visible world, player, camera, functional collision, and live input;
  then room transition and save/reload. Normal boot/progression remains
  mandatory before acceptance; diagnostic room requests do not substitute.
- Closed scope: new mailbox interactions, unmeasured global camera-layout
  substitution, revived rejected Route A tuning, mobile, another BlueWake
  process, and another Simulator. Required actors remain in the product scope.
- Terminal condition remains the full PRD definition of done; this checkpoint
  does not complete the durable BlueWake goal.

## 1. Master objective

Build and validate BlueWake as a complete Wind Waker product for Apple Silicon
macOS first, then a shared iPhone/iPad application. Continue through compile,
link, authentic boot, gameplay, compatibility, Apple UI, Simulator, physical
device, performance, full-game, and release gates. Do not stop merely because
one architecture fails; record the evidence and take the next permitted route
toward the same product.

The loop may run for weeks. Its unit of progress is not elapsed time or the
number of edits. Progress is a newly passing test, a reduced and explained
failure, a measured architectural decision, expanded compatibility coverage,
or a durable artifact another agent can reproduce.

## 2. Non-negotiable operating rules

1. Read `docs/PRD.md` at the start of every resumed run.
2. Work on the earliest unmet gate or its highest-priority reproducible
   blocker.
3. Keep the user's disc, original binaries, generated translations, saves,
   captures, logs, device data, and signing material private and ignored.
4. Never acquire illicit game data or leaked source.
5. Never use captured emulator state, manual process trees, or forced scenes as
   proof of authentic boot.
6. Never turn a required service into an unlogged no-op to make a test green.
7. Never weaken/delete a test because it exposes a failure.
8. Never equate build, link, install, PID, logo, frame, Simulator, or synthetic
   warp evidence with a later acceptance gate.
9. Make the smallest change that tests one falsifiable hypothesis.
10. After three materially identical failures, change the experiment; do not
    repeat the same action.
11. Preserve a last-known-good checkpoint and user data before risky work.
12. End every iteration with enough state for another agent to continue.
13. Evidence from a rejected or opt-in workaround cannot establish an
    authentic-route blocker; reproduce the route authentically first.
14. Treat algebraically incoherent symptom values as presumed upstream clock,
    translator, or ABI corruption until the owning subsystem is verified.
15. If diagnostic hooks are added and removed more than twice, promote the
    subsystem or reframe the blocker instead of adding another hook.
16. Never make guest-visible behavior depend on whether an outer host turn
    happens to stop at a particular guest PC. Attach semantic actions to the
    authentic guest instruction, MMIO observation, callback, or device event;
    host-turn PC checks may report state but must not mutate it.
17. Derive hardware cadences that share a physical clock (VI, AI, DSP,
    timebase, and decrementer) from one promoted guest-clock source. Dispatched
    block counts may bound or profile a run, but may not drive guest devices.
18. Do not schedule authentic input by a fixed retrace index until retrace
    cadence has a reference-backed clock oracle. Prefer an observed retail
    readiness marker and deliver the edge through the authentic PAD path.
19. Classify source-text assertions as drift tripwires, not behavioral proof.
    Every promoted runtime service requires an executable state-transition or
    timing oracle.
20. Do not add another address-specific scheduler/exception carve-out after a
    subsystem has crossed the promotion threshold. Open one owner-level
    interrupt-safe-point/context-coherence blocker instead.
21. Deliver hardware-originated guest callbacks under the owning interrupt
    subsystem's scheduler-lock/context contract and preserve the interrupted
    architectural register image. An asynchronous callback may retain guest
    memory and wake effects, but must not schedule inline as an ordinary call
    on the intercepted API caller's context.
22. Treat a coherent guest UI wait as an input frontier, not a runtime stall.
    For multi-step dialogs and menus, arm each button or axis event from the
    retail consumer's current state; do not infer later inputs from elapsed
    retraces after an earlier edge.
23. Treat function-entry PCs sampled only at host dispatch boundaries as a
    lossy oracle for statically translated code. Direct intra-chunk calls and
    branches may execute retail functions without surfacing their entry PC to
    the host loop; prefer persistent retail object/manager state or explicit
    generator-level call instrumentation before concluding that a function
    was not reached.
24. A tier that re-implements a complete, marker-free retail unit already in
    `ref/tww` is a horizontal proxy unless the ledger records why the original
    cannot compile. Three such tiers in succession trigger anti-stall review.
25. Record a source-port tier as bounded only with measured adaptation density
    (changed lines divided by compiled lines) or a classified unresolved-symbol
    count. A passing smoke alone does not establish boundedness.
26. Host facts with no meaningful Mac equivalent, including DTV status, reset
    code, and progressive mode, may be logged constants with unique IDs; they
    do not require a policy subsystem.
27. Do not report native FPS until representative Outset content renders
    through J3D. Both execution routes require an explicit frame-pacing service
    before an FPS claim can satisfy a product gate.
28. Prefer whole original translation units after their compile portability is
    demonstrated. Do not introduce another tier for such a unit; classify and
    compose its link/runtime owners. Any remaining slice needs a recorded
    concrete compile blocker and a path into authentic scheduler/frame behavior.
29. Before reconstructing an empty upstream function, inspect current licensed
    upstream source for a bounded backport. Preserve pins and patch reproducibility.
30. Validate the diagnostic tool's supported command/state contract before
    attributing a rejection to the game. Native pointer-bearing captures require
    live owners or explicit rebinding; byte dumps alone are not replay artifacts.

## 3. Persistent project state

The implementation must create and maintain these public-safe files as soon as
P0 begins. JSON/YAML equivalents are acceptable if schemas remain documented.

```text
config/dependencies.lock.json     dependency URLs, SHAs, licenses, patches
docs/status/CURRENT.md            concise current gate and next action
docs/status/GATES.md              gate ledger and evidence links
docs/status/BLOCKERS.md           active/resolved blocker ledger
docs/status/DECISIONS.md          architecture and scope decisions
docs/status/COMPATIBILITY.md      coverage summary and open content failures
docs/status/PERFORMANCE.md        dated measured performance results
docs/status/RELEASE.md            release checklist and artifact identities
tests/coverage/catalog.json       public-safe generated content/test catalog
```

Private ignored state belongs below `local-research/`, `generated/`, and build
directories:

```text
local-research/input/             aliases and private input hashes/paths
local-research/manifests/         complete disc/DOL/REL manifests
local-research/evidence/          traces, screenshots, audio, logs, saves
local-research/reproducers/       minimized private failure inputs
local-research/checkpoints/       recoverable generated/save snapshots
generated/                        translated/derived game code and objects
```

### 3.1 Current status minimum fields

`CURRENT.md` must answer:

- What is the earliest unmet PRD gate?
- What exact blocker prevents it?
- What is the stable failure signature?
- What was the last hypothesis and result?
- What is the next smallest action?
- What is the last-known-good BlueWake commit and dependency lock?
- What private input alias and platform were used?
- Is user input, hardware, signing, or authority currently required?

### 3.2 Blocker record

Every blocker gets an ID such as `BW-REL-0007` and:

- title and category;
- first/last seen build;
- gate and content IDs affected;
- exact reproducer command/route;
- normalized signature;
- measured facts;
- current inference;
- ruled-out hypotheses;
- attempts with commit/evidence links;
- regression test added;
- owner/state/priority;
- next action; and
- resolution and verification matrix when closed.

## 4. Loop states

Every iteration ends in exactly one state:

- `ADVANCE` — current exit criteria pass; move to the next gate.
- `RETRY_NEW_HYPOTHESIS` — same blocker, new evidence-backed experiment.
- `DECOMPOSE` — blocker is too broad; split into smaller blockers/tests.
- `PIVOT_SUBSYSTEM` — replace or rework a subsystem at a stable API boundary.
- `PIVOT_ROUTE` — measured architecture review selects another execution route.
- `SUPERSEDED_BY_DECISION` — a route-specific criterion is replaced by named
  mandatory criteria under a recorded route decision; it is not silently
  waived.
- `WAIT_EXTERNAL` — only a user-owned input, physical device, signing action,
  service recovery, or upstream state prevents meaningful progress.
- `ASK_USER` — new authority or a product choice would materially change
  scope/result.
- `COMPLETE` — all PRD definition-of-done criteria pass and no required work
  remains.

`WAIT_EXTERNAL` is not valid while independent tests, tooling, documentation,
source reconstruction, or other gates can still advance safely.

## 5. The core iteration

### Step 0 — recover safely

1. Read PRD, current state, gate ledger, blockers, decisions, last handoff,
   `docs/status/FINISH_LINE.md`, `docs/status/TECH_DEBT.md`,
   `docs/status/REORIENTATION_2026-08-22.md`,
   `docs/status/REORIENTATION_2026-09-01.md`,
   `docs/status/INDEPENDENT_REVIEW_2026-08-25.md`, and
   `docs/status/INDEPENDENT_REVIEW_2026-08-30.md`, and
   `docs/status/INDEPENDENT_REVIEW_2026-09-01.md`.
2. Inspect `git status`, current branch, recent commits, dependency lock, and
   ignored work areas.
   The lock includes recursive submodule SHAs, translator/runtime ABI versions,
   and their tested compatibility matrix.
3. Preserve unrelated user changes.
4. Verify last-known-good artifacts still exist or can be reproduced. For
   ignored private-derived executables, validate the expected hash and exported
   address/ABI namespace; a familiar path or filename is not artifact identity.
   Reject the replay at Step 0 when either differs from the qualified record.
5. Check for new upstream revisions only when relevant; do not silently repin.
6. Mark evidence stale if its dependencies or inputs changed.
7. Verify that every active device cadence uses the promoted guest-clock
   source. Mark fixed-retrace schedules stale whenever that cadence changes.

### Step 1 — choose work

Select the earliest unmet gate. Within it, choose in order:

1. S0 data-loss, provenance, privacy, or security risk;
2. deterministic build/compile/link blocker;
3. crash, hang, corruption, or save loss;
4. authentic progression blocker;
5. systemic correctness issue;
6. performance blocker supported by profiling;
7. compatibility gap on the critical route;
8. Apple shell/UX requirement for the active phase;
9. broader optional coverage or cleanup.

Do not choose cosmetic work while the core cannot pass its active gate unless
the cosmetic work is an independent prerequisite or no core progress is
possible without external input.

### Step 2 — establish the baseline

Before editing, capture:

- exact command or input route;
- BlueWake commit and dirty state;
- dependency lock/SHA set;
- toolchain, SDK, architecture, and build type;
- private input alias/revision;
- platform/device/OS;
- exit code, crash/guest address, and normalized log signature;
- relevant REL/stage/room/scene/save IDs;
- current metrics; and
- smallest existing test that reproduces it.

If the failure cannot be reproduced, instrument first. Do not guess-edit.

### Step 3 — classify the blocker

Use one primary category:

- `WORKSPACE` — missing tool, bad pin, non-hermetic dependency, disk/path.
- `INPUT` — unsupported revision, extraction, manifest, malformed topology.
- `TRANSLATOR` — decode, CFG, codegen, ABI, FP, instruction semantics.
- `SOURCE_MATCH` — missing function/source, compiler match, layout, MWCC quirk.
- `COMPILE` — C/C++/ObjC/Swift/Rust compile or deployment target.
- `LINK` — symbols, duplicate generated names, LTO, object size.
- `MODULE_ABI` — descriptor/range/hash/chunk metadata.
- `RELOCATION` — DOL/REL import or relocation semantics.
- `REL_LIFECYCLE` — load/unload/reload/address reuse/registration.
- `DISPATCH` — direct/indirect target, linked/runtime translation, fallback.
- `CPU_RUNTIME` — exceptions, interrupts, timing, idle loops, SMC, memory.
- `OS_SERVICE` — thread, sync, alarm, heap, DVD, CARD, reset.
- `GRAPHICS` — GX/VI/EFB/vertex/texture/shader/presentation.
- `AUDIO` — JAudio1/DSP/AI/ARAM/device/synchronization.
- `GAMEPLAY` — actor, scene, event, collision, camera, item, story logic.
- `SAVE` — CARD semantics, serialization, migration, corruption.
- `INPUT_RUNTIME` — PAD, controller, touch merge, rumble, stuck state.
- `APPLE_HOST` — window/layer/lifecycle/import/settings/diagnostics.
- `PERFORMANCE` — measured CPU/GPU/memory/I/O/thermal bottleneck.
- `TEST_HARNESS` — oracle, synchronization, flake, artifact mismatch.
- `PROVENANCE` — license, private data, generated artifact, donor risk.

### Step 4 — reduce

Create the smallest safe reproducer:

- one synthetic instruction or function;
- one DOL/REL pair;
- one relocation/import;
- one link/unlink/reload sequence;
- one direct/indirect dispatch;
- one OS call sequence;
- one draw/material/texture state;
- one audio event/buffer;
- one stage/room transition;
- one save operation;
- one touch/controller event sequence; or
- one Simulator lifecycle transition.

Private game-derived reproducers remain ignored. Add a synthetic public
version whenever it can preserve the failure semantics.

### Step 5 — research narrowly

Inspect in this order:

1. active BlueWake implementation and tests;
2. pinned selected runtime/translator;
3. `zeldaret/tww` symbols, source, maps, and matching status;
4. SunPad for Apple host/input/import/diagnostic behavior;
5. Dusklight/Aurora for source-native GameCube behavior;
6. gcrecomp/WW experiment for bounded algorithms and failure observations;
7. other licensed primary references;
8. reference emulator behavior as an oracle, not initialization data.

Record donor license and exact revision before copying or adapting code.
Prefer existing narrow seams over broad rewrites.

### Step 6 — form one falsifiable hypothesis

Write:

```text
Hypothesis:
Because <measured evidence>, <specific cause> produces <failure signature>.
If <small change/instrumentation> is applied, then <specific observable>
will change while <control observable> remains unchanged.
```

If there is no predicted observable, the hypothesis is too vague.

### Step 7 — implement the smallest change

- Touch only required files.
- Add instrumentation before behavioral change when cause is uncertain.
- Put generated behavior in the generator, patch manifest, or host hook—not an
  undocumented edit to generated C.
- Assert impossible states and fail with a stable ID.
- Preserve diagnostics and fallback counters.
- Avoid unrelated formatting/refactors.
- Update the dependency patch snapshot immediately if an ignored upstream
  checkout is changed.

### Step 8 — verify in tiers

Run the narrowest tier first and stop at the first failure that invalidates the
hypothesis:

1. focused unit/reproducer;
2. subsystem suite;
3. generator/compile/link;
4. active gate smoke route;
5. affected chapter/content routes;
6. accumulated regression suite;
7. platform/form-factor matrix required by the phase;
8. performance/soak only when correctness tiers pass.

Do not spend hours on a broad suite after the focused test disproves the
change. Do not skip broad regression after it passes.

### Step 9 — compare with an oracle

Use the strongest applicable oracle:

- exact generated hash or module range coverage;
- byte-matched decomp object/function;
- synthetic expected state;
- unmodified reference execution trace;
- visual/frame-state comparison;
- audio event/fingerprint comparison;
- save hash/field/reference interchange;
- known scene/REL/actor/story milestone; or
- previous released/last-known-good Apple UI snapshot.

Separate measured equality from visual/manual judgment and unresolved
differences.

### Step 10 — record evidence

Update blocker, gate, compatibility, and performance ledgers before moving on.
Include failed attempts; they prevent repeated dead ends. Hash private
artifacts and reference them by safe alias rather than path/content.

### Step 11 — checkpoint

A checkpoint is eligible when:

- the intended focused test passes;
- required regression tiers pass;
- generated outputs reproduce;
- repository/private-data audits pass;
- no unrelated change is staged; and
- current status and next action are recorded.

Commit a coherent increment when authorized. Do not force-push, rewrite user
history, or publish without scope authority. Preserve failing diagnostic work
on a branch or documented patch if it is needed for the next iteration.

### Step 12 — decide and continue

Select one loop state. Unless `WAIT_EXTERNAL`, `ASK_USER`, or `COMPLETE` is
genuinely reached, begin the next iteration immediately.

## 6. Anti-stall protocol

Normalize failures by category, test, exit/signal, top stack/guest PC, module,
and key error text. Track the experiment **shape** separately from the exact
command or signature: shape includes the owning subsystem, oracle, and kind
of next smallest action, such as tracing one boundary earlier.

After the third repetition of the same experiment **shape**, where shape
includes the owning subsystem, oracle, and kind of next smallest action (for
example, tracing one boundary earlier), not merely the command or normalized
signature:

1. prohibit another unchanged command/edit cycle;
2. add new instrumentation or a differential trace;
3. reduce the reproducer further;
4. test a different layer of the hypothesis;
5. inspect a different licensed donor or current upstream change;
6. ask another agent for an independent diagnosis when available;
7. split the blocker; or
8. trigger subsystem/route review.

An experiment shape that moves a probe one function boundary upstream without
changing the owning layer counts as the same experiment. Three repetitions
trigger the anti-stall response even when commands, PCs, or trace labels
differ. At that point the next action must zoom out to the subsystem, use a
different oracle, or record `SUPERSEDED_BY_DECISION`; repeating the same
boundary-earlier trace is prohibited.

Evidence gathered under a rejected or opt-in workaround cannot justify a
blocker on the authentic route. Reproduce the behavior authentically first;
only then promote it to an authentic blocker.

If a symptom contains algebraically incoherent values, presume an upstream
clock, translator, or ABI/ownership problem and escalate to that owning
subsystem instead of tracing farther downstream. If diagnostic hooks for one
subsystem have been added and removed more than twice, promote that subsystem
to a required runtime service or reframe the blocker; do not add another
temporary hook.

Do not use a horizontal proxy as product progress after it has answered its
feasibility question. Examples include translation units compiled, headers
made portable, callbacks introduced, or synthetic smokes linked. After three
successive iterations of the same proxy shape, the next tier must cross an
authentic vertical product milestone (retail process, asset, frame, input,
audio, save, or scene transition) or trigger subsystem/route review. A passing
proxy may prove the foundation systematic; it does not increase the completion
estimate unless its documented causal link to a PRD milestone is exercised.
Re-implementing a marker-free retail unit is one such proxy unless the ledger
records a concrete original-source compile blocker. A source tier is bounded
only when its adaptation density or unresolved-symbol closure is measured.

A negative function-entry-PC observation at the host dispatch loop is not a
negative execution result unless the translated control-flow graph proves that
entry must cross a dispatch boundary. When direct intra-chunk control flow is
possible, change the oracle to persistent retail state or instrument the
translator boundary explicitly; do not repeat nearby PC probes.

A transient host/network failure may be retried unchanged before source is
modified. Do not “fix” source because DNS, signing, Simulator service, Xcode,
or GitHub temporarily failed.

### 6.1 Multi-agent coordination

- Assign exactly one owner to each blocker and each edited file set.
- Delegated research/diagnosis is read-only by default and bounded to a named
  question.
- Do not allow overlapping edits in a shared worktree. Use isolated worktrees
  or explicit non-overlapping paths when parallel implementation is necessary.
- Serialize gate/blocker ledger updates through the owner.
- A delegated implementation returns a patch/commit plus tests and evidence;
  the parent/owner reviews it before integration.
- Do not merge two independently green patches until their combined regression
  suite passes.
- Physical-device, Simulator, save, and importer operations have one active
  operator to avoid ambiguous evidence or destructive races.

## 7. Specialized loops

### 7.1 Compile/link loop

```text
configure clean target
  -> compile until first deterministic error
  -> classify ownership (BlueWake/generated/dependency/toolchain)
  -> minimize affected target
  -> fix one cause
  -> compile narrow target
  -> compile full active configuration
  -> run linked-artifact audit
  -> record new frontier
```

Do not accumulate dozens of speculative compile fixes without rebuilding.
Warnings affecting ABI, deployment target, undefined behavior, or generated
code count as blockers even if the linker succeeds.

### 7.2 Decomp/source matching loop

Use when Route B or source reconstruction is active:

1. select a function/object required by the earliest failing content;
2. confirm symbol, original range, callers, callees, globals, types, and
   version;
3. create/reproduce its objdiff/decomp scratch;
4. reconstruct behavior without leaked source;
5. compile and compare;
6. iterate on types, inlines, control flow, compiler flags, and MWCC idioms;
7. obtain exact match when practical;
8. if functionally equivalent but not matching, add behavioral differential
   tests and label it accurately;
9. integrate into the full object/module;
10. run the content route that required it; and
11. contribute upstream only under separate authority and project rules.

“Matched” means byte-matched. “Equivalent” means tested behavioral source.
“Stubbed” means incomplete and logged. Never blur these statuses.

### 7.3 Static translation loop

1. isolate the exact input image/range/instruction;
2. compare decoded semantics with PPC documentation and a reference oracle;
3. add a synthetic translator test;
4. fix decode/CFG/codegen/ABI or mark an explicit fallback reason;
5. regenerate from clean input;
6. compare manifests/ranges/object output;
7. run lockstep or state differential when available;
8. run the affected runtime route; and
9. record fallback hotness after correctness passes.

### 7.4 REL loop

1. identify module ID, sections, build-time ranges, runtime ranges, imports,
   and lifecycle event;
2. reproduce with a synthetic module pair if possible;
3. verify relocation math and metadata;
4. verify registration and address translation both directions;
5. dispatch direct and indirect calls;
6. unload and prove aliases/caches are invalidated;
7. reload at a different address;
8. load a different module at the reused address;
9. run authentic content that performs the same lifecycle; and
10. profile mapping cost.

### 7.5 Authentic boot/progression loop

At each milestone:

1. start from clean normal boot;
2. synchronize on original scene/story markers;
3. record active RELs, fallback, OS calls, frame/audio/save markers;
4. stop at the first divergence or stall;
5. compare with a reference run from the same input;
6. reduce to the earliest differing event;
7. fix the owning subsystem;
8. add a regression checkpoint; and
9. replay every earlier milestone before advancing.

Milestone synchronization must use a named retail state/event when available.
Fixed block or retrace indexes are controls only after their underlying clock
has been measured against a reference oracle; they are not readiness signals.

Debug warps shorten diagnosis after the divergence is understood, but the
promoted route always starts normally.

### 7.6 Graphics loop

Use a canonical scene and isolate one state/material/effect. Capture private GX
state and resulting image/digest, compare with reference, change one renderer
behavior, run narrow render test, then replay the canonical scene corpus.
Performance tuning follows visual correctness.

### 7.7 Audio loop

Synchronize on named game events. Compare requested sequence/sample/voice
events, buffer cadence, channel state, and output fingerprint. Fix one
JAudio1/DSP/device boundary, run underrun/drift/deadlock tests, then replay
field/interior/battle/cutscene/conducting scenarios.

### 7.8 Save loop

Before every risky save test, hash and copy the fixture. Exercise create,
write, read, error, quit, restart, migration, and reference interchange.
Inject interruption/failure at write boundaries. A failed test restores the
fixture; it never continues on a possibly corrupt live save.

### 7.9 Apple shell loop

Implement behavior through BlueWake-owned interfaces in this order:

1. host/`CAMetalLayer` smoke and runtime adapter;
2. normalized input mixer and conditional serialized-adapter safety;
3. three-dot menu and pause-reason state machine;
4. independent phone layout/defaults;
5. independent tablet layout/defaults;
6. grouped D-pad/touch editor and input suppression;
7. controller mapping, slots, reconnect, and touch handoff;
8. staged atomic import with fault injection and destructive-removal safety;
9. lifecycle/audio interruption recovery;
10. bounded guided diagnostics and privacy tests;
11. package/Files/provenance audit; and
12. physical acceptance.

Run the focused regression suite after each applicable substep. Then run unit
tests, iPhone Simulator, iPad Simulator, physical iPhone, and physical iPad in
that order. Boot and test one Simulator at a time and shut it down fully before
switching form factor. Keep phone/tablet layouts independent.

App updates install in place to preserve the container. Game data is replaced
only through the staged atomic transaction; saves/preferences are never
replaced wholesale. Test same-filename reimport, interrupted extraction,
manifest failure, failed activation swap, stale-staging cleanup, and exact-scope
data removal with pre/post hashes.

On both Simulators, replay normal boot plus representative multi-area gameplay,
transitions, combat, menus, saves, and REL sets before moving to hardware.
Prove actual core pause before suspension, bounded startup-window retries,
same-process resume, renewed frame production, neutral input, controller
reconciliation, and audio-session reactivation—not only callback invocation.
Synthetic/virtual Simulator controllers must not falsely hide touch controls.

Preserve the evidence boundary: Simulator coverage cannot certify physical
performance, thermals, real controller lifecycle, or touch ergonomics.

### 7.10 Performance loop

Do not optimize from FPS alone.

1. choose a deterministic representative route;
2. record FPS, speed, frame distribution, AOT/fallback, CPU subsystems, GPU,
   memory, I/O, thermals, and energy;
3. name the dominant measured cost;
4. form one optimization hypothesis;
5. preserve correctness digests;
6. measure before/after on the same route/device/state;
7. reject regressions hidden by average FPS; and
8. replay extended thermal/soak tests after wins.

## 8. Route-pivot protocol

A pivot is an engineering result, not a failure of persistence.

### 8.1 Trigger

Open a route decision when a PRD Route A review condition is measured. Attach:

- blocker history and failed bounded fixes;
- representative profiles and correctness evidence;
- estimated affected surface;
- current TWW source census;
- available donor maturity/licensing;
- migration cost and preserved reusable work; and
- a falsifiable spike for the candidate route.

### 8.2 Decision options

Preferred escalation:

```text
fix bounded translator/runtime defect
  -> add bounded REL registration
  -> use tww source/symbols as diagnostic knowledge
  -> replace whole subsystem behind a narrow API
  -> adopt a more suitable translator/runtime
  -> pivot whole execution route to source-native tww + Aurora
  -> reconstruct missing source behavior and validate content
```

Do not build an undocumented per-function bridge between incompatible live
memory models.

### 8.3 Pivot preservation

Retain and reuse:

- topology and dependency locks;
- extracted public-safe manifests;
- symbols and coverage catalog;
- Apple host/runtime adapter interfaces;
- normalized input, settings, importer, saves, and diagnostics;
- test routes, reference digests, and blocker history; and
- compatible clean host services.

Invalidate and rerun evidence tied to the former execution core.

## 9. Gate advancement checks

Before marking any gate `PASS`, answer yes to all:

- Are all exit criteria in PRD satisfied?
- Was the authentic path used where required?
- Is evidence current for the exact dependency/input/platform set?
- Did focused and accumulated regressions pass?
- Are private/proprietary artifacts absent from Git/package?
- Are save/data safety requirements preserved?
- Are compilation, Simulator, device, gameplay, and hands-on evidence labeled
  separately?
- Are unresolved symptoms listed rather than hidden?
- Can a clean agent reproduce the result?

If any answer is no, the gate is not passed.

## 10. Iteration report template

Use this concise status at the end of every meaningful iteration:

```markdown
## Iteration <ID> — <date/time>

- Gate: <P# and criterion>
- State: <ADVANCE | RETRY_NEW_HYPOTHESIS | ...>
- Build/input/platform: <commit, lock, alias, target>
- Blocker: <ID and normalized signature>
- Measured facts: <what was observed>
- Hypothesis: <cause and predicted observable>
- Change: <smallest implementation/instrumentation>
- Verification: <commands/routes and pass/fail>
- Regression impact: <new and replayed tests>
- Private-data audit: <result>
- Remaining uncertainty: <explicit>
- Next action: <one concrete step>
```

Do not flood status with raw logs. Store artifacts privately and report safe
hashes, signatures, and relevant excerpts.

## 11. Completion audit

The loop may enter `COMPLETE` only after:

1. every applicable PRD gate and replacement gate is `PASS` and current, with
   each `SUPERSEDED_BY_DECISION` criterion linked to its decision/replacement;
2. the full definition of done is checked line by line;
3. clean reproduction and package audits pass;
4. macOS, physical iPhone, and physical iPad acceptance is recorded;
5. complete-game and touch/controller matrices pass;
6. performance, thermal, memory, audio, save, and lifecycle reports meet
   requirements;
7. no S0/S1 blocker remains;
8. no required content ID lacks coverage;
9. licensing/provenance/SBOM/corresponding-source obligations are complete;
10. public claims match the evidence; and
11. `docs/status/CURRENT.md` has no next required action.

If a user chooses to ship a preview before completion, record it as a preview
with explicit open gates; do not mark the master goal complete.

## 12. Ready-to-use master goal prompt

The following block may be supplied to the implementation agent as its durable
goal. The agent must still read the full PRD and loop.

```text
Build BlueWake to completion according to docs/PRD.md, using
docs/GOAL_LOOP.md as the mandatory operating procedure.

Work persistently through the earliest unmet gate. Start with macOS and prove
authentic normal boot, controllable gameplay, real transitions, valid saves,
audio, representative compatibility, and sustained original game speed before
building the shared iPhone/iPad product. Then implement the SunPad-derived
three-dot menu, editable touch interface, normalized controller/touch input,
staged Files import, save/data separation, lifecycle recovery, guided bounded
diagnostics, Simulator matrices, physical-device matrices, exhaustive content
coverage, and release audits.

Use public reference repositories under ignored ref/ and pin every dependency.
Use a user-owned GZLE01 image only as private local input. Never download,
commit, publish, or expose Nintendo data, original binaries, generated game
code, saves, captures, device data, signing material, credentials, or leaked
source. Preserve all user data and unrelated work.

Static recompilation, source reconstruction, compatible subsystem replacement,
and evidence-based route pivots are allowed. Runtime PowerPC JIT and captured
emulator state as normal initialization are not. Interpreter fallback may be
used as an instrumented bring-up/lockstep oracle, but the release corpus must
record zero interpreter-executed guest instructions. Do not hide missing
behavior behind silent stubs. Do not conflate compilation, link, installation,
PID, rendering, Simulator, device launch, gameplay, performance, or
complete-game acceptance.

For every iteration: load persistent state; select the earliest blocker;
reproduce and classify it; reduce it; research the narrowest relevant sources;
form one falsifiable hypothesis; implement the smallest change; run tiered
verification and oracle comparison; record evidence; checkpoint coherent green
work; and immediately continue. After three materially identical failures,
change the experiment, add instrumentation, decompose the blocker, consult a
new donor, or trigger the documented subsystem/route pivot. Do not blindly
repeat.

The overall goal ends only when every PRD definition-of-done requirement passes
with current reproducible evidence and no required work remains. A failed
architecture gate redirects the project; it does not complete or abandon the
BlueWake goal.
```
compose the seam branch that converts. Two earlier conclusions are corrected: the "uninitialized stage
object" reading was a consequence of this, and the capacity `271` measured since says `mpRoomDt` is a
host-initialized object after all. **(31) DONE - the conversion is in the type, and the endian reading is
withdrawn.** Patch 0056 makes the stage file's structural fields endian-aware at the type level:
`dStageFileS32`/`U32`/`U16`/`F32` are `bluewake::route_b::BigEndian<T>` on the host and
`dStage_nodeHeader`'s `m_entryNum`/`m_offset` are those, while `m_tag` stays a plain integer - so tags
compare as byte strings and every count/offset read converts, on the room path too, because it is the
same struct. The `172` count was read off a pointer the instrument had mis-cast, so it is not a swap
artefact. What remains is the crash itself: `realloc(stage->mHostActorEntries, ...)` with `stage` being
`mpRoomDt`, whose host fields are neither `0` nor null in the two readings this run produced (`-1`,
`271`) - so that object has not been through `dStage_roomDt_c::init()` on this path. **(32) The next
step:** print `mpRoomDt`, its `mHostActorCapacity` and `mHostActorEntries` at `objectSetCheck`'s reload
call and again from `dStage_dt_c_roomLoader` if and when it calls `i_stage->init()`. **(32) DONE - the
object is initialized and passed correctly, and a third layout is where the crash reads.** The run shows,
in order: `dStage_dt_c_roomLoader` running with `i_stage->init()` setting capacity 0 and a null entries
pointer (`cap=0 entries=0x0 off=472 sz=552`); the first actor node growing it (`num=2 cap=0`); then
`objectSetCheck` calling the reloader with the same object now initialized (`cap=271`, entries on the
sanitizer heap, same `off=472 sz=552`). The failing call reads different state from the same pointer:
`num=172 cap=-1 entries=0xbf800000`, and `0xbf800000` is the bit pattern of `-1.0f`, so the capacity and
entries fields are not where the callee looks for them. The reloader and the actor handler both come from
the ROOM_RELOADER tier's translation unit, the third view of `dStage_roomDt_c` in this composition and
the one layout not yet printed. **(33) The next step:** print
`offsetof(dStage_roomDt_c, mHostActorEntries)` and `sizeof(dStage_roomDt_c)` inside
`dStage_dt_c_roomReLoader` (that region needs `<cstddef>`/`<cstdio>`); `472`/`552` are the numbers the
other two tiers report, so a different pair means two host layouts for one class in one composition and
the fix is at the header the tier includes. **(33) DONE - the layouts agree, so the callee is reading the
right object as the wrong class.** The ROOM_RELOADER tier's own print reads
`BW-RELOADER stage=0x106b783c0 cap=271 entries=0x62100061d100 off=472 sz=552` - the same `472`/`552` the
other two translation units report, a capacity the first actor node legitimately grew to `271`, and an
entries pointer on the sanitizer heap. So the class has one host layout here and the pointer is right.
But the failing call prints `num=172 cap=-1 entries=0xbf800000`, and `0xbf800000` is `-1.0f`: reading a
valid `dStage_roomDt_c` and getting a float's bit pattern out of its capacity field means the *body* is
dereferencing a different type. `d_stage.cpp` has more than one host body for the symbol the loader's
table names - one casting the argument to `dStage_stageDt_c*` (line 1117 after the revert), another the
console body using `i_stage->getRoomNo()` (2107) - so the link resolves both callers to whichever copy it
kept and the room path can execute the stage path's arithmetic. **(34) The next step:** give each caller
its own name - a distinct symbol for the stage body, with the stage loader's table pointed at it, so
`dStage_actorInit` means the room body for the room path. **(34) DONE - the room's actor handler has its
own name, and the run moves to a new defect.** `dStage_actorInit` had one body casting its argument to
`dStage_stageDt_c*` for the stage's actor pool and another casting to `dStage_roomDt_c*` for the room's,
and both callers named the same symbol, so the room tables got the stage body - the source of `cap=-1` and
the `-1.0f` bit pattern where the room's entries pointer should be. Patch 0286 names the room body
`dStage_roomActorInit` and points the room side at it (the room-reloader region's declaration, the
treasure helper, the layer loader's entry, the reload table's `ACTR`/`TGOB` entries), while the stage
tables keep `dStage_actorInit`. In the same patch the five room tiers' `dStage_roomDt_c::init` bodies
give way to their owner behind `BLUEWAKE_ROUTE_B_ROOM_CONTROL_INIT_COMPOSED`, leaving one definition and
one typeinfo record. The composition then links with no duplicate and no undefined symbol, and the
bogus-`realloc` crash is gone: the room path now walks the actor records and stops on a
**heap-buffer-overflow in `cXyz::set`** (`c_xyz.h` 92) inside the actor decode. **(35) The next step:**
that is a decode-size question - which record length the actor decode assumes against the length the
room's file provides - rather than a class or layout one. Note
