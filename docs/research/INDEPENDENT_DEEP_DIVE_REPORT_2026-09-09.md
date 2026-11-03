# Independent BlueWake deep-dive report — 2026-09-09

> Historical research: the Route B prototype this report describes (a native build from the decompilation) was later removed, so the `route_b` paths it names no longer exist. Current work is in [GOAL_LOOP.md](../GOAL_LOOP.md).

**Author:** independent senior reviewer (out of loop), at the user's request.
**Audience:** the implementation agent running `docs/GOAL_LOOP.md`.
**Status:** advisory and evidence-backed. `CURRENT.md` and the first entry of
`BLOCKERS.md` remain the policy authority; this report supersedes nothing by
itself. It records what the tree actually contains on 2026-09-09.
**Reviewed state:** `main` = `6949c27` plus uncommitted work (section 2).
Static review plus bounded, isolated compile experiments into the session
scratchpad. No probe was run, no GUI was opened, no file under `route_b/`,
`tests/`, `patches/`, `ref/` or `docs/status/` was modified, nothing was
committed. The two protected recompcore files were not touched.

Every material statement is tagged **VERIFIED** (read in a file, log, or
command output during this review), **HISTORICAL** (a claim recorded in the
ledgers, not rerun here), **HYPOTHESIS** (a conclusion from verified facts),
or **UNKNOWN**.

---

## 1. Verdict

**Demonstrated capability (VERIFIED by reading code, logs and ledgers).**
Route B executes original Wind Waker code on macOS in three bounded shapes:

1. A hand-composed diagnostic process runs original Link phase 1/2/3, 180
   input/update/collision/camera/draw frames on real Room44 geometry, and
   captures the original J3D opaque/translucent list consumers into a GX
   command buffer (85,024 bytes first/last frame, HISTORICAL from
   `ROUTE_B_ACTIVE_VERTEX_BINDING_2026-09-06.md`; binary
   `build/route-b-aurora-gx-sanitize/bluewake_route_b_private_player_render_probe`
   dated Sep 6 exists, VERIFIED).
2. A standard-process ROOM_SCENE lifecycle creates Room44 through original
   `dStage_roomInit(44)` and the original process creator, admits `g_profile_BG`
   as a real child, draws its three models into real J3D draw buffers, and
   deletes everything through original delayed deletion (HISTORICAL,
   `ROUTE_B_ROOM_SCENE_BG_CHILD_EXECUTION_2026-09-06.md`).
3. Route B has presented native frames once: the original 2D opening scene
   ran in a 640x480 Aurora Metal window at 60 Hz through Aurora's command
   processor (HISTORICAL, `ROUTE_B_OPENING_SCENE_PRESENTATION_2026-09-02.md`;
   the probes call `aurora_initialize`/`aurora_begin_frame`, VERIFIED in
   `tests/route_b_private_logo_composition_probe.cpp:163-177`).

**Not demonstrated.** No J3D byte has ever been parsed by Aurora's command
processor in Route B. In headless mode Aurora's `fifo::publish()` returns
immediately when no frame is active (`ref/aurora/lib/gx/fifo.cpp:172-174`,
VERIFIED), and the 85 KB captures were produced by FIFO redirection into a
user buffer (`GXBeginDisplayList`), never by `fifo::process`. Therefore the
whole J3D-to-Metal path (vertex layouts, index streams, XF matrix payloads,
texture objects, TEV state from BDL material display lists) is unobserved.
The `--visible` branch of the render probe that would observe it is compiled
and has never been run (HISTORICAL, consistent with build-tree timestamps).

**Primary integration bottleneck (HYPOTHESIS, high confidence).** The loop is
optimizing for link closure of hand-composed diagnostic executables rather
than for the application. Evidence:

- The patched `ref/tww` tree now carries **120 distinct `BLUEWAKE_ROUTE_B_*_TIER`
  macros** and **68 distinct `bluewake_route_b_*` host seams** (VERIFIED by
  grep). `d_stage.cpp` alone has 30 tier references and 24 `#if/#elif` tier
  branches selecting disjoint slices of one translation unit.
- The production link censuses have been frozen at 4/41/52 missing symbols
  across at least the last ten checkpoints (HISTORICAL banners in `CURRENT.md`);
  the diagnostics advanced, the production composition did not.
- The current world-render link failure was caused by the loop itself: the
  uncommitted CMake change removes `bluewake_route_b_player_stage_runtime_objects`
  from the world probe, which is the sole owner of the eight symbols the Sep 7
  build reports missing (section 3, finding F1, VERIFIED).
- Meanwhile the full translation units that the tiers slice up compile on
  TARGET_PC today: `d_s_play.cpp` (0 errors, 179 undefined symbols),
  `d_s_room.cpp` (0 errors, 46 undefined), `d_a_bg.cpp` (0 errors, 62
  undefined), and `d_stage.cpp` fails on exactly **14** relocation casts at
  eight sites (VERIFIED, section 3 finding F5). Whole-unit composition is
  being avoided, not blocked.

**Recommended direction.**

1. Observe the renderer now. Run the existing `--visible` render probe once
   the user authorizes a window. The "Simulator constraint" that has deferred
   this since Sep 6 does not apply (finding F7). Independently, a headless
   parse of the captured bytes through Aurora's own display-list reader can
   expose framing failures without a GPU (experiment E2).
2. Fix the world probe by restoring the stage-runtime owner and finishing
   patch 0210 as an extension of the existing runtime owner, not as a fourth
   copy of `getMemoryBlock` (finding F1/F2, experiment E1).
3. Stop adding tiers. Move to whole-unit composition for the four scene/stage
   units listed above, driven by the original process manager
   (`fpcM_Management`) instead of hand-called phase functions. That is the
   only path that leads from a selected-scene diagnostic to normal boot.
4. Keep Route A frozen as the behavioral and visual oracle. Nothing found
   here overturns the 2026-09-01 route decision.

---

## 2. Snapshot and access

### 2.1 Revisions (VERIFIED)

| Item | Value |
|---|---|
| Repository HEAD | `6949c27` "Preserve active J3D vertex bindings and prepare optional Metal presentation", 2026-09-06 22:57 -0500 |
| Dirty tracked files | `docs/archive/status/BLOCKERS.md`, `docs/archive/status/CURRENT.md`, `route_b/CMakeLists.txt`, `scripts/prepare_route_b.sh`, `tests/route_b_player_init_services.cpp`, `tests/route_b_player_phase_three_fences.cpp`, `tests/route_b_private_camera_run_probe.cpp` |
| Untracked | `docs/archive/status/NEXT_MODEL_HANDOFF_2026-09-06.md`, `docs/research/INDEPENDENT_DEEP_DIVE_PROMPT_2026-09-09.md`, `patches/tww/0209-compose-world-teardown-authentic-tiers.patch`, `patches/tww/0210-world-room-memory-tier.patch`, `tests/route_b_world_geometry_services.cpp` |
| `ref/tww` | `03d27aa` (upstream 2026-08-18) + patches 0001-0209 applied + one unregistered hunk in `src/d/d_stage.cpp` (finding F3) |
| `ref/aurora` | `8b690b6` (upstream 2026-08-20) + Aurora patches 0001/0002 applied (`git diff` shows only `GXManage.cpp`/`GXTexture.cpp`) |
| `ref/recompcore` | `a5a7652`, with pre-existing user edits; protected files not touched by this review |
| Disc image | `ref/The Legend Of Zelda The Wind Waker.iso` present |
| Dependency lock | `config/dependencies.lock.json`, modified Sep 6 20:15 |
| Simulator | `Simulator.app` PID 58388 is running (Xcode iOS Simulator). PID 43578 cited in the handoff no longer exists. No BlueWake process is running. |

### 2.2 Timeline of the uncommitted work (VERIFIED from mtimes and logs)

| Time (local) | Event |
|---|---|
| Sep 6 23:01 | The four test files were last edited |
| Sep 6 23:06 | `NEXT_MODEL_HANDOFF_2026-09-06.md` and `CURRENT.md` written (the "stop") |
| Sep 6 23:24 | `/tmp/bluewake-world-render-sanitize-build2.log`: the world probe fails to **compile** `d_tree/d_grass/d_magma/d_flower/d_wood.cpp` because generated asset headers such as `assets/l_Txa_swood_aTEX.h` do not exist |
| Sep 6 23:37-23:46 | `d_tree.cpp`, `JAIBasic.cpp` modified in `ref/tww`; `prepare_route_b.sh` edited (0209 registered); configure log written |
| Sep 6 23:57 | `route_b/CMakeLists.txt` last edited (tier object libraries added; stage-runtime objects removed from the world probe) |
| Sep 7 00:13 | `0209-compose-world-teardown-authentic-tiers.patch` written |
| Sep 7 00:18 | `0210-world-room-memory-tier.patch` written (malformed) |
| Sep 7 00:23 | `ref/tww/src/d/d_stage.cpp` modified (the 0210 hunk applied by hand) |
| Sep 7 19:16 | `/tmp/bluewake-world-render-sanitize-build.log`: link fails on eight `d_stage` runtime symbols |

Consequence: the handoff's statement that "no implementation was changed during
the stopping turn" is true of that turn but the working tree was changed for
roughly 80 minutes afterwards and rebuilt 19 hours later. The handoff's
eight-symbol list describes the state before 0209/0210 existed. Who performed
the later edits is UNKNOWN; treat them as unreviewed WIP.

### 2.3 Commands run during this review

Read-only: `git status/diff/log/reflog`, `git -C ref/{tww,aurora,recompcore}
status/diff`, `bash scripts/prepare_route_b.sh --check` (fails, finding F3),
`pgrep`, greps over `ref/tww`, `ref/aurora`, `route_b`, `tests`, `docs`,
inspection of `/tmp/bluewake-*.log`, and `WebFetch` of upstream
`zeldaret/tww` and `encounter/aurora` commit pages and raw files.

Isolated experiments (outputs only in the session scratchpad, build trees
untouched): `clang++ -fsyntax-only` and `-c` of full translation units using
the exact flags from
`build/route-b-aurora-gx-sanitize/CMakeFiles/bluewake_route_b_escape_restart_objects.dir/flags.make`
minus sanitizers, with no `BLUEWAKE_ROUTE_B_*_TIER` macro defined. Results in
findings F5 and F6.

### 2.4 Evidence not available

- No world-render runtime result exists; the target never linked.
- No `--visible` run exists for any J3D content.
- `/tmp/bluewake-world-render-sanitize-build.log` was overwritten on Sep 7;
  the log that produced the handoff's eight-symbol list is gone.
- No `compile_commands.json` in the private trees; flags were taken from
  `flags.make`.

---

## 3. Findings ranked by impact

### F1 — The current link failure is self-inflicted; the handoff's symbol list is stale (VERIFIED)

The Sep 7 link fails on `dStage_RoomCheck`, `dStage_changeScene`,
`dStage_chkPlayerId`, `dStage_restartRoom`, `dStage_turnRestart`,
`dStage_mapInfo_GetOceanX/Z`, and `dStage_nextStage_c::set`. All eight are
defined in `ref/tww/src/d/d_stage_runtime.inc` (lines 94-236), which is
compiled only under `BLUEWAKE_ROUTE_B_STAGE_RUNTIME_TIER` by
`bluewake_route_b_player_stage_runtime_objects` (`route_b/CMakeLists.txt:4341`).
The uncommitted diff removes exactly that object library from the world probe
(`route_b/CMakeLists.txt:4737-4738`, `list(REMOVE_ITEM ...)`).

Why it was removed (HYPOTHESIS, high confidence): the world probe adds
`bluewake_route_b_initial_room_request_objects` (`d_stage.cpp` under
`BLUEWAKE_ROUTE_B_INITIAL_ROOM_REQUEST_TIER`, lines 920-966) to obtain
`dStage_roomControl_c::getMemoryBlock`. That tier also defines
`createRoomScene`, `setStayNo` and `dStage_roomInit`, which duplicate
`d_stage_runtime.inc`. Linking both produces duplicate symbols, so the
stage-runtime objects were dropped, which produced the current eight gaps.
Patch 0210 is the half-finished attempt to escape this: a new
`BLUEWAKE_ROUTE_B_WORLD_ROOM_MEMORY_TIER` containing only `getMemoryBlock` and
`dStage_roomInit` with `createRoomScene` declared extern, so that the
initial-room-request objects can be dropped and the stage-runtime objects
restored. It is not referenced by CMake yet.

`getMemoryBlock` now has **three** copies in `d_stage.cpp` (lines 928, 1691,
3049): the full-unit body, the one added to the initial-room-request tier by
registered patch 0189, and the unregistered 0210 hunk.

Remedy (concrete): do not create a fourth owner. Add `getMemoryBlock` and
`dStage_roomInit` to `d_stage_runtime.inc` (owned by patch 0176's
`STAGE_RUNTIME` tier, where `createRoomScene` already lives), restore
`$<TARGET_OBJECTS:bluewake_route_b_player_stage_runtime_objects>` in the world
probe, and drop `bluewake_route_b_initial_room_request_objects` from it.
Re-export the change as patch 0210 and discard the current 0210 file. The
escape-restart and teardown tiers from 0209 can stay for now (F4). Verify with
experiment E1.

### F2 — The handoff's eight gaps are already closed except by the CMake regression (VERIFIED)

| Handoff symbol | Current owner in the tree |
|---|---|
| `dStage_escapeRestart()` | 0209, `d_stage.cpp:902-919` (`STAGE_ESCAPE_RESTART_TIER`), linked via `bluewake_route_b_escape_restart_objects` |
| `JAIZelBasic::seDeleteObject(Vec*)` | 0209, `JAIZelBasic.cpp:59-65` (`SE_DELETE_OBJECT_TIER`) |
| `dTree/dGrass/dFlower_room_c::deleteData`, `dMagma_room_c::deleteFloor`, `dWood::Packet_c::delete_room` | 0209, `WORLD_TEARDOWN_TIER` prologues of the five units |
| `dStage_roomControl_c::getMemoryBlock(int)` | registered patch 0189 (initial-room-request tier) and unregistered 0210 hunk |

So the world probe is one CMake correction away from linking, assuming no new
gap appears once the stage-runtime objects return (E1 tests exactly that).

### F3 — `ref/tww` has drifted from the patch series; the preparer check fails today (VERIFIED)

`bash scripts/prepare_route_b.sh --check` exits with
`ERROR: Route B patched file differs from the patch series: src/d/d_stage.cpp`.
Diffing the working file against the series-reconstructed blob shows exactly
one extra hunk: the 0210 `WORLD_ROOM_MEMORY_TIER` block at lines 3039-3065.
`patches/tww/0210-world-room-memory-tier.patch` itself is 13 lines, contains
the placeholder text `inc d_com`, `inc d_stage`, `check defined(X) here`, and
would not apply. Every qualification step that relies on the preparer
(`--check`, whitespace audit, patch replay) is blocked until this is
reconciled.

Clean reconstruction: (a) save the hunk (it is fully visible in the working
file), (b) `git -C ref/tww checkout -- src/d/d_stage.cpp`, (c) rerun
`scripts/prepare_route_b.sh` (it skips already-applied patches and re-applies
the series onto the restored file), (d) write the real 0210 as recommended in
F1 and register it. Do (b) only after (a); the hunk exists nowhere else.

### F4 — Patch 0209 is a horizontal proxy whose justification is unrecorded, and its `JAIBasic::deleteObject` stub is now behind upstream (VERIFIED)

Build2 (Sep 6 23:24) proves why 0209 exists: the complete vegetation units
include ~60 dtk-generated asset headers (`assets/l_*TEX.h`, `l_pos__*.h`,
`l_*DL.h`, `l_*MatDL.h`) that only a dtk decomp build produces, and
`ref/tww/assets/GZLE01` contains only `res/`, no generated headers.
The full units compile to exactly one error each today, the first missing
asset include (VERIFIED, isolated `-fsyntax-only`). 0209 therefore copies the
five deletion methods into per-unit `WORLD_TEARDOWN_TIER` prologues. Goal-loop
rule 24 requires the ledger to record why the original cannot compile; no
ledger entry does. This finding supplies that record.

The correct long-term remedy already exists in the repository:
`scripts/prepare_route_b_player_assets.py` maps pinned DOL addresses from
`config/GZLE01/symbols.txt` through the DOL section table and emits 33
private headers, including `custom_type: Vec` position arrays and `matDL`
display lists. The vegetation headers use the same four `custom_type` kinds
(`Vec` x12, `GXColor` x7, `cXy` x9, `matDL` x10) plus plain `u8[]` textures,
and every address/size is in `symbols.txt` (for example
`l_Txa_swood_aTEX = .data:0x80379CE0 size:0x800`). Extending the preparer to
consume `config.yml` extract entries generically would let the five units
compile whole, which is also the only way trees, grass, flowers and wood
ever get drawn. Until then, 0209 is acceptable diagnostic scaffolding, not
authentic teardown.

`JAIBasic::deleteObject(void*) { /* Nonmatching */ }` in 0209 copies the
pinned upstream state verbatim (`git show HEAD:src/JSystem/JAudio/JAIBasic.cpp`
lines 277-280, VERIFIED). It is reached: `daBg_c::~daBg_c` calls the five
`deleteRoom` methods, which call `mDoAud_seDeleteObject` per data node, which
calls `JAIZelBasic::seDeleteObject`, which calls `deleteObject`. Today it is
behaviorally inert because `JAIZelBasic::seStart` aborts on use in every PLAYER
probe, so no sound ever references an object. Once audio is real, an empty
`deleteObject` leaves registered sounds pointing at freed vegetation
positions. Upstream `zeldaret/tww` main (JAudio PRs #1196 on Sep 5 and
#1197 on Sep 6, VERIFIED via WebFetch of the raw file and commit page) now
implements `deleteObject` (walks `SeMgr::seRegist` per category, stops sounds
owned by the object or hands them to `DummyObjectMgr`) and reduces
`/* Nonmatching */` markers in `JAIBasic.cpp` from 21 (pinned) to 7, with real
bodies for `startSoundVec`, `startSoundDirectID`, `startSoundBasic` and
`processFrameWork`, all of which the 2026-09-06 audio reframe recorded as
empty. `JAIZelBasic::seStart` remains empty upstream (29 markers). Do not
certify teardown on 0209; record the gap and plan a bounded backport of the
upstream `JAIBasic.cpp` bodies as a patch (not a repin of 209 patches).

Also VERIFIED: in the diagnostic, `dComIfGp_getMagma/Grass/Tree/Wood/Flower()`
return pointers from `g_dComIfG_gameInfo.play` (`d_com_inf_game.h:779-783`),
which the PLAYER probe leaves null in raw storage, so `~daBg_c` never reaches
the 0209 bodies in the current world probe anyway. Linking them proves nothing
about teardown.

### F5 — Whole-unit composition of the scene/stage owners is avoided, not blocked (VERIFIED)

Isolated compiles with the private-tree flags and **no tier macro**:

| Unit | Result | Undefined symbols (`nm -u`) |
|---|---|---|
| `src/d/d_s_play.cpp` (full 7-phase play scene) | compiles, 0 errors | 179 |
| `src/d/d_s_room.cpp` (full ROOM_SCENE) | compiles, 0 errors | 46 |
| `src/d/actor/d_a_bg.cpp` | compiles, 0 errors | 62 |
| `src/d/d_stage.cpp` | 14 errors | n/a |
| `src/d/d_tree,d_grass,d_magma,d_flower,d_wood.cpp` | 1 error each: missing asset header | n/a |

The 14 `d_stage.cpp` errors are all of one kind, at eight sites: casting a
serialized big-endian `dStageFileU32` file offset to a pointer
(`d_stage.cpp:2030,2258,2265,2272,2282,2291,2298,2311`) and casting pointers
to `u32` (`2394,2395,2424,2442`). These are the same offset-to-pointer
relocations the existing "native view" tiers (patches 0101-0108, 0112) already
solve for their slices. Guarding those eight sites with TARGET_PC relocation
helpers would make the whole unit compile, retiring most of the 24 tier
branches in that file. The 2026-09-02 census that declared "whole-unit header
portability closed" predates the J3D/JStudio header work; it no longer holds.

This is the concrete evidence that the tier strategy has passed its
feasibility question and is now cost without product movement.

### F6 — The renderer path is proven only up to byte capture; the first real failure is still unobserved (VERIFIED where stated)

What is proven by reading the code and the recorded captures:

- `GXSetArray(attr, data, size, stride, le)` writes a `GX_AURORA_LOAD_ARRAYBASE`
  subcommand with a **64-bit host pointer**, byte size, and endian flag
  (`ref/aurora/lib/dolphin/gx/GXGeometry.cpp:218-234`); the command processor
  consumes it into `g_gxState.arrays[attr]` (`command_processor.cpp:629-647`).
  Patch 0208 routes `J3DShape::loadVtxArray` through this bridge with exact
  spans (VERIFIED in the patch), so the previously reached 31-bit truncation is
  gone.
- Legacy CP array-base writes are rejected by Aurora (`regs.cpp:679-703`), so
  any remaining `J3DLoadCPCmd(0xA0+...)` caller would fail loudly, not
  silently.
- BDL shape display lists are big-endian GameCube draw commands; Aurora's
  reader decodes `GX_INDEX8/16` attributes with `read_bits` defaulting to
  big-endian (`internal.hpp:93`, `dl.cpp` `attr_idx`), so raw shape DLs are
  supported as-is. `GXCallDisplayList` inside a capture copies bytes
  (`GXDispList.cpp:49-60`), so the capture contains the full shape DLs.
- The FIFO processor runs on its own thread (`fifo.cpp:27`,
  `kProcessingMode = Thread`); `end_frame` blocks until processed equals
  published (`fifo.cpp:240-262`). Array pointers are therefore dereferenced
  before the game's next frame overwrites transformed buffers. This is safe
  for the probe (submit then wait) and for the application (Painter draws in
  frame). It is unsafe for any design that replays a captured buffer in a
  later frame after J3D has re-deformed vertices.

What is unobserved (UNKNOWN):

- Whether any XF/BP/CP payload written by J3D's own FIFO writers (matrix
  loads via `J3DFifoLoadPosMtxImm`, GD-built VCD/VAT lists, BDL material
  display lists) is framed the way Aurora's command processor expects. The
  CP has hard `FATAL`s for unknown opcodes and vertex-data overruns
  (`command_processor.cpp:134,367,391`) and the reader fails on "unsupported
  Aurora subcommand" and overruns (`dl.cpp:229` and neighbors). Any of these
  would be the first real renderer failure, and none has had the chance to
  fire.
- Texture upload of J3D `ResTIMG` data through `GX_AURORA_LOAD_TEXOBJ`,
  TLUT handling, and TEV state correctness.
- The 2 MiB capture bound (`route_b_player_phase_three_fences.cpp:542`) is a
  diagnostic-only limit; Room44's `model.bdl` is 399,872 bytes and its shape
  DLs are copied verbatim into the capture, so the world probe may need a
  larger or growable buffer. The application never captures, so this bound
  cannot enter the product.

Experiments E2 and E3 are designed to expose the first failure headlessly
and visibly.

### F7 — The "Simulator constraint" blocking the visible test is a misreading (VERIFIED)

The ledgers have deferred the visible test since Sep 6 pending "permission to
close Simulator PID 43578". The policy sources (`docs/archive/PRD.md` sections 13-14,
`GOAL_LOOP.md` 6.1, the private-probe banners) say: one active operator per
Simulator/device, and never run more than one BlueWake process or Simulator at
a time. The running process is Xcode's iOS `Simulator.app` (now PID 58388,
VERIFIED with `pgrep`), a Route A/iOS artifact. A Route B `--visible` run opens
an SDL/Metal window on macOS; it is neither a second BlueWake process nor a
Simulator. No BlueWake process is running. Nothing in the rules requires
closing the Simulator to run it. What the visible test does require is the
user's authorization to open a GUI, which is a separate and legitimate ask.

### F8 — The world probe's structure, assertions and isolation (VERIFIED)

- **Shared collision owner:** `daBg_c::createHeap` constructs the `dBgW`,
  `Set`s the DZB, publishes it via `dStage_roomControl_c::setBgW`, and
  `daBg_c::create` registers it with `dComIfG_Bgsp()->Regist(bgw, this)`
  (`d_a_bg.cpp:433`). `bluewake_world_create_ground` returns that `dBgW`, and
  the PLAYER ground path asserts `ground.pm_bgd == roomGeometry`. There is one
  collision owner; the prior duplicate diagnostic ground is correctly removed
  under `WORLD_RENDER_DIAGNOSTIC`.
- **Room memory block:** `daBg_c::create` first asks
  `dStage_roomControl_c::getMemoryBlock(roomNo)`. In the PLAYER probe the room
  control storage is raw zero (`mStatus[44].mMemoryBlockID == 0`,
  `mMemoryBlock[0] == NULL`), so BG falls to `fopAcM_entrySolidHeap`. Both are
  original code paths, but retail sea rooms use MEMA-allocated blocks
  (`d_stage.cpp:2476`, `createMemoryBlock`), which only the full stage create
  sets up. The room lifecycle probe fakes this with
  `setMemoryBlockID(44,0); mMemoryBlock[0] = gRoomHeap` (VERIFIED,
  `route_b_private_room_lifecycle_probe.cpp:708-709`).
- **Visibility oracle:** `daBg_c::draw` calls `mDoLib_clipper::clip(model)`,
  which runs `J3DUClipper::clipByBox` and hides whole joints' shapes via
  `J3DShape::hide()` (`J3DUClipper.cpp:150-175`). `mDoExt_modelEntryDL` still
  enters every **material** packet; only `J3DMatPacket::draw` skips hidden
  shapes (`J3DPacket.cpp:354,366`). So the probe's assertion
  `seen[0] && seen[1] && !seen[2] && seen[3]` (`route_b_world_geometry_services.cpp:57`)
  is at packet level and is **independent of clipping**; it will hold even
  when a model is fully culled. It does not require all three models to be
  visible. The source-based oracle for what should be drawn is: after
  `bluewake_world_draw`, read `J3DShpFlag_Hidden` on each shape of models
  0/1/3 and compare against an independent frustum test of each joint's
  bbox with the camera's view/projection and the BG far plane of 100,000.
  Record hidden counts per frame; expect them to change as Link walks.
- **Draw-list order:** the diff's order (BG opa, P0, P1, opa, BG xlu, P1 xlu,
  xlu) matches the original `mDoGph_Painter` (`m_Do_graphic.cpp:1652-1694`)
  minus the Sky list. Fine for the diagnostic; the application must use the
  original Painter, which has zero TARGET_PC lines today (1,959 lines
  unported).
- **Isolation that cannot enter the application:** raw `g_dComIfG_gameInfo`
  storage with selected subobjects placement-constructed; hand-assigned
  process IDs 36/37/38/39 and hand-called `fpcBs_Create/SubCreate`; a
  diagnostic PLAY_SCENE profile whose only phase is original `phase_1`;
  `fpcLy_SetCurrentLayer` by hand; abort fences for `fopAcM_fastCreate`,
  `dStage_SetErrorRoom`, `getMapInfo2`, `seStart`, `linkVoiceStart`; `_Exit(0)`
  at the end of every probe instead of teardown; the 2 MiB capture. These are
  legitimate isolation for a diagnostic, but none of them is reusable in the
  product, and each one hides a production owner that is still missing.

### F9 — The gate ledgers and README are stale for Route B (VERIFIED)

`README.md` describes the Route A state of Aug 30 as "reaches real gameplay";
`FINISH_LINE.md` and `TECH_DEBT.md` TD-011 still state "Route B has not yet
initialized its first graphics frame", which the Sep 2 opening-scene
presentation superseded. `GATES.md` has no P4 milestone table for Route B;
its structured sections stop at P1-P3 evidence summaries. `WAITING_FOR.md`
still cites a Route B trigger of 90% matched functions; decomp.dev reports
75.58% decompiled / 62.84% fully linked at upstream `11d6aa5` (VERIFIED via
WebFetch, 2026-09-09). The Route B milestone ladder should be written into
`GATES.md` (section 8).

### F10 — Upstream movement since the pins (VERIFIED via WebFetch)

- `zeldaret/tww`: 3 commits to `JAIBasic.cpp` since the pin (PR #1189 Aug 31,
  #1196 Sep 5, #1197 Sep 6) and a "J3DModel OK" commit on Sep 7. Relevant to
  F4 and to the empty audio dispatch bodies recorded on Sep 6.
- `encounter/aurora`: 6 commits since `8b690b6`; nothing touching display
  lists, GX arrays or the command processor. `3251f4e` adds `aurora::thp`
  (THP video playback), relevant later for FR-010 cutscene video.

---

## 4. Dependency map

```
Sep 7 link failure (8 d_stage runtime symbols)
  -> restore stage-runtime objects; fold getMemoryBlock/dStage_roomInit into
     d_stage_runtime.inc; drop initial-room-request objects (E1)
  -> world probe links
  -> first headless world run: BG create via room block or solid heap,
     BG execute/draw each frame, packets from models 0/1/3, Link on BG's dBgW
  -> visibility oracle replaces packet-presence-only assertion
  -> GX capture of Link + Room44 (bound may need to grow)
        |
        +-> E2: headless parse of the capture through aurora::gx::dl::Reader
        |
        +-> E3: --visible run (user authorization for a window)
              -> first observed native frame containing Link + Room44 geometry
              -> live PAD input replaces the scripted stick (Aurora SDL input
                 already exists; PADSetVirtualStatus is the current seam)
              -> continuous session: movement, camera, collision, in one process
        |
   (parallel, independent of the window)
        |
  whole-unit composition (F5): d_s_play.cpp, d_s_room.cpp, d_a_bg.cpp compile
  whole today; d_stage.cpp needs 8 relocation-site guards; vegetation needs the
  generic asset-header preparer (F4)
  -> classify the 179/46/62 undefined symbols into owners (most are already
     present in the PLAYER probe's object set)
  -> replace hand-called phase functions with fpcM_Management over
     ROOM_SCENE-created BG and PLAYER (stop cancelling the PLAYER request in
     the room lifecycle path; remove the 15 room_phase4 seams)
  -> port mDoGph_Create/mDoGph_Painter/m_Do_main frame driver on TARGET_PC
     (currently zero TARGET_PC lines) so the Painter, not a test, consumes lists
  -> original boot spine: logo (done) -> title/name/open scenes -> play scene
     phases 0-6 -> room init -> transitions (dStage_changeScene already links
     in the runtime tier) -> save (CARD: Aurora lib/card exists,
     AURORA_ENABLE_CARD is OFF) -> audio (seStart empty upstream; DSP policy
     decision pending)
```

---

## 5. Next three experiments

### E1 — Close the world probe link authentically

- **Hypothesis:** the eight missing symbols are entirely owned by the
  stage-runtime tier and no further gap appears once it is restored and the
  duplicate initial-room-request tier is dropped.
- **Edit (minimal):** in `route_b/CMakeLists.txt` remove the
  `list(REMOVE_ITEM ... player_stage_runtime_objects)` and the
  `$<TARGET_OBJECTS:bluewake_route_b_initial_room_request_objects>` line from
  the world probe. Append `getMemoryBlock` and `dStage_roomInit` (bodies as
  in `d_stage.cpp:928-934` and `3059-3064`) to `d_stage_runtime.inc`, export
  that as the real `0210` after restoring `d_stage.cpp` per F3.
- **Command:**
  `cmake --build build/route-b-aurora-gx-sanitize --target bluewake_route_b_private_world_render_probe 2>&1 | tee /tmp/bluewake-world-render-e1-build.log`
  then
  `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 build/route-b-aurora-gx-sanitize/bluewake_route_b_private_world_render_probe "ref/The Legend Of Zelda The Wind Waker.iso" 2>&1 | tee /tmp/bluewake-world-render-e1-run.log`
- **Expected:** zero undefined symbols; the run prints
  `WORLD Room44: original BG profile creates three models ...` and reaches the
  180-frame loop, or aborts at a reached owner (most likely candidates:
  `dComIfGp_getMapTrans` room-info, the solid-heap size for BG models, or the
  capture bound).
- **Falsified if:** new undefined symbols appear (then the tier composition
  is wrong and F5's whole-unit route should be taken instead of another
  tier), or `prepare_route_b.sh --check` still fails after F3.
- **Inputs:** the disc image, the private build tree. **Stop after** one
  build and one run per configuration; do not add a tier to fix a runtime
  abort in this experiment.
- **On success:** add the visibility oracle (F8), then E2/E3. **On
  failure:** record the exact frontier and go to F5's whole-unit path.

### E2 — Headless parse of the captured GX stream through Aurora's own reader

- **Hypothesis:** the 85 KB Link capture (and later the world capture) is
  well-formed for Aurora's display-list reader; if not, the failure point is
  the first renderer defect and can be found without a GPU.
- **Edit:** under `BLUEWAKE_ROUTE_B_PLAYER_RENDER_DIAGNOSTIC`, write the
  first-frame capture to a private file under `local-research/evidence/` (it
  is derived game data; never track it). Add a tiny private tool linking
  `aurora_gx` that constructs `aurora::gx::dl::Reader(bytes, size, desc, fmts)`
  with the VCD/VAT state from the capture's own CP commands, iterates `next()`
  until `nullopt`, and prints the failing opcode/offset if `mFailed`.
- **Expected:** reader consumes the whole buffer; per-command histogram shows
  draw commands with plausible vertex counts.
- **Falsified if:** a `fail(...)` fires (unsupported subcommand, overrun, no
  layout). That locates the first renderer bug in the J3D writer.
- **Inputs:** none beyond the existing probe. **Stop after** two hours; if
  the reader API proves unusable standalone, skip to E3.
- **Next:** on pass, E3; on fail, fix the identified J3D/GD writer site and
  rerun E2 before any visible work.

### E3 — Observe the first native frame (needs user authorization for a window)

- **Hypothesis:** the existing render probe presents a recognizable Link for
  180 frames; then the world probe presents Link on Room44 geometry.
- **Command:**
  `build/route-b-aurora-gx-sanitize/bluewake_route_b_private_player_render_probe "ref/The Legend Of Zelda The Wind Waker.iso" --visible 2>&1 | tee /tmp/bluewake-player-visible-e3.log`
  (one process; the iOS Simulator may stay open, F7). Repeat for the world
  probe after E1.
- **Expected:** a 640x480 window; a recognizable Link silhouette; the log ends
  with `PLAYER presentation: 180 submitted frames`. Wrong colors, missing
  textures or lighting are acceptable first results and must be recorded with
  a screenshot kept private.
- **Falsified if:** Aurora `FATAL`/`CHECK` in the command processor, a blank
  window with a 180-frame success log, or an abort. Each is a real, named
  renderer defect.
- **Inputs:** user authorization to open a GUI. **Stop after** one run per
  probe; take one screenshot; do not tune anything visually in this
  experiment.
- **Next:** on pass, replace the scripted stick with Aurora SDL PAD input
  and run a continuous session; on fail, the failing Aurora check names the
  owner.

---

## 6. Implementation plan

Ordered; each step has an acceptance check. Product behavior is the metric,
not test counts.

1. **Reconcile the tree (F3).** Preserve the 0210 hunk, restore
   `d_stage.cpp`, rerun the preparer, write the real 0210 into
   `d_stage_runtime.inc`. Accept when `prepare_route_b.sh --check` passes and
   the 0209/0210 entries are registered.
2. **Link and run the world probe (E1).** Accept when it runs 180 frames in
   sanitize with the visibility oracle recording per-frame hidden-shape
   counts, and the capture bound is measured, not guessed.
3. **Headless renderer parse (E2).** Accept when the reader consumes the
   Link and world captures without failure, or a named writer defect is fixed.
4. **Observed frame (E3).** Accept when a screenshot shows Link and Room44
   geometry. Record the visual defects as named blockers with the owning GX
   state, as `FINISH_LINE.md` step 2 prescribes for Route A's hair defect.
5. **Live input in the same session.** Replace `PADSetVirtualStatus` scripting
   with Aurora SDL gamepad/keyboard for the render probe. Accept when the
   user moves Link and the camera follows in the window.
6. **Scheduler-driven world.** Compose ROOM_SCENE (whole `d_s_room.cpp`,
   46 undefined) creating BG and PLAYER through `fpcCt_Handler`, executed and
   drawn by `fpcEx_Handler`/`fpcDw_Handler` via `fpcM_Management`, with the
   Painter consuming lists. Remove the 15 `bluewake_route_b_room_phase4_*`
   seams by linking the real owners. Accept when no hand-called phase
   function remains in the world path.
7. **Whole-unit stage/play owners (F5).** Guard the eight `d_stage.cpp`
   relocation sites; compose full `d_s_play.cpp` (179 undefined, classify by
   owner). Retire tiers as their full units link. Accept when the tier macro
   count in `ref/tww` goes down at each checkpoint, and the production
   censuses change from 4/41/52 for the first time.
8. **Vegetation asset preparer (F4).** Generalize
   `prepare_route_b_player_assets.py` over `config.yml` extract entries so
   the five vegetation units compile whole; drop the 0209 teardown copies.
   Accept when grass/trees draw in the world probe.
9. **Audio.** Backport upstream `JAIBasic.cpp` bodies (F10) as a patch;
   reconstruct `JAIZelBasic::seStart` (empty upstream) from the DOL analysis
   already recorded in `ROUTE_B_AUDIO_REFRAME_2026-09-06.md`; decide the DSP
   seam (user decision). Accept when a footstep SE is audible in the session.
10. **Boot spine, transitions, save.** Title/name/open scenes; play-scene
    phases 0-6; `dStage_changeScene` through the runtime tier; CARD via
    Aurora `lib/card` (enable `AURORA_ENABLE_CARD`). Accept per PRD P4
    milestones 3-9.

**Defer:** iOS/iPadOS (Aurora has `device_ios.mm`, so the path exists), THP
video, performance claims until representative Outset content renders
(rule 27), any Route A speed work.

**Regression scope:** steps 1-2 need the world probe and the existing 20
private probes in sanitize only; steps 3-5 add Debug/Release for the render
probe; steps 6-7 justify the full matrix. Do not run the full matrix for a
CMake-only change.

**Stable checkpoint criteria:** preparer check passes; world probe runs in
all three modes; one screenshot per visible milestone kept privately; ledger
banner names the observed behavior, not the byte count.

---

## 7. Unblock table

| Issue | Evidence | Action | Owner | External input truly required? | Independent work meanwhile |
|---|---|---|---|---|---|
| World probe does not link | Sep 7 log; CMake diff line 4737 | E1 | agent | No | — |
| `ref/tww` drift; preparer check fails | F3 | reconcile, real 0210 | agent | No | — |
| Visible frame never observed | F6, F7 | E3 | agent, after user says a window may open | Yes: authorization to open a GUI. Closing the iOS Simulator is not required | E1, E2, F5 whole-unit work |
| First renderer defect unknown | F6 | E2 | agent | No | — |
| Vegetation units cannot compile | F4, build2 log | generic asset preparer | agent | No (user's DOL is already present and locked) | — |
| Empty `JAIBasic::deleteObject`, empty audio dispatch | F4, F10 | backport upstream bodies as a patch | agent | No | — |
| `seStart` has no source anywhere | audio reframe doc; upstream still empty | reconstruct from DOL analysis | agent | Possibly later: user decision on DSP HLE seam | everything above |
| Stale ledgers/README | F9 | update per section 8 | agent | No | — |
| Tracked `local-research` evidence policy | disposition F8 (Sep 2) | user keep/delete decision | user | Yes, but cosmetic; blocks nothing | all |
| Repin tww to newer upstream | F10 | not now; cherry-pick JAudio bodies | agent | No | — |

Nothing in the current state is a genuine external block on headless
progress. The only user-owned decision on the critical path is permission to
open a window for E3, and even that does not block E1, E2, or steps 6-8.

---

## 8. Goal-loop correction

**Measurable milestone per iteration.** Each iteration must name one of these
observables and either move it or record why not:

1. world probe links (yes/no);
2. world probe runs N frames headless with hidden-shape counts recorded;
3. capture parses through Aurora's reader (yes/no, failing offset);
4. a frame is observed (screenshot hash, defects named);
5. live input moves Link on screen (yes/no);
6. number of hand-called phase functions in the world path (must fall);
7. number of `BLUEWAKE_ROUTE_B_*_TIER` macros and `bluewake_route_b_*` seams
   in `ref/tww` (must fall from 120 / 68);
8. production census counts (must change from 4/41/52).

**Repeated-shape detection.** The Sep 6 history shows 25 checkpoints in
twelve hours with one shape: "hand-compose the next owner slice into a
diagnostic until it links, strict-run to the next abort, add a tier". Under
rule 24/25 the ledger must record adaptation density or a concrete compile
blocker for every new tier; 0209 has neither. Add a hard rule: a new tier
macro in a unit that compiles whole on TARGET_PC (F5 lists four) is
prohibited; compose the whole unit instead.

**Escalation criteria.** If two consecutive iterations end with the same
observable unchanged, the next iteration must change layer: renderer work
switches between E2 (headless) and E3 (visible); composition work switches
between tier and whole-unit. If the visible test remains unrun for one more
iteration after user authorization, treat it as a process failure and stop
other work until it runs.

**Evidence required before claiming:**

- **visible:** a screenshot from a Route B process showing Link and room
  geometry, with the run log, kept privately; not a byte count.
- **playable (diagnostic):** one continuous session with live input, camera,
  collision, and no `_Exit`, in one process, for at least several minutes.
- **normal boot:** `fapGm_Create/fapGm_Execute` through original logo, title,
  file select, new game, play scene phases 0-6, and `dStage_roomInit`, with no
  hand-created process, no `bluewake_route_b_room_phase4_*` seam, and no
  cancelled retail request.
- **performance:** only after representative Outset content draws through
  J3D via the original Painter, with a pacing owner (rule 27, TD-011).
- **complete acceptance:** PRD P4 milestone 9 then P5 campaign; never inferred
  from probes.

**Ledger updates proposed.**

- `CURRENT.md`: new banner stating the actual tree state (F1-F3), that the
  handoff's symbol list is superseded, that the Simulator is not a constraint
  on a macOS window (F7), and the E1-E3 order.
- `BLOCKERS.md`: keep BW-P4-0126 active; add BW-P4-0127 "world probe link
  regression and ref/tww drift" (S1, agent-owned, no external input) and
  BW-P4-0128 "renderer consumption unobserved" with E2/E3 as its experiments.
- `GATES.md`: add a Route B P4 milestone table (1 entry point, 2 logo: passed
  Sep 2; 3 title: open; 4-5 open; 6 Outset by original scene code: partial,
  ROOM_SCENE proven, PLAYER request cancelled; 7 controllable Link: headless
  only; 8-9 open).
- `TECH_DEBT.md`: correct TD-011 (Route B has presented 2D frames); add
  TD-012 "tier/seam proliferation in ref/tww" with the counts and the retire
  rule; record F4's asset-header blocker.
- `GOAL_LOOP.md`: add the whole-unit-first rule and the eight observables
  above; strike the Simulator-closure wording from the continuation section.

---

## 9. Copy-ready continuation brief

```
BlueWake Route B — continuation brief (after the user resumes implementation)

Read first: docs/research/INDEPENDENT_DEEP_DIVE_REPORT_2026-09-09.md (this
file), then the CURRENT/BLOCKERS banners. Validate these locally before
changing plan: (a) `git status` matches section 2.1; (b)
`bash scripts/prepare_route_b.sh --check` fails only on src/d/d_stage.cpp;
(c) `/tmp/bluewake-world-render-sanitize-build.log` (Sep 7) lists eight
d_stage runtime symbols; (d) route_b/CMakeLists.txt:4737 removes
player_stage_runtime_objects from the world probe; (e) with the flags from
CMakeFiles/bluewake_route_b_escape_restart_objects.dir/flags.make and no
tier macro, d_s_play.cpp / d_s_room.cpp / d_a_bg.cpp compile and
d_stage.cpp has 14 relocation-cast errors.

Do, in order:
1. Save the 0210 hunk from ref/tww/src/d/d_stage.cpp:3039-3065, restore the
   file from git, rerun scripts/prepare_route_b.sh, then implement 0210 as an
   extension of d_stage_runtime.inc (getMemoryBlock + dStage_roomInit).
2. In the world probe: restore player_stage_runtime_objects, drop
   initial_room_request_objects. Build sanitize; run headless with the disc
   and strict ASan/UBSan. Record the first reached frontier. Do not add a
   tier to get past it.
3. Add the visibility oracle (hidden-shape counts vs. independent frustum
   test) to route_b_world_geometry_services.cpp; keep the packet identity
   check but stop describing it as "all three models observed".
4. Headless: dump the first-frame capture privately and parse it with
   aurora::gx::dl::Reader (E2). Fix the first failing writer if any.
5. Ask the user once: "May I open one Metal window for the render probe?"
   The iOS Simulator does not need to close. On yes, run --visible for the
   PLAYER probe, then the world probe; keep one screenshot privately.
6. Then live input, then scheduler-driven ROOM_SCENE/PLAYER (stop cancelling
   the PLAYER request), then whole-unit d_s_play/d_s_room/d_stage.

Do not: add new BLUEWAKE_ROUTE_B_*_TIER macros to units that compile whole;
certify 0209 as authentic teardown; treat capture byte counts as rendering;
repin ref/tww (cherry-pick upstream JAIBasic.cpp bodies as a patch instead);
run more than one BlueWake process; modify the protected recompcore files.

Unresolved disagreements to preserve (hypotheses, not authority):
- Why player_stage_runtime_objects was removed is inferred from duplicate
  definitions, not recorded anywhere; confirm by re-adding it and reading the
  duplicate-symbol error before deleting initial_room_request_objects.
- Whether BG should take the MEMA memory-block path or the solid-heap path in
  the diagnostic is undecided; both are original code, only the former is the
  retail sea path.
- Whether Aurora's reader can run standalone without a device (E2) is
  untested; if it cannot, E3 is the only discriminator.
- The 2 MiB capture bound may or may not suffice for Room44; measure.
```
