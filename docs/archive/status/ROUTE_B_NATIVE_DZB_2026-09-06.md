# Native DZB resource owner and real Room44 queries — 2026-09-06

- Gate: P4 native player/camera composition; BW-P4-0126 remains active.
- State: ADVANCE within the collision dependency, not P4 completion.
- Base checkpoint: `e79c0b8`; Apple Silicon macOS, Debug/Release/strict sanitizers.
- Input: locked private `GZLE01-disc-image`; source/lock/protected runtime pins unchanged.

## Result

Patch 0149 adds native DZB ownership to the original dRes_info_c resource
loader. It uses the archive's expanded resource length, validates serialized
tables and graphs, constructs stable native arrays, and publishes cBgD_t through
the original resource lookup. The synchronous stage_dres and asynchronous stage
resource targets both enable this owner. They no longer publish raw DZB bytes
as if they were native collision structures. Retail ConvDzb remains unchanged.

The new private probe executes original archive decompression/mount, resource
admission/load/lookup, cBgW::Set, cBgS registration/queries/release, and resource
destruction on actual Room44 data: 3,641 vertices, 4,154 triangles, 1,121 blocks,
1,356 tree nodes, and 93 groups. Thirty-two spatial samples per load exercise
original ground and line intersection. Results agree within 0.1 game units
with an independent double-precision XZ barycentric scan of decoded triangles;
the oracle does not call the original plane, tree, or intersection helpers.
Four complete loads per mode pass. Two malformed private mounted copies fail
with `BW-DZB-DECODE` and are fully retired; the disc/source archive is unchanged.

This is a diagnostic real-asset route, not normal boot. Other resource types
remain unconverted in this narrowly composed private probe. The existing full
BG and Room44 regressions additionally prove decoder publication through their
already-qualified model/animation and asynchronous resource paths.

## Ownership and bounds

BlueWake's NativeDzb owns its source/name storage and six native vectors. It
is noncopyable/nonmovable so published pointers cannot be invalidated by owner
movement. Resource records retain an intrusive list and destroy it before their
archive is unmounted. Host-owned native buffers are explicitly RAII-managed;
they do not rely on the game solid heap to free ordinary host allocations.
Original collision acceleration arrays retain the separately qualified 0148
solid-heap allocation policy.

The decoder checks:

- 52-byte unconverted header; input bounded to 64 MiB;
- nonempty tables, u16-index-compatible counts, ranges, alignments, nonoverlap;
- finite vertex/group transform scalars and signed rotation conversion;
- vertex, triangle-information, group, block, node, and group-tree indices;
- strictly increasing block starts covering the triangle list;
- terminated group names, verified with one linear reverse scan;
- one reachable group root, parent/child agreement, no cycles or repeated
  nodes, and full group/tree reachability before original recursive traversal;
- maximum tree/group nesting of 128, with explicit rejection rather than
  truncation; and
- allocation failure returned as a decode error, without publishing partial data.

These are the currently admitted format bounds, not proof of compatibility
with every game's DZB. Empty geometry, deeper nesting, or other valid variants
outside this contract require measured expansion, not silent bypasses.
The decoder retains serialized flags plus the original converted marker;
serialized bytes themselves are never mutated.

## Tests and failed attempts

New public CTest covers native endian values, signed angles, independent source
lifetime, all 198 truncations before the synthetic fixture's required final
terminator, 14 malformed table/index/topology/scalar cases, and owner-list
cleanup. Live-owner diagnostics return to zero after rejected decodes and
explicit destruction.

Private Room44 queries pass four load/query/unload cycles plus two malformed
loads in all three modes. Native owners return to zero and JKR parent free
space returns exactly to baseline. The existing BG test checks native header
counts/marker/publication. The room lifecycle test checks one Ekao DZB owner,
then two with Room44 loaded, back to one after room unload, then zero after
Ekao deletion. Thus persistent object resources are not incorrectly freed with
the room. These assertions cover decoder ownership, not every unrelated
allocation in the broader runtime.

Failed fixture attempts retained in the evidence history:

1. Direct resource-info set without original reference admission made lookup
   report missing resource; use original dRes_control_c::setRes.
2. A 4 MiB collision-heap request exceeded remaining synthetic arena space;
   the actual geometry fits the measured 1 MiB test heap.
3. A zero-owner assertion before room load ignored the already-loaded Ekao
   archive. Source inspection established its independent lifetime; the test
   now checks the exact room +1/-1 change and final object-owner retirement.

## Verification

- Public default builds and CTest: 68/68 PASS in Debug, Release, ASan/UBSan.
- Original synthetic background-query probe: PASS in all three modes.
- New private real-DZB probe: PASS in all three modes, 128 ground plus 128
  line samples per mode and two malformed-load cleanup attempts.
- Existing private Toripost heap, McaMorf, BG profile, Stone2, Room44 lifecycle,
  and camera-construction probes: PASS in all three modes.
- Player asset generator: five tests PASS.
- Preparation through 0149, whitespace checks: PASS.
- Protected recompcore SHA-256s and dependency-lock hash: unchanged.
- Audit retains exactly the known 20 tracked local-research files and no new
  private-data failure. No app or Simulator was launched.

Reproduce the public decoder with CTest or target
`bluewake_route_b_native_dzb_test`. Build the optional
`bluewake_route_b_private_dzb_probe` in the Aurora-GX tree and pass the verified
disc path as its only argument. Strict environment:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Logs remain ignored under
`local-research/evidence/route-b-native-dzb-20260906/`.

## Next executable boundary

Replace the existing BG/Room44 fixture's collision constructor/Set/registration
seams with the qualified original background owner, now fed by real decoded
DZB. Replay actor creation, queries, delete/unload, and recreation together.
Keep unrelated audio/gameplay fences explicit. Then compose the remaining
camera roof/line/ground services, attention, and actual PLAYER runtime.

The camera Run census has not been recomposed here; its previous 60-symbol
count is historical. No full collision-response, playable Outset, normal boot,
performance, or full-game gate is claimed. The complete PRD goal remains active.
