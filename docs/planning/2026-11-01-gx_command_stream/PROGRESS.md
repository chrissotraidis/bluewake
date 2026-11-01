# PROGRESS — Profile the GX command stream in Outset

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Stage-visible profile

- [ ] M0-T01: Builder option keeping gather stages visible to the profiler; builder test; build with host line tables.
- [ ] M0-T02: Outset per-frame table by stage (Game Mode, worst-angle state).

### M1: Counts (depends: M0)

- [ ] M1-T01: Opt-in word, command-kind and flush counters at the hand-off; parity test.
- [ ] M1-T02: Outset and a lighter scene; cycles per word and per command.

### M2: Bound (depends: M1)

- [ ] M2-T01: Ceiling for the largest stage; Game Mode A/B against the #503 stack; go/no-go.

### M3: Candidate (depends: M2)

- [ ] M3-T01: Exact change with focused tests.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints and pictures; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
