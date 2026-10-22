# PROGRESS — Native dispatch and numeric-path evaluation

Status: Planned

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Parent M3](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Attribute and select one M3 surface

Architecture: [native routines](../../architecture/NativeRoutines.md) · [compiled component tests](../../architecture/Testing.md#compiled-component-tests).

- [ ] M0-T01: Build or identify one matched Deck control and record its provenance.
- [ ] M0-T02: Capture the fixed-route profile and normal-exit dispatch/native reports.
- [ ] M0-T03: Annotate one candidate; record a serial selection or reject M3.

### M1: Implement one selected serial reduction (depends: M0 select)

- [ ] M1-T01: Specify the selected fallback and state-equivalence contract.
- [ ] M1-T02: Make one isolated code change; do not change worker defaults.
- [ ] M1-T03: Add or update only the focused source/component test required by that change.

### M2: Qualify the selected change (depends: M1)

- [ ] M2-T01: Run the focused test and relevant existing work-pool/guard tests.
- [ ] M2-T02: Verify checkpoints and activation on a matched Deck candidate.
- [ ] M2-T03: Run five interleaved pairs and the island check; record retain or reject.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
