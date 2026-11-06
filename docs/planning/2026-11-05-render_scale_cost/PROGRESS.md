# PROGRESS — Why higher render settings slow the game when the GPU has room

Status: In progress. Cause confirmed for the render scale: CPU/GPU power sharing. RecompCore #53's opt-in staging-copy experiment has been measured and its data recorded; its code and BlueWake integration remain present pending an explicit disposition decision.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Size the slowdown
- [x] M1-T01: render_bench, render scale 3 vs 4, Smooth Motion on: −1.1 % overall, Outset −6.3 % by the paired CPU-profile result; 4x's launches spread as power management settles differently; evidence: artifact=stack afb6c35d8805; method=render_bench with three interleaved pairs plus warm-up and CPU profile; result=Outset 27.45 to 25.73 FPS with identical checkpoints
- [x] M1-T02: The same with Smooth Motion off, desktop display: Outset +2.0 % (every 4x launch ahead), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %

### M2: Attribute it (depends: M1)
- [x] M2-T01: `--cpu-profile` at 3 vs 4: the game thread's cycles and instructions a frame are the same (Outset 119.9/119.1 M cycles, 189.7/188.8 M instructions, 98.7 % busy), but cycles/frame × frames/second gives 3.29/3.06 GHz. The 7.0 % clock loss accounts for the 6.3 % frame-rate loss; evidence: artifact=render-scale-3v4 CPU profiles; method=cycles per frame times frames per second; result=3.29 versus 3.06 GHz with unchanged work per frame
- [ ] M2-T02: `BLUEWAKE_GX_FLUSH_CENSUS=1` at 3 vs 4: draw-done wait per frame
- [ ] M2-T03: Present time per frame (opt-in diagnostic if needed)
- [ ] M2-T04: Synchronous readbacks: count and time (opt-in diagnostic if needed)
- [x] M2-T05: GPU capped at 1100 MHz: 4x holds 3.34 GHz and reverses the Outset loss to +6.4 %; all six 4x launches beat all six 3x launches

### M2b: GPU work (depends: M1)
- [ ] M2b-T01: Deferred — per-pass GPU capture on the Deck, only if a concrete follow-up needs it
- [x] M2b-T02: Source inspection: Smooth Motion replay skips ordinary staging-copy ops; real frames incrementally upload vertex, uniform, index, storage, and ordered texture data
- [x] M2b-T03: Source inspection: the in-between frame reruns render passes and EFB work; it does not repeat ordinary staging copies
- [ ] M2b-T04: Deferred — characterize `[gpu-prof]` busy before using it as elapsed occupancy

### M3: First source experiment (depends: M2, M2b)
- [x] M3-T01: Implement opt-in non-texture staging-copy batching in RecompCore #53, stacked on #49; default path and texture order unchanged; evidence: artifact=RecompCore d72f71b9; method=source diff and commit inspection; result=flag-gated buffer-copy command buffer with ordered texture uploads unchanged
- [x] M3-T02: Compile the modified Aurora translation units; `git diff --check` clean; evidence: artifact=common.cpp and gpu_prof.cpp objects; method=host Clang 22 using existing BlueWake defines and includes plus symbol check; result=compiled with one pre-existing shader-type warning and matching DeferredZone symbols, with no registered CTest tests
- [x] M3-T03: Update #563's RecompCore pin, dependency lock, exported patches, opt-in GUI setting, and tests; evidence: artifact=BlueWake #563 pin 05b2493f via upstream draft refs/pull/53/head and patches 0197-0207; method=profile/lock inspection, exact RecompCore format-patches, settings-table/state tests, and RecompCore PR CI workflow; result=#49/#53 are pinned to the upstream draft and exact SHA before merge, the GUI defaults off and persists its next-launch value, and both copy plans have executable coverage
- [x] M3-T04: Produce a matched build with baseline and candidate selected only by the opt-in environment key; evidence: artifact=BlueWake a555c3aed7f4 executable 77754f5e58d7 and module 717a582e5661; method=same executable and game module for A and B; result=only DOL_AURORA_BATCH_STAGING_COPIES differed
- [x] M3-T05: Run 12 interleaved Deck launches per setting; evidence: artifact=render-power-batch/report.txt; method=24 measured runs, A/B/B/A, five states, 4x, Smooth Motion one step; result=+0.03% overall, slower in four scenes including Outset −1.23%, all scenes loaded and checkpoints identical; stated performance gate not met
- [!] M3-T06: Blocked pending an explicit decision after M3-T05 did not meet the performance gate; picture comparison has not been run

### M4: Decide (depends: M3)
- [x] M4-T01: Name the render-scale cause with ≥ 90 % of the Outset frame-rate loss attributed: shared APU power lowers the game-thread clock; evidence: artifact=uncapped and 1100-MHz capped Deck profiles; method=compare game-thread work, effective clock, and FPS; result=7.0 % uncapped clock loss accounts for the 6.3 % FPS loss and the cap reverses the result
- [ ] M4-T02: If #53 passes every gate, keep its pin in #563; otherwise revert it and record the no-go result
- [ ] M4-T03: If batching is insufficient, plan the next isolated experiment around work rerun for Smooth Motion's in-between frame

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01, M2-T01 | stack `afb6c35d8805`, gate and huge pages on | `render_bench` A=scale 3, B=scale 4, 3 pairs + warm-up, `--cpu-profile`; effective game-thread clock = cycles/frame × game frames/second | Steam Deck, headless gamescope, Smooth Motion 1 step | 4x vs 3x: Outset frame rate −6.3 % (27.45 → 25.73) and game-thread clock −7.0 % (3.29 → 3.06 GHz); lava 0 % clock/+3.3 % FPS; Hyrule 0 %/−1.6 %; butterfly −1 %/−0.4 %; shipwreck −2 %/−2.6 %. The game thread does the same work and is 98.7 % busy. Checkpoints identical. `[gpu-prof]` busy is not interpreted as elapsed occupancy because it can exceed frame time |
| M1-T02 | same build | `render_bench` A=scale 3, B=scale 4, Smooth Motion off, `--display desktop` | Steam Deck, 3–4 launches per scene | 4x vs 3x: Outset +2.0 % (33.47–33.72 vs 32.80–33.08), lava −1.5 %, Hyrule +2.0 %, butterfly −1.7 %, shipwreck −2.6 %. `[gpu-prof]` busy exceeds the frame time in this mode, so it's not used |
| M2-T05 | same build | Same 3x/4x run with the Deck GPU fixed at 1100 MHz; six measured launches per setting plus one CPU-profile launch | Steam Deck, headless gamescope, Smooth Motion 1 step | 4x vs 3x: Outset +6.4 % (median 29.38 vs 27.61 FPS; ranges 29.14–30.15 vs 27.08–28.19), lava +3.1 %, Hyrule 0.0 %, butterfly +0.1 %, shipwreck −4.3 %, overall +0.6 %. Every 4x Outset launch beat every 3x launch. Effective game-thread clock 3.34 vs 3.30 GHz; instructions/frame −0.4 %. The profile also has −5.3 % cycles/frame and +5.2 % IPC at 4x, so do not attribute the reversal's exact size to clock alone. All 12 runs loaded every state; guest checkpoints identical. Picture results ignored because Smooth Motion was on. Deck restored to 200–1600 MHz and `auto` |
| M2b-T02, M2b-T03 | RecompCore #49 head `a4dbaaa8` | Read Aurora operation replay and staging upload paths | RecompCore source | Smooth Motion replay skips ordinary staging-copy operations and reruns render/EFB work. Texture uploads cannot be moved wholesale without changing overwrite/sample order; buffer arenas can be isolated for an opt-in experiment |
| M3-T01, M3-T02 | RecompCore #53, `d72f71b9` | `git diff --check`; compile modified `common.cpp` and `gpu_prof.cpp`; symbol check | Host Clang 22, existing BlueWake build defines/includes | Opt-in `DOL_AURORA_BATCH_STAGING_COPIES=1` batches only vertex/uniform/index/storage copies before render submission. Default path and texture order remain unchanged. Compilation passed with one pre-existing shader-type warning; no CTest tests were registered in the available build |
| M3-T04, M3-T05 | BlueWake `a555c3aed7f4`; `render-power-batch/report.txt` | Same executable, A=batching on, B=off; 12 measured launches each, interleaved A/B/B/A | Steam Deck, headless gamescope 1280x800, five states, 4x, Smooth Motion one step | A vs B: overall +0.03%; Outset −1.23%, lava −0.69%, Hyrule −0.93%, butterfly +0.34%, shipwreck −0.37%. Median reported GPU work 26.627 to 22.188 ms. All 24 runs loaded all scenes; checkpoints identical. Data posted; no disposition action taken |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | The 1100 MHz cap confirms shared APU power/governor behavior: it restores the 4x game-thread clock from 3.06 to 3.34 GHz and reverses Outset from −6.3 % to +6.4 %. If GPU execution time were the limit, lowering its ceiling would have made 4x worse, not better |
| 2026-11-05 | Stop collecting render-scale data. The capped profile's IPC difference means the exact +6.4 % reversal should not be decomposed further; the mechanism is sufficiently established to move to Aurora source work |
| 2026-11-05 | Corrected attribution: at Outset, 4x plus Smooth Motion raises GPU power enough to lower the busy game thread's clock by 7.0 %, accounting for the 6.3 % slowdown. This is CPU/GPU power competition, not proof that the GPU ran out of frame time |
| 2026-11-05 | GPU-side reductions remain the fix direction because they free APU power for the bottlenecked CPU. First targets: Smooth Motion's in-between frame and staging copies; test batching the copies before the first pass |
| 2026-11-05 | Source inspection narrows copy batching: Smooth Motion does not replay ordinary staging copies, and texture uploads must retain operation order. #53 batches only non-texture buffer arenas behind an opt-in flag |
| 2026-11-05 | Use BlueWake #563 for the RecompCore pin, exported patch, build, and Deck qualification. Do not open a separate BlueWake PR |
| 2026-11-05 | Accept #53 only if it is ahead in every one of at least six interleaved Deck launches, checkpoints match, and pictures are identical with Smooth Motion off; otherwise revert it in #563 and record the result |
| 2026-11-05 | Staging-copy result recorded without removing code: +0.03% overall; four of five scenes slower, Outset −1.23%; reported GPU work 26.627 to 22.188 ms; checkpoints identical. Await explicit direction on disposition |
| 2026-11-05 | Superseded: cause for the render scale was attributed to Smooth Motion halving the GPU's time budget. `[gpu-prof]` busy cannot be read as elapsed frame occupancy and exceeds frame time with Smooth Motion off |
| 2026-11-05 | Superseded: H4 (GPU full) led based on `[gpu-prof]` busy per presented frame. The effective game-thread clock instead identifies shared APU power as the mechanism |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck while the GPU reports idle time |
