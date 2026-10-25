# BlueWake playability critical-path review — 2026-09-06

**Latest checkpoint:** patch 0147 executes original camera construction and
destruction against typed synthetic inputs; Run has a 60-symbol unresolved
closure. PLAYER creation and continuous control remain unproven. Current
details: `ROUTE_B_CAMERA_CONSTRUCTION_2026-09-06.md`.

**Subsequent checkpoint:** patch 0146 closes the full PLAYER and camera source
compile boundary in all three modes, with original typed camera storage.
Constructor linking remains incomplete; no controller/player runtime is
accepted. Current evidence and next action are in
`ROUTE_B_CAMERA_STORAGE_CENSUS_2026-09-06.md`. The diagnostic counts below
describe the earlier patch-0145 checkpoint, not the current compile result.

## Decision and product truth

Reframe the native gameplay work around one continuous controllable Outset
session, then transition/save/reload. BW-P4-0126 (PLAYER/camera runtime) becomes
the sole active blocker; Toripost lifecycle BW-P4-0124 is deferred. This changes
implementation priority, not the full PRD scope or normal-boot acceptance.

The previous turn answered the user's zoom-out question but changed no code.
This turn verifies the suspected integration gap and advances the original
player source rather than continuing the next prop by momentum.

Route A already executes retail boot/new-game/Outset, but its last accepted
whole-route measurement remains 343.88 wall seconds for about 235 game seconds.
That is not sustained gameplay FPS or a new measurement. No unscripted human
session is recorded on the accepted route. The 2026-09-01 execution-route
review rejected the tested translated-C and LLVM optimization opportunities;
this review supplies no new speed mechanism and does not reopen them. Route B
remains the selected native-speed investigation, not a proven faster game.

## Actual integration gaps

| Required link | Current evidence | Still missing |
|---|---|---|
| Normal app boot to gameplay | Application smoke calls original root but registers LOGO_SCENE only; private logo presentation/preload exists | Continuous opening/title/new-game/play integration; staged callbacks do not certify it |
| Room creates player | Original DZR submits PLAYER/METER/AGB | Room lifecycle fixture explicitly cancels all three in `cancelPhase2Children` |
| Player plus camera | Original full player source is available | TARGET_PC camera process has only base view/phase storage, no dCamera_c gameplay controller |
| World and movement | Selected room/actor lifetimes and real draw submissions pass | Functional collision queries/response, player motion, camera updates, input and visible scene together |
| Remaining room actors | BG, weather, grass, wood, Stone2 retained in the room fixture | 48 phase-3 requests canceled; this is request count, not 48 distinct profiles |
| Playable-session acceptance | No native continuous session | Live control, visuals/audio observation, normal room transition, save/reload |

Concrete sources: `tests/route_b_application_smoke.cpp`;
`tests/route_b_private_room_lifecycle_probe.cpp` (`cancelPhase2Children`,
`cancelPhase3Children`); `ref/tww/include/f_op/f_op_camera.h`;
`route_b/src/f_op_camera_manager_adapter.cpp`. Existing fixture cancellation
is useful diagnosis and remains labeled; it may not become production startup.
Required props and story actors are not removed from scope by prioritization.

## Player census and implementation

New `bluewake_route_b_player_census_objects` explicitly attempts original
`d_a_player.cpp`, `d_a_player_main.cpp` (including its gameplay `.inc` files),
and `d_a_player_npc.cpp`. It is EXCLUDE_FROM_ALL and is not runtime registered.
The source/include inventory spans about 34.5k lines; that is not a matched-code
or completed-code metric. No gameplay method was replaced by a stub.

The first attempt lacked a generated embedded display list. New
`scripts/prepare_route_b_player_assets.py` verifies the user-owned DOL against
the dependency lock, maps the pinned `l_sightDL` address/size through its DOL
section table, and generates the exact 137-byte, 32-byte-aligned private
include. The only output lives in ignored `generated/route_b_player/`.
No game bytes are added to the repository. Five synthetic tests cover mapping,
determinism, identity rejection, malformed extents/ranges, and ambiguity.

With that include present, Debug reports 31 diagnostics: four camera
interfaces, eleven pointer truncations, nine narrowing conversions, six
fixed-width bit-template mismatches, and one ambiguous abs call. Patch 0145
addresses the 27 non-camera diagnostics:

- Preserve native addresses in player model user data and animation-buffer/
  texture offsets. Widen the shared blend-calculator user-data field, setter,
  callback signature, and both original player callback parameters together.
- Make existing color-byte and unsigned collision-mask conversion explicit;
  use fixed-width bit masks; disambiguate abs for a value restricted to 0/1.
- Keep original gameplay bodies and camera accesses intact. No global
  narrowing suppression or camera fence is added.

The existing public allocation test now checks the real blend callback's API
type and storage/setter width, and passes a real address above UINT32_MAX
through its declared callback type. This is a synthetic ABI contract test,
not original player joint-calculation or asset-lifecycle acceptance.

## Remaining measured failure

In Debug, Release, and strict-sanitizer configurations, the explicit player
census now exits 2 with the same four compile errors:

1. Base player `changePlayer` accesses `camera_process_class::mCamera`.
2. Player demo procedure requires `dCam_getBody`.
3. Crawl procedure requires `dCam_getBody`.
4. Grab procedure accesses `mCamera.ForceLockOn`.

These establish the full-source compile boundary, not proof that all four
branches execute during first idle creation. There is still no complete
player object/link census or runtime success. The separate NPC-player unit
now compiles; this does not qualify possessed-NPC behavior.

## Verification and reproducibility

Base checkpoint `8001ee5`, Apple Silicon macOS 26.6.2; input alias
`GZLE01-disc-image`. Dependency lock unchanged. The optional census remains
FAIL as documented; it is not counted as a passing public test.

- Five synthetic asset-generator tests pass; exact locked DOL preparation
  succeeds and output is confirmed ignored.
- Full default builds and 67/67 CTests pass in Debug, optimized Release,
  and strict ASan/UBSan.
- Private Toripost heap, McaMorf, BG, Stone2, and Room44 parent probes pass
  in all three configurations after the shared callback ABI change.
- Patch preparation passes through 0145; whitespace checks and both protected
  recompcore hashes are unchanged. Audit retains exactly the known 20 tracked
  private-evidence files and a valid dependency lock.
- No BlueWake window or Simulator is launched; no new gameplay-speed claim.

Reproduce private preparation/census with the commands in `route_b/README.md`.
Default tests use `ctest --test-dir build/route-b-{mode} --output-on-failure`;
private probes use the corresponding Aurora-GX build tree and validated disc.
Strict runs use `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. Private logs are retained under
`local-research/evidence/route-b-playability-critical-path-20260906/`.

## Next executable milestone

Inspect and qualify the original native camera/controller storage and
create/update/delete ownership, preserving already-qualified view/process
contracts. Then complete full PLAYER compilation and classify its actual
link/create dependency set. A fixed camera or empty controller is not a fix.
If full-source diagnostics expose another shared owner, classify it against
the continuous-player milestone rather than drifting back to unrelated props.

After camera/player ownership, compose actual resource construction, input,
functional ground collision, camera update, and visible player/world submission
in one session. Tie each checkpoint to the missing integration link it closes.
Then join the authentic boot route and qualify transition/save/reload. All
P0–P10, full-game, audio, performance, mobile, and release requirements remain.
There is no evidence-backed calendar completion date or useful percent-complete
number; isolated test counts must not be used as either.
