# Route B original camera construction — 2026-09-06

## Outcome

P4 / BW-P4-0126 / `RETRY_NEW_HYPOTHESIS`. Base `53ae52e`, Apple Silicon
macOS 26.6.2, unchanged dependency lock and `GZLE01-disc-image` identity.

Patch 0147 composes original camera support and executes the original
`dCamera_c(camera_class*)` constructor, Start/Stop/Stay, and destructor for
16 cycles in each of Debug, Release, and strict ASan/UBSan. Four stage/yaw
variants cover normal sea and ship-offset initialization. This is the first
controller-lifecycle execution probe, not merely a storage or link test.

The actor is a real typed `fopAc_ac_c` constructed by its original constructor,
including original event-info initialization. Its pose/attention and stage
name are synthetic test inputs. The qualified game-info owner is constructed
normally, but its existing unqualified teardown/world/attention methods stay
behind abort fences. No capture-derived object bytes or raw fake game-info
singleton are used. This is not the PLAYER profile, original camera-process
phase two, normal boot, Run, draw, input polling, or a playable scene.

## Dependency composition

Use original angle/globe, vector/matrix, random, ground-query base, spline,
camera manager, controller-pad storage, game-info, actor, room/window, and J3D
owners. Native GX linkage is supplied by the existing Aurora integration;
no window/device is created and no focus-line draw is executed.

Two source partitions are justified by measured full-unit failures/retention:

- Full d_drawlist hits 27 missing private embedded asset headers in unrelated
  shadow/alpha/mirror code. The camera tier compiles the original random and
  effect-line methods in place, without copying or replacing their bodies.
  Existing window and draw-list owner tiers supply their separate methods.
  Effect-line virtual drawing remains linked but unexecuted, not accepted.
- Full d_com_static retains unrelated NPC smoke/light global constructors.
  The ship-offset tier retains the original getShipOffsetY body without
  constructing those unrelated globals. This does not remove the globals
  from the full source or future product scope.

The initial full support build also exposed nine u32/unsigned-long mask errors
in d_a_npc_os and two signed-angle initializer narrowings in d_com_static.
Fixed-width masks and explicit signed conversions preserve their values.
Original room camera/arrow accessors are exposed to the game-info owner;
original event-info construction is exposed to the camera's support variant.
Existing process phase-two/update/draw/delete fences are unchanged.

The obsolete constructor-only link census is removed and replaced by the
executable probe. Its source remains recoverable in Git history. The new Run
census is the diagnostic boundary; neither optional target is a default CTest.

## Execution-discovered defects and oracle

The first Debug execution crashed in JMASSin with an uninitialized table,
reached by ship-offset initialization. This was a missing probe prerequisite,
not grounds to replace the original offset calculation. The fixture now calls
original JMANewSinTable(12), matching m_Do_machine's JMASinTableBitSize=0xC.

Strict UBSan then exposed a real native undefined conversion at camera
initialize: cM_rndFX can produce a negative value assigned directly to u32
m080. The first failing input was -32767. Merely converting via signed int
would not reproduce this retail operation.

The locked DOL is verified by SHA1 before decoding the two call sites:
0x80161B04 calls cM_rndFX at 0x802463E8, and 0x80161B08 calls
__cvt_fp2unsigned at 0x80328E10. The pinned reconstructed runtime helper in
PowerPC_EABI_Support/Runtime/Src/runtime.c returns zero for negative values.
For this finite bounded random range, TARGET_PC now explicitly clamps negative
values to zero and otherwise performs the unsigned conversion. The retail
source path is unchanged. This is a narrow behavior-preserving fix, not a
general emulation of every floating-point conversion or NaN case.

The probe seeds the original RNG deterministically, verifies that both positive
and negative cases are reached, resets the seed before construction, and checks
the resulting counter against that helper behavior. No sanitizer is disabled.

## Runtime assertions

Each cycle verifies:

- original dCam_getBody lookup returns the placement-constructed body;
- camera/player pointers and camera/pad IDs are correct;
- center tracks supplied attention position plus original style height;
- eye offsets match independent cardinal-direction expectations at radius 200;
- main/C-stick and trigger histories preserve distinct synthetic input values,
  with zero initial deltas;
- sea, Abship, Obshop, and A_umikz select the correct offset mode; ship phases,
  step, scale, and numerical sine offset match expectations;
- positive/negative random-counter conversion matches retail;
- Start/Stop/Stay change their original flags; and
- explicit destruction clears actor stopStatus before the same storage is reused.

After controller destruction the test uses _Exit, as existing game-info owner
tests do, to avoid claiming unqualified global teardown. Original Run is never
called. STAG/map-tool overrides, real player creation, ground response, frame
pacing, graphics output, audio, and save/transition integration remain open.

## Next measured frontier: original Run

The new volatile member-function reference retains the original Run closure
against the constructor's linked owners and existing abort fences. Linking
fails with the same 60 unresolved symbols in all three configurations:

| Owner group | Symbols |
|---|---:|
| Functional collision: ground/line/roof/sphere, moving BG, mass camera | 17 |
| Matrix, geometry, projection | 16 |
| Event/demo camera | 5 |
| Graphics state/blur | 5 |
| Actor shared state | 5 |
| Attention/lock-on | 3 |
| Sea/wind | 3 |
| Actor/process search | 2 |
| Audio | 2 |
| Vibration | 1 |
| Assertion support | 1 |

These are unresolved symbols in this composed tier, not 60 absent source
functions and not a gameplay coverage percentage. Retained mode branches need
not all execute in initial sea mode. Existing abort-fenced dependencies must
also be qualified if reached; an eventual successful link alone is insufficient.

Next: qualify functional ground/line/camera collision and attention owners,
compose the original update closure, and integrate actual PLAYER creation,
execution, and drawing. No constant collision answers, fixed camera, or empty
event/audio methods may become required runtime behavior.

## Verification and reproduction

- Full default builds and 67/67 public CTests pass in Debug/Release/sanitizers.
- Full PLAYER and six camera units plus the new support partitions compile in
  all three configurations.
- New constructor probe passes all 16 cycles in each configuration.
- Private Toripost heap, McaMorf, BG, Stone2, and Room44 parent probes pass in
  all three modes; their original fixture limitations are unchanged.
- Run link census fails with 60 symbols in each mode and is excluded from the
  default test/build result.
- Five synthetic player asset-preparer tests pass. Patch preparation through
  0147, whitespace, protected runtime hashes, and dependency identity pass.
- Audit retains exactly the known 20 tracked local-research files; no new
  private/game-derived data is tracked. New logs are ignored under
  local-research/evidence/route-b-camera-construction-20260906/.

Using the configured Aurora-GX trees described in route_b/README.md:

```sh
cmake --build build/route-b-aurora-gx-debug --target bluewake_route_b_camera_constructor_probe -j8
build/route-b-aurora-gx-debug/bluewake_route_b_camera_constructor_probe
cmake --build build/route-b-aurora-gx-debug --target bluewake_route_b_camera_run_link_census -j8
```

Repeat with release/sanitize trees. Strict execution uses
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and UBSAN_OPTIONS=halt_on_error=1;
this is not a process-wide leak-free claim. Private regression probes still
take the validated disc argument. No BlueWake app or Simulator was launched,
and no new performance or visible-playability result is claimed.
