# PROGRESS — Ground collision walk, revisited

Status: M0 done: go (prototype walk 8.1× faster; ≈ +1% expected); M1 (exact native walk) next  
[PRD](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Is there anything to gain? (local, no native in the game)

- [x] M0-T01: `native_bg_test` runs on Linux (`bluewake_native_bg_test`), with the current direct-call handshake (v2); both leaf natives exact against the #503 control module.
- [x] M0-T02: Census of the walk in Outset: ≈ 87 group visits and ≈ 10 tree tests per ground check, 72 checks per frame, three levels. The cost is the group visits' overhead; Option A (native walk, translated tree tests). The host census missed calls within a chunk; the module-side census (`--bg-census`) sees all.
- [x] M0-T03: Local bench: prototype walk **8.1× faster** than the translation (8.6 vs 70 ns per group visit); a separate-check decline costs +3.1%, so M1 checks as it walks. **Go.**

### M1: Exact native walk with hand-over (depends: M0 go)

- [ ] M1-T00: Census: the share of top-level calls whose subtree reaches a polygon tree (sizes the native-to-return vs hand-over paths).
- [ ] M1-T01: Native walk, opt-in (`BLUEWAKE_NATIVE_GROUND_WALK=1`); certify-and-hook script; hand-over at the first polygon tree.
- [ ] M1-T02: Differential test at every hand-over, short turns included.

### M2: Qualify (depends: M1)

- [ ] M2-T01: Trained build; Game Mode A/B (≥ 6 launches); checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Method | Environment | Result |
|---|---|---|---|---|
| M0-T03 | `native_bg_test MODULE --walk-bench` (this commit); control module `a64af0e` | Synthetic ground check shaped like the census: 26 / 16 / 48 groups at depths 1 / 2 / 3, 90 visits, all passing, no polygon trees; best of 7 rounds | Linux workstation, `-march=x86-64-v3`, three runs | **Translated 6,290 ns per check (70 ns per visit, 9,238 guest cycles); prototype native 777 ns (8.6 ns per visit): 8.1× faster. Declined (separate check pass, then the translation): +3.1%.** Speed gate passes widely; the decline gate (≤ 2%) misses with a separate check pass, so M1 checks as it walks. Value in play ≈ 6,300 visits × 70 ns ≈ 0.44 ms of a 37 ms frame: ≈ +1% if most is removed. **Go**, with the polygon-tree hand-over and cycle accounting still to design |
| M0-T02 | `298e71a` (`bench/542-census2`, `--bg-census`, own training; AppImage in `artifacts/t542-census/298e71a683ed`); local `measure-542/census2.log` (private) | Module-side observers at the entry labels; one Game Mode launch, 90 s, direct calls on | Deck, Game Mode, worst-angle Outset; ≈ 2,200 game frames | **Per game frame: ≈ 72 ground checks, ≈ 6,300 group visits (≈ 87 per check; typically 64–127, max 121), ≈ 710 tree tests (≈ 10 per check; typically 8–15, max 36), ≈ 11,700 group filters (all walks).** Depth 1/2/3: 4.2 M / 2.6 M / 7.2 M visits, none deeper. With the profile's ≈ 1.9 M cycles/frame for `GroundCrossGrpRp`, that is ≈ 300 host cycles per group visit, mostly per-call and per-block overhead; tree tests ≈ 1,600 cycles each. Design: native group walk, translated tree tests (Option A). Rough ceiling ≈ +1–1.5% |
| M0-T02 (host census, partial) | `08be56c` (`bench/542-census`, `--reuse` over the #503 control); local `measure-542/census.log` (private) | `BLUEWAKE_BG_CENSUS=1`, `BLUEWAKE_DIRECT_CALLS=0`; one Game Mode launch, 90 s, worst-angle Outset state | Deck, Game Mode; ≈ 1,660 game frames | Per game frame: `GroundCross` ≈ 66, `ChkGrpThrough` ≈ 11,700, `cM3d_CrossY_Tri_Front` ≈ 290, `GroundCrossRp` ≈ 4.4, `GroundCrossGrpRp` ≈ 1.4. **The walk's own counts are far too low** (the profile gives `GroundCrossGrpRp` ≈ 1.9 M cycles/frame): the host sees only calls that pass its edge service, and calls within a chunk do not. `ChkGrpThrough` (a virtual call, shared with the wall, roof and other walks) is counted every time. Shape so far: tens of thousands of small group visits per frame, light polygon work. Next: a module-side observer hook for exact counts |
| M0-T01 | `native_bg_test` port; control module `a64af0e` (#503 stack, private) | `bluewake_native_bg_test MODULE 60000 2000000`, built with `-march=x86-64-v3` | Linux workstation | `ChkSameActorPid`: 43,612 identical, 16,388 declined unchanged. `ChkGrpThrough`: 40,888 identical (16,330 across the chunk boundary), 19,112 declined unchanged. 0 mismatches. Speed per call, translated through the dispatcher vs native: 15.2 vs 6.4 ns; 22.6 vs 10.1 ns (ground group); 20.4 vs 8.3 ns (water group, across the chunk). An empty dispatch is 8–9 ns: most of a tiny function's translated cost is the call itself |
| Planning | #436 `PROGRESS.md` M3-T04; `native_bg.c` on `docs/groundcross-native-plan` | Review of #436's code and evidence | — | Early exits only (99.5% of declines needed the walk); every call paid a ≈ 3.5 KB CPU-state copy, checks and lookups; the declined path's cost was never timed; accepted path 226 vs 179 ns |
