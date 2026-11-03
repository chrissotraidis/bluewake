# PROGRESS — Ground collision walk, revisited

Status: M0-T01 done; M0-T02 host census partial (calls within a chunk unseen), module-side census next  
[PRD](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Is there anything to gain? (local, no native in the game)

- [x] M0-T01: `native_bg_test` runs on Linux (`bluewake_native_bg_test`), with the current direct-call handshake (v2); both leaf natives exact against the #503 control module.
- [ ] M0-T02: Census of the walk in Outset. *Host census done (partial: calls within a chunk are invisible to it); module-side observer next.* (groups, bounds rejections, `GroundCrossRp` calls and nodes, depth, cycles per call); ceiling in cycles per frame.
- [ ] M0-T03: `--bench`: translated vs prototype native walk vs declined path, on census-shaped trees; go/no-go (prototype ≥ 1.2×, declined within 2%).

### M1: Exact native walk with hand-over (depends: M0 go)

- [ ] M1-T01: Native walk; hand-over at what it does not cover.
- [ ] M1-T02: Differential test at every hand-over, short turns included.

### M2: Qualify (depends: M1)

- [ ] M2-T01: Trained build; Game Mode A/B (≥ 6 launches); checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Method | Environment | Result |
|---|---|---|---|---|
| M0-T02 (host census, partial) | `08be56c` (`bench/542-census`, `--reuse` over the #503 control); local `measure-542/census.log` (private) | `BLUEWAKE_BG_CENSUS=1`, `BLUEWAKE_DIRECT_CALLS=0`; one Game Mode launch, 90 s, worst-angle Outset state | Deck, Game Mode; ≈ 1,660 game frames | Per game frame: `GroundCross` ≈ 66, `ChkGrpThrough` ≈ 11,700, `cM3d_CrossY_Tri_Front` ≈ 290, `GroundCrossRp` ≈ 4.4, `GroundCrossGrpRp` ≈ 1.4. **The walk's own counts are far too low** (the profile gives `GroundCrossGrpRp` ≈ 1.9 M cycles/frame): the host sees only calls that pass its edge service, and calls within a chunk do not. `ChkGrpThrough` (a virtual call, shared with the wall, roof and other walks) is counted every time. Shape so far: tens of thousands of small group visits per frame, light polygon work. Next: a module-side observer hook for exact counts |
| M0-T01 | `native_bg_test` port; control module `a64af0e` (#503 stack, private) | `bluewake_native_bg_test MODULE 60000 2000000`, built with `-march=x86-64-v3` | Linux workstation | `ChkSameActorPid`: 43,612 identical, 16,388 declined unchanged. `ChkGrpThrough`: 40,888 identical (16,330 across the chunk boundary), 19,112 declined unchanged. 0 mismatches. Speed per call, translated through the dispatcher vs native: 15.2 vs 6.4 ns; 22.6 vs 10.1 ns (ground group); 20.4 vs 8.3 ns (water group, across the chunk). An empty dispatch is 8–9 ns: most of a tiny function's translated cost is the call itself |
| Planning | #436 `PROGRESS.md` M3-T04; `native_bg.c` on `docs/groundcross-native-plan` | Review of #436's code and evidence | — | Early exits only (99.5% of declines needed the walk); every call paid a ≈ 3.5 KB CPU-state copy, checks and lookups; the declined path's cost was never timed; accepted path 226 vs 179 ns |
