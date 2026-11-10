# PROGRESS — The game thread at busy views

Status: Planned — follows #586
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Attribute the game thread's time
- [x] M1-T00: Found on #586 (its M4-T23): `8-busiest` (~1,950 draws) runs 26.53 in Game Mode with #586's switches; the game thread is 98.6 % busy, ~89 % in translated code, flat (largest function 1.5 %)
- [ ] M1-T01: Profile `8-busiest` with names (huge pages off, or the copy's addresses mapped back)
- [ ] M1-T02: Group the time by game system (actors, models and joints, draw lists, collision)
- [ ] M1-T03: Per-frame counts of the game's work at `8-busiest` against the bench's lighter Outset view

### M2: Cheaper game-thread work
- [ ] M2-T01: First candidate from M1, opt-in, exact (guest checkpoints identical)
- [ ] M2-T02: Game Mode, 6 launches each way, every launch ahead, `8-busiest` and #586's seven scenes

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T00 | `render-prof-busy/report.md`, `raw/runs/02-A/8-busiest/perf-*` | render_bench `--cpu-profile`, one profiled run, headless, paced | Build `9dde6aa9d`, `8-busiest`, 3x + HD + Smooth Motion, #586's switches | Game thread 98.6 % busy; 124 M cycles a frame; IPC 1.52; ~89 % translated code |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: plan the game thread at busy views separately from the GPU side (#591); #503, the earlier Outset game-thread work, is merged |
