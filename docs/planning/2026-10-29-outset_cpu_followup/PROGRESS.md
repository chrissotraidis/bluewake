# PROGRESS — Outset game-thread follow-up

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Freeze the #501 Outset control

Architecture: [performance workflow](../../performance/Overview.md#performance-workflow).

- [x] M0-T01: Record BlueWake/RecompCore/AppImage/module hashes, build flags, PGO identity and Deck settings.
- [x] M0-T02: Load the fixed Outset state and verify the expected stage, room, checkpoints and game-thread bottleneck.
- [ ] M0-T03: Run at least five interleaved A/A pairs and record FPS/cycles noise.

### M1: Attribute the flat game-thread profile (depends: M0)

PC mapping and opt-in routing counters may run in parallel; both integrate into one ranked cost table.

- [ ] M1-T01: Capture game-thread-only flat/call-graph profiles and cycles/instructions per frame.
- [ ] M1-T02: Map hot chunk PCs to exact decomp symbols and source ranges.
- [ ] M1-T03: Separate attempted, accepted and declined native routing costs without changing default execution.
- [ ] M1-T04: Select one removable cost only if it exceeds measurement noise.

### M2: Implement one isolated candidate (depends: M1)

- [ ] M2-T01: Add the smallest source change matching the selected cost and preserve fallback behavior.
- [ ] M2-T02: Add or extend the surface's focused correctness test.
- [ ] M2-T03: Build matched control/candidate artifacts and record identities.

### M3: Qualify and decide (depends: M2)

- [ ] M3-T01: Run at least five rendered A/B/B/A pairs at Outset with matching checkpoints.
- [ ] M3-T02: Run the full five-scene rendered tour and focused CI tests.
- [ ] M3-T03: Perform the Outset walk/camera player test and record retain/reject reasoning.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T02 | #501 AppImage `6fe3d1a7868b` (`BlueWake-alias-lookup`, the Game Mode shortcut); `scripts/gamemode_measure.py` from [#522](https://github.com/chrissotraidis/bluewake/pull/522) | `gamemode_measure.py sample` (2 × 15 s per label), plus the session log's `[fps-dip]` lines | Steam Deck Game Mode, Outset (`stage=sea room=44`), render scale 3, camera still | Verified: Outset, game-thread bound (`cause=game-thread`, game thread 98% busy). Game thread at 3.445–3.457 GHz with the APU at 11.4–13.8 W, below its 15 W cap: **not power-limited** (unlike the Lava Room, bluewake #504) |
| M0-T03 | Same build and session (local `measure-503/outset/windows.jsonl`, private) | Game Mode A/A: labels `aa1`, `aa2` and `smooth-on-again` (Smooth Motion on, 6 windows); `smooth-off` between them (2 windows) | As above | Partial (Game Mode, not yet the ≥ 5 rendered A/A pairs). Smooth Motion on: IPC 1.646–1.659 (spread 0.013), clock spread 0.01 GHz, 23.8–23.9 game FPS. **Smooth Motion off: IPC 1.735–1.742 (+5.2%, beyond the spread), clock unchanged, 25.1 game FPS (+5%)**: its helper threads cost the game thread through shared cache and memory, not clock. Cycles per frame ≈ 145 M (on) and ≈ 138 M (off); 30 FPS at 3.45 GHz needs ≤ 115 M, **17–20% less** |
| M0-T01 | Control: local `bench/503-control` = #504 `473c78d` + `main` 0.10.0 (`f4bda45`); RecompCore #49 `a4dbaaa`; AppImage `e237759ccb05`, module `bc86d0de`, profile `35c4f7bc` (clang 18.1.3, x86-64-v3, `--gather-alias-cache`) | `build_script control-503 bench/503-control … --gather-alias-cache`; BuilderProvenance.json | Steam Deck Game Mode: render scale 3, Smooth Motion on (never pause), Efficient soft effects on | Control frozen. Later candidates rebuild with `build_script --reuse last` and share its profile |
| M0-T03 | Same control, Game Mode, Outset; local `measure-503/control/` (private) | `gamemode_measure.py`: A/A `ctl-aa1`, `ctl-aa2`; then Efficient soft effects off | As above | A/A: IPC 1.648–1.661, clock 3.447–3.449 GHz, 23.4–23.9 game FPS, every in-between frame delivered. Efficient soft effects off: no change (IPC 1.662–1.664, 23.5–23.7 FPS) |
| M1-T01 | Same control; Deck `~/m1-outset/perf.data` (private) | `perf record -F 2999 -t <game thread>` 30 s (89,518 samples); `perf stat` 10 s | As above, stage select on | ≈ 147 M cycles per frame, IPC 1.65. Top: `func_802456E0` 5.6%, **`host_direct_can_skip_stage_select` 4.5%**, `func_800A56E0` 3.5%, `bluewake_native_game_math_try` 3.4%, `func_802ED6E0` 3.1%, `aurora_backend_gx_write` 2.4%, **`bluewake_stage_select_observes_body` 1.7%**, `native_math_try` 1.8%, `native_vec_try` 1.7%, `bw_search_word` 1.2%, `host_chassis_requires_full` 0.75% |
| M1-T04 | Same control, relaunched with `BLUEWAKE_STAGE_SELECT_TOGGLE=0` (environment over the player's setting); `perf-nostage.data` (private) | `gamemode_measure.py` 4 windows (`ctl-nostage`) against the A/A; 30 s profile | As above, stage select off | **Game FPS 23.6 → 26.1 (+10.9%, A/A spread 0.5)**; in-between frames 23.5 → 26.0 per second; IPC +0.8%; clock unchanged. With the stage select on, the module's inline direct-call skip (v3) is disabled (`main.c` `want_fast … && !bluewake_stage_select_live`), so every direct guest call asks the host; off, those host functions leave the profile |
