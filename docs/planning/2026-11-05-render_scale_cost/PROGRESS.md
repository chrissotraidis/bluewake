# PROGRESS — Why higher render settings slow the game when the GPU has room

Status: In progress. Cause found for the render scale: CPU/GPU power sharing. At Outset with Smooth Motion on, 4x lowers the busy game thread's effective clock by 7.0 % (3.29 → 3.06 GHz) while its work per frame is unchanged, accounting for the 6.3 % frame-rate loss. With Smooth Motion off, 4x costs little because the GPU draws half as many frames. Next: confirm directly with a GPU clock cap, then cut the GPU work that takes power from the CPU
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Size the slowdown
- [x] M1-T01: render_bench, render scale 3 vs 4, Smooth Motion on: −1.1 % overall, Outset −6.3 % by the paired CPU-profile result; 4x's launches spread as power management settles differently
- [x] M1-T02: The same with Smooth Motion off, desktop display: Outset +2.0 % (every 4x launch ahead), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %

### M2: Attribute it (depends: M1)
- [x] M2-T01: `--cpu-profile` at 3 vs 4: the game thread's cycles and instructions a frame are the same (Outset 119.9/119.1 M cycles, 189.7/188.8 M instructions, 98.7 % busy), but cycles/frame × frames/second gives 3.29/3.06 GHz. The 7.0 % clock loss accounts for the 6.3 % frame-rate loss
- [ ] M2-T02: `BLUEWAKE_GX_FLUSH_CENSUS=1` at 3 vs 4: draw-done wait per frame
- [ ] M2-T03: Present time per frame (opt-in diagnostic if needed)
- [ ] M2-T04: Synchronous readbacks: count and time (opt-in diagnostic if needed)
- [ ] M2-T05: Cap the GPU clock at 3x and 4x and test whether the game thread holds ≈ 3.3 GHz, directly testing CPU/GPU power competition

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
| M1-T01, M2-T01 | stack `afb6c35d8805`, gate and huge pages on | `render_bench` A=scale 3, B=scale 4, 3 pairs + warm-up, `--cpu-profile`; effective game-thread clock = cycles/frame × game frames/second | Steam Deck, headless gamescope, Smooth Motion 1 step | 4x vs 3x: Outset frame rate −6.3 % (27.45 → 25.73) and game-thread clock −7.0 % (3.29 → 3.06 GHz); lava 0 % clock/+3.3 % FPS; Hyrule 0 %/−1.6 %; butterfly −1 %/−0.4 %; shipwreck −2 %/−2.6 %. The game thread does the same work and is 98.7 % busy. Checkpoints identical. `[gpu-prof]` busy is not interpreted as elapsed occupancy because it can exceed frame time |
| M1-T02 | same build | `render_bench` A=scale 3, B=scale 4, Smooth Motion off, `--display desktop` | Steam Deck, 3–4 launches per scene | 4x vs 3x: Outset +2.0 % (33.47–33.72 vs 32.80–33.08), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %. `[gpu-prof]` busy exceeds the frame time in this mode, so it's not used |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | Corrected attribution: at Outset, 4x plus Smooth Motion raises GPU power enough to lower the busy game thread's clock by 7.0 %, accounting for the 6.3 % slowdown. This is CPU/GPU power competition, not proof that the GPU ran out of frame time |
| 2026-11-05 | GPU-side reductions remain the fix direction because they free APU power for the bottlenecked CPU. First targets: Smooth Motion's in-between frame and staging copies; test batching the copies before the first pass |
| 2026-11-05 | Superseded: cause for the render scale was attributed to Smooth Motion halving the GPU's time budget. `[gpu-prof]` busy cannot be read as elapsed frame occupancy and exceeds frame time with Smooth Motion off |
| 2026-11-05 | Superseded: H4 (GPU full) led based on `[gpu-prof]` busy per presented frame. The effective game-thread clock instead identifies shared APU power as the mechanism |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck while the GPU reports idle time |
