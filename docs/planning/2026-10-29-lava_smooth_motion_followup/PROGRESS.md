# PROGRESS — Lava Room Smooth Motion follow-up

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Reproduce and freeze the Lava Room control

Architecture: [graphics synchronization](../../architecture/Graphics.md#synchronization-and-completion).

- [ ] M0-T01: Record #501/RecompCore/AppImage hashes, Deck mode, display rate, renderer and Smooth Motion settings.
- [ ] M0-T02: Capture at least five fixed windows each with automatic pacing, never-pause pacing and Smooth Motion off.
- [ ] M0-T03: Record game/display FPS, GPU busy/clock, present/slot/staging waits and current pacing logs.

### M1: Separate pacing, verdict and overload causes (depends: M0)

RecompCore counters/tests and BlueWake log parsing may proceed in parallel and integrate before M2.

- [ ] M1-T01: Add typed overload cause and wait-duration accounting without changing policy.
- [ ] M1-T02: Count delivered, budget-skipped and content-verdict-rejected frames separately.
- [ ] M1-T03: Extend frame-interpolation tests for cause classification, drop threshold and recovery.
- [ ] M1-T04: Select one cause only when the fixed-window totals reconcile.

### M2: Correct the proven cause (depends: M1)

- [ ] M2-T01: Change one pacing, presentation or GPU-resource behavior selected by M1.
- [ ] M2-T02: Preserve camera-cut/content rejection and genuine-overload game-speed protection.
- [ ] M2-T03: Produce matched control/candidate builds with explicit dependency identities.

### M3: Qualify and deliver (depends: M2)

- [ ] M3-T01: Run fixed-view Lava Room A/B with Smooth Motion on and forced-overload protection checks.
- [ ] M3-T02: Run the full rendered scene tour, pictures/checkpoints and Linux/Windows CI.
- [ ] M3-T03: Open the RecompCore change, pin/export it in BlueWake and record retain/reject evidence.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|

