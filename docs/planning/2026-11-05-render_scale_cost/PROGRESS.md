# PROGRESS — Why higher render settings slow the game when the GPU has room

Status: In progress. First measurement: the game thread's work is the same at 3x and 4x; the GPU is at its limit at 4x because Smooth Motion presents two frames per game frame (H4). Confirming with Smooth Motion off
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Size the slowdown
- [x] M1-T01: render_bench, render scale 3 vs 4, Smooth Motion on: −1.1 % overall, Outset −6.1 % with 4x's launches spread; GPU 17.1 → 21.2 ms a presented frame against an ≈ 18.5 ms budget
- [ ] M1-T02: The same with Smooth Motion off, desktop display (running)

### M2: Attribute it (depends: M1)
- [x] M2-T01: `--cpu-profile` at 3 vs 4: the game thread's cycles and instructions a frame are the same (Outset 119.9/119.1 M cycles, 189.7/188.8 M instructions, 98.7 % busy): H1, H2 and H5 don't fit
- [ ] M2-T02: `BLUEWAKE_GX_FLUSH_CENSUS=1` at 3 vs 4: draw-done wait per frame
- [ ] M2-T03: Present time per frame (opt-in diagnostic if needed)
- [ ] M2-T04: Synchronous readbacks: count and time (opt-in diagnostic if needed)

### M3: Other settings (depends: M2)
- [ ] M3-T01: HD look on vs off at scale 3, M1 and M2 again

### M4: Decide (depends: M2, M3)
- [ ] M4-T01: Name the cause per setting, with ≥ 90 % of the frame-time increase attributed
- [ ] M4-T02: Open a follow-up PR per change worth making, or record that it's the GPU

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01, M2-T01 | stack `afb6c35d8805`, gate and huge pages on | `render_bench` A=scale 3, B=scale 4, 3 pairs + warm-up, `--cpu-profile` | Steam Deck, headless gamescope, Smooth Motion 1 step | 4x vs 3x: Outset −6.1 %, lava +3.8 %, others ±1 %; overall −1.1 %. GPU busy per presented frame at Outset 17.1 → 21.2 ms (staging copies 7.4 → 8.1; in-between replay 6.4 → 7.2; IB EFB ≈ 9.1 → 10.4; EFB ≈ 3.2 → 5.7). Game thread identical. Checkpoints identical |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | GPU busy is per presented frame: with Smooth Motion the budget is half the game frame's. H4 (GPU) leads; H1, H2, H5 don't fit the first profile |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck while the GPU reports idle time |
