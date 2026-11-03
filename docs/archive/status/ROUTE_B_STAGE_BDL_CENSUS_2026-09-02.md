# Route B Stage BDL Census

**Date:** 2026-09-02
**Blocker:** `BW-P4-0093`
**State:** `PASS`
**Input alias:** `GZLE01-disc-image`

## Archive Evidence

The private census reads `/res/Stage/sea_T/Stage.arc` through Aurora DVD,
expands its Yaz0 payload with original `JKRDecomp`, and mounts it with original
`JKRMemArchive`. Its 23 entries contain 14 directories and nine files:

- four `BDL ` resources;
- one `DZS ` resource;
- three `TEX ` resources;
- one `DAT ` resource.

The original `dRes_info_c::loadResource` path transforms only the four BDLs;
the other five entries remain archive-backed raw resources.

All four models are `J3D2/bdl4` and validate their declared byte size and
complete block walk:

| File | Bytes | Blocks |
| --- | ---: | --- |
| `vr_back_cloud.bdl` | 40,096 | `INF1,VTX1,EVP1,DRW1,JNT1,SHP1,MAT3,MDL3,TEX1` |
| `vr_kasumi_mae.bdl` | 3,808 | same |
| `vr_sky.bdl` | 6,496 | same |
| `vr_uso_umi.bdl` | 3,072 | same |

## Execution Evidence

One excluded target compiles original `J3DModelLoader.cpp` plus eight direct
owners for model data/hierarchy, joints, shapes, materials, attachment, and
name tables. Initial compilation stopped at 20 host-width diagnostics. The
direct factories exposed 13 more. Patch 0046 resolves all 33 with native-width
address arithmetic, explicit narrowing for real 32-bit fields, and the donor-
confirmed 30-bit material-ID encoding. The bounded closure now compiles.

Patches 0047-0054 then make only the measured nine-block `bdl4` contract
executable:

- 0047 replaces serialized block pointers with exact 32-bit relative offsets
  and asserts reached file-layout sizes;
- 0048 separates native runtime pointer widths from serialized widths;
- 0049 converts the INF1 hierarchy stream;
- 0050 converts VTX1/SHP1 arrays and parses serialized enum words before
  forming native enum values;
- 0051-0053 convert MDL3 offsets, GX command words, MAT3 index tables, and the
  cull-mode table; and
- 0054 converts JNT1 indices and complete joint records while representing the
  retail one-byte scale-compensation flag without host `bool` UB.

The original `J3DModelLoaderDataBase::loadBinaryDisplayList(..., 0x2020)` now
loads every authentic BDL in one process:

| File | Shapes | Materials | Textures |
| --- | ---: | ---: | ---: |
| `vr_back_cloud.bdl` | 3 | 3 | 4 |
| `vr_kasumi_mae.bdl` | 1 | 1 | 0 |
| `vr_sky.bdl` | 2 | 2 | 0 |
| `vr_uso_umi.bdl` | 1 | 1 | 0 |

Debug, optimized Release, and strict ASan/UBSan produce the same results and
exit zero. The 15 public Route B foundation executables also pass sequentially
in all three configurations. Patch replay through 0054 succeeds from the
pinned source.

Strict qualification was materially useful. It successively rejected native
loads of a serialized SHP1 vertex enum, the retail JNT1 `0xff` flag byte, an
unswapped joint record, a second SHP1 enum load, and the MAT3 cull-mode enum.
Each fix moved conversion to the owning serialized/native boundary. Debug and
Release had already loaded all four files, so sanitizer success is additional
representation correctness rather than the sole route proof.

An optional all-target GX build still links older excluded private probes that
lack Aurora global configuration/name owners. That pre-existing aggregate
build-graph debt does not affect the 15 public targets or this private census.

## dRes Publication Evidence

Patch 0055 promotes only the existing `BDL ` case and its exact
`setToonTex(J3DModelData*)` post-processing into the target-PC System-resource
partition. BMD/BMT, alternate BDL modes, animations, clusters, and collision
conversion remain compiled out. The isolated executable supplies the same two
toon-image static storage owners already defined by the qualified logo tier;
the original helper and its display-list patching execute unchanged.

Original `dRes_info_c::setRes` and `loadResource` publish converted pointers at
archive indices 6, 7, 8, and 9. Each differs from its archive byte pointer and
has the expected model counts above. The DZS, three TEX, and DAT entries remain
pointer-identical to archive storage. All nine names resolve through original
`dRes_control_c::getRes` to those exact published pointers. Debug, optimized
Release, and strict ASan/UBSan all exit zero, including dRes unmount/destruction.

## Successor

`BW-P4-0095 / ROUTE_B_STAGE_INFO_OWNER` is resolved. Patch 0056 represents the
reached DZS file count, node count/offset, and STAG numeric fields with exact
big-endian wrappers and asserts the retail 0x04, 0x0c, and 0x20 layouts. It
leaves the node tag raw for the original memory-order four-character compare.
On target PC, offsets remain serialized until `dStage_stagInfoInit` resolves
the STAG pointer from the file base; no 32-bit slot is overwritten with a
native pointer.

The private owner census independently walks all 13 chunks and locates STAG,
then calls original `dStage_infoCreate`. The original stage object publishes
that exact pointer and invokes save/dungeon initialization with table 0; its
particle accessor returns scene 1. Debug, optimized Release, and strict
ASan/UBSan agree. Patch 0056 changes 80 of 3,652 compiled header/source lines,
for 2.19% adaptation density. Full stage/room decoding was not admitted.

The successor is `BW-P4-0096 / ROUTE_B_PLAY_PHASE2_PARTICLE_REQUEST`. It must
execute unchanged play `phase_2` through the qualified sync/STAG owners and
original `dPa_control_c::readScene`, then prove one asynchronous read of
`/res/Particle/Pscene001.jpc`. Particle construction, phase 3 completion, and
play `phase_4` remain outside the tier.
