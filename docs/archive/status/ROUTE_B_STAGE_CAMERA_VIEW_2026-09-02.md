# Route B Stage RCAM Host View - 2026-09-02

## Boundary

`RCAM` is the next present Outset `stage.dzs` chunk after `MULT` in aggregate
loader order. Its retail node has the same 12-byte serialized
`{big-endian count, relocated u32 offset}` representation that cannot be
aliased as `{int, native pointer}` on 64-bit macOS. Unlike `MULT`, its handler
immediately enters camera process creation.

## Result

Patch 0102 adds a stage-lifetime-owned native camera view. On target PC,
unchanged `dStage_cameraInit` resolves the entry array from the immutable file
base and serialized node offset, publishes the view through the real stage
owner, and calls the original `dStage_cameraCreate`. The original camera
manager then submits a real `CAMERA` standard-create request on the current
16-list process layer. The test inspects the queued retail append payload and
does not execute or replace the CAMERA profile.

The pinned initializer writes position `x`, `y`, and then `x` a second time;
`z` is intentionally left untouched. The qualification preserves that source
behavior and does not read the indeterminate field. Two missing standalone
include dependencies in the original camera-manager unit were made explicit.

All 44 public tests pass in Debug, optimized Release, and strict ASan/UBSan.
Patch replay through 0102 passes. Debug SHA-256 values are:

- isolated `d_stage.cpp.o`: `139950939981330d1add3be95e1ba58dde718d2b57e191dbe304b5f9f7582204`
- test executable: `3777659b8a55a503a98fe4c2551e8f15e090ba5470fa60313fdaa1ffb8abd844`

Patch 0102 is 103 lines across 3,811 reached source/header lines.
This is a portability and request-submission result, not CAMERA process
creation, aggregate stage-loader success, or a native frame.

## Next Boundary

Measure the queued CAMERA profile's real standard-create execution and
constructor/lifecycle closure. Require authentic profile lookup, process
allocation, list insertion, and reached camera state while fail-closing
execute, draw, delete, room creation, maps, events, and later DZS handlers.
Do not add another per-handler stage-view experiment before promoting a shared
counted-view builder when a third handler needs this representation.
