# PROGRESS — The GPU's share at busy views: HD effects, power and Smooth Motion

Status: Planned — follows #586
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: What the GPU costs at the busiest view
- [x] M1-T00: Found on #586 (its M4-T23): at the busiest Outset view (`8-busiest`, ~1,950 draws), Game Mode, #586's switches on: 26.53 fps; Smooth Motion off 30.02; HD look off 29.19; distant islands off 26.57. Game thread ~98 % busy throughout
- [ ] M1-T01: Sampler with each CPU core's clock beside GPU busy, GPU clock and chip power, tagged per launch
- [ ] M1-T02: Each HD effect off in turn at `8-busiest` in Game Mode, 3 launches each, with the CPU clock logged
- [ ] M1-T03: Confirm the power sharing: CPU clock with the HD look on against off

### M2: Cheaper HD effects at busy views
- [ ] M2-T01: The largest effect from M1-T02 made cheaper, opt-in, exact or within one shade step
- [ ] M2-T02: Game Mode, 6 launches each way, every launch ahead; pictures; #586's seven scenes plus `8-busiest`

### M3: Smooth Motion by reprojection (opt-in)
- [ ] M3-T01: The in-between frame's GPU and CPU share at `8-busiest`
- [ ] M3-T02: Motion pass (positions only, both frames) and warp; HUD after the warp
- [ ] M3-T03: Pictures against today's in-between frames (Smooth Motion dump); blended draws handled
- [ ] M3-T04: Hole fill from the previous frame; Game Mode fps; owner's play test

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T00 | #586 tracker M4-T23; Deck `~/bluewake-render/gm/busy` | Game Mode through the owner's shortcut, 3 launches each | Build `9dde6aa9d`, 3x + HD + Smooth Motion, #586's switches | 26.53 / SM off 30.02 / HD off 29.19 / islands off 26.57 |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: wrap up #586 with its data; plan the GPU side of busy views here (HD effect costs with power, and reprojection) and the game thread on #592 |
