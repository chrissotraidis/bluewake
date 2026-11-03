# BlueWake Route Reorientation

**Date:** 2026-09-01
**State:** `DECOMPOSE`
**Priority:** macOS remains first; iOS and iPadOS remain deferred.

## Bottom Line

BlueWake is not stuck on compilation or launch. The signed Route A app boots
retail `GZLE01`, reaches Outset, renders gameplay, executes authentic DSP work,
and preserves the canonical copied card. It is stuck between two execution
architectures that each miss a different product requirement:

- Route A is the only working behavioral oracle, but its accepted Outset run
  executes about 235 seconds of guest time in 343.88 seconds. Visible evidence
  measured 6.0 FPS median in Outset and 13.55 FPS median in Omasao. Multiple
  correctness-aware translated-C mechanisms and one LLVM qualification did
  not expose a credible finish-line gain.
- Route B has the only credible native-speed and maintainability ceiling, but
  pinned TWW is reconstructed GameCube source, not host-portable source. It
  needs a systematic Dusk-style ABI, endian, pointer, UB, asset, REL, and
  platform pass before its otherwise-present boot spine can run on macOS.

There is no remaining small optimization or existing donor that turns the
current app into a stable 30 FPS product. The next work is a native-port
foundation, not another Route A timing experiment and not a blind whole-game
compile.

## Finish-Line Reassessment

### Independent review correction

The independent finish-line review delivered later on 2026-09-01 changes the
vertical tier's ownership, not the selected route. The archive/policy/audio
sequence had repeated the same BlueWake-owned adapter pattern above complete
retail implementations. In particular, `ObjectResources` cannot implement the
entry-pointer contract required by original `dRes_control_c`, so composing it
into the default application is closed. A one-shot typed WS diagnostic parsed
authentic group-2 metadata as one group and 67 records, but its runtime parser
was removed before promotion for the same reason.

The active vertical tier now begins with original JKernel heap/archive/
decompression/file-finder source and `JUTTexture`, adapted at exact-width,
big-endian, offset, Aurora memory, and Aurora DVD seams. Its acceptance owner is
original logo phase one receiving the toon `ResTIMG*` through original
`dRes_control_c`. BlueWake-owned code remains appropriate below the retail
layer for OS threads, message queues, alarms, presentation, AI/DSP, and host
input. See `INDEPENDENT_REVIEW_2026-09-01.md` and `BW-P4-0077`.

The native-port foundation has now passed its horizontal process-layer test:
an empty frame, the process runtime, 15 generic `f_op` units, and a camera-
manager adapter link and execute under focused tests. That is enough evidence
that the lowest ABI/ownership pattern is systematic. It is not evidence that
the game boot path is close.

Continuing with environment/message wrappers would repeat the same experiment
shape for a fourth framework leaf. The active work therefore changes from
horizontal coverage to a vertical proof rooted at the authentic logo-scene
request and retail profile. The proof must inventory and then execute the
smallest path from `f_ap_game.cpp` through `fpcNm_LOGO_SCENE_e`,
`g_profile_LOGO_SCENE`, and authentic completion of logo `phase_0`. Platform services may be
explicitly replaced; game behavior may not be replaced with fabricated state
or success-returning stubs.

This tier is intentionally decisive. If its dependency/ownership closure is
bounded, implement it before returning to generic framework work. If it
requires an unbounded whole-engine admission, unexplained empty behavior, or
fine-grained calls into Route A guest memory, classify Route B as long-horizon
and reopen architecture selection. Do not keep accumulating native leaf-unit
counts while the first retail process remains unexecuted.

The application-root half of that tier now passes. Original application,
HostIO, and counter code issue `fpcNm_LOGO_SCENE_e`; native request processing
resolves a profile through the static REL registry and invokes its create
method. The smoke uses a labeled synthetic profile, so it does not satisfy the
retail milestone. The remaining vertical frontier is now specifically the
original logo create/phase-zero body and its real services, not application or
profile plumbing.

The next partition now runs the real GZLE01 profile and retail phase-zero
control flow through the original phase handler. Its staged services prove
readiness and mount ordering but do not perform production I/O. Reorientation
therefore moves one boundary deeper without changing the gauge: native
resource mount/sync/heap ownership is the next decisive tier, followed by real
VI/reset and JAudio readiness. More synthetic callbacks or generic framework
units remain non-milestones.

## New Evidence

`scripts/route_b_native_abi_probe.py` tests the untouched TWW process, save,
player, and J3D header graph with Apple Clang. It then repeats the graph with a
temporary fixed-width diagnostic prelude, without modifying `ref/tww`.

The untouched primitive ABI fails first:

- TWW defines `s32` as `signed long` and `u32` as `unsigned long`;
- both are 8 bytes under macOS LP64, versus the required 4-byte GameCube
  contract; and
- non-Metrowerks `STATIC_ASSERT` expands to nothing, so ordinary parsing would
  silently hide layout drift.

After the diagnostic prelude restores fixed integer widths and supplies the
small math-intrinsic namespace surface, the same graph reaches explicit
pointer-to-`u32` truncation errors in J3D texture, packet, and material code.
Those are architecture boundaries, not warning cleanup.

The scale comparison is equally important. Pinned TWW contains zero explicit
`TARGET_PC` conditionals. The proven Dusklight native port contains 1,498 such
conditional directives across 293 source/header files; its existing research
census records roughly 62,000 changed lines, 1,058 endian-wrapper uses, and
238 offset-pointer uses. Route B is therefore a porting program, not an SDK
shim.

Evidence:
`local-research/evidence/route-b-native-abi-probe-v1-20260901/`; report
SHA-256
`209af06d902819c28e97d8e256c5ee5895375fbbd3cf83a64db003386b8ee794`.

## What Is Closed

- Another Route A boundary, callback, budget, deadline, partition, coalescer,
  or current-host LLVM experiment is closed without a materially new measured
  mechanism.
- A direct whole-player Route B compile is closed until primitive widths,
  endian/offset ownership, and process-framework pointer policy are explicit.
- A muted or stubbed audio build remains compile/boot evidence only. It cannot
  establish gameplay or product acceptance.
- Fine-grained calls between native Route B objects and Route A guest memory
  remain prohibited.

## Reoriented Work

Decompose Route B at its lowest reusable boundary:

1. Create a BlueWake-owned native portability foundation from reviewed
   Aurora/Dusklight concepts: exact-width game primitives, `BE<T>`,
   `OffsetPtr`, host intrinsic helpers, and compile-time invariants.
2. Apply it only to the process/profile/layer/phase foundation and one static
   REL registry slice. Do not include player, J3D, audio, or broad game state
   yet.
3. Require Apple Clang C++20 syntax and object compilation with no pointer
   narrowing, no disabled BlueWake ABI assertions, and no native/guest-memory
   bridge.
4. Measure the patch size and adaptation density. Continue into machine/scene
   code only if the slice remains systematic; otherwise reclassify Route B as
   a long-horizon reconstruction track and record that the present product
   goal has no short implementation route.

Route A remains frozen and runnable as the oracle throughout this work.

## Honest Gauge

The game already compiles and launches through Route A, so “percent until it
compiles” is 100%. That wording no longer measures the problem. For a stable,
playable macOS product, the defensible estimate is **25-30% complete**. Boot,
rendering, input automation, save preservation, and substantial runtime
correctness exist, but the selected finish-line architecture has only its
native process foundation and no retail scene. Stable speed, unrestricted human
play, visual/audio acceptance, broad campaign coverage, and packaging remain.

Route B itself is only **5-10% implemented**: its native process substrate
runs, but no retail scene does. This estimate is deliberately lower than the
previous 35-45% gauge because
the ABI probe converted an assumed shim-sized Route B risk into measured broad
porting work.

## First Foundation Result

The decomposed action passed without weakening the ABI gate. BlueWake's
standalone Route B target now provides exact-width primitives,
`BigEndian<T>`, signed `RelativeOffset32<T>`, and a static native REL/profile
registry. It compiles 30 of the 31 pinned TWW `f_pc` translation units under
Apple Clang C++20 with pointer narrowing treated as an error. Original
node/list/phase/method behavior and the wire wrappers pass Debug, Release, and
ASan/UBSan tests.

Only one source patch was required: two process-pause callback payloads now
convert through `uintptr_t`. The patch is tracked and replayed by
`scripts/prepare_route_b.sh`; no native/guest-memory bridge was introduced.

`f_pc_manager.cpp` is the sole deliberate exclusion. Its dependency surface
crosses directly into J2D/J3D, JAudio1, DVD, GX, game state, and embedded
resources. That is the next owner-level boundary, not evidence that the basic
process framework failed. The successor must separate process-loop ownership
from DVD-error presentation and define serialized J3D offsets before compiling
the manager; it may not suppress the strict narrowing failures.

## Manager Boundary Result

The broad include graph was not the scheduler boundary. More than 200 of the
manager's 341 lines implement GameCube DVD-error presentation. Route B now
compiles a host-neutral scheduler contract and the thin original `fpcM_*`
entry-point/root-layer adapters without J2D, J3D, GX, JAudio, embedded assets,
or Route A memory. Normal frame ordering and all three early exits are tested.

The next falsification tier is process link closure. Inventory unresolved
symbols across the complete process object set and admit only the first
SComponent/f_pc ownership frontier. Do not widen into f_op or game systems to
make a smoke link pass.

## Process Link Result

The tier passed. Removing the superseded all-actor profile table reduced an
apparent 522-symbol frontier to 16 adjacent project services. After adding
only SComponent iterators/clear, native allocation, static REL lookup, draw
hooks, and fatal assertion reporting, Route B links and executes root-layer
initialization plus one empty frame.

The next one-shot `f_op` census compiled 12 of 22 units unchanged. Ten stop at
scene/camera/game-state/J3D ownership. Continue with only the thin scene and
camera framework group; actor manager and message manager remain outside the
foundation gate.

That follow-up now links. Scene and scene-manager are admitted through explicit
host notification and width-safe payload contracts; a native camera-manager
adapter avoids the concrete camera graph. The linked tier contains 14 original
`f_op` units plus the adapter. The next decomposition owner is the shared
camera header: split portable process-base state from the `dCamera_c`/J3D
extension before compiling the camera framework body.

The portable camera body now compiles, links, and passes focused menu/pause
behavior coverage. `dCamera_c` and J3D remain deferred. Continue with only the
environment/message process wrappers and their explicit policy/payload host
services; broad actor and message managers remain outside the gate.
