# PROGRESS — Why higher render settings slow the game when the GPU has room

Status: Planning; nothing measured yet
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Size the slowdown
- [ ] M1-T01: render_bench, render scale 3 vs 4, same build, six launches each: FPS, game thread busy %, GPU busy (mean, p95, max)

### M2: Attribute it (depends: M1)
- [ ] M2-T01: `--cpu-profile` at 3 vs 4: per-thread cycles and instructions per frame, hottest functions on the game thread and the render worker
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

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck while the GPU reports idle time |
