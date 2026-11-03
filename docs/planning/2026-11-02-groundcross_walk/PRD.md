# PRD — Ground collision walk, revisited

Status: Done, no-go for speed. An exact native walk measured 1.03× the translation; the tools and findings are kept  
Source: the [#503](https://github.com/chrissotraidis/bluewake/pull/503) stack (`b538550`)  
Follows: [#436](https://github.com/chrissotraidis/bluewake/pull/436) (closed no-go) · [Technical plan](TECH_PLAN.md) · [Tracker and evidence](PROGRESS.md)

## Problem

Every actor on the ground asks the collision code for the floor under it each frame. `cBgW::GroundCrossGrpRp` walks the stage's collision groups: a filter, height bounds, a polygon-tree test and the children, recursively. In #503's Outset profile the ground check's functions take about 4.3% of the game thread, and the walk itself 1.48%.

#436 replaced it with a native that was correct but not faster. This plan asked whether a native covering the whole walk could win.

## What we tried

1. **Measure first, without native code (M0):**
   - port the collision test to Linux;
   - count the walk's shape on the Deck;
   - time the translated walk against a prototype native on census-shaped data.
2. **An exact native walk (M1):** the translated walk transcribed block by block, stopping only where the translation itself can hand the dispatcher a `pc`, so it stays byte-exact.

## What we found

| Finding | Evidence |
|---|---|
| **The walk is many small group visits.** ≈ 72 ground checks per Outset frame, each visiting ≈ 87 groups over three levels and reaching ≈ 10 polygon-tree tests. 96% of top-level walks are a single group; the 3% big walks hold two thirds of the visits | Module-side census on the Deck (a host-side census missed calls inside a chunk) |
| **A prototype native looked 8.1× faster** (8.6 vs 70 ns per group visit) | Local walk bench |
| **The exact native is only 1.03× faster** (68.5 vs 70.3 ns per visit), and exact in 400,000 random cases | Local walk bench; differential test against the translation |
| **The prototype's gain was the cost of exactness.** The translation's prepaid fast copies are already close to native C. A visit's cost is what the guest can observe: the stack frame and five saved registers, the FPSCR bits of six float compares, the cycle charges, the direct-call readiness checks and the exact group filter | Profile of the exact native: no hotspot, the time spread over that work |

## Decision

**No-go for the native walk.** Its exact form measured 1.03×, worth about +0.1–0.2% of a frame at best, for about 500 lines of intricate code. The code and its test are kept in history (`f102f07`), not in this PR.

**Kept, because they are useful beyond this walk:**
- `native_bg_test` running on Linux against the game module, with the current direct-call handshake;
- the walk bench (translated, prototype, decline);
- two diagnostic censuses, off by default: the host's (`BLUEWAKE_BG_CENSUS=1`, calls through the edge service) and the module's (`--bg-census`, every call).

## Long-term impact

- **A rule for future natives:** a prototype that skips guest-visible effects overstates the gain. Before building a native, price what it must keep (frames, saved registers, flags, cycles), or time an exact version on the bench. That probably also explains #436's slower native.
- **Where natives can still win:** functions whose translated cost is mostly per-call machinery (many tiny leaves, called through the dispatcher), or whose observable effects are few compared with their work. Skinning (#541) was one: heavy arithmetic per call, little state to reproduce.
- **Collision stays translated.** The wall, roof and water walks have the same shape, so the same conclusion applies to them.

## Constraints that applied

| Rule | How it was applied |
|---|---|
| Exact or nothing | The native was built to be byte-exact; the differential test compared every CPU byte and the test memory |
| Measure before building | M0 (census, bench) before M1; M1 benched before any Deck build |
| Opt-in until qualified | The native would have been off unless `BLUEWAKE_NATIVE_GROUND_WALK=1`; it never reached a build |
| Private game data stays off GitHub | Modules, logs and censuses kept local; only numbers here |
