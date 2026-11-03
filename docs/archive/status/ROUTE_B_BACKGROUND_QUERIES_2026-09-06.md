# Original native background queries — 2026-09-06

- Gate: P4 native player/camera composition; BW-P4-0126 remains active.
- State: RETRY_NEW_HYPOTHESIS. Base checkpoint `e292923`.
- Build: Apple Silicon macOS, Debug, optimized Release, strict ASan/UBSan.
- Input: public synthetic typed geometry; private regressions use the existing
  locked `GZLE01-disc-image`. Dependency lock and protected runtime unchanged.

## Measured result

The new optional `bluewake_route_b_background_query_probe` links and executes
original cBgS/cBgW/dBgW geometry and polygon-filter bodies. Eight independent
heap cycles per configuration prove ground and line intersection, miss,
nearest-background selection, actor-ID exclusion, release, and exact heap
reclamation. Original camera and Link ground-check classes distinguish their
respective polygon-through flags. Moving background vertices are separately
owned, transformed at creation, updated after translation, and unchanged after
a repeated identical transform; source vertices remain unchanged. Exhausted
solid heaps produce the original Set failure and cleared partial geometry.

No query result is provided by a test seam. The fixture has one upward-facing
triangle, one leaf tree, one group, and an additional translated moving
background. It does not cover the full geometry algorithm, group hierarchy,
roof/sphere queries, collision response, or real room content.

## Failures and owner repair

The initial six-unit census failed with 20 diagnostics, all in ConvDzb:
six pointer-to-int alignment checks and fourteen pointer-to-u32 offset casts.
The other five units compiled. The serialized 52-byte GameCube header cannot
be overlaid by native cBgD_t, whose pointer fields have grown. Widening casts
would not repair layout, endian conversion, bounds, or lifetime.

Patch 0148 excludes only the retail in-place ConvDzb definition from the
explicit native query tier. It provides no substitute: a caller still fails
to link. Retail builds and existing root-only tiers are unchanged. The query
target instead accepts explicitly typed synthetic native geometry. Six original
query units plus original math/matrix sources link without unresolved symbols
in this probe; unused methods are dead-stripped, not runtime-qualified.

The first executable reached both expected intersections and release, then
failed `findFromRoot(world.pm_tri) == heap`. Six cBgW allocations used ordinary
host new despite the original solid-heap contract and bulk-only cleanup.
Patch 0148 applies the already-qualified JKR_NEW policy to transformed vertices,
triangle planes, triangle links, block lists, node bounds, and group bounds.
Host global new remains unchanged. The ownership and OOM tests then pass.

One fixture link failed because it called the PPC-specific PSMTXIdentity name;
using the existing platform-selected MTXIdentity API fixed the fixture.

## Verification

- New probe: PASS in Debug, Release, and strict sanitizers, eight successful
  geometry lifetimes and eight exhausted-heap attempts per mode.
- Public default build/CTest: 67/67 PASS in all three modes. The new optional
  probe is separately invoked and is not counted in those 67 tests.
- Existing private Toripost heap, McaMorf, BG profile, Stone2, Room44 lifecycle,
  and original camera-construction probes: PASS in all three modes.
- `scripts/prepare_route_b.sh --check`: PASS through 0148.
- `git diff --check`: PASS.
- Both protected recompcore hashes and dependency lock hash: unchanged.
- Audit: existing failure only, exactly 20 tracked local-research files;
  no new private data tracked. No app or Simulator launched.

Reproduce: configure the existing `build/route-b-{mode}` tree, build target
`bluewake_route_b_background_query_probe`, then run that executable. Strict
environment: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`.
Private logs: ignored `local-research/evidence/route-b-background-queries-20260906/`.

## Next required action and claim boundary

Implement a bounded, endian-aware DZB-to-native owner using actual archive
resource size and lifetime, then exercise the original queries on Room44's
real collision resource. Original dRes_info_c::loadResource calls ConvDzb with
only a pointer; its current native tier skips that branch. The private BG
fixture still substitutes collision construction/registration, so it cannot
certify the functional owner established here. Replace those seams only once
the resource decoder and ownership are proven. Do not pass raw DZB bytes to
the native cBgW::Set.

Camera Run's last measured 60-symbol census is unchanged historical evidence;
it has not been recomposed with this owner. Follow real-resource integration
with roof/line/ground camera dependencies, attention, and actual PLAYER
creation/update/draw. No normal boot, playable Outset, speed, or P4 completion
is claimed. The full PRD goal remains active.
