# PROGRESS — The GPU's share at busy views: HD effects, power and Smooth Motion

Status: Planned — follows #586
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: What the GPU costs at the busiest view
- [x] M1-T00: Found on #586 (its M4-T23): at the busiest Outset view (`8-busiest`, ~1,950 draws), Game Mode, #586's switches on: 26.53 fps; Smooth Motion off 30.02; HD look off 29.19; distant islands off 26.57. Game thread ~98 % busy throughout
- [x] M1-T01: Sampler with each CPU core's clock beside GPU busy, GPU clock and chip power (`sampler3.sh`, every 0.25 s). Tags each Game Mode launch; headless launches go untagged
- [ ] M1-T02: Each HD effect off in turn at `8-busiest` in Game Mode, 3 launches each, with the CPU clock logged. Blocked in Game Mode for now: since 10:25 Steam stops each launch at a "controller configuration sync failed" prompt that needs a button press (the owner chose to keep the prompt). Measured headless instead (M1-T02h)
- [x] M1-T02h: Headless, `8-busiest`, #586's four switches on, 2 launches each (build `eaa85f0f1`): as played 25.19 game fps, GPU 20.3 ms a frame; Smooth Motion off 29.00 (+15.1 %); **HD look off 26.63 (+5.7 %, 18.6 ms)**; each effect off alone within about 1 %: AO +1.1 %, far blur +0.5 %, clouds +0.2 %, edge light −0.3 %, bloom −0.2 %, torch light −1.3 % (noise). Game thread 97.7 % busy in every setup. No single effect is the cost; the HD look's 5.7 % is in parts without their own switch (M1-T04)
- [x] M1-T04: The HD look's cost split, headless, `8-busiest`, 4 launches each (build `382f6ec8c`, checkpoints identical in every run): as played 25.24; in-between frame without the HD effects (diagnostic `DOL_AURORA_FRAME_INTERP_NO_HD`, RecompCore #57) 25.96 (+2.9 %); HD look on with all six effects off 26.86 (+6.4 %); HD look off 26.98 (+6.9 %); Smooth Motion off 29.19 (+15.7 %). The HD look's cost is its six effects, about half of it on the in-between frame
- [x] M1-T05: Fixed overhead or the effects themselves? 4 launches each: all six on 25.29 (20.4 ms GPU a frame); all off 26.89 (18.6 ms); only torch light on (no torches in view: the scene pass still splits for the effects, but no torch pass runs) 26.82 (18.6 ms); only edge light on (one full-screen pass) 26.58 (18.9 ms). **The split is free; each full-screen effect pass costs about 1.2 %**, and the six add up. At this view about 3.5 % of frame rate per millisecond of GPU time, through power. Single-effect runs at 2 launches (M1-T02h) were within noise

- [ ] M1-T03: Confirm the power sharing: CPU clock with the HD look on against off

### M2: Cheaper HD effects at busy views
- [ ] M2-T00: Fuse the per-pixel effects (occlusion apply, edge light, torch light, cloud shadows) into one full-screen pass, opt-in: colour × (occlusion × cloud) + (edge + torch) × cloud, read depth once, write colour once. Within one 8-bit step (one rounding instead of one per pass)
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
| M1-T02h | `render-fx-busiest/report.md`, `fxh-samples.txt` | render_bench headless, paced, 9 setups × 2 launches, interleaved | Build `eaa85f0f1`, `8-busiest`, 3x + HD + Smooth Motion, #586's switches | As above |
| M1-T00 | #586 tracker M4-T23; Deck `~/bluewake-render/gm/busy` | Game Mode through the owner's shortcut, 3 launches each | Build `9dde6aa9d`, 3x + HD + Smooth Motion, #586's switches | 26.53 / SM off 30.02 / HD off 29.19 / islands off 26.57 |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: wrap up #586 with its data; plan the GPU side of busy views here (HD effect costs with power, and reprojection) and the game thread on #592 |
