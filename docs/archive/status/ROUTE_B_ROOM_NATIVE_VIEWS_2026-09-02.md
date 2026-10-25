# Route B Room Native Views - 2026-09-02

## Boundary

The Room44 census identified six representation-only handlers in the initial
`sea_T` room loader: `FILI`, `2DMA`, `LBNK`, `SOND`, and the linked
`RPPN`/`RPAT` graph. Their retail overlays combine big-endian records and
32-bit pointer fields, so none can be published directly as native 64-bit
objects. Their lifetime belongs to `dStage_roomDt_c`, not the already
qualified stage-data owner.

## Result

Patch 0112 adds room-owned native storage for all six handlers. `FILI` decodes
its word and sea-level float; `2DMA` converts all 13 floats and preserves its
four byte fields; `SOND` converts both three-float positions and preserves the
remaining bytes. `LBNK` retains immutable archive bytes behind a native
counted wrapper. `RPPN` converts all points, while `RPAT` validates alignment
and point bounds before linking each native path to the room-owned point
array.

The room lifecycle releases every owned allocation and clears every published
view on reinitialization. The public test uses the exact title-route
cardinalities: one FILI, one 2DMA, 12 LBNK bytes, two SOND records, 271 RPPN
points, and 40 RPAT paths. It checks every generated field, all path ranges,
and reset ownership. The separate private census remains the evidence for the
retail payload's exact offsets and spans; aggregate retail-value qualification
remains closed until all eight handlers can compose.

## Evidence

- Patch SHA-256:
  `50fc737af692052fdaafcff74e4ccf5caabbe0fc9f711c2386282903b0e60866`.
- Debug room-view object SHA-256:
  `490415e6bc26d841b79e274e0ec2848b1873d96e410aff0358d1bfc21c3adab2`.
- Debug focused executable SHA-256:
  `f5e9128f96c8d5a409e81e3dd46a5fa72a83d449f2bd11932cb3e6dcef419ec8`.
- Release focused executable SHA-256:
  `caa9e9f50c76a76caf6d836d242e2bd3b713e364d6604800339d4fff9cb9746b`.
- ASan/UBSan focused executable SHA-256:
  `85113faa349c410fec72a2527e2cb7cb0ddac9c38e84264ec69b0ec9fd43a6b5`.
- Patches 0001 through 0112 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 53 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan.

The patch changes 296 lines across the 4,790-line header/source boundary, a
6.18% partition-and-adaptation density. No SHIP state, PLYR request, aggregate
room phase, BlueWake process, or Simulator was admitted.

## Next Boundary

Qualify the single title-route `SHIP` record as a room-owned native view and
run the unchanged ship initializer against bounded start-state scenarios.
Prove exact publication and state preservation/clearing behavior while
fencing PLYR, aggregate phase 2, ship actor construction, room reload, another
BlueWake process, and another Simulator.
