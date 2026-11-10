# PROGRESS — The GPU's share at busy views: HD effects, power and Smooth Motion

Status: **On hold** (owner, 2026-11-10) — measurements done; the fused effects pass removed; reprojection (up to ~8.7 % at the busiest view, M3-T00) not started. The game thread (#592) goes first
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
- [x] M2-T00: Fuse the per-pixel effects into one full-screen pass, opt-in: `DOL_AURORA_FUSED_EFFECTS=1`, menu "Draw the HD effects in one pass" (RecompCore #57 `kFusedShader`, generated from the four shaders' own terms; bluewake `04ebe869b`). Blended as scene × alpha + rgb: ((scene × ao + edge + torch) × cloud), one 8-bit rounding. Used when two or more of the four apply, not with the sun shadows or a debug view. Runs on the Deck without shader errors (build `0e1138649`)
- [x] M2-T01b: First version, four effects fused (occlusion, edge, torch, clouds; build `0e1138649`), headless. Pictures (Smooth Motion off, eight scenes): lava identical; Hyrule 70.8 dB, owner's state 70.8, butterfly 62.4, shipwreck 62.1, heavy 60.1, busiest 54.2, **Outset 50.7: sunlit rims up to 27 of 255 brighter under cloud shadow**. Separately, the target clamps the rim at white before the clouds darken it; fused, the clamp comes after. Speed at `8-busiest`: 25.80 vs 25.57 (+0.9 %), fused ahead in each interleaved pair. Fusing saves the reads and writes, not the effects' own shader work, which is most of their cost
- [x] M2-T01c: The cloud shadows stay their own pass (RecompCore #57 `cloud shadows leave the fused pass`); occlusion, edge and torch light sum the same with one clamp (both lights add). Build `7139b315c`
- [x] M2-T01d: Three-effect version (build `7139b315c`), headless: pictures within 1–2 of 255 in every scene and frame (62.3–81.8 dB; lava identical), except one Outset capture taken a moment apart (every edge moved; its other four frames 79 dB). Speed at `8-busiest`, 6 launches each way: **25.68 vs 25.73 (−0.21 %)**, not ahead. **Removed** (RecompCore #57 `767eefe8` reverts; bluewake `090dcb4e2` removes the menu switch). Fusing saves the reads and writes, not the effects' own shader work
- [ ] M2-T01: The largest effect from M1-T02 made cheaper, opt-in, exact or within one shade step
- [ ] M2-T02: Game Mode, 6 launches each way, every launch ahead; pictures; #586's seven scenes plus `8-busiest`

### M3: Smooth Motion by reprojection (opt-in)
- [x] M3-T00: The ceiling: `DOL_AURORA_FRAME_INTERP_REPEAT=1` (RecompCore #57, diagnostic) presents the real frame again instead of replaying the scene. First run invalid: the skip sat in `present_with_in_between`, but the default path is the clocked `present_steps` (the replay still showed 7.3 ms). Fixed (`9a206173`), build `298ac17b5`, headless, `8-busiest`, 4 launches each: as played **25.70**; no replay **27.95 (+8.7 %)**, GPU time per frame 20.2 → 1.6 ms; Smooth Motion off **29.64 (+15.3 %)**. Of Smooth Motion's cost here, ~8.7 % is the replay (GPU, and the render worker encoding it) and ~6.6 % the CPU work beside it (recording each draw, blending its matrices; the interp worker 16 % busy)
- [ ] M3-T00b: Reprojection's reach from M3-T00: a camera-only warp replaces the replay (up to ~8.7 % less its own full-screen pass) and needs no per-draw blending (part of the other ~6.6 %); with per-object motion it keeps the per-draw blending, so up to ~8.7 %
- [ ] M3-T01: The in-between frame's GPU and CPU share at `8-busiest`
- [ ] M3-T02: Motion pass (positions only, both frames) and warp; HUD after the warp
- [ ] M3-T03: Pictures against today's in-between frames (Smooth Motion dump); blended draws handled
- [ ] M3-T04: Hole fill from the previous frame; Game Mode fps; owner's play test

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M3-T00 | `render-repeat2/report.md` (`render-repeat` invalid) | render_bench headless, paced, 3 setups × 4 launches, interleaved | Build `298ac17b5`, `8-busiest`, #586's switches | 25.70 / no replay 27.95 / SM off 29.64 |
| M2-T01d | `render-fused3-pics/report.md`, `render-fused3-speed/report.md` | As M2-T01b | Build `7139b315c` | Pictures 62.3–81.8 dB; −0.21 % |
| M2-T01b | `render-fused-pics/report.md`, `render-fused-speed/report.md`, comparison images | render_bench headless, paced; pictures with Smooth Motion off, `--every 300`, eight scenes; speed 6 launches each way | Build `0e1138649`, 3x + HD, #586's switches | Pictures 50.7–70.8 dB (Outset rims); +0.9 % at `8-busiest` |
| M1-T02h | `render-fx-busiest/report.md`, `fxh-samples.txt` | render_bench headless, paced, 9 setups × 2 launches, interleaved | Build `eaa85f0f1`, `8-busiest`, 3x + HD + Smooth Motion, #586's switches | As above |
| M1-T00 | #586 tracker M4-T23; Deck `~/bluewake-render/gm/busy` | Game Mode through the owner's shortcut, 3 launches each | Build `9dde6aa9d`, 3x + HD + Smooth Motion, #586's switches | 26.53 / SM off 30.02 / HD off 29.19 / islands off 26.57 |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: hold #591; reprojection is the riskier lead. #592 (the game thread) first |
| 2026-11-10 | Remove the fused per-pixel pass: with the cloud shadows apart it isn't ahead (−0.21 %), and with them it changes sunlit rims. The effects' cost is their own shader work, about 1.2 % each at the busiest view |
| 2026-11-10 | Owner: wrap up #586 with its data; plan the GPU side of busy views here (HD effect costs with power, and reprojection) and the game thread on #592 |
