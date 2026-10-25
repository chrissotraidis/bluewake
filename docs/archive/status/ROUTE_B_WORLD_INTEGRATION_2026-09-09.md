# Route B world integration — 2026-09-09

Active BW-P4-0126, user-resumed whole-unit loop. Baseline `6949c27`, dirty WIP
preserved under ignored `local-research/checkpoints/reorientation-20260909-start`.
No new stable checkpoint or P4 acceptance yet. Actual Aurora/Metal submission now
runs 180 frames under strict sanitizers. After the envelope and original camera
operand repairs, identifiable Link remains visible beside original Outset
buildings in the first and final observed frames. Complete visual fidelity,
continuous live input and normal scene ownership remain open.

## Iteration 1 — restore coherent stage ownership

Hypothesis: world composition removes the shared runtime owner needed by the
eight September 7 missing symbols. Reconstructing 0210 in that owner and removing
the competing initial-room-request object will restore reproducible linking.

An isolated index reconstructed registered patches through 0209. The only
stage-file drift was the appended WORLD_ROOM_MEMORY block. Preserved that block,
removed it with a targeted edit, moved the original `getMemoryBlock` and
`dStage_roomInit` bodies from the full-unit branch into its existing shared
runtime include, and registered a regenerated 0210. This also preserves the
full-unit inclusion path without duplicate bodies. No new tier was introduced.

Results: source preparer check passes; strict world target builds/links (exit 0).
Original BG process creates three models and publishes one shared collision owner.
First world execution stops at the diagnostic `seen[0/1/3]` assertion (exit 134).
Protected recompcore files and dependency lock match the recorded SHA-256 values.
Logs: `/tmp/bluewake-reorientation-source-check.log`,
`/tmp/bluewake-reorientation-world-sanitize-{build,run}.log`.

## Iteration 2 — source-grounded visibility and missing stage placement

The independent report's F8 assertion is disproven by source and execution:
`J3DJoint::entryIn` skips `J3DShpFlag_Hide` before material-packet entry.
`J3DShape::hide()` sets Hide (0x1), distinct from packet Hidden (0x10).
Material sorting can also merge shape packets under a different material owner.
The diagnostic must verify visible shape ownership through those lists, not
require a top-level material packet from every model on every frame.

Replaced the invalid oracle with independent perspective half-plane tests of
all eight transformed joint-box corners, checked against original shape Hide
flags. Every expected visible shape must occur exactly once in the BG material
packets' shape lists with the correct model identity; hidden shapes must not.
Strict replay completes 180 frames, but all world shapes are culled and captures
remain Link-only at 85,024 bytes. This is not world rendering success.

Observed first-frame model origins (0,0,20000), (-20000,0,-20000), and (0,0,0)
versus camera eye approximately (-192563,652.5,318980). Stage MULT pointer is null.
Original BG create uses `dComIfGp_getMapTrans`, which returns false without MULT,
so required room transforms are never applied. The diagnostic's selected stage
metadata loads RTBL/STAG/lighting but omits MULT. This is another concrete reason
to replace selected-phase initialization with the complete scene/stage owners.

Next experiment reuses the existing native MULT loader and object target in the
world diagnostic, with serialized extent checks and required Room44 placement.
No fabricated transform or new tier. After its focused result, proceed to actual
Aurora processor/Metal observation rather than extending capture-only work.

Logs: `/tmp/bluewake-reorientation-world-packet-run.log`,
`/tmp/bluewake-reorientation-world-visibility-run.log`,
`/tmp/bluewake-reorientation-world-bounds-run.log`.

## Acceptance boundary

The strict 180-frame diagnostic with all BG shapes clipped does not close a
world-visible milestone. Normal scheduler, actual input, boot, populated audio
teardown and complete application lifetime remain open. Review findings are
advisory and are corrected when execution or source establishes contrary evidence.

## Iteration 3 — original room placement

Reused the existing native MULT loader/object with serialized extent checks.
Original BG create now receives the Room44 stage transform. Model origins become
(-200000,0,320000), (-220000,0,280000), and (-200000,0,300000).
The strict 180-frame world diagnostic passes the independent clipping/packet
oracle: visible shape counts begin at 8/8/0/1 and end at 7/3/0/1. Captures grow
to 215,712 bytes, and original delayed BG deletion completes. These are CPU-side
world integration results, not pixel or complete application-lifetime evidence.
Log: `/tmp/bluewake-reorientation-world-placement-run.log`.

## Iteration 4 — actual renderer exposes texture ownership failure

The first actual `--visible` run aborts in Aurora texture hashing with an ASan
global-buffer-overflow. Original material display-list BP registers describe a
large material texture, but the native slot still points to J3DSys NullTexData:
legacy image3 addresses do not carry the required host pointer. Additionally,
the default 4x4 IA8 texture needs 32 bytes; the original 16-byte declaration
relied on retail alignment padding.

Patch 0211 binds native GX texture objects after each original material list,
including palette, dimensions, filtering and LOD, and supplies the full native
fallback texture storage. It follows the existing Dusk J3D loadGX/material-load
pattern at local pin `5f0f3d4ebe6a866b09ff4b304dd90ca9642b5a50` (CC0); no dependency
repin. All six base/patched/locked material load paths are covered.
Source-series check and strict visible 180-frame execution pass. However a
native screenshot taken after the final-frame readiness marker is **black**.
Evidence remains private and ignored at
`local-research/evidence/reorientation-20260909/world-metal-frame180.png`.
Logs: `/tmp/bluewake-reorientation-native-textures-visible.log`,
`/tmp/bluewake-reorientation-world-observed-ready.log`.

## Iteration 5 — empty native J3D matrix operations

LLDB at Aurora's first draw shows a malformed position matrix and uninitialized
normal matrix despite finite values. Source inspection finds seven J3DTransform
operations with only `__MWERKS__` assembly and no host implementation, including
inverse-transpose, normal copies and draw-matrix array concatenation. The current
TWW upstream file also lacks scalar bodies; Dusk contains portable equivalents.
Patch 0212 adapts those operations inside the original full translation unit,
preserving the retail branches and singular-matrix no-write behavior.

A numerical regression covers nonuniform scale/shear, rotation, translation,
inverse-transpose, normal copies, projection multiplication, contiguous matrix
array stride, zero count and supported in-place operations. NaN-poisoned output
detects empty routines. Strict numerical checks pass, but the completed Metal
frame remains black. This disproves empty matrix operations as the sole cause.
Header-inline paired-single vector overloads still need a reachability
audit; this patch does not claim all assembly portability is complete.

Upstream inspected: [TWW J3DTransform](https://github.com/zeldaret/tww/blob/main/src/JSystem/J3DGraphBase/J3DTransform.cpp).
Portable precedent: `ref/dusk/libs/JSystem/src/J3DGraphBase/J3DTransform.cpp` and
its header at the pin above. No game data is included in these patches/tests.

## Iteration 6 — serialized DRW1 index owner

Compared the first original J3DFifoLoadPosMtxImm producer with Aurora's first
draw. The corrupt position matrix is already present in J3D: original
J3DShapeMtxConcatView receives matrix index 2048 where the serialized DRW1
entry is 8. The matrix lookup crosses allocations inside the custom game heap;
ASan's arena bounds do not detect that object-boundary violation. The restored
inverse-transpose routine consequently produces infinities from near-singular
garbage. Viewport, scissor, color writes and the camera view matrix are coherent.

Patch 0213 makes the borrowed DRW1 index table explicitly big-endian, using the
existing serialized-value type. It does not mutate the resource or allocate a
second table. Added raw-byte/index/owner-bound checks for all four BDLC models
in Link (six DRW1 indices) and owner bounds for all three BG models. The world
oracle also requires some visible BG geometry on every scripted frame.

Strict headless and actual Metal 180-frame runs pass. A native screenshot after
the final-frame marker now shows textured blue/green terrain surfaces instead
of black. **The view is malformed, and Link is not visibly identifiable.** This
establishes actual J3D pixels, not visual correctness or playable-world acceptance.
One screenshot attempt timed out as the bounded app exited; the next bounded
observation succeeded. The diagnostic app exits automatically after observation;
unrelated applications remain untouched.

Logs: `/tmp/bluewake-reorientation-j3d-producer.log`,
`/tmp/bluewake-reorientation-render-state-matrix.log`,
`/tmp/bluewake-reorientation-draw-indices-{headless,visible,observed}.log`.
Private screenshot alias: `world-metal-draw-indices-frame180.png` under the
ignored evidence directory above. SHA-256:
`aef5419d4c80068aef42d809b08543563847904194dc971831f038c109f12d09`.

Verification at this continuation boundary: source-series check through 0213
passes; `git diff --check` passes; protected recompcore files and dependency lock
retain their starting SHA-256 values. Strict numerical tests and strict
headless/Metal world runs pass. Debug world link passed before the later
texture/matrix changes and is not current qualification. Release and affected
broad regressions have not been rerun. Repository audit still reports the known
20 pre-existing tracked `local-research/` files; no new private evidence was
staged or tracked. Audit is not claimed green. Nothing was committed or pushed.

## Current continuation

### Resumed iteration 7 — weighted Link skeleton and actual character pixels

Whole-model inspection found `J3DModel::calcWeightEnvelopeMtx` empty in both
the pinned source and current TWW upstream. The active Link model has 42 joints,
120 weighted envelopes and 270 draw matrices. EVP1 indices, weights and inverse
joint matrices were also exposed as native scalars despite retaining serialized
byte order. Patch 0214 supplies the existing Dusk scalar matrix-blending
algorithm inside the original TWW model owner and makes all borrowed EVP1
tables explicitly big-endian. Dusk pin remains `5f0f3d4` (CC0), no repin.

An independent double-precision oracle reads the actual EVP1 bytes and checks
all 120 output matrices against 260 influences, weight sums, joint bounds and
scale flags on the first and last frames. Strict 180-frame execution passes.
An optional bounded selected-frame hold now supports numerical-to-visual
comparison without changing game ticks or introducing catch-up frames.

**Observed frame 1:** identifiable Link in his blue Outset clothing, standing
beside original buildings and cliffs. Private screenshot alias:
`world-metal-envelope-frame1.png`. This establishes actual character/world
pixels. Lighting/material fidelity, sky, effects and complete frame composition
are not qualified. The final camera still loses Link, as explained below.
Logs: `/tmp/bluewake-reorientation-envelope-run.log` and
`/tmp/bluewake-reorientation-envelope-visible.log`.

### Resumed iteration 8 — original camera interpolation operand

The camera-space center of Link's body starts at approximately
(0.8,-17.5,-157.0) and ends at (324.5,91.3,-91.5), far outside the horizontal
frustum. His attention position remains attached to his body; camera center
drifts about 337 units in Z away from it. This is not an Aurora projection error.

Source `followCamera` constructs a per-axis smoothing vector, then erroneously
adds the componentwise **square of the position error** to its center. Current
TWW upstream retains that expression. Read-only disassembly of the user's
private game image confirms the original call at `0x8016B438` takes the error
temporary and the separately constructed gain vector; symbols identify the
called componentwise multiplication. Dusk's corresponding camera also uses
the gain vector. Patch 0215 corrects that one operand in the original function.
No replacement camera, fabricated transform or fixed screen position.

The route now checks that Link's projected body center is inside the frustum
after released-input settling. Strict execution passes all 180 frames. Final
body-center camera coordinates are approximately (-0.18,-17.5,-260.4), safely
inside the view. Link displacement is 104.331 units and camera center displacement
105.693, replacing the erroneous 331.581-unit camera drift. The observed final
frame clearly retains Link and original buildings/cliffs. Private binary
analysis remains ignored (`follow-camera-retail.txt`); no original binary or
disassembly is included in public patches. Next coherent regression qualification,
live input and complete scene ownership.

Private visual evidence SHA-256:

- `world-metal-envelope-frame1.png`:
  `e6414df0c9360be17b4e6962c939d947010bf4f1a420ac4c8e653990805fe585`
- `world-metal-follow-frame180.png`:
  `5ee1be23258f3d2f1f0f103e075bc509503ff4249287d1ec9c58e25b155bdb58`

## Combined qualification — 0210 through 0215

Actual Metal first/final observations and the complete 180-frame strict run pass.
Debug, Release and strict sanitizer configurations each pass:

- Public CTest: 82/82.
- Private integration matrix: 23 executables, including world/Link rendering,
  the independent weighted-skeleton checks and numerical J3D matrix test.
- Separate original PLAYER event-identity control.
- Animation parameter oracle: 11,076 cases; emission oracle: 12,880 cases.

The 11 asset-preparer tests and both TWW/Aurora source checks pass. All three
production link censuses remain 4/41/52.
Those unresolved production owners still block complete scene integration.
Logs are under `/tmp/bluewake-reorientation-{public,private,census}-*` and
`/tmp/bluewake-reorientation-{debug,release,sanitize}-*-oracle.log`.

`git diff --check` passes. The protected recompcore files and dependency lock
retain their starting SHA-256 values. Repository audit reports exactly the known
20 baseline tracked research metadata files; no new category or private material
was introduced. The whole audit is not claimed green. Private screenshots and
retail analysis remain ignored. This qualifies the combined checkpoint from
`6949c27`; the durable goal remains active and full PRD acceptance remains open.

## Next experiment — continuous live input

Hypothesis: Aurora's existing real PAD path can drive the same original Link,
collision, camera and renderer owners continuously after removing scripted
virtual input. Aurora keyboard defaults are unbound; configure explicit local
bindings without writing user preferences. Pump input before the original PAD
read, observe press/release and displacement in the actual window, then close
through the diagnostic's cleanup path. A finite scripted replay does not prove
this hypothesis. Record any new reachable game-owner failure rather than hiding
it with a success stub or alternate movement/camera implementation.

Then replace hand-called phases with complete ROOM_SCENE/PLAYER scheduling and
Painter ownership. Raw global fixture lifetime, pending room requests, full sky/
effects/audio and normal boot remain unqualified. Keep Route A frozen and the
full PRD acceptance criteria unchanged. Avoid another series of isolated source
slices or treating submission counts as progress toward visual correctness.

## Live-input iteration 1 — WIP after stable ab8a9e9

The combined rendering checkpoint `ab8a9e9` is committed and pushed. A world-only
`--live` path now builds on the same owner composition: clear virtual PAD, install
process-local WASD/C-stick/button mappings, pump events before original CPAD,
then original Link update/collision/camera/draw until Escape or window close.
It logs actual movement/release transitions and displacement. Scripted mode
retains its 180-frame assertions; live input retains per-shape clipping/ownership
checks while allowing a camera to face away from the world.

Debug, Release and strict builds pass. Strict scripted world and Link replays
still pass 180 frames. The first actual live run completes 1,574 neutral frames
and exits via Escape, original BG deletion and Aurora shutdown, exit 0. Automated
W taps reach SDL as down/up pairs but PAD never samples a held axis: movement and
displacement remain zero. These very short injected taps do not establish held
keyboard behavior. The second instrumented session was closed through Escape after 17,755
updates (17,754 presented; close arrived between update and submission). It exited
0 through BG/renderer cleanup with zero movement. No human held-key response
arrived; the window is now closed. Do not claim live movement from neutral frames.

Logs: `/tmp/bluewake-live-input-{build,world-headless,player-headless,visible}.log`
and `/tmp/bluewake-live-input-{keys,debug-build,release-build}.log`. Full scene
teardown remains unqualified because the parent diagnostic owns raw global
fixtures and exits with `_Exit` after its bounded owner cleanup. No new live-input
checkpoint; preserve this WIP until the focused movement hypothesis is measured.

## Whole-unit continuation measurement

While the human input observation is pending, isolated compilation with the
current native flags and **no tier macro** confirms full `d_s_room.cpp`,
`d_s_play.cpp` and `d_a_bg.cpp` compile without errors. Full `d_stage.cpp` produces
14 errors: eight serialized-header-to-pointer casts and six pointer-narrowing
casts in room/path relocation. Merely silencing the casts would not establish
correct native record ownership; compose the existing native view/decode owners.

An actual isolated link adds the complete ROOM_SCENE object and forces retention
of `g_profile_ROOM_SCENE` alongside the current world executable's objects and
libraries. It has no duplicate-definition failure and reaches **10 unresolved
symbols** (the earlier standalone-object comparison had 17 game dependencies;
archive extraction resolves several and exposes connected particle owners):

- `dComLbG_PhaseHandler`;
- original room loader and reloader;
- `daNpc_Md_c::m_seaTalk` and `daSalvage_c::init_room`;
- particle manager clear, emitter index-duplication check, resource-manager
  constructor/texture replacement, and `dPa_name::s_o_id`.

Existing complete particle resource/manager objects and native room-loader
composition are available in CMake. Salvage room reset and Medli static-state
owners need source/provenance verification. Next compose these owners and the
original profile/scheduler path, preserving actual particle and room lifetimes.
This link experiment is not a scheduler execution or a changed production census.
Evidence lives under the ignored `whole-owners/` evidence directory. Neither the
normal build target nor the reference source was changed by the experiment.

## ROOM_SCENE composition — patches 0216–0217, unqualified WIP

The world target now registers the original full `g_profile_ROOM_SCENE`, compiles
complete `d_s_room.cpp` and `d_com_lib_game.cpp`, and links an archive made from
existing complete JParticle resource/manager/emitter objects. The world-specific
`createSimpleEmitterID` abort fence is removed. Native room loader/reloader and
player/ship request objects are composed; no new tier macro is introduced.

**0216 (ownership correction):** the native inline salvage wrapper and Medli
state match the pinned source behavior and the inspected retail instructions.
The earlier note incorrectly called their upstream implementations absent:
`src/d/d_com_static.cpp` already defines both. Inspecting the actor headers alone
missed that shared resident-state unit. 0221 composes that complete unit, keeps
one native inline definition for the standalone room consumers, and excludes
its duplicate native definitions in the full source. The original source is the
owner reference; retail analysis was corroboration, not necessary reconstruction.
The independent report itself remains preserved unchanged.

**0217:** RCAM, LGHT, LGTV and FLOR were abort-only absent-tag fixtures in the
old room lifecycle tests. The new shared source handlers retain archive offsets,
borrow byte-only camera records, decode native light/floor scalar arrays, and own
those arrays on the stage/room data object. Original init and base destruction
clear this storage. Complete `d_stage.cpp` and current native room composition
use the same handlers. Eight obsolete test fence definitions are removed.

Focused strict `bluewake_route_b_room_native_views_test` passes after extension:
raw-byte float/stride/tail checks, unchanged archives, native wrapper identity,
zero light-vector count, allocation reuse and reset; plus null/populated salvage
controller cleanup using real emitter objects, sea-entry preservation, room
emitter invalidation and Medli state toggling. Complete native ROOM_SCENE,
PLAY_SCENE and BG still compile; full stage now has 12 compile errors (down from
14). Source-series check through 0217 and whitespace check pass. Protected file
hashes remain unchanged; report preserved unchanged.

The actual world link progresses through the initial full-room/particle gaps,
then exposes **18 remaining undefined symbols**, from diagnostic PLAYER/SHIP
stage slices: 14 global declarations for getters that are normally inline in
`d_com_inf_game.h`, plus ship lookup, player-existence, Ikada initialization and
scene-name hooks. These are a composition boundary, not missing source evidence.
The metadata test passed separately because the failing world link prevents a
combined target build from reaching later targets. No new world runtime or broad
regression qualification, checkpoint, scheduler execution or gameplay acceptance.

After repeated link-only iterations, **change the experiment shape:** integrate
the authentic stage request owners with their real native game-info/process
context and share native data adaptations with the complete stage unit. Do not
fill the world test with substitute getter definitions or cancel the original
PLAYER request. Preserve the original Ikada/ship branches and their effects.
Then run original ROOM_SCENE creation through the request scheduler and measure
the first runtime failure. Hand-called PLAYER/BG phases remain temporary debt.

Latest logs: `/tmp/bluewake-room-composition-build{2,3,4}.log`,
`/tmp/bluewake-room-metadata-{source-check,test-build,test-run}.log`,
`/tmp/bluewake-resident-room-cleanup-test-{build3,run}.log`.
Stable remains committed/pushed `ab8a9e9`. Current combined WIP does not link;
no new checkpoint or goal completion is claimed. No BlueWake GUI remains open.


## Native stage request context — 0218

The combined world target now links, replacing the previous 18 undefined
PLAYER/SHIP contracts with original inline game-info accessors and actual
process lookup. PLYR and SHIP native views live in shared source includes used
by both the existing composition and complete `d_stage.cpp`. Isolated tests
retain controlled fixture context; the world does not define substitute getters.
No new tier macro is added. Complete stage compilation now reports 11 remaining
serialized relocation/path errors, down from 12; it is not yet a linkable whole
owner.

PLAYER lookup uses the actual game-info slot; the original actor-name table
selects the creation profile/arguments. Scene lookup follows the real room-control
process ID. TITLE, METER and AGB requests remain intact. Ship lookup uses the
original actor search and the already-linked complete `d_a_ship_static.cpp`
placement owner, including position/angle, gravity and emitter cleanup. Ship
and raft lifecycle behavior is not qualified merely because these owners link.

The native raft decoder searches ACTR then ACT0–ACTb by the original name and
ship-ID rules. It decodes serialized records into caller-owned native storage,
leaves the archive unchanged, and keeps the original matrix transform, sea
execution/height lookup, room/ship parameter and rotation effects. A raw-byte
fixture calls the actual raft initializer through real game-info state. It
checks ACTR priority despite reversed archive order, wrong-ID rejection, ACT0
fallback, quarter-turn/identity positions, packed angles/parameters, and archive
immutability. This fixture covers inactive sea; active wave effects remain open.
State and matrix stack are restored before world execution.

Strict ASan/UBSan execution passes the raft checks and all 180 original world
input/update/collision/camera/draw iterations. Final locomotion displacement is
104.331; released input settles to zero. Independent BG clipping/packet ownership
and all 120 envelope matrices/260 raw influences still pass. Focused PLAYER
request, SHIP state and room native-view/cleanup tests pass individually under
the same strict options. CTest discovers no tests in this build configuration;
the three actual executables were run directly. This is focused sanitizer
qualification, not the all-mode regression matrix or a new visible observation.

Logs: `/tmp/bluewake-stage-native-requests-{build,world-run,oracle-build,oracle-run,tests-build,tests-run,check}.log`.
Complete-stage compile evidence: ignored `whole-owners/d_stage-0218.log`.

**Next experiment:** run original ROOM_SCENE creation through the original
request scheduler and preserve its PLAYER/METER/AGB/BG and actor requests. The
current replay still hand-calls PLAYER/BG phases; registering the complete room
profile alone does not execute its lifecycle. Record the first runtime boundary,
then replace diagnostic PLAYER phase ownership and integrate the original
scheduler/Painter. Continue retiring stage slices by sharing valid native views
with the full unit. Stable checkpoint remains `ab8a9e9`; 0216–0218 WIP is not
committed or broadly qualified. Goal active, no external blocker, no GUI open.


## Original room-request scheduler and complete HUD composition

The world probe now has a headless `--scheduler` experiment. It selects the
original `g_profile_PLAYER` with native actor layout, requests Room44 through
`dStage_roomInit`, and pumps original scene-request and creation handlers.
PLAY_SCENE phase one and KANKYO remain diagnostic bootstrap steps; scheduler
mode now obtains their IDs from the original ID generator to avoid collisions
with subsequent child IDs. The preceding camera fixture releases only its
verified stack-actor player slot before room admission. Camera/attention and
pre-mounted resource setup still remain bootstrap debt.

This experiment reaches original ROOM_SCENE phase two and its PLYR handler.
The first real source failure is ASan `strcpy-param-overlap` in
`dStage_startStage_c::set`: PLYR passes `getStartStageName()`, aliasing the setter's
own name buffer. 0219 preserves that name while updating room/point/layer.
A direct self-name/control check and the next strict scheduler run pass this
boundary. That run records original ROOM_SCENE profile 20 and child PLAYER 169
and METER 497 requests. The observer stops at missing METER registration before
its request is canceled; the room loader also creates the original AGB request.
No child cancellation or successful replacement profile is introduced. Request
creation is not completed actor admission.

Complete METER source initially has three C++ narrowing errors in one grayscale
color expression. 0220 makes the original byte conversion explicit and gives
its clock-pane global a native-only name to avoid the host C `clock` function.
The original METER and AGB profiles are now registered. Two narrower METER HIO/
pause objects are removed from the world graph in favor of the complete unit.

Full `d_com_static.cpp` exposes three compile errors: duplicate native owners
from 0216 and an untyped actor-name query. 0221 reconciles the duplicates and
uses the known `daStandItem_c` type for the query. The world replaces both the
thin resident-state source and camera ship-offset object with the complete unit.
Its original salvage globals replace the diagnostic pointer and `-1` ID fixture;
the original BSS ID starts at zero. This change requires runtime requalification.
The earlier claim that the salvage/Medli implementations were absent is corrected
above. Source ownership already existed in the pinned shared unit.

Complete J2DScreen, J2DWindow, d_metronome, d_timer, d_item_data,
d_a_npc_cb1_static and m_Do_hostIO are also composed. Timer compilation exposes
three implicit 64-to-32 conversions; 0222 preserves the declared signed 32-bit
millisecond return width explicitly. These whole units compile together without
new tier macros. Full HUD/instrument/GBA runtime is not yet qualified.

**Historical intermediate world link stopped at nine symbols (superseded below):**

- `GBAGetProcessStatus` — original SDK status owner and GBA control state.
- `dMsg_getAgbWorkArea` — complete message unit owns the work-area state.
- `dStage_checkRestart` — original stage restart decision remains outside the
  currently composed stage runtime.
- J2DTextBox stream constructor, JSUMemoryInputStream buffer setter and vtable —
  existing complete opening J2D owners can supply the missing resource readers.
- `JAIZelInst::getMelodyPattern` — original instrument unit and melody tables.
- `JAIZelBasic::heartGaugeOn` — original audio state update.
- MyScreen vtable — complete `d_file_error.cpp` owns its destructor.

This is two measured link steps, 26 then nine gaps; no new successful world
runtime exists after full HUD/resident composition. Latest compiled source is
not a runnable qualified checkpoint. Next compose those complete message,
resource-reader, instrument and SDK owners, sharing original stage/audio state
methods with their full units, then resume the same scheduler experiment. Keep
all original requests and effects. After three link-only attempts, change the
experiment shape per the goal loop instead of accumulating getter substitutes.

Evidence: `/tmp/bluewake-room-scheduler-run{,2,3,4}.log` (the last reaches the
METER registration boundary); `/tmp/bluewake-room-scheduler-meter-build2.log`
(26 gaps), `/tmp/bluewake-room-scheduler-resident-build2.log` (nine gaps).
Complete-unit compiler evidence is also under ignored `whole-owners/`.
Source replay through 0222 is checked separately. No new all-mode regression,
visible/live movement qualification, commit or push. Stable remains `ab8a9e9`;
full goal active, no external block, no BlueWake GUI open.


Focused shared-runtime regression after these changes: the strict
`bluewake_route_b_private_player_render_probe` builds and passes all 180 frames,
including the new start-stage self-name/control assertions and the existing
input, camera, active-array and envelope checks. Logs:
`/tmp/bluewake-room-scheduler-player-regression-{build,run}.log`.
This executable retains the prior PLAYER-only diagnostic graph and does not
qualify the expanded full HUD/resident world graph, which still fails link.
Source replay through 0222, whitespace checks and protected-file hashes pass.

## Expanded world passes; original startup resource handoff is next

This continuation supersedes the nine-symbol link frontier above. Complete
`d_msg.cpp`, `d_file_error.cpp`, `JAIZelInst.cpp`, `GBA.c` and
`GBAGetProcessStatus.c` compile and are composed into the world, along with an
archive of existing complete opening J2D owners. Patch 0223 shares the original
stage restart decision and audio heart-state method with their native owners.
No new tier macro is introduced.

The third link-only attempt leaves `JAIBasic::startSoundVec`, an empty pinned
body. The experiment changes to private retail instruction comparison and a
public sound-registration regression. Retail constructs an Actor record from
three copies of the supplied position pointer plus the supplied context, then
delegates to `startSoundActor`, preserving fade and owner. Patch 0224 restores
that wrapper on TARGET_PC. The strict regression checks null/non-null position,
full-width argument forwarding and a real SE allocation/stop/pool retirement.
The callback copies the stack record synchronously before the wrapper returns.
Private disassembly remains ignored; no retail bytes are added to public patches.

Complete owners now replace twelve instrument abort fences, the copied
instrument constructor, a timer deletion fence and five camera-global fixture
definitions in the world. The instrument global itself remains diagnostic-owned;
this does not qualify complete audio initialization, DSP or instrument playback.
The current world builds successfully and its strict default 180-frame replay
passes: final capture 234,656 bytes, displacement 104.331, released speed zero,
with original BG clipping/shape ownership and 120-envelope/260-influence checks.

Original `--scheduler` now enters METER Create. It fails UBSan at
`d_meter.cpp:7060`, a member call on a null J2DPicture. Read-only startup
instrumentation confirms menu, message, font, item-icon and action-icon archive
pointers are all null. METER's original screen setup uses the menu archive to
load `main_parts1/2/3.blo`; the diagnostic bootstrap has not supplied the original
startup resource context. This is a measured bootstrap omission, not evidence
that the original HUD needs a skipped pane or successful replacement loader.
PLAYER/METER/AGB requests exist; completed room/PLAYER admission remains unproven.

The original producer is `d_s_logo.cpp`: preload creates archive and common
particle commands; `dScnLogo_Delete` publishes more than twenty archives and
creates common particle resources before releasing command wrappers. Next
compose and execute that lifecycle before room admission. Do not set only the
menu archive or bypass the HUD failure. Complete logo compiles without a logo
tier both in clean native and actual world flags. An isolated full-logo profile
link against current world objects has 29 dependencies across GX/OS/VI startup,
JFWDisplay/JUTVideo/Xfb, audio wave initialization, ARAM/archive/font owners,
dynamic-module startup and actor data. This experiment does not change the
current world graph or executable. Reuse existing real owners where applicable;
older logo diagnostics have restricted tiers and are not proof of full startup.

Evidence: `/tmp/bluewake-vector-sound-test-{build,run}.log` passes;
`/tmp/bluewake-room-scheduler-owner-cleanup-build.log` and
`/tmp/bluewake-room-startup-context-build.log` pass;
`/tmp/bluewake-room-complete-owners-world-regression.log` passes;
`/tmp/bluewake-room-startup-context-run.log` records the five null archives and
strict scheduler failure. Ignored `whole-owners/d_s_logo-world-context.log`
records full compilation; `d_s_logo-world-link.log` records the 29 dependencies.
Source replay through 0224 passes (`/tmp/bluewake-room-message-final-source-check.log`).
Stable remains committed/pushed `ab8a9e9`; this WIP is uncommitted, with no new
all-mode regression, GUI observation, held-key movement or production census.
No GUI is running and the full PRD goal remains active without an external block.

## Full startup owner portability and actor-data loading

A compile experiment removes all BLUEWAKE defines from the native world flags
and checks thirteen complete startup dependencies. Five compile unchanged:
JUTXfb, JKRAram, JKRAramHeap, JKRAramArchive and m_Do_DVDError. The remaining
failures identify concrete portability work:

| Complete owner | Measured compile boundary |
|---|---|
| JFWDisplay | Two narrowing casts of the retrace message token |
| JUTVideo | Retrace counter cast to a native message pointer |
| JUTCacheFont | Six pointer/address arithmetic casts |
| JKRDvdArchive / JKRCompArchive | Unported serialized entry cache fields, offsets and native archive-entry widths |
| m_Do_audio | Five heap conversions lack the complete JKRExpHeap declaration |
| JAIZelBasic | The full unit omits the WaveBankMgr declaration available in its earlier wave tier |
| d_s_actor_data_mng | Five casts expose in-place 32-bit relocation of serialized actor data |

Ignored temporary copies prove that adding the two missing audio headers makes
both complete audio units compile. They do not yet change world composition or
qualify audio behavior. Compiler evidence is under `whole-owners/*-startup-full.log`
and `*-startup-includes.log`.

Patch 0225 ports the complete actor-data unit and removes its constructor-only
tier restriction. TARGET_PC reads big-endian block headers and string offsets,
keeps those serialized offsets intact and builds native pointer arrays owned by
the character-table object. Existing cDT lookup/index logic and byte-valued data
access remain original. Replacing a valid table releases its previous pointer
arrays; destruction releases the final arrays. Backing resource bytes remain
owned by their original resource lifetime. Console relocation is retained.

The strict public regression covers unaligned input, reordered/unknown tags,
duplicate-name ARG selection, every original format index, independent owners,
replacement and destruction. Optional private replay loads the real 1,984-byte
ActorDat.bin: 26 formats, 45 names and 1,170 data values match independently
decoded resource offsets/bytes, and repeated loading preserves the input.
Logs `/tmp/bluewake-actor-data-{build,run}.log` pass. ASan/UBSan are strict;
LeakSanitizer is unsupported on this platform and is disabled, so this is not a
leak-detector claim. Source replay through 0225 passes in
`/tmp/bluewake-actor-data-source-check.log`. The expanded world and game-info
owner targets rebuild successfully, and strict game-info checks and the complete
scripted 180-frame world replay pass after the native layout change. Logs:
`/tmp/bluewake-actor-data-world-{build,run}.log` and
`/tmp/bluewake-actor-data-game-info-run.log`. This qualifies the focused actor-data
increment, not completed logo startup or a broad all-mode checkpoint.

Patch 0226 now adds the missing JKRExpHeap and WaveBankMgr headers to the complete
audio branches. Actual patched m_Do_audio, JAIZelBasic and actor-data sources all
compile without BLUEWAKE defines (`whole-owners/*-startup-patched.log`). The
world has not yet replaced its selected audio owners with these complete units;
no audio initialization or playback result follows from this compile evidence.

Final checks for this continuation: source replay through 0226 and the rebuilt
world target pass (`/tmp/bluewake-startup-owner-source-check.log`,
`/tmp/bluewake-startup-includes-world-build.log`). A strict repeat of `--scheduler`
still reaches the same original METER null-picture failure with five startup
archives null (`/tmp/bluewake-startup-owner-scheduler-run.log`, exit 134). No HUD
guard or substitute archive was added. Next port the measured complete display,
cache-font and archive owners and compose original logo startup. WIP remains
uncommitted; no broad/all-mode, normal-boot or gameplay gate advances.

## Original display messages and complete archive modes

Patch 0227 ports the original JUTVideo post-retrace message and JFWDisplay
consumer to pointer-width message transport with a 32-bit counter payload.
The native comparison subtracts unsigned counters before interpreting the signed
delta, preserving wraparound without signed-overflow UB. Console code is retained.
The strict test compiles the complete display unit into the test translation
unit to reach its internal waitForTick without changing production linkage, and
links complete JUTVideo/JUTXfb sources. Original construction, callback registration,
message production, queue consumption, rate-zero fallback and callback restoration
execute through the real host OS queue. Deterministic VI/GX/time fixtures bound
the test; tick-based sleeping and pixel presentation are not tested here.

The first run fails UBSan in original JUTVideo::setRenderMode: the constructor
reads mSetBlack before initializing it. Patch 0228 initializes that native state
before the first configuration. The subsequent original two-frame black startup
sequence remains intact. Strict replay now passes both the signed counter
boundary and UINT32 wraparound, including stale messages, eligible zero-payload
messages and future messages left in the queue. Evidence:
`/tmp/bluewake-display-retrace-{configure,build,run}.log` (final pass).

Patch 0229 ports complete JKRDvdArchive and JKRCompArchive. They reuse the existing
native file-data pointer table while retaining serialized entry stride 0x14;
archive-relative pointer arithmetic and stack-buffer alignment preserve native
width. Composite memory-data addresses are native-width. Disc fetches explicitly
use the signed 32-bit entry numbers supplied by these constructors; the shared
archive field also supports wider memory identities in other archive types.
No new source tier is introduced. The existing complete archive-mode object
target now compiles all three non-memory archive owners.

The new private archive-modes target uses the existing ARAM probe with an optional
`--all-modes` argument. It mounts the real itemres archive successively through
MEM, ARAM, DVD and COMP, verifies stable cached access, removes/reloads the cache,
and compares uncached readResource output against the decoded MEM resource.
All four modes pass strict ASan/UBSan for check_00.bti: 598 stored bytes, 1,632
decoded bytes. The first comparison incorrectly required cached getResource
bytes to match across modes. Source and runtime show that MEM/composite memory
access retains compressed data while ARAM/DVD expands it; the final oracle
respects those original API semantics. No production behavior was changed to
force that incorrect comparison to pass. COMP's reported expanded-size behavior
also differs for memory-only archives; this port preserves it.

Evidence: `/tmp/bluewake-complete-archive-build.log`,
`/tmp/bluewake-archive-modes-{configure,build,run}.log` pass. The earlier failed
comparison is in `/tmp/bluewake-archive-modes-detail.log`. Source replay through
0229 passes (`/tmp/bluewake-display-archive-source-check.log`). This is a single
private resource across four mount modes, not broad compression/content coverage.
The complete display/archive owners are not yet composed into full logo startup.

Next measured prerequisite is JUTCacheFont. Its six compiler casts are not the
entire portability problem: native glyph cache records cannot alias serialized
GLY1 headers, pointer arrays need native element sizes, and page strides/data
alignment must account for native links and texture objects. Its raw integer
block walker also bypasses the existing big-endian resource fields. Port those
owners together, then compose the complete logo handoff. Stable remains ab8a9e9;
WIP is uncommitted, the full goal is active and normal boot remains unqualified.

Final shared regression: world and existing ARAM diagnostic rebuild and pass
strict execution after these changes. The world retains its 180-frame result,
104.331 displacement and zero released speed. Evidence:
`/tmp/bluewake-display-archive-regression-build.log`,
`/tmp/bluewake-display-archive-world-run.log` and
`/tmp/bluewake-display-archive-aram-run.log`. No new GUI was launched. Native
startup display/archives remain prerequisites awaiting full logo composition;
the normal-boot and gameplay gates remain open.


## Complete cache-font ownership and measured startup link (0230)

Patch 0230 ports the complete JUTCacheFont owner without a new tier. Serialized
GLY1 headers remain big-endian resource records; native cache metadata is copied
field by field instead of aliasing those headers as pointer-bearing page records.
Native pointer arrays, 32-byte page alignment and the 128-byte native page header
are reflected in page stride and m_Do_ext cache allocation. Block traversal uses
the existing typed big-endian headers. The console layout remains unchanged.

Strict public fixtures cover three sheets with owned storage, three sheets with
caller-owned storage, and four distinct glyph blocks with only two cache pages.
The real USA font adds nine sheets, with maximum sheet size 8,192 bytes. Every
sheet matches an independent raw-byte oracle. Cache hits, eviction, pinning,
all-pages-pinned refusal, invalidation and reload pass. The source buffer is
poisoned after construction; later loads still work through owned ARAM data.
Heap/ARAM capacity returns to baseline after destruction, while caller-owned
cache storage survives. This uses real JKR heaps, ARAM transfer and GX texture
object initialization; it does not claim rendered text or allocation-failure
coverage. Evidence: `/tmp/bluewake-cache-font-run.log`.

`bluewake_route_b_startup_runtime` now compiles complete display, video, XFB,
cache-font, DVD-error, archive and ARAM owners together. It is not yet part of
the default world's executable graph. A fresh full-logo compile under the world
flags succeeds. Its isolated link with this library resolves archive/font/ARAM/
actor-data ownership and leaves 20 symbols: GX reset/thread control, locked-cache
disable, OS alarms/progressive/reset/save-region services, VI black/retrace/DTV,
static module readiness and four game audio readiness/scene owners. The exact
log is ignored at `local-research/evidence/reorientation-20260909/whole-owners/
d_s_logo-startup-230-link.log`; it supersedes the old 29-gap list. No full-logo
executable was produced. Registry readiness can reuse the existing validated
static registry; alarms require real callback and sleeping-thread behavior.

Source replay through 0230 passes (`/tmp/bluewake-cache-font-source-check.log`).
The rebuilt strict default world passes 180 frames, displacement 104.331 and zero
released speed (`/tmp/bluewake-cache-font-world-run.log`). This does not change
the missing original logo handoff or scheduler/HUD failure. Stable remains
ab8a9e9; WIP uncommitted, full goal active, no GUI launched.


## Native startup alarm and module-readiness integration

State: ADVANCE. The previous status-only reply changed no authoritative state;
this iteration resumed implementation against the current tree. The full-logo
link's alarm symbols revealed a runtime requirement: original JFWDisplay sleep
schedules a stack alarm, masks interrupts, suspends the calling thread, and resumes
through JFWThreadAlarmHandler. The native main thread previously had no registered
suspension state, so OSSuspendThread returned immediately. Holding the native
interrupt mutex across a blocking suspension would also prevent callback delivery.

The native main thread now has host suspension state. OSSuspendThread publishes
suspension under the interrupt gate, releases that gate while asleep, then restores
the caller's prior interrupt state. A native alarm owner implements one-shot and
periodic scheduling, cancellation and callback delivery under that same gate.
Cancellation waits for in-flight callbacks; callbacks may cancel or rearm themselves.
The scheduler does not access a delivered one-shot alarm after its callback, matching
stack lifetime. Periodic scheduling preserves absolute phase and skips elapsed
periods. OSGetTime is sampled at most every millisecond while alarms are pending,
so Aurora game-clock pause/scale changes are honored. This is host scheduling,
not a claim of console interrupt latency. Native callbacks receive a zeroed context;
there is no interrupted PowerPC register state.

The existing cDyl_InitAsyncIsDone implementation moved into the shared static
registry owner, removing an accidental dependency on diagnostic process-adjacent
services. Absent/invalid/valid/uninstalled registry controls pass. No unconditional
readiness result was added and no new source tier was introduced.

Five focused binaries pass with `UBSAN_OPTIONS=halt_on_error=1` and
`ASAN_OPTIONS=detect_leaks=0`: host_alarm_test, host_os_runtime_test, foundation_test,
display_retrace_test and aurora_alarm_clock_test (all prefixed bluewake_route_b_).
The alarm test covers frozen/advanced clocks, ordered deadlines, cancellation,
self-rearm, periodic skipping/no burst, cancellation during a held callback and
100 stack-alarm suspensions each on main/worker threads. The display test retains
its counter-wrap checks and executes the original JFWThreadAlarmHandler through
100 main-thread stack alarms; it does not construct a complete GPU display or
execute JFWDisplay::threadSleep itself. The Aurora test uses actual OSGetTime and
time control: a pending alarm remains pending for 30 ms while paused, then expires
only after its game deadline at 4x scale. No GUI or private input is needed for
these five tests. Evidence: `/tmp/bluewake-startup-alarms-test.log`.

Verification correction: this private build has BUILD_TESTING internally OFF and
stale CTest metadata. An initial CTest filter ran only foundation_test; that result
was insufficient. All five current binaries were subsequently run directly with
15-second timeouts and pass. The new tests are registered in CMake for enabled
public test trees. The initial Aurora-clock test link needed explicit aurora::os
and aurora::vi dependencies; its final build and execution pass.

The rebuilt private world passes the same strict 180-frame loop with 104.331
displacement and zero released speed (`/tmp/bluewake-startup-alarms-world-run.log`).
Source replay through 0230 passes (`/tmp/bluewake-startup-alarms-source-check.log`),
and protected CPU/HLE/lock hashes are unchanged. The isolated full-logo link now
has 16 remaining dependencies (`local-research/evidence/reorientation-20260909/
whole-owners/d_s_logo-startup-alarms-link.log`): GXAbortFrame, GXGetCurrentGXThread,
GXSetCurrentGXThread, LCDisable, OSGet/SetProgressiveMode, OSGetResetCode,
OSResetSystem, OSSetSaveRegion, VIGetDTVStatus, VIGetRetraceCount, VISetBlack,
mDoAud_setSceneName, JAIZelBasic::checkFirstWaves/loadStaticWaves and mInitFlag.
No full-logo executable was produced and its startup library remains isolated
from the default world's graph. This is the second isolated startup link shape
since the cache-font runtime experiment; no third unchanged link attempt is needed.

Next: integrate original video/retrace/XFB lifetime with Aurora frame ownership.
Local source inspection shows Aurora's VI owner currently supplies configuration
and window settings but no pre/post-retrace callback implementation. JUTVideo's
pre callback can call GXCopyDisp/GXFlush, so inventing an independent callback
thread would require proving graphics ownership/concurrency. The current world
presentation instead calls aurora_begin_frame/end_frame manually around captured
commands. Resolve that lifecycle and reset behavior, then startup audio and the
original logo preload/delete handoff before admitting ROOM_SCENE/PLAYER. A 16-symbol
link census does not prove all unreached boot services are implemented. Keep the
existing scheduler/HUD failure visible; do not inject archive pointers or skip panes.
Full PRD goal active; focused qualification only, no new stable commit or GUI run.


## Original native video/XFB lifecycle (TWW 0231–0232, Aurora 0003)

State: ADVANCE. The previous turn made verified native timing progress. This
iteration inspected the original display ownership before continuing symbol closure.
Aurora's existing VI unit lacked pre/post callbacks, retrace count, black state and
XFB selection. GXCopyDisp is empty, as are display-copy source/destination, filter
and gamma setters; GXSetDispCopyYScale returns zero. Therefore successful linking
alone cannot establish original display behavior. The earlier observed world uses
manual aurora_begin_frame/end_frame around captured commands and bypasses this path.

Aurora patch 0003 extends its VI owner with staged/flushed/current framebuffer and
black state, original pre/apply/post ordering, callbacks and retrace counts. Native
callbacks run cooperatively on the thread that initializes VI; no independent
video callback thread was added. VIWaitForRetrace services callbacks on that owner.
DVD retry workers, including calls before VIInit, wait in game time without taking
ownership or running callbacks. Cadence is an explicit native 60/50 Hz policy;
late service coalesces elapsed retraces. This does not emulate physical scanlines,
exact console frequency or interrupt latency. output_black/current-XFB are state
only until the renderer applies them. A paused window will also need event handling
at the wait/frame boundary before interactive display acceptance.

TWW 0231 lets the actual JFWDisplay wait consumer drain its original message queue
nonblockingly and call VIWaitForRetrace when empty. The original post callback
still produces the messages and the existing unsigned wrap-safe eligibility test
is retained. This makes the cooperative owner supply its own awaited retraces.

Source review also found Aurora's FIFO processor invokes draw-done callbacks on
its worker thread. TWW 0232 queues that completion atomically and applies the
original JUTVideo callback body at the next video pre-retrace, keeping XFB and
sDrawWaiting mutation on the video thread. The complete JUTDirectPrint owner needed
its native framebuffer byte-count field kept at the SDK's u32 width for DCFlushRange;
console layout remains unchanged. No new source tier or success stub was added.

Complete JUTXfb construction through the null-mode path exposed missing
GXGetYScaleFactor/GXGetNumXfbLines. Aurora 0003 supplies the pinned SDK quantization
and line-rounding algorithm, including the 240-to-480 scale producing 479 lines.
This is allocation/query behavior; the missing copy setters remain explicit.

The new native_video_lifecycle_test uses real original JUTVideo, JUTXfb and
JUTDirectPrint, real JKR heaps, actual display wait consumption and native VI.
It verifies initial black interval, selected buffer pointers, stale-message
consumption, two-retrace waiting, mode-change black duration, callback restoration,
heap capacity restoration, paused clock and worker waits before/after VIInit.
Real Aurora FIFO processing delivers completion both from explicit drain and
asynchronous frame publication; the worker leaves XFB state unchanged until the
next owner retrace. A separate recording driver uses Aurora's real headless
recording facility with GPU submission suppressed. It records zero draws; no
texture pixels, complete JFWDisplay constructor/frame or GPU submission is claimed.

Two setup failures informed this test. First, GXInit alone initialized registers
without starting the FIFO worker, so drain timed out at 15 seconds. Starting that
worker then exposed the missing recording session. A real headless FramePacket
recording supplies the required lifecycle; no callback was manually invoked to
force success. Compilation also exposed legacy MSL/native-STL collisions when a
test included the original display unit alongside native STL headers. The final
test compiles the original unit in a separate driver; attempted broad JGadget/new
header changes were reverted. No such unrelated header port remains in 0232.

Final focused strict verification passes seven binaries: native_video_lifecycle_test,
display_retrace_test, host_alarm_test, host_os_runtime_test, foundation_test,
aurora_alarm_clock_test and cache_font_test (all bluewake_route_b_ prefixed).
The rebuilt private USA font additionally passes nine sheets and allocation release.
The rebuilt default world still passes 180 frames, displacement 104.331 and released
speed zero. Source replay passes through TWW 0232 and Aurora 0003. Evidence:
`/tmp/bluewake-native-video-focused-tests.log`,
`/tmp/bluewake-native-video-private-font.log`,
`/tmp/bluewake-native-video-world-run.log`,
`/tmp/bluewake-native-video-source-check.log`,
`/tmp/bluewake-native-video-aurora-check.log` and
`/tmp/bluewake-native-video-regression-build.log`. These are focused sanitizer-mode
runs, not a new all-mode qualification. CTest remains disabled in this private tree;
current binaries were run directly with timeouts. No GUI was launched.

A fresh whole-logo compile under current world flags passes. Its isolated link
now retains 13 dependencies: GXAbortFrame, GXGetCurrentGXThread,
GXSetCurrentGXThread, LCDisable, OSGet/SetProgressiveMode, OSGetResetCode,
OSResetSystem, OSSetSaveRegion, mDoAud_setSceneName,
JAIZelBasic::checkFirstWaves/loadStaticWaves and mInitFlag. Evidence is under the
ignored whole-owners directory as d_s_logo-startup-video-232-{compile,link}.log.
No full-logo executable was produced. This census does not include unimplemented
functions that already link, particularly the display-copy path.

Next implement actual native display copying and EFB/XFB persistence/selection
in Aurora, then execute the complete original display manager/frame lifecycle.
Original exchangeXfb_double copies the prior EFB at the beginning of a later
render cycle. Aurora begin_recording creates an EFB pass whose default attachment
clear flags are true; preservation and clear ordering therefore need an explicit
experiment rather than simply wrapping the existing renderer frame calls.
After that, complete reset/audio and original logo preload/delete handoff before
ROOM_SCENE/PLAYER admission. Full normal boot, transitions, save/reload, audio and
original-speed gameplay remain unqualified. Goal active; stable ab8a9e9 unchanged.


## GPU display copy and worker startup (TWW 0233–0234, Aurora 0004)

State: ADVANCE. The previous implementation turn left an unbuilt display-copy
patch and a live configure handle. This turn polled that handle to successful
completion, then compiled and executed the actual GPU experiment. The intervening
progress answer was status only; it was not treated as implementation evidence.

Aurora 0004 implements display-copy source, destination width and quantized Y
scale, serializes each GXCopyDisp request through Aurora's command processor,
and resolves into a GPU texture owned by the original XFB address. Copy clear
uses existing color/alpha/depth masks. VI-initialized frames preserve existing
EFB/depth targets; the first frame and replaced targets still initialize them.
Presentation waits for FIFO consumption, retains the selected GPU texture for
the render worker, and applies the current VI framebuffer and black state.
The existing manual world route does not initialize VI and keeps its established
EFB presentation behavior. TWW 0233 evicts the GPU snapshot when original JUTXfb
releases its CPU allocation. No new source tier or production success stub.

The new asset-free display_copy_probe opens an actual Metal window, submits five
frames and reads back center pixels from the GPU. It verifies red/blue copies,
copy-before-clear ordering, blue EFB retention into a new native frame, old red
XFB retention after further drawing, and address eviction. Four readbacks return
exact expected RGB values at 1280×960 backing resolution. Frame 3 selects an old
red XFB while EFB is green; its window capture was inspected and is red. Frame 4
selects the same XFB with VI black enabled; its inspected window is black. Both
observation runs exit zero and close their owned window. Unrelated Simulator and
applications were preserved. Device-destroyed warning occurs at normal shutdown.

Build corrections: missing VI header, Vec2 field access and an explicit
absl::flat_hash_map dependency for the probe. Without the latter its internal
Aurora header included Homebrew Abseil instead of the source-built matching
version and failed linking; no ABI-check suppression or library repin was used.

Focused regressions initially passed six binaries but cache_font_test timed out
at 15 seconds. A fresh bounded reproduction also stuck. A sampled stack showed
main waiting for its first ARAM command completion while the ARAM worker waited
on an empty service queue. Source inspection found each service initialized its
queue inside run(), after native resume could return to the caller; an early
send can fail before queue publication. This is the supported startup-race
explanation, not a claim that the sample captured the exact discarded send.
TWW 0234 moves queue initialization into the original ARAM/stream/decomp
constructors before resume and removes the later native reset. Console behavior
is preserved. No sleep or synthetic successful transfer was substituted.

After 0234, ten fresh strict cache-font processes pass, then the real USA font
passes all nine sheets. Rebuilt default world passes 180 iterations, displacement
104.331, 119 changing foot matrices and zero released speed. These are sanitizer
runs, not a new all-mode public/private qualification. The original scheduler/HUD
archive failure remains open; this test does not establish normal startup.

Evidence uses /tmp/bluewake-display-copy-* logs: build, run, visible-red,
visible-black, regression-build, focused-tests (retains initial timeout),
font-recheck, font-sample.txt, queue-build, font-repeat, private-font, world-run,
source-check and aurora-check. The inspected window captures are red.png and
black.png with the same prefix. A recovery checkpoint preserves these privately.

Remaining copy fidelity includes GXSetCopyFilter, GXSetDispCopyGamma,
GXSetCopyClamp, source-region/format edge behavior and CPU YUV XFB access;
these were not qualified by flat-color tests. Complete JFWDisplay ctor/beginRender/
endRender/endFrame has not run. Next compose its full existing ProcBar/DbPrint/
Console and graphics dependencies and execute that lifecycle, including prior-EFB
copy timing and event handling during waits, then reset/audio and original logo
preload/delete resource handoff before ROOM_SCENE/PLAYER admission. Last measured
logo link frontier is still the prior 13-symbol census, not refreshed this turn.
Full boot, transitions, saves, audio and original-speed complete gameplay remain
open. Stable ab8a9e9 remains unchanged; all new work is uncommitted.


Final regression: seven focused binaries pass after the queue fix. Five rebuilt
private probes also pass: archive_modes (--all-modes), aram_archive,
async_aram_archive, particle_request and play_lkd (bluewake_route_b_private_
prefix). The first expanded archive run caught an ASan ODR violation before main:
legacy fixtures still defined Aurora g_config/g_gameName while native VI/display
composition now retained their real owner. CMake no longer enables those duplicate
fixture definitions for the five affected targets. All five build and run with
strict ASan/UBSan; no sanitizer suppression. Their existing diagnostic fences and
bounded assertions remain unchanged, so these are resource regressions only.
Logs: final-regression-build, archive-modes (initial failure), archive-owner-build,
final-focused-tests and final-archive-tests under the same evidence prefix.

Source reproduction passes through TWW 0234 and Aurora 0004; git diff --check
passes. Protected exception/HLE files and dependency lock retain their recorded
hashes. Repository audit still fails on exactly the existing 20 tracked
local-research metadata files; these match HEAD and no new private assets were
tracked. No commit or push was made. Recovery checkpoint:
`local-research/checkpoints/reorientation-20260909-display-copy/`.


## Original display frame execution and reached depth failure (0235 / 0005–0007)

State: ADVANCE. The previous goal turn produced measured GPU copy/VI evidence
and fixed an observed startup queue race. This turn moved to original frame
execution, retaining the full PRD goal and original startup-resource frontier.

The reusable startup_runtime now composes whole JGadget linklist, JUTFader,
JUTProcBar, JUTDbPrint, JUTConsole, JUTAssert, JUTDirectPrint, JUTFont and J2D
GrafContext/OrthoGraph alongside existing JFWDisplay/Video/Xfb/CacheFont owners.
The test driver uses a C ABI to keep TWW/MSL and Aurora/native STL headers separate.
Host JUT logging moved unchanged out of jkr_host_support.cpp into a separate
jut_host_diagnostics.cpp object in all four resource libraries. This lets the
original console/assertion owners resolve those symbols without duplicate host
implementations; ordinary diagnostic consumers keep their prior stderr behavior.

TWW 0235 adapts ProcBar's address-to-pixel conversion to native MEM1 addresses.
Its first runtime exposed the native JKRHeap code bounds being NULL (code lives
outside MEM1), so both absent code endpoints represent an empty interval. Console
allocation sizes and alignment checks use native-width intermediates with the
SDK allocation-width check retained. No new source tier. The original console
manager/list and console buffer now construct, print and release in the probe;
no rendered font claim (DbPrint's font is null in this asset-free fixture).

Aurora 0005 stores the original copy-clamp flags, preserves texture-copy register
bits and supports the native renderer's default top/bottom edge-clamped display
resolve. Unclamped display copies explicitly abort at the copy boundary. This is
not a full filter/clamp/gamma implementation. Original JFWDisplay uses both clamps.
The timeout path's GXAbortFrame and GXReadXfRasMetric are explicit abort-on-use
fences in the test executable only, not the reusable startup library. Native GPU
hang recovery and hardware-counter behavior remain unimplemented. The ordinary
runs retain the original alarm guard and do not reach those fences.

First execution aborted because Aurora required GXEnd after every GXBegin.
The original SDK declares GXEnd as an empty inline; counted draw packets terminate
by their vertex count and JFWDisplay::clearEfb omits GXEnd. Aurora 0006 permits
that original use while retaining a matching-end requirement for its GX_AUTO
extension. The actual original draws are the positive runtime control; an
expected-abort control verifies the GX_AUTO error signature remains enforced.

The next run reached color readback but copied black instead of red. Temporary
instrumentation in the actual GX render function recorded skipped pending shader
pipelines, including the clear and ProcBar draws. The trace was removed. Aurora
0007 uses its existing blocking pipeline path for required GX and clear pipelines,
so EFB content is not omitted during shader compilation. Cold-cache replay then
passed first-frame red content; later red/blue/green transitions and VI-selected
buffer readbacks pass across 12 complete beginRender/endRender/endFrame cycles.
Original display/console destruction completes. These are color/lifecycle checks,
not full rendering fidelity, normal initialization or gameplay acceptance.

A further source audit found GXSetZTexture still empty. The original clear draws
a Z24X8 texture containing maximum depth; color success cannot prove depth clear.
The default probe now requests Aurora's real GPU depth snapshot on frame zero
and requires ffffff at the center. It FAILS with 000000, reproduced from a fresh
shader-cache directory. This is the active integration failure. Only explicit
BLUEWAKE_DISPLAY_COLORS_ONLY=1 bypasses that oracle for the earlier diagnostic;
the default remains failing. Do not treat its color result as display acceptance.

Next implement original Z-texture depth semantics through command decoding,
shader sampling/depth output and relevant early/late depth behavior. Preserve the
original clear draw and its resource; do not replace it with a synthetic clear or
turn off depth checking. Validate clear depth plus nonuniform/add/replace controls,
then event processing during native waits, reset/audio and original logo preload/
delete handoff before scheduler ROOM_SCENE/PLAYER admission. The previous full-logo
13-symbol link census was not refreshed and does not account for this runtime bug.

Focused strict regressions pass seven binaries (native_video_lifecycle,
display_retrace, host_alarm, host_os_runtime, foundation, aurora_alarm_clock,
cache_font), real USA font, private archive_modes --all-modes, rebuilt 180-frame
world, and the five-frame GPU display-copy probe. GX_AUTO negative control aborts
with its expected signature. These are sanitizer-mode checks only; no all-mode or
full public/private matrix claim. The world retains 104.331 displacement, 119
changing foot matrices and zero released speed. Full goal active; no external
block, commit or push. Stable ab8a9e9 remains the last qualified checkpoint.

Evidence prefix: /tmp/bluewake-original-display-. Preserve build/configure logs,
run (missing GXEnd), counted-run (NULL code endpoint), procbar-run (black pixels),
trace-run (skipped pipelines), cold-run and colors-run (passing color lifecycle),
depth-run (active failure), auto-negative, copy-regression, focused-tests,
private-font, archive-modes, world-run, source-check and aurora-check. The temporary
pipeline trace is absent from ref/aurora; source patches reproduce the final tree.


## Actual Z-texture rendering and original clear depth (Aurora 0008)

State: ADVANCE. The preceding implementation turn reached a failing real GPU
clear-depth oracle; the intervening user status answer changed no repository
state. This turn revalidated that failure in source, preserved all WIP through
0235/0007 in `local-research/checkpoints/reorientation-20260909-original-display`,
and implemented the missing rendering path. Protected file hashes and the 20
pre-existing tracked local-research metadata files were verified unchanged.
The prior green window capture is preserved in that recovery snapshot; it is
color evidence only, not retroactive proof of the previously failing depth.

0008 encodes original SDK BP F4/F5 bias/format/op, decodes their state and marks
bias uniforms / shader configurations dirty. PE depth-test location now also
invalidates shader configuration. Z texture forces the last enabled raw texture
sample into shader dependencies even when TEV color/alpha does not reference it.
It decodes Z8 from alpha, Z16 from alpha:red and Z24 from RGB, adds bias and
optional raster depth, wraps to 24 bits, supplies fog depth, and writes fragment
depth only for late depth testing. Early mode retains geometry depth. Bias is a
uniform rather than a shader key. Pipeline cache version advances to 14.

The original display then exposed an additional source-format hole: native
GXTexObj metadata retains GX_TF_Z24X8 while static texture hashing/upload only
accepted its sampled RGBA8 layout. Static source resolution now normalizes the
three depth aliases to I8/IA8/RGBA8 in a local copy, preserving public object and
copy metadata. This is actual texture sampling, not a substituted EFB clear.
No TWW change or new source tier was needed.

Semantics were checked against the pinned SDK GXTev.c encoding and independent
[Dolphin pixel shader generation](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderGen.cpp)
and [depth-component constants](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/PixelShaderManager.cpp).
The implementation follows Aurora's existing normalized-depth interval; this
is not a bit-exact hardware-depth claim. Early alpha/depth ordering is an existing
renderer limitation: retaining geometry depth does not prove depth is updated
before a rejected alpha fragment. The added fog path is source-integrated but
not yet GPU-qualified. No claim of arbitrary indirect/Z-texture combinations,
MSAA fidelity, complete display filtering or original-speed performance.

The default original_display probe passes on an isolated shader cache: exact
`ffffff` at the center after the original clear draw, then all 12 frame cycles,
GPU XFB colors and VI-selected identities. Sixteen independent GPU regions pass:
disabled raster depth; distinct RGB texels; consecutive bias-only changes; bias
and add wrap; Z8 alpha and Z16 alpha:red packing; add/replace; early geometry
depth; late alpha rejection; disabled writes; final-stage sample selection;
and actual Z8/Z16 tiled storage (Z24 storage covers the other textured cases).
Float raster/readback controls allow one LSB; original far-depth remains exact.

Preserved investigative failures: the first original run hit the existing
one-second display watchdog and the counter failure fence; a subsequent run
reached invalid texture-source hashing. That second runner also selected a log
file as a cache directory, so its cache errors are not product failures. Final
runs use newly created directories explicitly. The source-format correction
and passing default run do not establish cold-load watchdog reliability under
arbitrary host contention; its alarm and failure fences remain intact.

Evidence prefix: `/tmp/bluewake-ztexture-`. Build, configure, original/controls
final-run and source-check logs retain the actual outcomes. Current default
display result supersedes the depth failure in the historical section above.
Next return to native wait/event servicing and original startup reset/audio/logo
preload/delete handoff, then scheduler ROOM_SCENE/PLAYER. Full PRD goal remains
active; no normal-boot, audio, save/reload or complete-gameplay acceptance.

Final focused regressions after 0008: rebuilt GX-state and display-copy probes
pass, strict headless 180-frame world replay passes, and `--visible` submits all
180 world/Link frames through Metal and shuts down successfully. The scripted
world retains 104.331 displacement, 119 changing foot matrices, zero released
speed and 105.693 camera-center displacement. This turn did not perform a new
visual screenshot review or qualify sustained live controls. Aurora patch replay
and both repository diff checks pass. Audit still fails solely on the known 20
tracked local-research metadata files, unchanged from HEAD; no all-green audit
claim. No probe process remains. Protected hashes unchanged. WIP recovery and
logs are saved in `local-research/checkpoints/reorientation-20260909-ztexture`;
no commit/push, and stable ab8a9e9 remains unchanged.


## Complete audio control and authentic sound retirement (TWW 0236)

State: ADVANCE. The preceding goal turn repaired and GPU-qualified original
display depth. This turn moved into original startup ownership and adopted the
review's deferred JAIBasic cleanup correction. It added complete m_Do_audio.cpp
to the world through a reusable object target, replacing the world's diagnostic
mTact storage and getTactDirection failure fence with the original owner. This
also supplies original init/reset flags and the scene-name wrapper. The world
still manually creates its JAI pools/listener and does not call mDoAud_Create or
mDoAud_Execute as a normal boot loop; no audio-init/playback claim.

The pinned JAIBasic deleteObject body remained empty. Upstream source commit
[4c0a2960c9c0cfcf9280de49a1fd1c78f527b21d](https://github.com/zeldaret/tww/blob/4c0a2960c9c0cfcf9280de49a1fd1c78f527b21d/src/JSystem/JAudio/JAIBasic.cpp)
contains its implementation and the connected actor-wide stop traversal.
Independent private-disc disassembly verifies the GZLE01 bodies at 80290c74
(size 120 hex), 802909c0 (size 9c) and 80290884 (size 64): cache the next list
entry before stopping, match actor identity, use switch bit 8000 for retained
positions, reuse one dummy per deletion call, and use the original immediate/
fade-one stop paths as appropriate. Source bytes and commit/hash metadata are
preserved under the existing private whole-owners evidence directory; no repin.

0236 restores deleteObject, stopAllSound(void*) and stopActorSoundOneBuffer in
both the existing deletion selection and complete JAIBasic source. It corrects
DummyVec's three integer placeholders to an actual Vec, matching the retail
three float loads/stores. Complete JAIDummyObject.cpp now supplies allocation,
used/free-list movement, expiry and release. The existing delete-object tier
moves to one shared object target used by world and sound tests; no new tier.
The world initializes the real dummy pool alongside its existing manual SE pool
setup. No production success stubs or new diagnostic backend implementations.

Strict tests pass actual SE-registry removal across categories without skipping
entries, preservation of other actors, repeated deletion, retained positions
independent of the changed original position, 600 original check ticks and
expiry, both dummy expiry modes, exhausted-pool fallback, and actor-wide stop
traversal. Sequence/stream release routing and fade one are observed at explicit
test callbacks; they are not playback. Retail's shared dummy/last-handle behavior
for multiple retained sounds is preserved and tested, not silently replaced
with a per-sound allocator or claimed to prove arbitrary multi-tail playback.
Every isolated fixture returns its JAI heap capacity. The private regression
passes 8 categories, 2202 real SE entries, 160 parameter records, and 91 BAS
resources / 186 records / 4686 scheduled fixture frames / 6242 live-slot checks.

Rebuilt headless and --visible world runs both pass 180 frames. Metal reports
180 submissions and clean renderer shutdown; displacement 104.331, 119 changing
foot matrices, released speed zero, camera-center displacement 105.693 remain.
No new screenshot review, live-control acceptance or original audio frame loop.

A fresh complete logo compile succeeds with current world flags and no logo
tier. The isolated forced-profile link now has 12 unresolved dependencies:
GXAbortFrame, GXGetCurrentGXThread, GXSetCurrentGXThread, LCDisable,
OSGetProgressiveMode, OSSetProgressiveMode, OSGetResetCode, OSResetSystem,
OSSetSaveRegion, JAIZelBasic::setSceneName, checkFirstWaves and loadStaticWaves.
The old 13-symbol list is superseded. No complete-logo executable was produced.
A whole JAIBasic compile was also attempted: only two hard errors remain, both
size_t-to-u32 path-allocation lengths in initResourcePath; its non-returning
Nonmatching bodies also emit warnings. The restored lifetime methods compile.
The path builder additionally uses a redundant NUL through sprintf's %c and its
normal terminator; native allocation bounds need runtime verification while
porting this complete owner. Do not add another source tier around these errors.

Current upstream JAIZelBasic::zeldaGFrameWork is still empty, as is the pinned
body. This is a concrete audio implementation gap, not a reason to bypass
original audio startup or claim that linking the control owner enables output.
Next compose the native reset/platform services and complete JAI startup owners,
repair those path allocations, and bring the original logo preload/delete
handoff into scheduler admission. Preserve explicit failures for unreached
backend behavior; do not replace original initialization with success flags.

Evidence: /tmp/bluewake-audio-control-* and /tmp/bluewake-audio-retirement-*;
private whole-owners directory contains deleteObject-retail.txt,
stopActorSound-retail.txt, upstream/JAIBasic-source.json, and the current full-logo
compile/link logs plus exact command arrays. Stable ab8a9e9 remains unchanged;
WIP uncommitted, full PRD goal active, no external blocker.

Final source reproduction passes through TWW 0236 / Aurora 0008. Repository diff
checks pass; protected CPU/HLE/lock hashes remain unchanged. Audit still fails
only on the 20 pre-existing tracked local-research metadata files, unchanged
from HEAD. No owned probe process remains. Recovery, WIP diffs, untracked source
patches and logs are saved in local-research/checkpoints/
reorientation-20260909-audio-retirement; no commit or push this increment.

## Complete audio path and sequence callback owners (TWW 0237–0240)

State: ADVANCE. The immediate preceding user-facing turn only answered the
progress question and made no code changes; this continuation revalidated the
live worktree and finished the pending negative-control handle (73675, exit 0).
It then advanced original audio ownership. The full PRD objective is unchanged.

0237 removes the complete JAIBasic path-allocation compile errors. Its native
builder reserves root + suffix + two zero bytes, checks the SDK size boundary,
and preserves both bytes written by the original sprintf format. A real-heap
fixture covers 65 lengths, absent prefixes, alternating interface child heaps,
repeated ownership and adjacent allocations. Parent solid-heap destruction owns
child disposal; the final test no longer tries to individually free a child from
a solid parent. Every iteration restores root free capacity. A final temporary
one-NUL allocation variant aborts at the next allocation's boundary assertion.
The test links complete JAIBasic and the expanded complete audio runtime; earlier
spatial-method and track-registration test fences were removed. Only unreached
StreamLib::init remains an explicit test-only abort. Normal Zelda initialization
uses a null audioResPath, so this qualification is not an audio-startup claim.

0238 restores JAIBasic::setParameterSeqSync from the previously inspected
upstream JAIBasic source (commit 4c0a2960c9c0cfcf9280de49a1fd1c78f527b21d), checked
against all retail instructions at 80290e50–80291034. Command zero finds the first
matching sequence parent or grandparent, initializes its original outer/port
owner and publishes its track bitmap; command one copies SE track parameters
with the original output-mode Dolby rule; command 127 writes the scene port.
Unknown commands return zero without touching the track. Native flags use the
existing borrowed-record big-endian reader, and the bitmap operation preserves
PPC slw's six-bit count/zero rule rather than invoking a signed/oversized shift.
The connected SequenceMgr::getPlayTrackInfo was also empty; the four retail
instructions at 802982c0 verify an array lookup. Native indexing uses the full
native SeqUpdateData size. Two signed oscillator table constants are expressed
as their negative values so the complete player-table owner compiles; their
16-bit patterns are unchanged. No repin or new source-tier macro.

The opt-in audio_track_runtime compiles complete original owners for tracks,
ports, interrupts, registers, notes, channels, DSP-channel metadata, oscillator,
rate, DSP interface, driver interface, command queues, global parameters, sound,
sound table, sequence manager, system interface and supporting data/heaps. The
full JAIZelBasic source separately compiles without a tier as well. These are
usable composition building blocks, not proof that their unreached Nonmatching
bodies implement every behavior, and they have not replaced the world's selected
JAI bootstrap yet.

0239 preserves logical-channel owner identity as uintptr_t on TARGET_PC across
allocation, storage, release and allocation-queue callers; it remains u32 on
console. This field is host logical metadata, not a DSP address. The test uses
real TDSPChannel::initAll/alloc and real DSPBuffer initialization against a fixture
buffer array, then checks that an owner differing only above bit 31 cannot free
the channel. The exact owner releases it and restores the 64-channel counters.
No hardware or audio buffer is started by that fixture.

0240 exposes the actual unresolved DSP transport boundary instead of truncating
five host pointers into hardware command arguments. Complete DSPInterface now
passes its typed host channel/effect buffers and original filter/delay inputs to
bluewakeRouteBSetupDspBuffers on TARGET_PC. There is deliberately no provider,
success body or test substitute for this startup function. Console DsetupTable/
DsetDolbyDelay remain unchanged. Native FX pointer alignment/reporting is width
safe. This is one new necessary hardware boundary declaration, not another source
tier and not an implemented audio backend. DSP serialization/transport, mixer,
thread lifecycle, hardware callbacks and audible output still require real
implementation/qualification; adding this declaration does not close that work.

Strict final sequence-sync tests execute the original registered callback,
actual TTrack/TOuterParam/TrackPort/IntrMgr, SequenceMgr lookup, SoundTable reader,
SystemInterface outerInit and JAS command queue. They cover four SE indices
including 255, all three output modes, four scene values and delivered interrupt,
empty sequence lists, unknown commands, skipped empty slots, first matching owner,
single/two-level parent matching, unrelated parents, real queued parameter
updates, borrowed big-endian flags and track bit 31. The >31 shift rule is
source/retail verified, not exercised through out-of-range sequence arrays. A
final temporary native-endian flag variant fails the parameter-mask assertion.
This is control/data delivery evidence, not synthesized or heard sound.

Final strict runs exit 0 for sequence_sync_test, audio_resource_path_test,
se_registration_test and the rebuilt 180-frame headless world. World displacement
104.331, 119 changing foot matrices, released speed zero and camera displacement
105.693 remain. No new Metal/window run this increment; prior visible evidence
belongs to 0236. The latest isolated full-logo link remains the earlier 12-symbol
result, not a fresh whole-audio boot census. Original scheduler still needs the
logo archive handoff before HUD admission. The empty zeldaGFrameWork and other
remaining audio bodies cannot be bypassed with initialization-success flags.

Evidence logs: /tmp/bluewake-audio-240-*-final.log, source-check.log and audit.log;
/tmp/bluewake-240-negative-controls.json records exact compile/link arrays and
expected-abort results. The private whole-owners evidence directory now includes
sequence-sync-retail.txt and sequence-track-info-retail.txt, plus the complete
JAIZelBasic compile command/log. Source reproduction passes through 0240; diff
checks pass; the protected CPU/HLE and dependency-lock hashes are unchanged.
The audit still fails solely on the same 20 tracked local-research metadata
files, unchanged from HEAD. No commit/push or new all-mode product qualification.

Next: compose original startup using these complete owners, resolve native DSP
transport and the remaining source bodies with explicit runtime/oracle evidence,
and restore the original logo preload/delete handoff before scheduler admission.
Keep the world's current renderer oracle and full PRD objective intact. Recovery
is saved at local-research/checkpoints/reorientation-20260909-audio-track; this
is a private WIP recovery snapshot, not a qualified release checkpoint. No
external blocker and no owned probe process remains.

## 2026-09-09 — Native Zelda DSP candidate and original wave-table allocation

The intervening yes/no status reply was no implementation progress. This
continuation revalidated the live worktree and polled the actual pending handle
12652: native DSP controls, sequence callback and 180-frame headless world all
terminated with exit 0. The full PRD objective remains active.

The opt-in native audio candidate adapts the complete ZeldaAudioRenderer class
from the already present RecompCore donor at
5c3611e2bcb03d1578274c956bc3d7b913527ec2. Its underlying component is Dolphin and
is GPL-2.0-or-later. All five donor source/notice/license hashes match that exact
commit, recorded in route_b/src/audio/provenance.lock.json; copyright, COPYING
and the full license accompany the adapted source. This adds a licensed candidate
to the opt-in tests, not to world/product links. Corresponding-source and Apple
distribution review remain required before product adoption/publication. No
dependency repin, protected runtime modification or public distribution occurred.

The adaptation removes the game-CPU/mailbox/savestate wrapper and uses bounded
DSP record storage plus borrowed original ARAM. It preserves the whole renderer
algorithm and uses the donor's Wind Waker flag value (CRC 86840740, flags zero).
Only the renderer is compiled with -fwrapv for its signed DSP arithmetic.
Original JAS voice buffers and original coefficient tables feed the adapter;
native halfwords and eight native 32-bit fields are explicitly serialized.
Only the mutable first 0x100 bytes of each 0x180-byte voice are written back.
No captured game state or second game-CPU engine participates.

0241 declares all six mixer buses already indexed by original DSPBuffer methods,
preserving the structure size and later offsets. Strict public controls use the
sixth bus and verify constant PCM8, PCM16, AFC HQ and AFC LQ samples against a
scalar fixed-point oracle, completion/readback and subsequent silence. Two voices,
both stereo outputs, loops, six pitches and changing volume remain identical
across reconstruction from native JAS state after every subframe. This is a
continuity oracle, not an independent hardware fidelity comparison. Invalid ARAM
access and unregistered effects are rejected explicitly. Source renderer
approximations remain, including volume-ramp behavior; other formats/effects and
full DSP fidelity are not established by these controls.

The private native-DSP probe uses original BankWave/WSParser/WaveBankMgr,
asynchronous wave loading and actual ARAM storage from the verified local GZLE01
input. Group 2 loads 495,328 bytes at ARAM address 0x4000 and reaches status 2;
the probe checks the loaded bytes against the private archive. It then renders
the first 32 registered wave IDs for 64 subframes each through original
DSPBuffer setup and the native adapter. Every wave yields nonzero PCM: 163,840
stereo sample frames in total. No audio file, device output or GUI is created.
This proves original loaded-wave-to-PCM data flow. It does not prove sound-event
sequencing, original pitch selection, heard quality, real-time behavior or normal
audio startup. The test bootstrap still manually admits the bank and voice.

During startup review, the complete original WaveBankMgr::init was found to
allocate only count * 4 bytes for its native pointer table. The real-heap resource
test now allocates a neighboring guard and asserts that the table fits before
accessing every slot. The unchanged original allocator aborts at that extent
assertion (baseline exit -6). 0242 allocates native pointer-sized/aligned entries
on TARGET_PC with an SDK-size overflow assertion; console allocation remains
unchanged. Seven capacities through 256 pass alignment, zero initialization,
every-slot registration/readback, range lookup, unchanged adjacent guard and
full parent-heap reclamation. The original 65 path/heap cases also still pass.
The test's borrowed identity tokens are never dereferenced; it does not simulate
wave playback. The actual private loaded-wave probe was rebuilt and passes again
after this fix. No new source tier was introduced.

Fresh complete-source compile attempts using the strict audio runtime flags
identify the next transport blockers: JASAiCtrl has two pointer-to-u32 DMA casts;
JASDSPBuf has two pointer-to-u32 frame-buffer casts; JASAudioThread has one
OSMessage-to-int cast. Exact commands, source hashes and diagnostics are in the
private whole-owners audio-startup-native-dsp-242-commands.json record. These are
five observed compile errors in three whole units, not a full-startup link census.
The prior 12-symbol logo result is historical and has not been requalified with
complete original audio startup. The native setup provider remains unresolved.
The adapter also needs genuine FX-ring ownership, DSP subframe completion,
original audio-thread/DAC lifecycle and output-device/sample-clock integration.
Missing JAI frame/scene/wave bodies still require source/retail reconstruction;
do not substitute initialized flags or successful no-ops.

Evidence includes native-dsp-final-{control,sequence,world}.log,
private-native-dsp-test.log, wave-table-baseline.log,
wave-table-final-{public,private}.log and native-dsp-242-source-check.log. Source
reproduction passes through TWW 0242; protected CPU/HLE and dependency-lock hashes
match their prior values. The strict world retains displacement 104.331, 119
changing foot matrices, zero released speed and camera displacement 105.693.
No new visible/live gameplay, normal boot, audio acceptance or gate advancement.

The repository audit initially failed on the same 20 tracked research files,
all byte-identical to HEAD. This continuation resolved that policy violation:
each file was backed up, removed from the Git index only, and hash-verified still
present locally under the existing ignored local-research directory. The audit
now passes without changing its rules. Commit a954ee5 contains only these tracking
removals; it is local and was not pushed. Historical Git objects still contain
the previously committed files; this was not a history purge. Runtime checkpoint
ab8a9e9 and protected Route A files are unchanged. The remaining implementation
and documentation WIP is uncommitted, with no staged unrelated work.

Recovery: local-research/checkpoints/reorientation-20260909-native-dsp contains
current tracked/ref diffs, untracked implementation, logs, source/provenance
evidence and a verified hash manifest. The separate research-tracking recovery
contains all 20 original files. This is WIP recovery, not a qualified product
checkpoint. Next connect the original audio lifecycle to the native transport,
preserving real buffer/FX ownership and sample-clock ordering; then restore the
logo archive handoff before original scheduler/HUD admission. Do not spend
another iteration merely expanding synthetic wave formats. Full goal active.


## 2026-09-09 — Original audio thread, DSP completion and DMA consumption

Previous goal turn: progress (native wave-bank allocation fix, source evidence,
tracking-policy repair and recovery). This continuation revalidated the worktree
and advanced from direct mixer calls into complete original driver/thread owners.
Full PRD scope and the world/product acceptance criteria remain unchanged.

0243 removes the five measured narrowing casts in complete JASAiCtrl, JASDSPBuf
and JASAudioThread. Native DMA and DSP-frame calls pass live typed buffers, while
OSMessage dispatch compares its full uintptr_t value. Console calls are preserved.
The opt-in native_audio target composes these complete units with JASCallback,
JASProbe, JASKernelDebug, JASHardStream and JKRStdHeap; no new tier macro. The initial
link exposed missing native AI streaming leaves, Aurora OSGetTick ownership and
whole JKRStdHeap. Whole heap composition and Aurora's existing OS clock close
those dependencies; active hardware-streaming requests explicitly fail until
that separate peripheral is implemented. Original HardStream::main executes its
actual disabled-state return in this test, not a replacement success function.

native_audio.cpp implements the previously unresolved DSP setup provider over
original buffers, tables and actual ARAM storage. It accepts DSP commands only
from the original audio owner, validates ordered channel groups 0–3 and pending
frame bounds, renders 80 samples per subframe and writes the original planar
buffers. Completion calls original syncDSP, which reads the native peripheral's
mailbox and posts the original audio worker's message. The worker decrements its
original snIntCount and continues updateDSP/finishDSPFrame itself. No test loop
calls those JAS phases manually. Auxiliary DSP work and active FX rings still
fail explicitly; no effect or auxiliary-task success is fabricated.

The passive native AI transport borrows original aligned R/L DMA memory and
returns native L/R pairs to its sample consumer. Only complete buffer consumption
invokes original syncAudio; that callback posts work instead of executing the
audio worker on the consumer thread. Stop serializes with an in-flight consumer
and its callback. Original JAIBasic::initAudioThread sets mixer gain from its
caller after starting the worker, so gain is atomic and accepts that real calling
context. The earlier owner-only gain restriction was corrected before the full
JAIBasic run. A real output device has not been attached, and its sample clock,
relationship to Aurora's game clock, pause behavior and native rate remain open.

The public lifecycle test calls complete JAIBasic::initDriver, including original
DVD/audio worker startup, track-pool creation, registered sequence callback and
mixer configuration. It uses original heap/ARAM initialization and a public
constant PCM wave. One original registered DSP callback runs on the audio thread,
allocates a real TDSPChannel, programs its original voice and starts playback.
The original completion callback frees that exact native owner. Wrong-owner DSP
commands and out-of-order group release are rejected without advancing state.
The full JAIBasic vtable retains one unreached test-only StreamLib::init abort;
this test does not call initInterface or advertise stream initialization.

The first unpaced consumer got ahead of its producer: the sixth DMA buffer was
consumed with only ten DSP subframes complete. Its right-channel assertion then
caught repeated samples from the original underrun path. This was a test cadence
error, not evidence that normal device playback fails. The final headless test
uses explicit producer backpressure instead of a fabricated elapsed-time rate.
Partial 79/1/480-frame reads verify exactly one completion per 560-frame DMA
buffer. Sixteen buffers yield exactly 112 original DSP updates and 1,603 nonzero
left-channel frames; right remains zero. The voice retires once, all 64 channels
return free and the worker handles the original stop message before host teardown.
The parent heap regains its full pre-test free capacity. Three original solid-
heap warnings come from the DVD thread's individual stack/record/message frees
during parent disposal; the entire parent allocation is reclaimed. This does not
qualify in-process JAS restart. Original load-shedding reports attempts to break
an already free channel after playback; its heuristic is retained, not disabled.

Final strict lifecycle, native DSP format/state and sequence-callback tests all
exit 0. A private temporary variant that omits DSP completion aborts because the
original audio worker cannot reach seven subframes. This establishes that the
completion path drives the observed progression. Exact negative compile/link/run
commands and logs are in private evidence/reorientation-20260909/audio-lifecycle;
actual source was not altered for that control. Logs in the recovery snapshot
include audio-lifecycle-final-build.log and final-*.log, the initial underrun
reproducer and native source reproduction through 0243. The prior 32-private-wave
and world rendering evidence is separate; no fresh private-wave, Metal or world
run was performed for this opt-in-only increment.

The GPL-2.0-or-later candidate remains outside world/product links. Provenance
now includes the native transport files. No protected CPU/HLE or dependency-lock
change, public release, audio-device launch, new gate pass or full-audio claim.
Repository audit passes; HEAD remains tracking-only cleanup a954ee5. Implementation
WIP is uncommitted and preserved at
local-research/checkpoints/reorientation-20260909-audio-lifecycle with tracked/ref
diffs, untracked sources, exact evidence and a verified SHA manifest.

Next: connect original JAI interface/FX and sound-event initialization to this
working driver lifecycle, implement owned FX-ring transport, and attach a real
sample-clock consumer. Missing game-audio bodies, original logo archive handoff,
HUD/scheduler admission and the full PRD remain open. Do not use another synthetic
format expansion as the next milestone. No external blocker; full goal active.

## 2026-09-09 — Original effect-scene ownership and native delay rings (0244)

The preceding implementation turn made progress by composing original audio
lifecycle owners. The intervening yes/no status response made no implementation
progress. This continuation revalidated the current worktree, completed the
pending effect qualification and advanced the original asset-owner diagnosis.
Full PRD scope remains active; no external blocker or product acceptance.

0244 composes complete JAIFx.cpp without a new tier macro. Its original heap,
scene and buffer-allocation loops remain the owners. TARGET_PC decodes bounded
big-endian scene records into native configurations instead of indexing console
pointer layouts. Validation covers the scene-offset table, all four records per
scene, buffer maxima, destination indices and active block lengths. The native
pointer table is pointer-sized/aligned and zero initialized. Native setBufferPointer
registers the actual full allocation with the DSP bridge. Console behavior is
preserved. JAIInitData case 7 now has a bounded setter connection ready for future
whole-owner composition; this does not qualify the full original AAF loader.

DspMemory registers four borrowed sample spans in a separate DSP address range.
DSP protocol records remain big-endian; native delay samples use native copying
with checked slot, alignment and extent. JasDspBridge explicitly serializes the
32-byte effect protocol from native FXBuffer fields, preserving coefficients,
signed volumes, destinations and active ring size without pointer truncation.
Original OS interrupt locking serializes effect registration/configuration against
rendering. Full allocation capacity is retained because the inherited renderer
keeps a circular cursor/history across configuration changes. Scene switching,
restart and hardware fidelity are not established; no invented reset masks that
uncertainty. Broader JAS callback-list concurrency is also not qualified here.
Unsupported surround/backmix, hardware streams and auxiliary DSP tasks still
fail explicitly. The GPL candidate remains opt-in, outside world/product links.

Public native-DSP controls compare all four effect modes with a scalar oracle:
signed sparse eight-tap coefficients, pre/post filtering, twelve subframes over
four circular wraps, exact left samples, zero right samples, complete ring
writeback, adjacent guard samples and rejection when the active ring exceeds
its registered allocation. Existing four-format, looping, pitch, two-voice and
state-readback checks pass. The oracle validates this adapter and inherited
algorithm against scalar math, not exact hardware behavior.

The lifecycle test's public two-scene table runs original JAIFx::init before
original-thread playback. An original registered callback routes a public voice
to dry left and an effect send. The first hypothesis expected a 160-sample delay;
the source renderer includes eight historical samples before the current block,
so the observed 168-sample delay is correct for that inherited algorithm. The
final synthetic run produces 1,440 nonzero left and 6,551 nonzero right frames,
with an effect tail beyond the dry voice. A temporary source copy without effect
protocol upload compiles/links but aborts the wet-output assertion with zero
right output. Actual implementation source was not altered for this control.

A separate private mode opens the supported disc through AuroraDisc and uses the
existing bounded AudioInitData diagnostic parser to locate its effect section.
Original JAIFx initializes both real scenes and four buffers (initial active
lengths 32, 32, 56, 56 blocks). With the public test wave, the observed private
configuration produces 4,801 nonzero left frames and zero right frames. The test
checks actual ring writes and a tail beyond the source's 1,600 samples; it does
not conflate absent right output with effect failure. All dry/public/private
lifecycle modes complete exactly 16 DMA buffers / 112 DSP subframes, original
voice retirement, stop-message handling, 64 free channels and full parent-heap
capacity recovery. Original solid-heap individual-free warnings and DSP load-
shedding logs remain; headless backpressure and burst-driven variable dry counts
are not real-device timing or an in-process restart qualification.

The AudioInitData helper now exposes its already range-validated case-7 section.
Its public regression covers the accessor and invalid extent. Because this
changes the helper's layout, both private-wave and world executables were rebuilt.
Final strict runs all exit 0: native DSP; lifecycle dry/public-FX/private-FX; AAF
parser; 32 private waves (all nonzero, 163,840 stereo frames); headless world
(180 iterations, displacement 104.331, 119 changing foot matrices, released
speed zero and camera displacement 105.693). The world run captures commands;
no new Metal presentation, live input or speed acceptance is claimed. Exact
commands, logs and negative-control artifacts are saved under private
`evidence/reorientation-20260909/audio-fx` and the recovery snapshot below.
Source replay through 0244, diff whitespace and the unchanged repository audit
pass. No GUI/audio device was opened; no protected CPU/HLE or dependency pin
was changed. The prior twenty private research files remain local and ignored.

Next whole-owner evidence: strict compilation of complete JAIInitData.cpp reports
four failures: path-length narrowing and bank, wave and scene pointer relocation.
The source also reads big-endian command words as native u32 and copies 12-byte
console bank/wave records into native pointer-bearing structures. The next change
must preserve original control flow while providing bounded endian-aware records,
native pointer tables and asset lifetimes. Casting through uintptr_t alone cannot
fix this. Current-source inspection also corrects the earlier continuation
assumption: JAIBasic::initReadFile and initInterfaceMain already have source
bodies. Their complete linkage and startup execution are unqualified; other
bodies, including processFrameWork, remain Nonmatching. Reuse these existing
entry points rather than reconstructing them. Then connect original logo
preload/deletion and scheduler admission. No fresh
whole-logo link census or all-mode checkpoint claim follows from these tests.

Implementation remains uncommitted over a954ee5 (tracking cleanup only); the
qualified runtime checkpoint is ab8a9e9. Current WIP, patches, untracked sources,
logs and integrity metadata are recoverable at private
`checkpoints/reorientation-20260909-audio-fx`. This is recovery, not a new
product checkpoint. Continue toward the full PRD rather than another synthetic
format expansion.

## 2026-09-09 — Original AAF loader, instrument owners and combined startup (0245–0246)

Previous goal turn: progress (original effects qualified, regressions rerun,
documents corrected and recovery verified). This turn reused the implemented
JAIBasic::initReadFile/initInterfaceMain entry points. It did not reconstruct
an absent loader or replace BankWave::init with manual admissions. Full PRD scope
remains active, with no external blocker or product acceptance.

0245 fixes all four strict JAIInitData compile errors and its native layout
assumptions. Original dispatch reads bounded big-endian AAF words; its bank/wave
records become native pointer-bearing tables with a complete null terminator.
Section bytes are copied onto the original JAI heap so native records remain
valid after either the temporary DVD allocation or external memory input retires.
Scene offsets produce an aligned native pointer table into an owned scene blob;
the stream table has a typed pointer/length header instead of overwriting an
8-byte console record with a native pointer. SoundTable and Fx retain their
original owners. Preflight validates command/table termination, duplicate known
sections, bank counts/section extents, scene table/category extents, SoundTable
and Fx records. It is not comprehensive nested BNK/WS malformed-data coverage.

Native memory mode requires an explicit length. The actual mDoAud_Create caller
now passes its DVD command's getMemSize through a sized global-parameter overload;
the legacy unsized overload clears any prior native extent and cannot silently
reuse it. File mode measures the original DVD file, runs original loadTmpDVDFile,
dispatches the whole AAF, frees the temporary tail and clears its registration.
The fallback resource path reserves both terminators written by the original
formatter and checks narrowing/allocation. Console behavior retains its original
representations. A temporary stale patch snapshot was caught by CMake's preparer
check after a source edit; refreshed 0245/0246 replay now passes without overrides.

Complete instrument-bank composition exposed the paired native issues fixed by
0246: console big-endian fields/envelope words, BankMgr's four-byte pointer-table
allocation, half-cleared native pointer arrays, a virtual bank ID read in native
byte order, and oscillator pointer-difference narrowing. Whole BNKParser, BankMgr,
Bank/BasicBank/BasicInst/DrumSet and effect owners now compile together. Complete
StreamMgr also compiles; its DirectPCM path crosses a typed native sample-mapping
boundary that explicitly fails until main-memory stream ownership is implemented.
No success-returning stream substitute or new source-tier macro was added. The
lifecycle test's previous StreamLib::init abort replacement is removed because
the full original owner now supplies that function.

The new public audio_init_owner_test calls original JAIBasic::initReadFile in
memory mode with a synthetic sound, bank, scene and effect table. Its one melodic
instrument has a nontrivial pitch/volume, velocity mapping and signed envelope.
The test checks native virtual/physical registration, all unused instrument slots,
section ownership after overwriting the input, malformed top-level ranges and
scene extents, and full parent capacity recovery. A temporary BankMgr variant
that reads the virtual ID in native byte order compiles/links but aborts the
original bank-range assertion. This proves the data path reaches the actual
registration owner; the real source was unchanged for this control.

The private mode calls original file-based initReadFile, not the helper parser.
It registers all 65 banks and 65 wave banks from the disc and checks native bank
associations. A separate byte-reading oracle compares 778 melodic programs,
2,463 velocity regions and 787 oscillator configurations with serialized input,
including float bit patterns and signed envelope tables. Percussion presence is
also checked, but its full parameter fidelity is not established. Original
metadata copies remain valid after file-tail release. The first run reached all
these owners and then failed UBSan because the new test explicitly destroyed the
parentless root heap. Its original destructor assumes a parent; the corrected
probe destroys only its owned audio child, verifies full parent recovery and
leaves the application-lifetime root intact, as existing lifecycle tests do.
Original solid-heap individual-free warnings remain visible.

Combined lifecycle --init runs original driver startup, complete file-based AAF
loading and original Fx::init before playback on the original audio thread.
The initial 2-MiB arena exhausted while allocating an instrument table. Measured
steady allocations are 474,912 bytes for the driver and 1,097,176 bytes for the
standalone full AAF owners; file mode also holds a temporary 540,416-byte AAF
while constructing those owners. It therefore uses the loader probe's 8-MiB
diagnostic arena. This does not change the game heap or establish a product
memory budget. Combined driver + AAF + FX retain 1,600,592 bytes in file mode.

The added --init-memory mode follows the game's preloaded-memory handoff and
continues to pass in the existing 2-MiB arena. After original initReadFile, the
test clears the borrowed AAF registration, overwrites and frees the external
buffer before original effect initialization/playback. Driver + AAF + FX retain
1,600,560 bytes. Both combined paths produce the real configuration's left effect
tail with a public test wave, exactly 16 DMA completions / 112 DSP subframes,
original voice retirement, 64 free channels, original worker stop and full
parent-heap recovery. Observed nonzero counts vary with retained original DSP
load-shedding under headless backpressure; these are not real-device deadlines,
hardware audio fidelity or a representative gameplay sound corpus.

All twelve pre-memory-mode strict runs pass: public/private init owner, combined
file startup, dry/public-FX/private-FX lifecycle, native DSP, resource path,
sequence callback, AAF helper, 32 private waves and 180-frame headless world.
The new memory-mode combined run also passes; final lifecycle verification uses
the latest binary. Five latest-binary lifecycle reruns all pass. Their measured
allocations vary slightly: the observed driver range is 474,844–474,944 bytes
and combined steady range is 1,600,528–1,600,592 bytes. The individual numbers
above are specific runs, not deterministic allocation contracts. World metrics remain displacement 104.331, 119 changing foot
matrices, released speed zero and camera displacement 105.693. No new Metal/live
input or full-scene scheduler claim. Exact commands, executable hashes, positive
and failed logs, source reproduction and negative controls are preserved in
private evidence/reorientation-20260909/audio-init-owner and its recovery snapshot.

A fresh link-only experiment replaces the diagnostic initReadFile call with
original initInterface(0). It compiles but reports six dependencies: C_MTXLookAt;
JKRDvdArchive, JKRAramArchive and JKRCompArchive constructors; DummyObjectMgr::init;
and HeapMgr::init. These are the next coherent original owners to compose before
running interface/archive/sequence initialization. The temporary experiment was
not executed. The older twelve-symbol whole-logo census is separate and was not
rerun. Remaining interface/game event bodies, device sample clock, main-memory
PCM/hardware streams, normal logo preload/deletion, scheduler admission and the
full PRD still require work. No isolated symbol-only checkpoint or full-audio
acceptance follows from this subsystem run.

Source replay through 0246 and the unchanged repository audit pass. The protected
CPU/HLE files and dependency lock are unchanged; the twenty formerly tracked
private research files remain local, ignored and hash-identical. No window/audio
device, publication, repin or history rewrite. Implementation is uncommitted over
tracking-only a954ee5; runtime checkpoint ab8a9e9 stays unchanged. Recovery is at
private checkpoints/reorientation-20260909-audio-init-owner with complete WIP,
ref diffs, untracked sources and a verified SHA manifest. Continue with the whole
initInterface closure, then original game startup, without reducing the full goal.

## 2026-09-09 — Complete interface composition and sequence-source gap (0247–0249)

Previous goal turn: progress (original AAF owners, combined driver/FX paths,
serialized-data oracles and recovery). This continuation closes the six observed
initInterface composition dependencies with complete original archive-mode,
DummyObjectMgr, JAISequenceHeap and SDK matrix owners. Matrix composition exposed
the two original scalar vector leaves, supplied by the whole SDK vector unit.
No new source-tier macro or success stub was introduced. Full PRD scope remains
active; source gaps do not constitute an external blocker.

0247 ports complete JAISequenceHeap's retained-buffer cursor arithmetic to native
addresses. It preserves the original strict-less-than capacity rule, 32-byte
rounding, cache numbering and counters, and rejects requested-size overflow
without mutating state. Public checks exercise high native addresses, rounded
allocations, exact-limit rejection, exhaustion and original cache lookup.

The first real --interface run failed UBSan in JKRArchive::mount: its explicit
four-byte alignment constructed a native JKRDvdArchive at an address ending in
0x934. 0248 preserves the factory's head/tail direction but uses native object
alignment, with compile-time checks for all four concrete archive classes. The
next run completed archive creation and original AAF/pool/effect/camera/heap
initialization, then crashed inside StreamLib::allocBuffer with a truncated
native address. This was a real original startup path, not a synthetic factory
call or a null-pointer substitution.

0249 fixes the owning TSolidHeap, whose previous compile-oriented native code
still stored addresses in 32-bit fields. Native start/current/last addresses now
retain their full width; size/count fields retain their original role. Allocation
accounts for initial alignment loss, checks remaining capacity by subtraction,
rejects negative sizes and preserves exact-fit solid allocations. Null/invalid
reinitialization clears its owned range. StreamLib's outer/inner pointer tables
and size estimate now use actual native pointer widths while preserving the
original per-allocation padding policy. Public tests check unaligned starts,
negative sizes, exact fit, adjacent guards, exhaustion and freeAll/reset.
Streaming decode and DSP sample mapping remain unimplemented; allocation success
does not imply stream playback.

Full initInterface now links and reaches original SeMgr::startSeSequence. Its
unmodified assertion fails because seHandle remains null. A private temporary
copy of the whole SequenceMgr unit instruments storeSeqBuffer and aborts at that
entry. It observes ID 0x80000800, fade 1, owner 4, a valid SoundInfo and mounted
archive, and a null destination handle. The same observation verifies the
initialized stream owner: 31 aligned, bounded, mutually disjoint allocations,
920 bytes remaining, and an aligned archive. It then deliberately aborts; this
is initialization evidence, not a passing full-interface test. The original
SequenceMgr source and SE assertion were not altered by the observation.

The pinned storeSeqBuffer is an empty Nonmatching body. The component review
finds fifteen empty SequenceMgr functions spanning 6,752 retail bytes, including
registration, entry/cache processing, DVD completion, read-to-track admission,
updates, stopping and release. Current upstream at the inspected immutable commit
also leaves these functions empty; upgrading the pin would not supply this
lifecycle. [Inspected upstream source](https://raw.githubusercontent.com/zeldaret/tww/11d6aa597a6b6218a38eff2726c841a31f851a57/src/JSystem/JAudio/JAISequenceMgr.cpp).
No upstream code was imported and neither dependency pin was changed.

This changes the next experiment from allocator/link repair to source restoration.
Retail traces from the supported private disc cover all fifteen functions and
map direct calls against the pinned symbol table. They retain fourteen unsupported
instruction words as explicit raw fallbacks; do not treat those as decoded or
proven equivalent. The review used the already available offline Capstone 5.0.7
runtime, with exact script, DOL/function hashes and outputs kept private. A
version-constrained offline resolver attempt failed despite the available runtime;
the existing offline environment succeeded, without dependency installation or
reconfiguration of the selected runtime.

The coherent next scope is sequence registration and admission: storeSeqBuffer
calls original handle checks, track stop/reuse, LinkSound allocation, SeqParameter
initialization and JAISound::initParameter. checkEntriedSeq then selects actual
sequence-cache storage and synchronous/asynchronous ResArcLoader operations;
checkDvdLoadArc owns completion, and checkReadSeq supplies data to TTrack and
starts it. Restore these effects and their release/update behavior together.
Do not fabricate a non-null seHandle, skip startSeSequence or declare a returned
initInterface sufficient while downstream sequence routines remain empty.

Final strict public/private heap and AAF tests, memory/file/dry/public-FX/private-FX
lifecycle modes, native DSP, resource-path and sequence-callback controls, private
32-wave playback and 180-frame headless world all exit 0 (twelve runs). The full
--interface probe remains exit 134 at the original SE-sequence assertion. The
instrumented observation also aborts deliberately after checking initialized
owners. Old graphics/live-input/performance limits remain unchanged. No device,
GUI, full interface playback, new game acceptance or all-mode checkpoint claim.

Source replay through 0249, diff whitespace and the unchanged repository audit
pass. Protected CPU/HLE files and the dependency lock are unchanged; twenty
formerly tracked private research files remain ignored and hash-identical. WIP
is uncommitted over tracking-only a954ee5; runtime checkpoint ab8a9e9 remains the
last qualified committed runtime. Complete WIP/ref diffs, untracked sources,
commands, logs, source review and hashes are saved in private
checkpoints/reorientation-20260909-audio-interface. This is recovery of both
qualified repairs and the failing integration frontier, not product acceptance.
Continue with the coherent original sequence lifecycle, then normal startup,
original scheduler/gameplay, device integration and the full PRD.


## 2026-09-09 — original sequence registration and release (0250)

The preceding user-facing progress answer made no implementation change; this
continuation revalidated the worktree and resumed the next available source
restoration. GZLE01 registration is no longer empty: original priority, loading
rejection, replacement, cross-track volume flags, pool admission, parameter init,
stop and fade/release requests now run in the complete SequenceMgr owner. The
initializer is restored from 616 retail bytes, including native typed port/cache/
wave-wait fields, without resetting fields the original retains. The complete
HeapMgr source moved into the shared track runtime, avoiding duplicate ownership.

The prior generic PPC traces mislabeled Gekko paired-single saves as modern VSX
instructions. The corrected private decoder explicitly handles primary opcodes
56/57/60/61 and retains opcode 4 raw. The original register initializer proves
SDA2 base 0x803ffd00; constants used here decode to 1, 0.5 and 0. The complete
review contains 17 functions and 7,396 bytes, including register/parameter init;
12 SequenceMgr bodies (5,748 bytes) remain empty. No upstream repin was made.

Public and private original-owner tests pass parameter reuse, retained fields,
priority and loading rejection, pool exhaustion, BE duck/exemption flags, fade
requests, cached-slot release and all three pool entries returning. An initial
test setup set control count before play count; original setParamSeqPlayTrackMax
also changes control count, so the corrected test sets control count last.
Twelve strict regressions pass, including all existing AAF/driver/FX modes,
sequence callback, private waves and the rebuilt 180-frame headless world.
World displacement is 104.331, 119 changing foot matrices, final released speed
zero and camera center displacement 105.693. These remain diagnostic metrics.

Actual private initInterface now passes original startSeSequence and returns
with ID 80000800 and linked sound/parameter/update owners. Its original
checkEntriedSeq call does nothing: state 0, pending bit 1, root 0. The updated
frontier explicitly asserts on that state, before the unrelated public waveform
oracle can imply sequence success. An earlier unguarded run reached that oracle
and failed parent-heap recovery at teardown; retain that additional open issue.

Next restore cache selection and the complete archive load/completion owner,
then read/track admission and update/release processing against the corrected
traces. Preserve asynchronous ownership and cancellation rather than setting
state by hand. Do not stop at a valid handle, bypass SE startup or infer audio
acceptance from synthesized-wave controls. Normal boot/resource handoff,
original scheduler, live gameplay, device timing and full PRD acceptance remain
open. Source replay through 0250 and repository audit pass. Implementation is
uncommitted WIP over a954ee5; runtime checkpoint ab8a9e9 is unchanged.

Private evidence: `local-research/evidence/reorientation-20260909/audio-sequence/`.
Recovery: `local-research/checkpoints/reorientation-20260909-audio-sequence/`.


## 2026-09-09 — original game sequence loading (0251)

Previous goal turn: progress (0250 original registration/release and qualified
controls). This continuation revalidated that state, then restored checkEntriedSeq,
checkDvdLoadArc and JAIBasic::getSoundOffsetNumberFromID from retail instructions.
Complete JASResArcLoader, JASDvdThread and JKRThread owners are available in the
shared track runtime; existing explicit object owners do not duplicate symbols.
The loaded-data field is a full native pointer, and the original synchronous
message result is compared as a native OSMessage. Native sequence admission,
cancellation and DVD completion synchronize ownership, releasing the lock around
blocking operations and publishing requests before dispatch. Future restored
frame/track routines must participate in this ownership protocol.

The real sound record proves the async stay path: SE 80000800 maps to archive ID 5,
flags ffff10, 115,456 bytes. Pinned HeapMgr's original setter unconditionally writes
sAutoHeap[255] when passed the stay sentinel. Its five retail instructions confirm
that behavior. The native setter now leaves automatic status untouched for 0xff,
consistent with completion/release semantics. The public sentinel oracle fails
when a private whole-unit negative control restores the old write. A guarded
asynchronous stay load completes through the real DVD worker.

A public serialized archive exercises direct/BE ID formats, actual synchronous
and asynchronous reads, byte equality, cache reuse, delayed cancellation followed
by replacement, capacity rejection, asynchronous stay allocation and full parent
heap recovery. The private test uses the actual AAF mapping and verifies all SE
bytes against uncompressed input with output guards. An initial private check
read archive ID 0 (15,008 bytes); it was not the SE resource and was corrected to
follow the actual original sound table. That superseded observation is retained.

The generic interface first stopped at its 65,536-byte stay capacity. Complete
JAIZelBasic/JAIZelSound/JAIZelParam compile and link; the new game-audio probe calls
original JAIZelBasic::init, which configures the authentic 118,784-byte stay heap.
An initial harness placed AAF registration before construction and was corrected.
The next run exposed the missing application ARAM service startup: the saved LLDB
thread trace shows the DVD worker waiting in JKRAramStream::sync for a real initial
wave load. The probe now invokes complete JKRAram::create with JFWSystem's original
8-MiB audio / 6-MiB graph partitions and 8/7/6 priorities. It does not bypass waves
or enlarge the sequence cache. Its parent arena is the existing diagnostic 8 MiB;
normal application memory/boot/teardown remain unqualified.

With the original services, JAIZelBasic::init returns: resource 5, size 115,456,
state Loaded (2), entry pending 0, DVD pending 0, root inactive (0). All sequence
buffer bytes match an independently indexed raw archive range. The following
original processGFrameSequence still leaves state 2/root 0; the explicit frontier
assertion fails instead of accepting loaded data as playback. Ten SequenceMgr
bodies (4,596 retail bytes) remain empty. Next restore checkReadSeq and coherent
track parameter/update/admission, then the original frame owner and playback.
Other incomplete JAI/game bodies and normal startup/resource handoff remain open.

Fourteen strict regression runs pass, including the rebuilt 180-frame world,
all prior AAF/driver/FX controls, sequence callback, private waves and the two new
loader modes. No all-mode, GUI, live input, device timing, sequence playback,
normal-boot or product acceptance. Source replay through 0251 and repository audit
pass. Protected hashes, twenty private research files and empty index are verified.
Implementation is WIP over a954ee5; runtime checkpoint ab8a9e9 is unchanged.

Private evidence: `local-research/evidence/reorientation-20260909/audio-sequence-load/`.
Recovery: `local-research/checkpoints/reorientation-20260909-audio-sequence-load/`.


## 2026-09-09 — original game sequence execution (0252–0254)

Previous immediate goal response: status only, no implementation progress. This
continuation revalidated the worktree and terminal build handle (60102 had exited
with a missing matrix declaration), then continued the existing implementation.
The full PRD objective is unchanged. The last implementation recovery was 0251.

0252 restores ten original SequenceMgr admission/frame functions plus sound
parameter dependencies and the JAIBasic frame owner. Review covers 23 original
functions / 7,936 retail bytes, including explicit Gekko load/store decoding and
ordered floating-point comparisons. These are source reconstructions, not a
claim of bit-exact numerical/hardware equivalence or complete spatial audio.
The original frame owner runs dummy cleanup, second-stay wave admission, SE,
sequence and stream processing in retail order, then increments its frame count.
The probe uses JAIZelBasic::gframeProcess rather than calling sequence phases.

Native numeric port arguments formerly inherited the 8-byte stride of a union
containing a pointer. TPortArgs has 32-bit numeric fields. Native PlayerParameter
now owns the typed record, with byte-copy setters/readers into the numeric words;
console layout is retained. Public tests exercise all numeric ports, original
queued volume/pitch/pan/fx/dolby/tempo delivery, multiplicative fade interpolation,
additive clamps and track bit 31 isolation. A private whole-unit negative control
restoring the old stride fails the public value oracle.

Reaching TTrack::startSeq pulled in complete JASSeqCtrl and JASSeqParser, which
were absent from the shared track runtime. 0253 ports cached console-address
jumps through original OS MEM1 translation and keeps debug string formatting
typed. No integer-width suppression or fabricated parser success was added.
The first real audio buffer then hit UBSan: readReg16 indexed element 8 of a
six-element array to reach an adjacent named register field. 0254 maps the full
register record explicitly, preserving console high-word order for 32-bit
members. Public controls exercise all 24 physical halfwords, named pan/bank
fields, all four address-register jumps, relative jumps and typed formatting.
This does not establish every logical opcode or malformed bytecode behavior.

Actual SE 80000800 still matches all 115,456 raw archive bytes. Original frame
and audio callbacks now change Loaded (2) to Ready (3) then Playing (4), advance
the real cursor and create two active root children. Sixteen headless DMA buffers
produce 112 DSP subframes and 17 original game audio frame-owner calls. No sound
is requested in that mode; zero PCM is observed and not treated as playback.
The sample consumer is the clock; no audio device or GUI was launched.

The original frame owner exposed two diagnostic setup/teardown problems. Its
second-stay phase failed an ARAM allocation with the earlier 8-MiB argument;
mDoAud_Create actually passes 0x00a00000. The probe now uses that original limit,
while retaining its diagnostic 8-MiB parent DRAM arena. After a real DVD fence,
all nine first/second-stay groups have TWaveArc loaded state and bounded positive
allocations. Worker activity originally outlived native static queue/mutex owners
and caused an exit exception. The probe now drains DVD work and joins the host
workers before exit. Full game heap destruction and normal ARAM/graphics sharing
are still outside this diagnostic's acceptance scope.

The explicit `--request` experiment calls the game's original seStart for
JA_SE_CURSOR_MOVE_1 after track readiness. It fails UBSan at the end of the
pinned value-returning method with no return. Its retail body is 7,728 bytes;
zeldaGFrameWork (4,244 bytes), checkNextFrameSe (1,892 bytes) and checkPlayingSe
(1,352 bytes) also remain unimplemented. Upstream was rechecked at immutable commit
`01edd8c27a7430157b0119145b401b7bb0d52d0d`: all four bodies remain unfinished in
[JAIZelBasic](https://github.com/zeldaret/tww/blob/01edd8c27a7430157b0119145b401b7bb0d52d0d/src/JAZelAudio/JAIZelBasic.cpp)
and [JAISeMgr](https://github.com/zeldaret/tww/blob/01edd8c27a7430157b0119145b401b7bb0d52d0d/src/JSystem/JAudio/JAISeMgr.cpp).
The dependency pin is unchanged. Next restore the coherent game
request/dispatch/frame owners from retail evidence and verify a real
requested voice through the existing sequence/DSP path. Do not route around the
game entry point or treat a synthetic DSP voice as requested game audio.

Fifteen rebuilt strict regression runs pass, including actual game sequence,
public/private loader controls and 180 world input/update/collision/camera/draw
frames (104.331 displacement, 119 changing foot matrices, released speed zero).
Source replay through 0254 and repository audit pass. No all-mode, visible/live,
device, normal boot, save/transition, complete gameplay or full-PRD acceptance.
WIP remains over a954ee5; committed runtime checkpoint ab8a9e9 is unchanged.
Private evidence: `local-research/evidence/reorientation-20260909/audio-sequence-track/`.
Recovery: `local-research/checkpoints/reorientation-20260909-audio-sequence-track/`.

## Original game requests and coherent game-state ownership — TWW 0255–0259

**ADVANCE, requested playback still failing.** The previous conversational
status answer changed no state. This continuation revalidated the pending build
handle as terminal, resolved its actual ownership/portability failures and ran
new executable controls. Full PRD scope remains active; no external blocker.

0255 restores the entire 7,728-byte retail seStart method, rather than a cursor
special case. Native fields retain full position and sound-handle pointers; the
eight retained positions use their real Vec type. A static dispatch comparison
finds all 108 special IDs with no missing or extra labels; this is a drift check,
not proof of every branch's behavior or floating-point equivalence. Executable
controls use real JAIZelBasic initialization and private sound tables. They check
pause/suppression gates, ring wrap, distinct pitch/volume arguments, signed reverb,
same-position duplicate reuse, distinct native position identity, slip/dialog ID
remaps, twelve parameter-curve points, distance parameters and retained positions.

The control exposed the original empty surround setter: a request for 0.3 kept
the default 0.236220479. 0259 restores setSeInterFxmix/setSeInterDolby, including
random-range forwarding and original parameter movement. Retail review covers
both 160-byte bodies and their 0/1 range constants. The controls check immediate
values and two-step interpolation. A whole-unit negative control reinstates the
empty setters in the final composition and fails the 0.3 value assertion.
They remain unfinished in the checked immutable
[upstream JAISound source](https://github.com/zeldaret/tww/blob/01edd8c27a7430157b0119145b401b7bb0d52d0d/src/JSystem/JAudio/JAISound.cpp).
No dependency was repinned.

Restoring real demo checks requires g_dComIfG_gameInfo. The probe now links its
original typed definition and complete game-info/save units. A shared static
library compiles 55 full original units without new tier macros, including
collision, attention dependencies, graphics, particles, vibration and sea. The
original complete draw-list, extension and attention objects also participate.
This is the dependency closure of actual constructors and vtables, not raw
zeroed storage or an isDemo=false replacement. Linking alone does not qualify
those subsystems' complete gameplay behavior.

Three portability/ownership repairs accompany that composition:

- 0256 excludes the console-only in-place DZB relocation on TARGET_PC; it casts
  serialized 32-bit fields to pointers and cannot operate on native layouts.
  Native resource ownership already uses bounded NativeDzb decoding/lifetime.
  All native collision-runtime methods compile, and public DZB controls pass.
- 0257 shares the original empty dStage_SetErrorRoom through the stage-runtime
  owner. Pinned source and the four-byte retail return instruction agree. The
  older camera failure fence is retained only where that runtime is absent.
  This does not replace a required room operation with a fabricated no-op.
- 0258 keeps save-buffer cursor differences at native pointer width, including
  their diagnostic formats, allowing the complete save unit to compile. This
  provides no save serialization, endian compatibility or roundtrip acceptance.

Full d_stage.cpp was freshly compiled with the same platform flags. It still
fails on 11 errors: five serialized metadata casts and six pointer-width casts
in room/path relocation. Existing stage/native metadata components therefore
remain. Their retirement needs coherent native resource handling, not merely
wider casts or new tiers. The earlier 14-cast report is not today's measured
compile count. Production link censuses were not rerun for this increment.

The original vibration destructor initially trapped on a null gamepad after
otherwise completing the idle run. The fixture now initializes JUTGamePad and
retains a real port-one object through global game-state destruction. It does
not claim full mDoCPd_Create/GBA startup or normal boot. Worker fences/shutdown
remain intact. Heap teardown and physical audio/input devices are unqualified.

Process and resource libraries now share one stderr diagnostic owner. This
removes duplicate OSPanic/OSReport_Error/JUTAssertion definitions and retires the
process-side silent reporting copies. An executable control retains process
services, observes all seven report/error/confirm/assert/log/warning/panic
messages and verifies that panic aborts. Reporting survives the owner consolidation.

Nineteen rebuilt strict regressions pass: the previous fifteen audio/sequence/
world runs, real request controls, native DZB, foundation and process smoke.
The world still records 104.331 displacement, 119 changing foot matrices and
zero speed after release, with 105.693 camera-center displacement across 180
frames. The idle audio sequence executes 112 DSP subframes with two active root
children and nine loaded resident wave groups. Those are diagnostic results,
not normal-boot, live-input, save, device-audio or full-game acceptance.

The explicit `--request` run now registers JA_SE_CURSOR_MOVE_1 successfully but
ends with that sound Stored (1), track 255 and zero nonzero samples. The nonzero
PCM assertion fails. Source still has empty checkNextFrameSe/checkPlayingSe,
parameter dispatch helpers and zeldaGFrameWork. Next restore the coherent
original SE admission/dispatch/frame path from the retained retail evidence,
then require actual requested PCM through the existing sequence/DSP engine.
Do not bypass the game entry point or create a diagnostic voice to pass it.

Source replay through 0259 and repository audit pass. Protected files, all 20
private research files and the empty index were checked. No GUI/audio device was
launched. WIP remains over a954ee5; committed runtime ab8a9e9 is unchanged.
Private evidence: `local-research/evidence/reorientation-20260909/audio-game-se-request/`.
Qualified recovery: `local-research/checkpoints/reorientation-20260909-audio-game-se-request/`.


## Original requested PCM and complete voice admission — TWW 0260–0264

State: `RETRY_NEW_HYPOTHESIS`; full PRD goal remains active. The preceding
status-only answer produced no new evidence; this continuation resumed the
confirmed build handle, observed its successful terminal result, and advanced
actual original sound playback. Twenty-one rebuilt strict regressions pass.

The unmodified game cursor request, JA_SE_CURSOR_MOVE_1, now reaches Playing (4)
on track 0, produces native PCM, and releases its handle through original
retirement. Its genuine sequence bytes, bank/instrument and loaded wave owners
feed the original audio thread and the existing opt-in DSP candidate. No test
voice or decoded sample was inserted. Before the request, the output is silent;
idle and request-suppression runs stay silent throughout. All modes stop and join
the original worker and fence DVD completion. Nine resident wave groups remain
loaded within the original 10-MiB ARAM upper limit.

Changes and provenance:

- 0260 restores SE ranking/admission, active-track dispatch, sequence mute,
  parameter combination/queueing and original track-port writes from the retained
  private retail comparison. Six external parameter pointers now have their
  native pointer types. Full original units remain composed; no new tier macro.
- 0261 avoids an unused old-destination read for assignment/random/table-load
  sequence operations on TARGET_PC. Retail reads beyond the register record for
  write-only aliases even though these operations discard that value. Arithmetic
  still reads its original destination. Public alias writes and arithmetic pass.
- 0262 backports complete bank noteOn/gateOn and 0263 completes logical-channel
  admission, unlink, transfer and limit owners from licensed TWW source at
  `01edd8c27a7430157b0119145b401b7bb0d52d0d` (CC0-1.0). The dependency pin is
  unchanged. Adaptations preserve pinned field names, explicit 32-bit ARAM
  addresses and unsigned key shifts. Original channel-list and bank instructions
  were compared privately. Source:
  [JASBankMgr](https://github.com/zeldaret/tww/blob/01edd8c27a7430157b0119145b401b7bb0d52d0d/src/JSystem/JAudio/JASBankMgr.cpp),
  [JASChannelMgr](https://github.com/zeldaret/tww/blob/01edd8c27a7430157b0119145b401b7bb0d52d0d/src/JSystem/JAudio/JASChannelMgr.cpp).
- 0264 decodes the original mixer word numerically instead of native byte/bitfield
  aliasing. Automatic mixing bypasses manual-bus setup: its 0xffff sentinel is
  outside the bus table and those manual routes are not consumed by the positional
  DSP path. Invalid used bus indices still fail. Public left/right/effect endpoint
  controls exercise the manual mixer; the actual cursor uses automatic mixing.

The failures were measured in order: register index 46 rejected by the native
bank; empty bank noteOn; empty getLogicalChannel; then manual bus index 255.
The first successful cursor replay returned 1,072 nonzero samples. After stronger
admission/pre-request silence/retirement assertions, a control replay returned
4,262 and the regression replay 1,636. This variability and visible original
DSP load-shedding prevent any deterministic fingerprint, duration or fidelity
claim. Establish the consumer/load clock relationship before device acceptance.

The public channel control verifies admission, middle unlink, four-list transfer,
owner/count updates, normalized pan weights, six manual bus endpoints and gate
updates. An initial test incorrectly expected pan weights of one; inspection
showed original normalization to one-third. The oracle was corrected without
changing runtime behavior. A private executable with only SE admission emptied
fails the new Playing-state assertion, proving the positive request needs the
restored owner. Original empty/mixer failures remain recorded separately.

Regressions: all previous nineteen audio/sequence/resource/world checks plus SE
registration and actual cursor PCM pass under ASan/UBSan. The 180-frame world
still records 104.331 displacement, 119 changing foot matrices, zero released
speed and 105.693 camera-center displacement. This replay is headless; it does
not renew visible/live-input acceptance. Source preparation through 0264 and
repository audit pass. No production link census, device or GUI was run.

Remaining measured limits and source findings:

- Original zeldaGFrameWork is still empty; complete game-frame audio behavior
  and normal logo/audio initialization/handoff remain unqualified. Compose the
  original startup archives before METER; do not skip HUD or inject one archive.
- writeRegParam retains the upstream uninitialized condition flag in several
  write-only aliases. Retail stores the inherited CPU register there. The
  discarded destination read fix does not establish those flag semantics.
- checkLimitStart's nonadvancing loops exist in both this licensed donor and
  the inspected retail instructions. They were preserved, not exercised as
  general voice-limit acceptance. Cursor playback does not qualify these paths.
- Multi-camera prioritization, contention/preemption, complete spatial audio,
  streams, device synchronization, audio fidelity and restart remain open.
  The DSP candidate is still opt-in and is not linked into world/product targets.
- Existing full-stage relocation work, normal boot, save/reload, live held-key
  qualification, original speed and complete gameplay remain open.

Next: restore the whole original game-frame audio owner, measure the clock and
load-shedding limitation, then carry this real initialization/playback path into
original logo preload/deletion and scheduler/Painter world admission. Preserve
all full PRD gates; requested PCM is one integration result, not product audio
acceptance. No external user action is required for the next implementation work.

Private evidence: `local-research/evidence/reorientation-20260909/audio-se-dispatch/`.
Recovery: `local-research/checkpoints/reorientation-20260909-audio-se-dispatch/`.
WIP remains over a954ee5; committed runtime ab8a9e9 is unchanged.


## Sample-clock diagnosis and original startup composition — 2026-09-09

State: `RETRY_NEW_HYPOTHESIS`; preceding turn classified as progress and its
195-file recovery manifest verified before editing. TWW patches remain 0264;
no original runtime or load-shedding threshold was changed in this increment.

The original DSP admission loop compares the first subframe interval with each
later interval, using OSGetTick and DSP_LIMIT_RATIO=1.1. The burst consumer was
feeding 8,960 stereo frames in approximately 0.10 seconds, well ahead of the
original 32,028.5-Hz sample cadence. It therefore changes the wall-time workload
that this original rule observes. The game probe now accepts `--paced-request`:
a headless consumer schedules cumulative consumed samples using the original
Kernel::getDacRate, avoiding per-buffer timing drift. This changes only the
consumer, not OS time, the renderer, the load-shedding rule or game state.

Three burst runs produced 1,256, 3,862 and 4,262 nonzero samples with different
PCM fingerprints. Their retained interval histories crossed the original
load-shedding threshold 7, 5 and 2 times. Three paced runs each produced 4,262
nonzero samples with the same private FNV-1a fingerprint, zero threshold
crossings and no free-channel load-shedding messages. The minimum first/later
interval ratios were 11.67–12.40, comfortably above 1.1. These paired results
support burst scheduling as the cause of the observed variation. Computed
threshold crossings are not a separately instrumented count of voices killed.
Paced consumer duration was approximately 0.2874 seconds including final worker
completion. Exact private commands, histories and fingerprints are retained.

This is a repeatability control for the same implementation, not a hardware
comparison, audible test, device clock, performance budget or complete audio
gate. The diagnostic still calls game-frame processing at its existing buffer
checkpoints; it does not establish the original video/game/audio relationship.
A device consumer must supply its actual sample clock, and synchronization with
original game-frame work remains unqualified. Keep the burst mode as an explicit
stress diagnostic; use paced mode for the next requested-audio comparisons.

A fresh private link composition replaces diagnostic audio slices with the
existing complete audio owners while retaining the original LOGO profile root
and world composition. This exposes five missing original scene tables. The
complete, unchanged JAIZelScene.cpp is now included in game_audio_state. No new
source tier, table transcription or runtime bypass was introduced. This source
supplies scene/island music metadata, spot names and first/second wave tables.
The subsequent private link has no duplicate definitions and exactly nine
undefined native-platform services:

- GXAbortFrame, GXGetCurrentGXThread and GXSetCurrentGXThread;
- LCDisable;
- OSGetProgressiveMode and OSSetProgressiveMode;
- OSGetResetCode, OSResetSystem and OSSetSaveRegion.

These come from original LOGO and mDoRst_reset paths. The former twelve-symbol
snapshot is stale. The experiment has not executed, and the existing world
executable still uses its established composition. Missing runtime behavior such
as zeldaGFrameWork, sceneChange and dynamic-wave transition handling is not
proved by link closure; do not replace the remaining native calls with silent
success merely to make a link pass.

Next: use the paced consumer when restoring original game-frame audio; finish
native reset/display ownership and carry complete audio plus original logo
preload/deletion into scheduler/Painter admission. The missing METER archives
still need that actual lifecycle, not injected pointers. Full PRD goal remains
active; no external permission or data is needed for these implementation steps.
Private evidence: `local-research/evidence/reorientation-20260909/audio-sample-clock/`.

Qualification: twenty-two strict regressions pass, including the new paced
request and the existing 180-frame world test. The final paced replay matches
the prior three fingerprints and again returns 4,262 nonzero samples. Repository
audit and diff whitespace checks pass. TWW/Aurora source diffs and patch files
are byte-identical to the preceding qualified replay; its source-reproduction
result remains applicable. Protected files, all twenty private research files
and the empty index were verified. No GUI/device opened; nothing committed.
Recovery: `local-research/checkpoints/reorientation-20260909-audio-sample-clock/`.


## Original cold-start creation and native heap lifetime — 2026-09-09

The loop resumed implementation after a status-only turn (classified no progress).
The fresh experiment composes complete LOGO, resource loading, original graphics,
original main (entrypoint renamed only in the private compilation), and complete
audio owners. It invokes the original LOGO profile's Create method; this is not
scheduler-driven boot. Retained raw game-info fixture storage and world test
helpers are absent from this link. The original game-info owner supplies its
normal global construction and `ct()` initialization.

The old nine-symbol failed link was insufficient evidence for absence of duplicate
definitions. Resolving host configuration and admitting the executable exposed
53 linker-reported duplicates, then 102 as other unresolveds closed. Startup
composition removes conflicting diagnostic helpers and places residual world
objects in a dependency archive so only required owners are admitted. The final
link map records whole main/resource/LOGO and the original game-info owner; its
link log has no duplicate definitions. This does not qualify full world+audio
composition or permit suppressing linker errors.

Changes driven by reached behavior:

- Host reset-code query reports a logged cold process. Progressive preference
  retains the SDK low bit within the process. No native restart or persisted SRAM
  behavior is claimed.
- The first heap fixture accidentally made the 4-KB command heap JKR's system
  heap. Correcting system-before-command creation restored ARAM worker allocation.
  The fixture uses an 8-MiB system child and 4-MiB Zelda child, with original USA
  command/archive/game sizes; these are diagnostic budgets, not full boot sizing.
- The selected diagnostic dRes loader rejected `/res/Object/System.arc`. Patch
  0265 composes the complete resource owner: complete heap declarations, native
  BAS offset arithmetic with bounds assertion, native DVD completion acquisition,
  and existing native DZB decoding without invoking the retail relocation again.
- Real DVD startup clears its allocation context. The old shared current-heap
  pointer also cleared the game thread's context and broke original solid-heap
  restoration. Patch 0266 makes native current-heap selection thread-local while
  retaining console behavior. A synchronized two-thread regression fails on the
  old state and passes with the port.
- Original `mDoGph_Create` now initializes display/fader/draw-list owners. The
  fixture reserves reset state from the arena before root creation and applies
  original cold-reset initialization. Whole LOGO Create then returns complete and
  queues the original archive requests. A command appended to the original DVD
  queue proves preceding preload commands retired before host worker shutdown.
- Original game-info destruction then reached an already-destroyed host mutex
  registry. LLDB traced the failure through dRes/solid-heap destruction to the
  registry mutex. SDK registries now have process lifetime; explicit shutdown
  still closes queues and joins workers. A global-destructor mutex regression
  fails before this change and passes after it. This retains registry bookkeeping
  through exit; it is not proof of bounded registry use across repeated sessions.

Validation: 24 strict regression runs pass after the final host change, including
180 world frames (104.331 movement, 119 changing foot matrices, final speed zero),
original audio registration/sequence/DSP paths, and both new lifetime controls.
Paced cursor output remains 4,262 nonzero samples with fingerprint
`f191eecd3a256c56`. Source preparer replay through 0266 passes. Protected three
files, all 20 private research files, dependency pins and empty staging index are
preserved. No GUI/audio device was launched, and no commit was made.

The final cold-start run exits zero under ASan/UBSan after whole LOGO Create and
DVD queue drain. Its original solid heap reports unsupported individual frees
and a size query; those warnings are retained and bulk lifetime reclamation is
not accepted. No LOGO draw/delete, preload-success audit for every archive,
common-particle creation from the loaded JPC, opening transition, or METER
admission is proved. Complete audio owners are linked, but game audio startup is
not executed in this fixture. The existing world executable retains its prior
diagnostic bootstrap.

Six private reset-path fences remain (GX abort/thread transfer, locked-cache
disable, save-region registration and system reset), plus the existing unsupported
GX metric path. MAT2 material and BLS cluster loading also fail explicitly if
reached. Compiling those full owners measured a MAT2 declaration/definition width
mismatch and a BLS index narrowing; source inspection additionally finds unported
serialized pointer/endianness layouts. Do not fix only their compile errors and
claim those formats work. Full main compiles unchanged with its entrypoint renamed
for this diagnostic, but original main/main01 has not run.

Evidence and exact reproduction commands:
`local-research/evidence/reorientation-20260909/logo-cold-start/` contains
`commands.json`, `build_probe.py`, `probe.cpp`, full-main compile commands,
`link.map`, failed attempts, `run-08.json/log` and `run-09.json/log`, both negative regressions,
LLDB traces, final regression logs and qualification metadata. The dependency
archive is rebuilt from current object files rather than reusing stale members.

Next: preserve this working cold-start path while connecting original LOGO
draw/wait/delete to the real scheduler and frame/Painter lifecycle. Audit every
archive/particle handoff before METER, and integrate original main/audio startup
rather than retaining another permanent hand-called fixture. Native reset, full
stage portability, legacy format decoding, normal transitions/save, actual audio
output, original-speed complete gameplay and Apple mobile work remain open under
the full PRD goal. This is a qualified implementation increment, not boot or
product acceptance.

## Original scheduler/audio startup and user-requested stop — TWW 0267–0268

State: **PAUSED_BY_USER after checkpoint verification and merge**. The user
requested documentation, pushing/merging to main and stopping for today. This
supersedes automatic continuation instructions; the full PRD remains unfinished.
See [the session handoff](SESSION_HANDOFF_2026-09-09.md).

The private `logo-scheduler` experiment replaces manual LOGO Create calls with
original `fapGm_Create`/`fapGm_Execute`, complete `f_pc_manager`, original graphics
API/Painter and native Metal begin/end frames. Complete manager compilation needed
three missing framework asset headers, now generated by the existing verified
private DOL preparer. Whole game-framework, camera-manager, host-IO, JFWSystem and
play-scene sources compile. JFWSystem init and the full play-scene lifecycle were
not executed; their compilation/dead-stripped linkage is not lifecycle closure.
The fixture uses a real JUT console, original scene-delete HIO callback, native
registry and actual heap/process allocation, not hand-constructed process trees.

Attempt 01 progressed through LOGO actions 0/1/3/4/10 and reached the original
opening request, then failed because audio startup was omitted. Adding original
`mDoAud_Execute` before `fapGm_Execute` exposed `JKRSetCurrentHeap(NULL)` member-call
UB. 0267 preserves the original exchange through static heap access. A focused
strict test fails before and passes after, retaining context isolation and actor
heap rollback checks.

Attempt 03 failed native bank allocation under the console 0x166800 audio budget.
A separately measured full initializer consumes 2,172,424 bytes. The fixture's
0x240000 reservation initially failed inside its arbitrary 4-MiB Zelda partition.
Following the original machine initializer's partition order/rule fixes that
fixture limitation: initialize system services, reserve system free space minus
0x10000 for Zelda, retain the original audio heap trimming. Total MEM1 remains
24 MiB. This is diagnostic sizing, not a normal-boot memory policy.

Attempt 05 completes original audio creation, then ASan catches the game DVD
worker's four-byte command-pointer copy at its callback on the JAS DVD worker.
0268 uses `sizeof(cmd)` and native alignment for JAS command objects and payloads.
The focused two-worker regression also reproduced misaligned command construction.
Its final form verifies two above-4-GiB commands, completion and actual JAS-worker
identity with no free warnings. An earlier test-fixture solid-system-heap warning
is preserved in the negative log; the final fixture uses an expandable system
heap for individually retired command allocations.

Final attempt 06 executes 600 scheduler/Painter Metal frame cycles with original
`mDoAud_Create` and subsequent original audio-frame calls. Adjusted audio heap:
2,168,352 bytes. LOGO creation finishes at frame 5, actions advance at 95, 125,
215 and 245, and original DVD-wait reaches `dComIfG_changeOpeningScene`. The
registry has only LOGO; real OVERLAP0/OPENING_SCENE integration is still required,
so LOGO remains in DVD-wait and the experiment ends at its explicit incomplete-
handoff fence. **Exit -6 is recorded as incomplete, not a passing boot test.**
No new pixel oracle, audible/device output, archive deletion/handoff or METER
acceptance is claimed. The solid-heap size-query warning remains. No graphics,
reset or legacy-format fence was reached in the 600 cycles. Full game audio
still contains incomplete behavior; scene-change log values are not playback.

Current accumulated regression: **25/25 strict ASan/UBSan runs pass**, including
world replay, original audio initialization, sequence/request/PCM controls,
resource/collision/process tests and both heap/worker fixes. The paced cursor
fingerprint remains `f191eecd3a256c56` with 4,262 nonzero samples. The asset preparer
passes 11 synthetic tests. TWW source replay through 0268 and Aurora through 0008
pass; repository audit and whitespace checks pass. Protected files, dependency
pins and the 20 private research files remain unchanged. No product gate advances.

Evidence and exact commands remain ignored under
`local-research/evidence/reorientation-20260909/logo-scheduler/`. The recovery
snapshot is `local-research/checkpoints/reorientation-20260909-logo-scheduler/`.
On a new user instruction, integrate original transition profiles and the full
LOGO archive/particle handoff, then normal machine/JFW/main startup. Preserve the
full PRD objective, previous world/audio oracles and frozen Route A.

Final closeout also rebuilds and passes all eight registered BlueWake public
smokes in the strict configuration. These are separate from the 25 accumulated
runs; unbuilt upstream dependency-test placeholders are not included.
