# PROGRESS — Train the optimization profile on Outset

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Coverage of the current profile

- [ ] M0-T01: Dump the current build's profile counts and rank functions.
- [ ] M0-T02: Join them with #503's Outset samples; report the share of Outset's hot chunks the profile treats as hot.

### M1: Train the player's direct-call path (depends: M0)

- [ ] M1-T01: Release the stage-select logic after a tour warp arrives; builder and host tests.
- [ ] M1-T02: Full build and A/B against the control.

### M2: Outset play in the tour (depends: M0)

- [ ] M2-T01: Add an Outset play stop; bump `TRAINING_VERSION`; builder tests.
- [ ] M2-T02: Full build and A/B against the control.

### M3: Combine and decide (depends: M1, M2)

- [ ] M3-T01: Full build with both; Game Mode (≥ 6 interleaved launches) and render_bench (checkpoints, five scenes).
- [ ] M3-T02: Retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
