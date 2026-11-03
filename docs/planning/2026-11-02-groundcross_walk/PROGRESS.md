# PROGRESS — Ground collision walk, revisited

Status: Planned; M0 next  
[PRD](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Is there anything to gain? (local, no native in the game)

- [ ] M0-T01: `native_bg_test` loads Linux modules; CMake target with the module's `-march`.
- [ ] M0-T02: Census of the walk in Outset (groups, bounds rejections, `GroundCrossRp` calls and nodes, depth, cycles per call); ceiling in cycles per frame.
- [ ] M0-T03: `--bench`: translated vs prototype native walk vs declined path, on census-shaped trees; go/no-go (prototype ≥ 1.2×, declined within 2%).

### M1: Exact native walk with hand-over (depends: M0 go)

- [ ] M1-T01: Native walk; hand-over at what it does not cover.
- [ ] M1-T02: Differential test at every hand-over, short turns included.

### M2: Qualify (depends: M1)

- [ ] M2-T01: Trained build; Game Mode A/B (≥ 6 launches); checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Method | Environment | Result |
|---|---|---|---|---|
| Planning | #436 `PROGRESS.md` M3-T04; `native_bg.c` on `docs/groundcross-native-plan` | Review of #436's code and evidence | — | Early exits only (99.5% of declines needed the walk); every call paid a ≈ 3.5 KB CPU-state copy, checks and lookups; the declined path's cost was never timed; accepted path 226 vs 179 ns |
