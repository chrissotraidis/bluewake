# PROGRESS — Native extensions for Outset's character work

Status: Planned  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Why skinning declines

- [ ] M0-T01: Decline-reason and model-size counters in `native_skin.c` (diagnostic define).
- [ ] M0-T02: Outset, Game Mode, one minute: declines by reason and size; accepted calls by size.

### M1: The native switches (can run beside M0)

- [ ] M1-T01: `--native-entries`, trained on its own code; Game Mode A/B (≥ 6 interleaved launches) against #503's stack; checkpoints.
- [ ] M1-T02: `--native-set4`, the same.
- [ ] M1-T03: Both together if each qualifies; retain or record.

### M2: Resumable skinning (depends: M0 confirms timing)

- [ ] M2-T01: Envelope-by-envelope native that stops at the translation's boundary with its exact state.
- [ ] M2-T02: Differential test at every stop point.

### M3: Qualify (depends: M2)

- [ ] M3-T01: Game Mode A/B and checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| Planning | #503 `measure-503/m1-native-counts.txt`, `m1-functions.txt` (private) | Native counters and the game-function mapping from #503's Outset session | Game Mode, Outset, stage select off | `calcWeightEnvelopeMtx`: native 1,080,359 runs, 114,745 declined (≈ 10%), yet 2.33 M cycles/frame still translated against 0.83 M in the native. Translated costs of functions whose natives are switched off: Maya `calcTransform` 0.59, key-frame 0.46 + 0.44, `JMAEulerToQuat` 0.40 M cycles/frame |
