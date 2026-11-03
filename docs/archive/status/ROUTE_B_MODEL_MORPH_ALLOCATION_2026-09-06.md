# Route B model/morph allocation ownership — 2026-09-06

## Implemented and verified

Patch 0142 routes 19 ordinary allocation sites through explicit `JKR_NEW`:
Toripost's morph object; McaMorf audio, transform and quaternion storage;
the shared model factory; J3D model buffers/packets; and display-list objects.
The macro uses the existing null-returning, native-aligned JKR placement
allocator on TARGET_PC and ordinary retail `new` elsewhere. Global host
allocation remains unchanged. This policy is for JKR-managed lifetimes;
ordinary host `delete` is not the matching release operation.

The previous default-failing Toripost probe now passes eight complete heap
cycles in Debug, optimized Release, and strict ASan/UBSan:

```
toripost-heap cycle=8 owned=14 heap=2432 reclaimed=1 pass
```

Every cycle invokes original `_createHeap`, checks the native actor user-area
pointer, verifies all 14 allocation-bearing pointers reached by this exact
BDL/morph graph, runs the qualified BCK transitions/calculation/material-packet
submission, and destroys the actual solid heap. Its parent returns to exactly
the same free-space count on every iteration. Static no-use matrices and
archive-owned model/vertex/material data are correctly treated as borrowed.
The adjusted solid data size increases from the misleading 736 bytes to 2,432
bytes when the escaped objects are actually included.

An undersized `0x7e0` fixture also exercises original callback failure and
reclaims the partial heap before the successful cycles. This is not a test of
`fopAcM_entrySolidHeap` itself: its source treats the argument as an estimate
and retries a failed callback with a maximum-size heap. That complete
manager-owned retry remains part of actor lifecycle composition.

The fixture restores its prior current heap correctly, clears draw queues,
and unpublishes borrowed diagnostic model/matrix pointers before destruction.
The archive is released after all eight cycles. This establishes the selected
allocation lifetime, not standard Toripost process creation/deletion.

## Public contract and regressions

New public `bluewake_route_b_game_allocation_test` proves 16 cycles of scalar,
array, native/default and 32-byte alignment, null-on-OOM, exact reclamation,
and host allocation independence both before heap initialization and while a
game heap is selected. Full public builds and **66/66 CTests** pass in all
three configurations.

Existing McaMorf, Room44 parent, Stone2, and BG private regressions pass in
all three configurations. New Stone2/BG assertions prove their model and
material/shape-packet arrays now belong to the actor heap. These assertions do
not certify all their animation or collision allocations.

Patch verification passes through 0142; protected recompcore hashes and the
dependency lock are unchanged. The known 20 tracked private-evidence files
still make the repository audit fail; no new private evidence is tracked. No BlueWake
window or Simulator was launched. Build and runtime logs are retained under
`local-research/evidence/route-b-model-morph-allocation-20260906/`.

## Remaining owner and next loop

`BW-P4-0125` remains active, narrowed by the audit: BG still uses ordinary
allocation for animation wrappers, animation state, TEV state, and `dBgW`;
MoveBG also allocates `dBgW` ordinarily. Its BG frame-control test seam has
the same issue. The observed BG BTK pointer still lies in host storage.

Qualify those lifetimes using the explicit game-domain policy. First inspect
which animation pointers are cached in shared model data and how they are
cleared/rebound across repeated BG creation, so migrating allocation cannot
turn a leak into a stale pointer. Require failure cleanup, repeated BG and
Stone2/parent lifetimes, and all three configurations before returning to
Toripost's resource/WAIT/execute/draw/delayed-delete composition. Arbitrary
global-new replacement and unqualified gameplay branches remain closed.
