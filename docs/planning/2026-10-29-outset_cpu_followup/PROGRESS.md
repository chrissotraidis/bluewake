# PROGRESS — Outset game-thread follow-up

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Freeze the #501 Outset control

Architecture: [performance workflow](../../performance/Overview.md#performance-workflow).

- [ ] M0-T01: Record BlueWake/RecompCore/AppImage/module hashes, build flags, PGO identity and Deck settings.
- [ ] M0-T02: Load the fixed Outset state and verify the expected stage, room, checkpoints and game-thread bottleneck.
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

