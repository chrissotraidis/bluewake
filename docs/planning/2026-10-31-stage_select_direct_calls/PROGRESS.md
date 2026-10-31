# PROGRESS — Stage-select direct-call cost

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Freeze the host-observation contract

Architecture: [translated dispatch](../../architecture/GameThread.md#translated-dispatch).

- [x] M0-T01: Record the discovery measurement, profile shares, build identity and affected stage-select/warp modes.
- [ ] M0-T02: Enumerate every stage-select observed address and prove how the generated and runtime watch paths cover it.
- [ ] M0-T03: Run current `v3` verification with stage selection and warp routes; record unsafe/conservative counts and A/A spread.
- [ ] M0-T04: Select and document the versioned fast-path representation and older-module fallback.

### M1: Keep the inline answer safely (depends: M0)

Parallel with M2; owns `main.c` and the direct-call ABI files; integrates at M3.

- [ ] M1-T01: Add the versioned module handshake without changing `v2` or `v3` layouts.
- [ ] M1-T02: Supply the complete stage-select watch contract and enable the inline answer when supported.
- [ ] M1-T03: Preserve conservative fallback for old or incomplete modules and diagnostic modes.

### M2: Lock coverage and measurement guidance (depends: M0)

Parallel with M1; owns tests and documentation; integrates at M3.

- [ ] M2-T01: Add component cases for watched addresses, mirrors, runtime states and compatibility fallback.
- [ ] M2-T02: Add source-contract coverage preventing a new stage-select hook from bypassing the watch contract.
- [ ] M2-T03: Document stage-select/warp state in benchmark evidence, using #522 after it lands.

### M3: Integrate and qualify (depends: M1, M2)

- [ ] M3-T01: Run focused tests, Linux/Windows CI, diff and repository audits.
- [ ] M3-T02: Exercise menu entry/return, warp arrival/music and file selection on the candidate.
- [ ] M3-T03: Run Game Mode A/A and on/off windows plus the rendered warp tour; require matching checkpoints.
- [ ] M3-T04: Record retain/reject findings and update #503 with the corrected benchmark interpretation.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | [#503 discovery record](https://github.com/chrissotraidis/bluewake/pull/503), control build `f4bda45` | `scripts/gamemode_measure.py`; 30 s `perf record -F 2999` on the game thread | Steam Deck Game Mode, Outset, scale 3, Smooth Motion on; `BLUEWAKE_STAGE_SELECT_TOGGLE` on/off | 23.6 → 26.1 FPS (+10.9%); host skip 4.5%, observer 1.7%; clock unchanged |

