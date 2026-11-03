# BlueWake continuation prompt — stopped 2026-09-06

Resume BlueWake in `the repository root` only when the user asks
to resume. The user explicitly requested this stopping point and handoff. The
durable goal is currently **paused**; do not treat this document as permission to
resume automatically. No implementation was changed during the stopping turn.

## Read first

Read the original objective at
`(a private attachment)`,
then `docs/archive/PRD.md`, `docs/GOAL_LOOP.md`, applicable repository instructions,
and the first authority banners in `docs/archive/status/CURRENT.md` and `BLOCKERS.md`.
Read `ROUTE_B_ACTIVE_VERTEX_BINDING_2026-09-06.md` and
`ROUTE_B_PLAYER_COMMAND_EMISSION_2026-09-06.md` for qualified evidence.
Inspect the actual diff before continuing; preserve all existing work.

## Honest product assessment

Wind Waker is **not yet demonstrated playable** here. Stable headless evidence
now covers 180 original Link input/update/collision/camera/draw-command frames.
It is not a rendered game, live-controller session, normal boot, or full-world
integration. Prior estimate of weeks for a limited Outset demo was low confidence;
there is no defensible full-game completion date. Prioritize an observed native
frame and a bounded live-input/world milestone over accumulating isolated tests.
BW-P4-0126 / ROUTE_B_PLAYER_CAMERA_RUNTIME remains active; P4 has not passed.

## Last stable checkpoint

`6949c27` on `main`, committed and pushed: Preserve active J3D vertex bindings
and prepare optional Metal presentation. Prior checkpoints: `512470d`, `051bc42`.

- Patch 0208 preserves native current J3D vertex pointers and bounded spans.
- All three configurations pass 180 captures with real GX initialization,
  85,024 bytes first/last frame, exact bindings for 21 body shapes.
- Actual main-body arrays are resource-backed in this replay. A separate
  metadata-only transformed-pointer fixture passes; its bytes are never drawn.
- Public tests 82/82 per mode; full private/event/oracle matrix passed.
  Production link censuses remain 4/41/52 expected gaps, not zero.
- Optional `--visible` Metal presentation is compiled but **never run**.

## Uncommitted world integration — compilation reaches eight link gaps

These files are unfinished WIP, not a qualified checkpoint:

- `route_b/CMakeLists.txt`
- `tests/route_b_player_init_services.cpp`
- `tests/route_b_player_phase_three_fences.cpp`
- `tests/route_b_private_camera_run_probe.cpp`
- New `tests/route_b_world_geometry_services.cpp`

New EXCLUDE_FROM_ALL target `bluewake_route_b_private_world_render_probe`
inherits the PLAYER render probe and adds original BG profile objects plus
`BLUEWAKE_ROUTE_B_WORLD_RENDER_DIAGNOSTIC=1`. It admits Room44 through the real
stage resource controller, registers the BG profile, creates BG process 39,
and uses its one published collision owner for Link. The frame loop adds
original BG execute/draw and BG opaque/translucent consumption. End-of-probe
deletion uses original process deletion. This is still a selected-scene diagnostic,
not normal boot or full scene teardown.

The initial sanitize configure succeeded and the build finished with a linker
failure. Compilation of the new service succeeded. **No world runtime was run.**
The eight unresolved symbols are:

1. `dStage_escapeRestart()`
2. `JAIZelBasic::seDeleteObject(Vec*)`
3. `dTree_room_c::deleteData()`
4. `dGrass_room_c::deleteData()`
5. `dMagma_room_c::deleteFloor()`
6. `dFlower_room_c::deleteData()`
7. `dStage_roomControl_c::getMemoryBlock(int)`
8. `dWood::Packet_c::delete_room(int)`

Evidence: `/tmp/bluewake-world-render-configure.log` and
`/tmp/bluewake-world-render-sanitize-build.log`. Previous tool session 67785
is historical; process inspection at stopping time found no matching active
world probe/build. Do not assume that session is still live or restart work
merely to recover its handle.

## Concrete next work after authorization to resume

1. Compare the new target with the qualified private BG profile and room
   lifecycle probes. Compose the existing authentic owners for the eight link
   gaps; inspect original source and side effects. Do not add success stubs,
   forced state, fake pointers, or bypass original deletion.
2. Build the new target in `build/route-b-aurora-gx-sanitize`. Then run headless
   with the local ISO and strict ASan/UBSan. Fix the measured first frontier.
3. Verify one shared BG collision owner and actual world packet identities.
   The WIP asserts all three Room44 models appear each frame; actual camera
   clipping may require a source-grounded visibility oracle. Do not blindly
   weaken assertions. Check resource conversion, original BG lifetime/deletion,
   and capture capacity (currently 2 MiB) against actual evidence.
4. Qualify the increment across sanitize/debug/release and the established
   regression matrix; document measured results. Commit/push stable increments
   only under the resumed goal's authority. Never call the entire goal complete
   merely because one increment passes.
5. Move toward observed Metal rendering, live input, and scene progression.
   Headless commands alone do not establish playability.

## Safety and verification constraints

- Simulator PID 43578 was still open at stop. A prior question asking whether
  it may be closed for one bounded visible test has no recorded approval.
  Leave it alone. Do not launch another BlueWake GUI/Simulator while it is open.
  Check current state on resume; do not repeatedly ask the same pending question.
- No subagents unless explicitly authorized by the user or applicable instructions.
- Use `apply_patch` for edits. Preserve existing dirty work. No destructive reset.
- Native Aurora edits belong in `ref/aurora`, not the protected recompcore copy.
  Export required patches through the repository's preparation workflow.
- Keep ISO/assets/generated game data/captures out of Git. ISO:
  `ref/The Legend Of Zelda The Wind Waker.iso` (GZLE01).
- Protected SHA256: `ref/recompcore/GXRuntime/src/core/cpu_exception.c` =
  `5b8ebbc60520b8bc6ee2166c344d760a88a3d2f2419058eaeaa210f5ec07def6`;
  `ref/recompcore/GXRuntime/src/hle/hle_core.c` =
  `dcb97681b2d325cd9fbf23e7d359a6788aa3f05980174fc31fa299a7b3c4716a`.
- Dependency lock SHA256:
  `35444a9868d3fd8d0b17b975bcd754b203d9558e12552fb819e0090a8f56c5e4`.
- Private builds: `build/route-b-aurora-gx-{sanitize,debug,release}`;
  public builds: `build/route-b-{sanitize,debug,release}`.
- Strict runtime environment: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`,
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- Existing full matrix and commands are recorded in the stable status reports.
  Animation oracles: 11,076 parameter / 12,880 emission; asset tests: 11.
  Run both preparers and whitespace checks. Repository audit has exactly 20
  previously documented local-research/evidence findings; distinguish new ones.

Stopping turn added this handoff and authority banners only. WIP remains
uncommitted intentionally; no new implementation, test run, commit, push, GUI
launch, or goal restart was performed.
