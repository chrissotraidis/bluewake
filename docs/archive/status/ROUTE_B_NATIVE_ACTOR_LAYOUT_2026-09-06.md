# Canonical native actor/process layout — 2026-09-06

Base `605b56e`; BW-P4-0126 remains active. No P4 promotion.
Apple Silicon macOS 26.6.2 / SDK 26.5; unchanged pinned dependencies and GZLE01.

## Result and scope

The previously failing actual-Link identity diagnostic passes in Debug,
Release and strict ASan/UBSan. Link now receives original fpcBs_Create storage
and enters original phase two through fpcBs_SubCreate and a diagnostic
phase-two callback. ID 37, parameters, profile, layer and tag back-pointers
survive original placement construction. Three ID and three parameter sentinel
comparisons also pass and restore the real metadata afterward.

This is a phase-two-only diagnostic profile, not normal g_profile_PLAYER
admission. Phase one, phase three, full Link execution/drawing/deletion and
input remain unqualified. The diagnostic still supplies camera/stage inputs
and aborts if unavailable audio executes. Link teardown is not claimed.

The public actor-pointer test exercises four paired lifetimes of simultaneous
same-profile polymorphic actors. Original process allocation, leaf creation,
method dispatch, execution queues, actor-tag lookup and delayed deletion run
with synthetic actor callbacks. IDs are 7/9, 17/19, 27/29 and 37/39. The test
proves identical addresses are reused, old IDs stay absent, original event
partner lookup distinguishes the live actors, deletion removes partners, and
the original JKR heap recovers exactly after each pair. Eight creates,
twelve executes, eight is-delete and eight deletes run. Header bytes and
tag pointers survive construction; virtual dispatch remains valid.

Private Stone2 and Room44 lifetimes pass after removing the old actor-field
mirror and parameter proxy. Room44 now finds Stone2 by its actual process ID,
not unique profile name. Existing real model, collision, draw-submission,
delayed-delete and resource/heap recovery assertions remain in force.

## Implementation and contract

Patch 0172 changes actor-specific native APIs from void* to fopAc_ac_c* through
a native/retail alias. Derived-to-base conversion therefore happens before
erasure at process APIs. Retail signatures stay void*. Event wrappers and
attention ID reads follow the same contract. Embedded rope actors pass their
actor member; opaque factory results use process APIs. Arbitrary erased
pointers are not guessed back into actor shapes.

Patch 0173 and BlueWake layout descriptors distinguish complete allocation
from canonical process base. Explicit NativeProcessLayout metadata records
object size, process offset and alignment. The compiler derives actor offsets
using a confined Clang offsetof extension; no fixed eight-byte rule, pointer
width heuristic, copied header or captured game state is used. Registry
validation rejects invalid explicit extents/alignment. Existing unspecified
entries retain their prior offset-zero contract; this does not qualify every
unintegrated actor profile. Registry entries must remain installed/resident
through their process lifetimes.

Original fpcBs_Create clears the complete allocation and initializes the
actual process subobject. Original manager/leaf/actor layers continue receiving
that canonical base. Stone2's erased callbacks explicitly recover its complete
C++ object using the same type-derived layout. fpcBs_Delete captures the
original allocation before destruction and frees it, rather than the interior
base address. Normal original fopAc_Create supplies Stone2 fields; the prior
42-line mirror/parameter test support is removed.

The root execution-queue test also exposed an original expression forming a
member address through a null root-layer owner. Patch 0173 moves that expression
inside the existing non-root short-circuit branch, preserving the algorithm.

## Failed attempts retained

- The typed-only repair passed actual Link but crashed the old Stone2 bridge:
  its append value combined overlapping process fields. This prevented a
  typed-only checkpoint and led to the allocation/callback repair above.
- The new public target initially lacked j3dDefaultLightInfo; it now links
  original J3DTevs.cpp, not a replacement constant.
- Importing the full registry header into Stone2 collided with the retained
  MSL headers. Layout metadata was separated into a lightweight header.
- The first lifecycle fixture omitted original leaf initialization, leaving
  subtype zero and accidentally matching an uninitialized node type. The
  final fixture uses original fpcLf_Create rather than injecting a subtype.
- Strict sanitizers exposed the root null-member expression described above.

## Verification

All modes: public CTest 74/74. The new test includes plain, polymorphic and
embedded actor API controls, paired managed lifetimes, original event partner
resolution, null/unknown IDs and malformed explicit layout rejection.

All modes: private PLAYER init (regular, --event-identity, --resources-only),
player model/services, camera Run/events, sea, attention, DZB, Toripost heap,
McaMorf, BG profile, Stone2, Room44 lifecycle and camera constructor/matrix/mass
regressions pass. No GUI or Simulator launched.

Production link censuses intentionally still fail: phase two four audio
symbols, phase-two/three 76, phase-two/execute 89 in each mode. These unchanged
counts are link dependencies, not executed calls or remaining-task estimates.
Patch preparers pass through TWW 0173 and Aurora 0001 (Aurora invoked with
bash); shell syntax, whitespace, ten player-asset tests, protected source and
lock hashes pass. Audit retains exactly its known 20 tracked research files.

Reproduce with cmake --build build/route-b-MODE followed by ctest --test-dir
build/route-b-MODE --output-on-failure. Build private targets in
build/route-b-aurora-gx-MODE, then run with the validated private disc; add
--event-identity to bluewake_route_b_private_player_init_probe. MODE is debug,
release or sanitize. Strict environment: ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
and UBSAN_OPTIONS=halt_on_error=1. Diagnostic global teardown/leak detection is
not acceptance; exact managed actor heap recovery is asserted independently.

Ignored evidence: local-research/evidence/route-b-native-actor-layout-20260906,
including failed-attempt traces, configuration/build/test logs and executable
hashes. Public repository contains no new game data.

## Next action

Return to original phase_3/makeBgWait: compose its remaining authentic owners
and execute it with real Room44 ground, initial behavior and model calculation
before PLAYER execute. Keep this process contract, real Link ID checks and
Stone2/Room44 lifetimes in regression. Adapt additional profile layouts and
erased callbacks as they are admitted; do not infer whole-game ABI coverage
from these actors. The organizing milestone remains continuous controllable
Outset, then transition/save/reload and normal-boot acceptance. No external
blocker; the full PRD goal remains active.
