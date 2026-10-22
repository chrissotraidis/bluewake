# PROGRESS — Guest alias-resolution evaluation

Status: Planned

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Parent M2](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Establish whether resolver work is worth changing

Architecture: [guest-to-host address resolution](../../architecture/GuestState.md#guest-to-host-address-resolution).

- [ ] M0-T01: Capture a normal-exit Deck alias-cost trace for the fixed route.
- [ ] M0-T02: Capture an aligned Deck CPU profile and identify resolver call context.
- [ ] M0-T03: Record select/reject decision with trace and profile evidence.

### M1: Implement the selected RecompCore guard (depends: M0 select)

Owns the RecompCore runtime PR; BlueWake source remains unchanged.

- [ ] M1-T01: Specify the selected guard and fallback contract.
- [ ] M1-T02: Add focused resolver cases for pointer, offset, precedence, bounds, and alias lifecycle.
- [ ] M1-T03: Build and validate the RecompCore change on supported host platforms.

### M2: Integrate an accepted runtime revision (depends: M1)

- [ ] M2-T01: Pin/export the accepted RecompCore revision in BlueWake.
- [ ] M2-T02: Build a matched Deck control and candidate with provenance.
- [ ] M2-T03: Confirm checkpoint and activation evidence before performance comparison.

### M3: Qualify and decide (depends: M2)

- [ ] M3-T01: Run five interleaved Deck control/candidate pairs.
- [ ] M3-T02: Re-profile the route and complete the player-facing island check.
- [ ] M3-T03: Record retain or reject decision and update parent M2.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
