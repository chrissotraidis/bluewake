# TECH_PLAN — Ground collision walk, revisited

Status: Done, no-go for speed  
[PRD](PRD.md) · [Tracker and evidence](PROGRESS.md) · #436's [contract map](https://github.com/irregularthanis/bluewake/blob/docs/groundcross-native-plan/docs/planning/2026-10-21-groundcross_native_reimplementation/GROUND_CROSS_CONTRACT.md)

## What we tried

`cBgW::GroundCrossGrpRp` (0x802491F4–0x80249368), from the disassembly:

```
GroundCrossGrpRp(this r3, check r4, group r5, depth r6):
  if this->vtbl[8](this, group, check->pass (+4), depth)  -> return 0   (ChkGrpThrough, virtual)
  b = this->bounds(+156) + 32*group
  if not (x, z inside b's x/z range)                        -> return 0
  if not (b.minY < check.y(+40)) or b.maxY < check.hitY(+52) -> return 0
  hit = 0;  g = this->table(+148)->groups(+36) + 52*group
  if g.tree(+46) != 0xFFFF and GroundCrossRp(this, check, g.tree) -> hit = 1
  for c = g.child(+40); c != 0xFFFF; c = groups[c].sibling(+38):
      if GroundCrossGrpRp(this, check, c, depth + 1)         -> hit = 1
  return hit
```

| Step | What | How |
|---|---|---|
| M0-T01 | The collision test on Linux | `native_bg_test`: a portable layer (module loading, page protection, timer); the current direct-call handshake (v2) |
| M0-T02 | The walk's shape in Outset | A host census (`BLUEWAKE_BG_CENSUS=1`), then a module census (`--bg-census`: read-only observer calls at the entry labels) |
| M0-T03 | Can a native win? | `native_bg_test --walk-bench`: a census-shaped check (90 visits) through the translation, a prototype and a decline |
| M1-T00 | Where the big walks reach a polygon tree | The module census, per top-level walk |
| M1-T01 | An exact native | The translated walk transcribed block by block (below); `--walk`, a differential test |

### The exact native, as built (`f102f07`, not merged)

- **Same effects:** the same registers, CR0, FPSCR, stack stores, inline `_savegpr_27`/`_restgpr_27`, cycle charges and suffix writes as the translation's prepaid fast copies, in the same order, through the same memory helpers.
- **Same exits:** it stops only where the translation itself can hand the dispatcher a `pc`:
  - budget checks;
  - helpers or calls that are not ready;
  - a polygon tree (`GroundCrossRp` stays translated);
  - a read that is not plain RAM.
- **Silent boundaries:** stopping is exact only where the dispatcher passes the boundary silently, so the native requires the edge filter on, the host quiet and the walk's addresses unwatched. It keeps 256 cycles in hand at every budget check, so a stop never moves the end of a turn.

The test found that last point: a stop with the budget already spent ended the turn early.

## What we found

| Measurement | Result |
|---|---|
| Module census (Deck, Outset, ≈ 2,200 frames) | ≈ 72 ground checks, ≈ 6,300 group visits and ≈ 710 tree tests per frame; three levels deep |
| Per top-level walk | 96% one group, no tree; the 3% of 32–127 groups hold two thirds of the visits, always reach a tree, the first after ≈ 11 visits |
| Bench: translated | 70 ns per group visit (9,238 guest cycles per 90-visit check) |
| Bench: prototype (not exact) | 8.6 ns per visit: 8.1× |
| Differential test, exact native | 400,000 random cases identical: 312,685 returned natively, 44,172 stopped and finished translated, 43,143 declined unchanged |
| Bench: exact native | 78.7 ns per visit (0.89×); after inlining the float compare, **68.5 ns (1.03×)** |

The exact native's profile has no hotspot. Its time goes on the effects the guest can observe, and the translation's fast copies spend about the same on them. The only machinery left to remove is the call dispatch around the filter, a few percent.

## Decision

**No-go.** Drop the native, keep the tools and the findings. Its best case is about +0.1–0.2% of a frame, for intricate code that would need its own hook script, certification and Deck qualification.

## Long-term impact

- **Price exactness first:** when the prototype skips guest-visible effects, it is an upper bound, not a forecast. The bench's comment now says so.
- **Tools kept:**
  - `native_bg_test MODULE [CASES] [BENCH_CALLS]`: the collision leaves, exact against the module, on Linux and Windows.
  - `native_bg_test MODULE --walk-bench`: the walk translated, as a prototype and as a decline.
  - `BLUEWAKE_BG_CENSUS=1` (host, with `BLUEWAKE_DIRECT_CALLS=0`) and `--bg-census` (module, Linux diagnostic builds): the ground check's call counts and shapes.
- **The other collision walks** (wall correction, roof, water) have the same shape and the same exact effects per visit, so they are not candidates either.

## Changes

| File | Change |
|---|---|
| `tests/native_bg_test.c` | Portable (Linux and Windows); direct-call handshake v2; `--walk-bench` |
| `runtime/host/CMakeLists.txt` | `bluewake_native_bg_test` (explicit, never run by public CI) |
| `runtime/host/src/bg_census.{c,h}`, `feature_dispatch.h`, `main.c`, the platforms' CMake lists | The host census, off by default; its addresses kept off the module's watch list |
| `cmake/composite/bg_census_module.{c,h}`, `scripts/windows/bg_census.py`, `scripts/linux/build.py`, `cmake/composite/CMakeLists.txt` | The module census (`--bg-census`), off by default, part of the build's fingerprint |
| `tests/feature_dispatch_test.c`, `tests/test_direct_calls.py` | The census's symbols in the dispatch tests |
