# PROGRESS — The game thread at busy views

Status: On hold — the named profile at the busiest view is the one the earlier Outset game-thread work already covered (M1-T01); waiting for the owner's decision on the structural option
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Attribute the game thread's time
- [x] M1-T00: Found on #586 (its M4-T23): `8-busiest` (~1,950 draws) runs 26.53 in Game Mode with #586's switches; the game thread is 98.6 % busy, ~89 % in translated code, flat (largest function 1.5 %)
- [x] M1-T01: Profile `8-busiest` with names (huge pages off, build `eaa85f0f1`, headless, perf): 129 M cycles a game frame, IPC 1.47, game thread 98.6 % busy. Top: `func_802456E0` (ground collision, per the lava-room work) 3.2 %, `bluewake_native_game_math_try` 2.0 %, `func_800A56E0` 1.6 %, `func_803256E0` 1.3 %, `func_802ED6E0` 1.2 %, `chassis_dispatch` 1.2 %, `func_802D56E0` 1.2 %, `func_8009D6E0` 1.1 %, native math 1.05 %, native vec 1.04 %. **The same flat list the earlier Outset work (#503's planning, M1-T01) found** at its worst-angle Outset state: `func_802456E0` 5.6 %, `func_800A56E0` 3.5 %, native game math 3.4 %, `func_802ED6E0` 3.1 %, at 147 M cycles a frame then. That work (direct calls, the dispatch loop, code layout, natives, training, the alias resolver) has covered this profile; what's left is structural (natives compiled from the decompilation's source, docs/PERFORMANCE.md)
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
| 2026-11-10 | Owner asked whether this repeats earlier work: the named profile says yes (same hot list as #503's planning, 12 % fewer cycles a frame since). No profile-and-trim round; the open option is natives compiled from the decompilation's source |
| 2026-11-10 | Owner: plan the game thread at busy views separately from the GPU side (#591); #503, the earlier Outset game-thread work, is merged |
