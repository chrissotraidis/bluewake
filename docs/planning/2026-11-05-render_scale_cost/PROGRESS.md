# PROGRESS — Why higher render settings slow the game when the GPU has room

Status: In progress. Cause found for the render scale: Smooth Motion. With it on, the GPU's budget halves and 4x goes over it (Outset −6.1 %); with it off, 4x costs little (Outset +2.0 %, others −2.6 % to +2.0 %). The game thread is unaffected. Next: the GPU work of the in-between frame and the staging copies (in Aurora)
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Size the slowdown
- [x] M1-T01: render_bench, render scale 3 vs 4, Smooth Motion on: −1.1 % overall, Outset −6.1 % with 4x's launches spread; GPU 17.1 → 21.2 ms a presented frame against an ≈ 18.5 ms budget
- [x] M1-T02: The same with Smooth Motion off, desktop display: Outset +2.0 % (every 4x launch ahead), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %

### M2: Attribute it (depends: M1)
- [x] M2-T01: `--cpu-profile` at 3 vs 4: the game thread's cycles and instructions a frame are the same (Outset 119.9/119.1 M cycles, 189.7/188.8 M instructions, 98.7 % busy): H1, H2 and H5 don't fit
- [ ] M2-T02: `BLUEWAKE_GX_FLUSH_CENSUS=1` at 3 vs 4: draw-done wait per frame
- [ ] M2-T03: Present time per frame (opt-in diagnostic if needed)
- [ ] M2-T04: Synchronous readbacks: count and time (opt-in diagnostic if needed)

### M2b: GPU work (depends: M1)
- [ ] M2b-T01: Per-pass GPU capture on the Deck (RenderDoc or Radeon GPU Profiler), one Outset frame, Smooth Motion on and off, 3x and 4x
- [ ] M2b-T02: What the staging copies move, and what is repeated every frame
- [ ] M2b-T03: What the in-between frame redraws that it could reuse
- [ ] M2b-T04: Read how Aurora computes `[gpu-prof]` busy (it exceeds the frame time with Smooth Motion off)

### M3: Other settings (depends: M2)
- [ ] M3-T01: HD look on vs off at scale 3, M1 and M2 again

### M4: Decide (depends: M2, M3)
- [ ] M4-T01: Name the cause per setting, with ≥ 90 % of the frame-time increase attributed
- [ ] M4-T02: Open a follow-up PR per change worth making, or record that it's the GPU

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01, M2-T01 | stack `afb6c35d8805`, gate and huge pages on | `render_bench` A=scale 3, B=scale 4, 3 pairs + warm-up, `--cpu-profile` | Steam Deck, headless gamescope, Smooth Motion 1 step | 4x vs 3x: Outset −6.1 %, lava +3.8 %, others ±1 %; overall −1.1 %. GPU busy per presented frame at Outset 17.1 → 21.2 ms (staging copies 7.4 → 8.1; in-between replay 6.4 → 7.2; IB EFB ≈ 9.1 → 10.4; EFB ≈ 3.2 → 5.7). Game thread identical. Checkpoints identical |
| M1-T02 | same build | `render_bench` A=scale 3, B=scale 4, Smooth Motion off, `--display desktop` | Steam Deck, 3–4 launches per scene | 4x vs 3x: Outset +2.0 % (33.47–33.72 vs 32.80–33.08), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %. `[gpu-prof]` busy exceeds the frame time in this mode, so it's not used |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | Cause for the render scale: Smooth Motion halves the GPU's budget per frame. The fixes are GPU-side, in Aurora (RecompCore): PRs there plus a pin bump here |
| 2026-11-05 | GPU busy is per presented frame: with Smooth Motion the budget is half the game frame's. H4 (GPU) leads; H1, H2, H5 don't fit the first profile |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck while the GPU reports idle time |
