# PROGRESS — Outset game-thread follow-up

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Freeze the #501 Outset control

Architecture: [performance workflow](../../performance/Overview.md#performance-workflow).

- [ ] M0-T01: Record BlueWake/RecompCore/AppImage/module hashes, build flags, PGO identity and Deck settings.
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
