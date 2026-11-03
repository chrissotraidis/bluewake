# PLAYER camera admission frontier — 2026-09-06

Investigation history from `c7ac69a`; BW-P4-0126 remains active. The resulting
headless 180-frame increment is now fully regression-qualified in
`ROUTE_B_PLAYER_DRAW_2026-09-06.md`. Earlier pending/running/failure statements
below are historical experiments, not current process status. No gameplay claim.

## Reached evidence

### Latest: strict 180 PLAYER draw frames with actual model packets

After invoking the original list constructor and removing the obsolete line
draw fence, strict PLAYER draw returns (`/tmp/bluewake-player-shadow-lifetime-
run2.log`). Empty-after-reset checks across all 15 original draw buffers and
bounded packet/shape traversal then prove actual Link packets: each matching
shape model's joint-zero matrix pointer equals PLAYER's published joint-zero
matrix. Original default opaque/translucent buffers are restored after drawing.
The one-draw strict run reports 26 packets and 21 body-shape references.

Promoted that check into every held/released frame, not just the last update.
Strict 180 input/update/collision/camera/PLAYER draw iterations pass. First and
last report 26 packets/21 body-shape references; every frame requires nonempty
actual-Link submissions. Motion remains 105.843, 119 changed foot matrices,
release ends at zero speed, camera displacement 331.581. This is queue-entry
evidence, not GPU consumption or a rendered screen.

Review corrected reset ordering: dComIfGd_reset now precedes camera_execute/
camera_draw so map/2D enqueue work is preserved, rather than clearing it just
before PLAYER draw. Corrected strict replay passes in
/tmp/bluewake-player-draw-loop-sanitize-run.log. Corrected Debug and Release
replays also pass (matching -debug-run.log and -release-run.log; pipeline 7738
completed zero). All 20 strict private probes and separate event identity also
pass in the broader matrix; Debug/Release broad matrix remains running.
Debug/Release audio reset focused tests completed successfully (pipeline8663).
Full public builds pass all modes; CTest execution pending. No stable checkpoint
or full regression claim.
No GUI launched. Complete normal scene/environment scheduling and teardown
remain open. Next all-mode/full regression qualification, then visible frame/
live-input integration toward gameplay.

### Latest: original KANKYO creation, real lighting/room data, shadow lifetime

The diagnostic registry now admits the unmodified original KANKYO profile
through fpcBs_Create/SubCreate after original scene phase one. Original
envcolor_init, light/wave/sound/wind initialization completes. Real sea stage
EnvR (50), Colo (5), Pale (33), Virt (31) records pass bounds and existing
native metadata loaders; environment pointers are checked against those stage
owners, not fallback palettes. This does not establish continuous environment
execution/draw scheduling or full room-scene boot.

0205 shares original scene pause storage/countdown and menu pause state/query/
setter, resolving process pause checks without fabricated no-menu returns.
0206 replaces three empty upstream audio position-reset methods with retail
semantics. Locked DOL SHA1 8d28bab68bb5078c38e43f29206f0bd01f7e7a67 shows
12-byte routines at 802AD008/802AD98C/802ADE68: load zero, store 32 bits to
member 1b80/1dd0/1ec0, return. The old source comment incorrectly described the
sea routine as four bytes; pinned symbols and actual bytes establish twelve.
All three corresponding native fields now clear. Updated dirtied-storage test
checks those fields, every other byte, singleton identity and idempotence.
That focused test passes Debug, Release and strict ASan/UBSan. Old 2026-09-02
empty-body evidence is explicitly superseded, not treated as gameplay proof.

Initial integrated link had duplicate audio singleton storage; PLAYER now
uses JAIZelAtmos position methods without the isolated test's extra singleton.
Strict KANKYO create and 180 locomotion/camera updates pass. PLAYER draw then
reached null FILI. Diagnostic observation establishes start/stay/actor/lighting
rooms ALL equal 44; the issue was missing room metadata, not wrong room choice.
The real Room44 FILI now passes original native conversion and raw-value checks.
Whole room native-view ownership replaces closed map accessors and the older
room-init variant, preserving native decoded metadata lifetime.

Strict run then reaches original real-shadow collision and fails at the virtual
shadow polygon receiver (d_drawlist.cpp:1098). The game-info diagnostic is raw
zeroed storage: list.init allocated buffers but never constructed embedded
polymorphic receivers. Added the original dDlst_list_c constructor BEFORE init;
this exposes an obsolete 3D-line draw fence conflicting with the already-linked
original implementation. Removed that fence only from PLAYER execution.
Shadow-lifetime replay is pending; no completed PLAYER draw/submission claim.

Evidence logs: /tmp/bluewake-player-kankyo-create-build5.log and -run5.log;
/tmp/bluewake-player-room-admission-run.log; /tmp/bluewake-player-room-fili-run.log;
/tmp/bluewake-player-shadow-lifetime-build.log (duplicate fence).
Pause patch export baseline /tmp/bluewake-scene-pause.v4zb4X/index.
prepare_route_b --check and both protected hashes pass; full matrix/checkpoint
still pending, stable c7ac69a. No GUI/Simulator launched.

### Latest: complete draw-list owner links; PLAYER draw reaches environment init

Compiled the complete original d_drawlist.cpp, without a new source tier or
altering its methods. First compile exposed missing embedded graphics assets.
Extended the pinned-DOL extractor with the remaining draw-list display lists
and native-endian Vec arrays, using the original symbols/config descriptors.
Outputs remain ignored generated data; 11 synthetic asset tests pass, including
all new byte descriptors, vertex bit-preserving conversion and descriptor
rejection. The locked private DOL produces 33 headers. Strict full-source
compile succeeds (`/tmp/bluewake-drawlist-full-build2.log`).

PLAYER now replaces the mirror, camera-line, window, 2D and insertion slices
with this full owner; a J2D-only object group preserves the other J2D sources.
Its original toon static storage replaces execute-diagnostic definitions.
Original drawlist.init/reset allocate and reset draw buffers, real shadows and
alpha models. The same Always raw I4 resource used by original logo startup
is supplied to original setSimpleTex. This uses retained diagnostic root
allocation, not complete mDoGph_Create/display boot or teardown qualification.

Strict PLAYER links (`/tmp/bluewake-player-full-drawlist-init-build.log`) and
passes the previous 180 camera/movement checks. Actual PLAYER draw is entered,
then UBSan exits 134 at d_kankyo.cpp:699: null mpSchejule in setLight_palno_get.
It has not returned or proved PLAYER draw submissions. Run evidence:
`/tmp/bluewake-player-full-drawlist-init-run.log`; session 21552 terminal.
No live builds/probes or new GUI instance. The five old missing links are gone.

Source review identifies original envcolor_init as the schedule publisher,
called by original dKy_Create together with light/wave/audio/wind initialization.
Next compose that scene/environment owner with authentic stage palette data;
do not fix the downstream failure by assigning only mpSchejule or bypassing
lighting. Full regressions and checkpoint remain pending; stable c7ac69a.

### Latest: camera draw/contracts pass; PLAYER shadow composition next

Review found upstream sceneChange and load2ndDynamicWave are logging-only
stubs. They must not be admitted as behavior. 0204 now also shares original
isDemo; explicit abort fences at setScene/load1stDynamicWave/load2ndDynamicWave
measure the unqualified transition boundary without fabricated success.
The first loader has prior isolated implementation evidence but its complete
runtime ownership is not established here. This does not qualify any of the
three fenced operations or audio transitions.

With those fences, strict 180 original camera execute/draw updates pass and
none reaches the scene/wave boundary. Added checks validate projection and
projection-view finiteness/positive scales, view-inverse product (tolerance
0.125 for translated world coordinates), and audio listener eye/matrix pointer
identity. A strict replay with ONLY the final PLAYER draw call temporarily
withheld passes all these checks, motion and release-to-stop. The final log
line announces draw WIP but that verification run did not invoke PLAYER draw.
The draw call has been restored in the current worktree.

Original PLAYER draw now retains five missing links: translucent depth-sort
insertion; shadow simple texture; shadow addReal, setReal2 and setSimple.
Next compose original shadow/list state and initialization as a whole owner,
then execute actual PLAYER draw and verify submissions before visible testing.
No draw-priority/map/room bypass, no silent audio, no visible acceptance.

Evidence: `/tmp/bluewake-camera-draw-fenced-build.log` and `-run.log` (exit 0),
`/tmp/bluewake-player-draw-entry-build.log` (five links, exit 2),
`/tmp/bluewake-camera-projection-contract-build.log` and `-run.log` (exit 0,
PLAYER draw temporarily withheld). Latest session 25464 is terminal; no live
builds/probes. Current source with draw restored fails that known link frontier;
full regression pending, stable `c7ac69a`, WIP uncommitted.

### Latest: shared rendering/map state and camera audio group compose

0203 shares original AGB static flags, the original NTSC/PAL render-mode
table/pointer and bounded draw-list insertion. The insertion body is shared
through d_drawlist_set.inc, not replaced by an observer. The existing PLAYER
heap-support source owns the render-mode data; no diagnostic mode table or
AGB flags were introduced. These changes link in the strict composition.

0204 shares the original four camera-audio methods through
JAIZelCameraState.inc. Listener pointers/sentinel countdown, polygon/map state
and group-transition effects are unchanged. The initial isolated include set
missed the Camera definition; adding its existing JAIConst header fixes that.
Current strict build compiles and reaches four remaining links: isDemo,
setScene, load1stDynamicWave, load2ndDynamicWave. Source group-change logic
still invokes them normally. No successful stubs or forced state bypass.

Next original audio scene/demo selection, tables and dynamic-wave owners as
one composition, then actual camera_draw with listener identity/projection
controls and PLAYER draw. Do not create an isolated symbol checkpoint.
Render-mode data availability is not display initialization; draw-list
insertion availability is not a initialized frame/list lifetime.

Logs `/tmp/bluewake-camera-shared-state-build.log` (four camera-state links),
`/tmp/bluewake-camera-audio-state-build.log` (incomplete Camera declaration),
`/tmp/bluewake-camera-audio-state-build2.log` (four scene/wave links).
Session 90337 terminal exit 2. Export baseline for 0203/0204:
`/tmp/bluewake-camera-state.pgLzr4/index`. No live builds/probes; WIP remains
uncommitted, full regression pending, stable `c7ac69a`.

### Latest owner review: whole map and AGB sources compile

Reused the existing full-source map census (no MAP_ROOMINFO tier): complete
original d_map.cpp compiles unchanged in strict configuration. Adding that
whole object to PLAYER resolves map draw and reveals only the two AGB
notification methods resetCursor/MapNoSet beyond the prior six camera owners.
Both calls are guarded by actual optional AGB presence in the original map
source; no room number, map visibility or camera priority was forced.

Complete original d_a_agb.cpp also compiles with existing portability patches.
It now supplies both methods in the PLAYER composition. Link reaches its
static mFlags, whose real definition is in d_com_static.cpp but excluded by
the currently selected TAG_LIGHT_DATA tier. Do not create duplicate diagnostic
flags. This is link composition, not AGB activation or transfer acceptance.

Current seven links: four JAIZelBasic camera state methods, draw-list set,
render mode pointer, and daAgb_c::mFlags. Audio source review confirms that
setCameraGroupInfo can invoke setScene plus first/second dynamic-wave loads;
retain these effects and qualify reached branches, not a setter-only rewrite.
Render-mode storage and its NTSC table live in original m_Do_machine.cpp;
draw-list insertion has an original bounded implementation in d_drawlist.cpp.
Next compose these source-owned state/initialization groups together, then
actual camera draw/listener and PLAYER draw. No single-symbol checkpoint.

Logs: `/tmp/bluewake-camera-map-owner-build.log` (full map compile exit 0),
`/tmp/bluewake-camera-map-compose-build.log` (eight links),
`/tmp/bluewake-camera-map-agb-owner-build.log` (seven links). Latest session
22427 exited 2; all sessions terminal. WIP remains uncommitted and not
regression-qualified; stable `c7ac69a`, no external blocker.

### Latest: real STAG/valid view pass; original camera draw links seven owners

0202 composes the existing stage-info source with original common-game
inlines. Its isolated build's external declarations could not supply getSave/
initDan. Real sea STAG passes byte bounds and original publication, with
near=1 and far=160000. Original getSave copies the selected in-memory stage
record and initDan initializes dungeon-stage state when the stage changes;
these are actual effects, not fixture callbacks or CARD access.

Strict replay now passes 180 original camera_execute calls, real view
publication and positive ordered near/far checks, alongside ICE binding and
Link motion/release controls. This still does not validate draw projection.
Log `/tmp/bluewake-player-real-stag-run2.log`, build `-build2.log`; pipeline
55514 exited zero. 0202 baseline `/tmp/bluewake-stag-patch.zYbwAc/index`.

0201 now also exposes unchanged camera_draw for native composition. The
diagnostic calls it after each camera_execute. The strict link currently
fails with seven owners (pipeline 98655 exit 2): JAIZelBasic getCameraInfo,
getCameraMapInfo, setCameraGroupInfo, setCameraPolygonPos; dDlst_list_c::set;
mDoMch_render_c::mRenderModeObj; dMap_c::draw. Evidence:
`/tmp/bluewake-player-camera-draw-build.log`.

Next treat these as a coherent camera draw/listener/render-state composition:
inspect original implementations and existing qualified owners together,
retain genuine state/lifetime, then measure actual camera_draw and PLAYER
draw. Do not suppress map drawing by forcing draw priority or return fake
successful listener/draw calls. Current WIP does not link; stable `c7ac69a`
is unchanged. No live pipelines or external blocker.

### Latest: camera execute/view publication reaches missing STAG

Focused Debug/Release pipeline 18282 finished successfully, matching strict
body-camera results. Added ICE controls independently decode raw MAT3 count
and names, compare the loaded native material table and validate the original
BTK's one resolved material binding. Strict replay passes these controls and
locomotion/camera. The first raw lookup incorrectly assumed an archive name;
the corrected check uses the existing authoritative ICE resource index.

0201 exposes the unchanged original camera_execute function for native
composition (console keeps static linkage). Replacing direct body Run with
that function executes preparation, store and view_setup. Initial strict
replay passed, but source review identified zero-filled diagnostic STAG near/
far planes. New checks prove view publication targets the real camera and
require valid positive near/far. Runtime now correctly fails the near-plane
check on first camera update (exit 134), rather than claiming a usable view.

Next publish real retained sea STAG via its original initialization owner
with save/stage effects accounted for; do not assign invented projection
constants. Then verify view matrices/projection and progress original camera
draw/audio listener and PLAYER drawing. Body-camera evidence is not rendered
view acceptance. Full matrix/checkpoint still pending.

Logs: `/tmp/bluewake-player-ice-binding-build2.log` and `-run2.log`,
`/tmp/bluewake-player-camera-execute-build.log` and `-run.log`,
`/tmp/bluewake-player-camera-view-check-build.log` and `-run.log`.
Latest pipeline 78658 terminal exit 134. Patch 0201 export baseline:
`/tmp/bluewake-camera-patch.EdYaTL/index`. No live builds/probes remain.

### Latest: strict actual-PLAYER camera updates pass

Entering original scene initialization in the retained non-root game heap
preserves that heap on return; an assertion checks restoration. The diagnostic
then explicitly returns to its root-owned resource/ground setup. No heap
sizes or allocator semantics changed. Strict full locomotion replay passes.

Original camera initialize now receives the published real Link after phase
three and two neutral updates; original attention Init selects that owner.
Each held/released update runs original camera Run after collision Move.
Strict replay passes 180 camera updates with finite eye/center values, center
within 10,000 units of Link and horizontal center displacement 331.581.
Link moves 105.843 units over the held section and ends at zero speed after
release. The changed motion relative to the old 103.517 case is observed with
the active camera; no exact retail camera/movement trajectory claim is made.

Correction to the earlier identity assessment: the old diagnostic DID manually
publish the Link pointers immediately before phase two; it was the camera's
cached subject that remained the placeholder. Those manual publications are
now excluded from the execute path, where original phase one owns them.
The earlier assertion that global player slots stayed on the placeholder was
incorrect. Non-execute diagnostic probes retain their explicit publication.

Strict logs: `/tmp/bluewake-player-scene-heap-context-build2.log`,
`/tmp/bluewake-player-scene-heap-context-run.log`,
`/tmp/bluewake-player-follow-camera-build.log`,
`/tmp/bluewake-player-follow-camera-run.log`; latest strict session 58700
exited zero. Debug/Release focused replay pipeline 18282 is running.

This proves camera body updates, NOT camera profile execute/draw or visible
rendering. Original camera draw still owns view/projection publication and
audio listener/map updates; it must be integrated before claiming correct
PLAYER drawing. Next qualify ICE binding controls and all-mode replay, then
camera profile/view/draw and PLAYER draw with retained scene owners. Full
regression/checkpoint is pending; no normal boot or P4 acceptance.

### Latest: BMT conversion passes; diagnostic root-heap context exposed

Patch 0200 shares original BMT/BMTM conversion and material-table toon setup
with the PLAYER resource tier. Strict build and Always loading pass; original
play phase one returns, scene lookup succeeds, original PLAYER phase one
publishes both player identities and attaches the stage layer, and phase two
returns NEXT. This is still diagnostic phase selection, not full admission.

The subsequent 1 MiB ground allocation fails. Instrumented replay proves
root free space is 14,028,336 bytes, but current heap changed from root to
the adjusted material solid heap, whose remaining free space is zero.
Original JKRHeap construction automatically makes a new child current when
the old current heap is root. dMat_control_c::create constructs that child
before capturing the previous heap with mDoExt_setCurrentHeap; its final
restore therefore keeps the material heap current in this root-only setup.
This is a diagnostic initialization/context defect, not evidence to enlarge
heaps, change original allocator behavior or patch the material-name lookup.

Next establish the proper retained non-root game/scene heap context for
original scene initialization, validate restoration, and replay PLAYER then
camera integration. Add explicit real BMT material-name/BTK binding controls
before qualifying 0200. No broad regression or checkpoint yet.

Logs: `/tmp/bluewake-player-bmt-build2.log`, `/tmp/bluewake-player-bmt-run.log`,
`/tmp/bluewake-player-bmt-heap-build.log`, `/tmp/bluewake-player-bmt-heap-run.log`.
Latest session 45065 exited 134; no live builds/probes remain.
Patch export baseline: `/tmp/bluewake-bmt-patch.lettuG/index`.

### Latest continuation: original scene initialization reaches ICE material

The new native-bootstrap object compiles the original play phases with real
headers (no reduced play_bootstrap scene layout). A diagnostic play-phase-one
profile now dispatches through original fpcNd_Create and fopScn_Create before
Link allocation. Original play phase one executes and prints `sea:44`.
This is authentic framework/phase execution with a diagnostic phase selection,
not full PLAY profile admission or boot.

Original d_material.cpp, asynchronous d_resorce owners, DVD command/ripper/
thread objects now compose. Reached linker duplicates exposed obsolete
diagnostic command/archive heap identities; the PLAYER composition excludes
those fixtures and retains the existing original m_Do_ext storage/accessors.
Scene deletion remains an explicit abort boundary, not a successful callback.

Strict build passes. Runtime exits 134 in J3DAnmTextureSRTKey material-name
lookup at J3DAnimation.cpp:923: JUTNameTab address is `0xffffffffffffffff`.
Source audit identifies the upstream mismatch: the native system-resource
tier excludes the retail BMT conversion branch, so Always ICE index 0x41 is
still archive bytes when original dMat_control_c::create expects a native
J3DMaterialTable. Do not repair the downstream name lookup or invent a table.
Next admit original BMT loading/material setup with real ICE BMT/BTK evidence,
then replay scene initialization and player/camera admission. Inspect loader
byte order, pointer width and material ownership before claiming conversion.

Logs: `/tmp/bluewake-play-native-bootstrap-build.log`,
`/tmp/bluewake-player-scene-build{2,3,4,5}.log` and
`/tmp/bluewake-player-scene-run.log`. All build/run sessions are terminal;
latest pipeline 53549 exited 134. No broad regression or stable commit yet.

### Earlier missing-stage experiment

The existing camera probe constructs and follows a non-PLAYER actor before
Link construction. The locomotion diagnostic invokes original actor Create
and phase two, but skips original PLAYER phase one. Consequently the global
player slot and camera subject still refer to the earlier placeholder, not
the moving Link. Finite/following camera evidence from that earlier loop
cannot establish a PLAYER-following camera.

The execute diagnostic now attempts original PLAYER phase one inside its
actor callback before phase two, with player-slot and Link-slot assertions
after it. Strict compilation succeeds. Runtime exits 134 at the original
`fopAcM_setStageLayer` assertion `stageProc != NULL` (source assertion 238).
There is no registered play scene corresponding to room control's process
ID. No assertion or layer lookup has been bypassed.

Evidence: `/tmp/bluewake-player-camera-admission-build.log` and
`/tmp/bluewake-player-camera-admission-run.log`. Session 84007 completed.
This failing run is expected WIP, not new movement or camera acceptance.

## Source-owned integration order

Original PLAYER phase one publishes player/Link identity, attaches the actor
to the play-scene layer, and initializes attention position. Original play
scene phase one publishes its actual process ID to room control. A raw layer
is insufficient: fopScnM_SearchByID searches registered scene tags.

The room lifecycle probe has a diagnostic raw process-node layer, not the
missing registered original play-scene owner. The play bootstrap frontier's
header substitutes a reduced scene type, so it must not simply be combined
with full PLAYER headers and treated as layout-compatible.

Next compose the scene/node ownership and original play initialization with
the retained real resources, then complete original PLAYER admission and
camera initialization against the published Link. Use existing original
scene framework services; do not hand-insert a fake stage ID or substitute a
successful stage-layer helper. Follow with input/execute/collision/camera
updates in one retained scene, then real draw submission and visible testing.
Full normal boot, room requests/transitions, audio and save/reload remain open.

No GUI or Simulator was launched. No protected source, user save or game data
was changed. The stable checkpoint's tests remain historical evidence; the
new admission experiment intentionally does not reach those assertions yet.
