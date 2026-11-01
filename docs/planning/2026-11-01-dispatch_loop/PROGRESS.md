# PROGRESS — Dispatch-loop edges in Outset

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Count

- [ ] M0-T01: Opt-in census at loop entry that does not change which path runs; unit test.
- [ ] M0-T02: Outset per-frame table by kind and reason (Game Mode, worst-angle state).

### M1: Attribute (depends: M0)

- [ ] M1-T01: Map the top sites to game functions and to the transform that skipped them.

### M2: Bound (depends: M1)

- [ ] M2-T01: Ceiling build for the largest group; Game Mode A/B against the #503 stack; go/no-go.

### M3: Candidate (depends: M2)

- [ ] M3-T01: Exact change with focused tests.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
