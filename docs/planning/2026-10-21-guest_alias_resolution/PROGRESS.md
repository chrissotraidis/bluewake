# PROGRESS — Guest alias-resolution evaluation

Status: M0 evidence captured; M1 selected for investigation

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Parent M2](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Establish whether resolver work is worth changing

Architecture: [guest-to-host address resolution](../../architecture/GuestState.md#guest-to-host-address-resolution).

- [x] M0-T01: Capture a normal-exit Deck alias-cost trace for the fixed route.
- [x] M0-T02: Capture an aligned Deck CPU profile and identify resolver call context.
- [x] M0-T03: Select a guarded resolver investigation; this is not approval to change the default path.

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
| M0-T01 | Deck `m2-alias-census/run.log` | `BLUEWAKE_TRACE_ALIAS_COST=1`, headless normal exit | Fixed `quick-23874.bwstate`, diagnostic AppImage, 24,000 retraces | Two reports: 12,111,577 calls / 3,930,380 iterations / 357,627 hits; 749,168 calls / 7,120,317 iterations / 651,784 hits. |
| M0-T02 | Deck `m2-alias-perf-headless/perf.data`, `report.txt` | `profile_deck.sh --seconds 20` at 99 Hz | Same state and AppImage; headless, unpaced; 0 lost samples | `ppc_guest_alias_resolve` is 1.60% sampled self time in `gGZLE01_recomp.so`. Callers were not unwound by this capture. |
| M0-T03 | M0-T01 + M0-T02 | Compare trace work and sampled cost | Same fixed route | Select M1 investigation: repeated lookup work and measurable module CPU time justify one equivalence-guarded RecompCore proposal. |
