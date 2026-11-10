# PRD — The draw side of the game thread

Status: Planned — follows #592
Source base: BlueWake `main`
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Related: #592 (where this was measured), #591 (the GPU's share at busy views, on hold), #586, #451 (the blocked native generators), #361, #365, #366, #381 (the native sets it blocks)

## The problem

At the busiest Outset view (about 1,950 draws a frame), the game thread is 98.6 % busy and runs 26.5 game frames a second at 3x with the HD look and Smooth Motion. #592 named its time by game function:

| | Light Outset view | Busiest view |
|---|---|---|
| Game-thread cycles per game frame | 110.5 M | 128.8 M |
| **Draw side** (model animation and matrices, material, TEV and light setup, shapes, packets, GX state, particles, sky, sea) | 19.9 M | **26.0 M** |
| Host GX writes and runtime | 13.2 M | 14.1 M |

The draw side is the part that grows with what is on screen, and it is flat: no game function reaches 1 % of the game thread. A native for any one of them saves a fraction of a percent, under what a measurement can show (#592 M3). A measurable gain needs many of them at once, or a different way of drawing.

## Two phases

| Phase | What | Why first |
|---|---|---|
| **1. Unblock the existing native sets** (#451) | Teach Elliott Tate's native generators today's translation: the `_ato` memory-access form that lean deadlines added (#277), and the current prepared-block layout. Then build rounds 4, 5 and 7 and measure each as an opt-in | Already written and exact, and they cover the draw side as a set: the GX SDK's FIFO writers (round 5), the environment light and animation (round 4), libm, rotations and geometry (round 7). Elliott measured: game thread 4.5–7 % faster (round 5, with two other changes), 1–3 % and 6 % in Hyrule Castle's room (round 4), about 3.8 points at Forest Haven (round 7) |
| **2. Drawing at the GX/J3D API level** | The host takes over whole drawing paths (a model's material and shape setup, then its packets) instead of running the game's display-list building translated and parsing the result | The structural option (docs/PERFORMANCE.md horizon 3; Wind Waker HD's draw batching was worth 47–53 %). Months of work, so only where phase 1 leaves a gap |

## Goal

Phase 1: the native sets build on a current tree, and each one kept is ahead in every launch at the busiest view and the bench scenes, exact (guest checkpoints identical), opt-in. Phase 2: a plan and a measured prototype of one drawing path, decided after phase 1.

## Constraints

| Measure | Target |
|---|---|
| Exactness | Phase 1: guest checkpoints identical (the natives are replayed from the translation). Phase 2: plays the same, with the translation as the fallback |
| Measurement | Game Mode when Steam's prompt allows (#590), else headless; paced; `8-busiest` plus the bench scenes; 6 launches each way, every launch ahead |
| Opt-in | Each set behind a switch, off by default, until qualified |
