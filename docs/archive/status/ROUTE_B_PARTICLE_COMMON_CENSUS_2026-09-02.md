# Route B Particle Common Census - 2026-09-02

## Scope

`BW-P4-0098` begins by qualifying the serialized boundary consumed by original
`JPAEmitterArchiveLoader_v10::load`. The private census reads
`/res/Particle/common.jpc` through the existing Aurora DVD owner but does not
publish or hash any user-owned disc bytes.

## Independent Oracle

`tests/route_b_private_particle_common_census.cpp` decodes all multi-byte
fields explicitly as big-endian, bounds-checks every header and block, requires
unique emitter resource IDs, reconciles key and field counts, and requires the
parsed extent to equal the file size.

The retail GZLE01 file passes with:

| Property | Observed |
| --- | ---: |
| Bytes / parsed extent | 425,280 / 425,280 |
| Emitter resources / unique IDs | 193 / 193 |
| Texture resources | 96 |
| Emitter blocks | 1,011 |
| Key blocks | 32 |
| Field blocks | 174 |
| Texture references | 354 |

Block counts are `BEM1=193`, `BSP1=193`, `ESP1=186`, `ETX1=10`,
`FLD1=174`, `KFA1=32`, `SSP1=30`, `TDB1=193`, and `TEX1=96`.

## Portability Finding

The original loader read the JPAC magic/version, resource counts, block counts,
block tags/sizes, and resource IDs as native integers. On little-endian macOS
that prevents authentic traversal. Patch 0059 makes only those serialized
archive-traversal fields endian-aware and adds exact target-PC layout asserts.
The unchanged loader then compiles under the strict Route B pointer/narrowing
diagnostics.

Patch 0060 ports the payload records retained by `JPADynamicsBlockArc`,
`JPABaseShapeArc`, `JPAExtraShapeArc`, `JPASweepShapeArc`,
`JPAExTexShapeArc`, `JPAKeyBlockArc`, and `JPAFieldBlockArc`. Scalar and vector
fields use size-preserving wrappers. The indirect-texture matrix, keyframe
arrays, and TDB indices are decoded into native heap-owned storage because
their existing APIs expose native pointers.

`tests/route_b_private_particle_resource_probe.cpp` constructs the complete
original `JPAResourceManager` on Aurora MEM1. Debug, optimized Release, and
strict ASan/UBSan all register 193 emitters and 96 textures, reconcile 32 key
and 174 field blocks plus 354 in-range texture references, and validate 308
finite native keyframe values. Patch 0060 changes 343 lines across 972 original
lines (35.29%). This admits resource decode, not particle calculation or draw.

## Retail Pool Owner

`tests/route_b_particle_manager_pool_test.cpp` executes the unchanged
`JPAEmitterManager` constructor with the retail `3000 / 150 / 200` arguments
inside a real `0x16e800` `JKRSolidHeap`. It walks every resulting intrusive
list and requires unique non-null objects, correct list supervisors, emitter
vacancy pointers aimed at the shared particle and field pools, empty active
groups, and zero active particle/emitter counts.

The fixture mirrors `mDoExt_createSolidHeap`: it aligns the requested payload,
adds `sizeof(JKRSolidHeap)` before the create call, and enables the heap error
flag. The full requested `0x16e800` therefore remains available to allocations.

The first link census found only two constructor dependencies outside existing
heap/list owners: qualified host mutex operations and the original
`JMath::TRandom_fast_` constructor embedded in each emitter. No JPA execution
source was admitted. Debug, optimized Release, and strict ASan/UBSan pass all
18 public tests. An initial sanitizer command requested unsupported Apple
arm64 leak detection and aborted before test code; rerunning the same binaries
with strict supported ASan/UBSan options passed 18/18.

This proves exact pool allocation and wiring. The test's resource-manager
pointer is an opaque constructor token because this constructor stores but
never dereferences it; authentic resource ownership remains proven separately
by the private resource probe and must be composed by `createCommon`.

## Always Model Owner

`tests/route_b_private_always_model_probe.cpp` mounts authentic `Always.arc`
through original JKR ownership and enumerates every display-list resource.
All 31 `BDL `/`BDLM` models are `J3D2/bdl4` files with the qualified nine-block
sequence and load once through the original display-list loader. The required
index `0x31` is `mpm_tubo.bdl`: 5,760 bytes, one joint, one shape, one material,
and six textures.

Patch 0061 exposes only the existing original `BDLM` dRes branch in the bounded
target-PC tier and admits its original animation/material owners. dRes converts
all 31 models from a fresh archive mount and publishes index `0x31` as a
distinct `J3DModelData` with a live `J3DMaterialAnm`. Authentic 256x8 and
256x256 System toon resources are installed before conversion; `mpm_tubo`
contains no `ZA`/`ZB` texture names and therefore needs zero substitutions.

The first composed probe loaded `mpm_tubo` directly and then asked dRes to load
the same mutable archive view again. The second load reported an unknown block;
a fresh-mount test proved this was a rejected harness shape, not a retail model
failure. A separate real defect did remain: `getMaterialAnm` treated native
64-bit pointers above guest address `0xC0000000` as packed sentinels. Patch 0061
returns the native pointer under `TARGET_PC` and preserves retail behavior.

Debug, optimized Release, and strict ASan/UBSan agree. Patch 0061 changes 26
lines across 2,150 compiled original lines (1.21%). All 18 public tests also
pass in the same three configurations.

## Particle Model Pool Owner

Patch 0062 partitions unchanged `mDoExt_J3DModel__create` from broad
`m_Do_ext.cpp`; original `J3DModel.cpp` and its retained cluster owner provide
the allocation closure. Patch 0063 makes the cluster display-list byte count's
32-bit destination explicit after native pointer subtraction. Patch 0064
partitions unchanged `dPa_modelControl_c` construction from `d_particle.cpp`
without admitting its destructor, model reuse/draw methods, callbacks, or
scene owner.

The private Always probe first creates one model directly, then makes the
exact `0x16e800` solid heap current and executes the retail 128-entry
constructor. Every entry starts idle, points to an independent non-null
`J3DModel`, retains the published `mpm_tubo` model data, and owns a differed
display-list object. A zero display-list payload size is valid for this model
and difference mask; both allocator-owned buffers still exist.

The first strict-sanitizer run found `J3DMatPacket::addShapePacket` branching
on an indeterminate `mpShapePacket`. Patch 0065 initializes that owned pointer
in the packet constructor. Debug, optimized Release, and strict ASan/UBSan
then pass the authentic route, and all 18 public tests pass in those same
configurations. Versioned logs and hashes are in
`local-research/evidence/route-b-particle-model-pool-v1-20260902/`.

## Composed Common Owner

Unchanged `dPa_control_c::createCommon` now executes in one authentic private
process over all qualified owners. The source loop finds seven of its eight
`dPa_name::j_o_id` values in retail `common.jpc`; each callback owns a non-null
emitter with the matching ID and normal/projection group, resource-manager ID
zero, zero particles, stopped emission, continuous lifetime, and count zero.
The manager reports seven active emitters and zero particles, both simple
counts are seven, and callback slots 7 through 24 remain empty. No
callback-execution fence fires.

The first strict run exposed a host alignment defect at the JKR ownership
boundary: GameCube's default four-byte placement was insufficient for
`JPAEmitterData` after pointers widened to 64 bits. Patch 0067 makes only
target-PC placement `new(heap, 0)` and its array form use
`std::max_align_t`. The exact retail `0x16e800` heap still fits. Debug,
optimized Release, strict ASan/UBSan, and all 18 public tests pass. Logs and
hashes are preserved privately under
`local-research/evidence/route-b-particle-create-common-v1-20260902/`.

Patch 0066 changes 2 original include lines. Patch 0067 changes 41 lines in
the 534-line original `JKRHeap.cpp` (7.68% adaptation density), but centralizes
the host allocation ABI instead of patching each widened object.

## Next Action

Unchanged play `phase_3` now passes over the real particle-command and
first-wave readiness owners. Execute only original `createScene` over those
authentic bytes next. Callback execution and broad phase-4 world construction
stay closed.
