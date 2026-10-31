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

- [x] M1-T01: Capture game-thread-only flat/call-graph profiles and cycles/instructions per frame.
- [x] M1-T02: Map hot chunk PCs to exact decomp symbols and source ranges.
- [x] M1-T03: Separate attempted, accepted and declined native routing costs without changing default execution.
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
| M1-T01 | Same control, stage select off (the default); Deck `~/m1-outset/perf-nostage.data`, local `measure-503/m1-profile-nostage.txt` (private) | `perf record -F 2999 -t <game thread>` 30 s | Game Mode, Outset, scale 3, Smooth Motion on | Default baseline: 86.7% of the game thread in the module, 11.5% in the host. Top: `func_802456E0` 5.9%, `native_game_math_try` 3.6%, `func_802ED6E0` 3.4%, `func_800A56E0` 3.4%, `aurora_backend_gx_write` 2.7%, `func_803256E0` 2.4%, `func_802D56E0` 2.3%, `native_math_try` 2.0%, `native_vec_try` 1.9%, `chassis_dispatch` 1.8%; flat below |
| M1-T02 | Same control, stage select off; Deck `~/m1-outset/perf-nostage.data`; the top 100 chunks (94% of chunk samples) recompiled from the build's own Ninja commands with `-gline-tables-only`, code identical to the shipped objects for the 84 that built (16 module/mod chunks not mapped); local `measure-503/m1-functions.txt` (private) | Sample offset → chunk source line → `label_`/`// PC` guest address → zeldaret/tww GZLE01 `symbols.txt` | As above | No dominant game function. Top: `calcWeightEnvelopeMtx` (J3D skinning) 2.1% of module samples, `GroundCrossGrpRp` 1.5%, `GroundCrossRp` 0.9%, `WallCorrectGrpRp` 0.8%, `WallCorrect` 0.8%, `wave_move` 0.8%. By system: background collision ≈ 22 M cycles per frame, J3D animation/skinning ≈ 14 M, J3D/GX draw setup ≈ 9 M |
| M1-T02 | Same control and settings, open ocean (steady 30 FPS, game thread 90% busy); Deck `~/m1-outset/perf-calm.data`; local `measure-503/m1-outset-vs-calm.txt` (private) | Same 30 s profile and mapping; `gamemode_measure.py` 2 windows (`calm`) | Game Mode, ocean view, scale 3, Smooth Motion on, Efficient soft effects on | **Outset ≈ 131 M cycles per frame against ≈ 103 M on the ocean.** The 28 M is spread across systems, not one function (M cycles per frame, Outset/ocean): collision 22.0/15.3, J3D animation/skinning 14.3/10.2, native helpers 10.8/8.2, other game code 18.1/15.7, memory/gather-pipe/paired-single helpers 5.8/4.1, direct-call overhead 6.2/4.8, draw setup 9.4/8.1; sea, sky and particles equal. Only actor-to-actor collision (`cCcD_DivideArea`, `dCcMassS_Mng::Chk`, cylinder checks) is near zero on the ocean (≈ 1.7 M in Outset). Outset is more of everything: the island's actors and mesh |
| M1-T04 | Same control, stage select off; `perf-nostage.data` resolved against the build's own module and host binary (local `measure-503/m1-samples.txt`, private); call-site counts from the 84 line-table chunk objects | Runtime and host symbols in M cycles per Outset game frame (≈ 129 M total; 30 FPS needs ≈ 113 M) | As above | **Shared runtime costs ≈ 44 M per frame, removable part ≈ 8–12 M.** Native routines 10.8 (their own work); direct-call check 5.6 (`bw_direct_call_ready`, 32,604 call sites, 252 bytes, kept out of line); GX command stream 5.1 (`aurora_backend_gx_write` 3.5, `bw_gather_pipe` 1.4); dispatch loop 4.3 (`chassis_dispatch` 2.3, `host_chassis_requires_full` 1.0, `bw_call_translated` 0.6, `bw_find_chunk` 0.3); native actor search 2.9 (`bw_search_word` 1.7, one `get_ram_ptr` per word); memory slow paths and aliases 2.2; paired-single quantized load/store 2.1 (generic GQR path, ≈ 3,500 call sites); FP helpers 1.2 (`fctiw`, `frsp`, `frsqrte`) |
| M1-T03 | Same session's exit reports (`[native-*]` lines; local `measure-503/m1-native-counts.txt`) | Accepted/declined counts over the session (22,417 retraces, ≈ 8,500 game frames) | As above | ≈ 304 M native runs and ≈ 10.7 M declines: **≈ 37,000 routing decisions per game frame**. The readiness callback (`host_can_skip_observation`) is only 0.33% of the game thread, so the `*_try` functions' 7.5% is mostly the native routines' own (inlined) work. Declines are dominated by `key-s` (79% declined) and `transform-simple` (80%), ≈ 1,060 per frame: a minor candidate. The 13% gap to 30 FPS has to come from the translated chunks (M1-T02) |
