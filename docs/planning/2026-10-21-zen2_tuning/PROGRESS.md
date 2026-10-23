# PROGRESS — Steam Deck Zen 2 tuning decision

Status: Complete; rejected / parked
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Parent M6](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Measure the isolated flag

- [x] M0-T01: Build matched module-only tuned and untuned variants from source `56a9537`.
- [x] M0-T02: Run four interleaved headless, unpaced rounds across five Steam Deck scenes.

### M1: Record the decision (depends: M0)

- [x] M1-T01: Compare scene medians and reject the option when no gain exceeds noise.
- [x] M1-T02: Preserve `x86-64-v3` and the portable default without adding a builder flag.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | Private matched builds from `56a9537` | Identical build options; candidate adds module-only `-mtune=znver2` | Steam Deck; `-march=x86-64-v3` retained | Valid isolated comparison pair. |
| M0-T02 | [PR #438 measurement report](https://github.com/chrissotraidis/bluewake/pull/438#issuecomment-6357949110) | Four interleaved headless, unpaced rounds per build across five fixed scenes | Steam Deck | Per-scene medians differ by -0.7% to +0.4%. |
| M1-T01 | Same report | Compare scene medians | Same workload | Mean difference: +0.0%; no measurable gain. |
| M1-T02 | This decision record | Reject candidate | Portable Linux build remains `x86-64-v3` | No `--mtune` interface or default change. |

Raw artifacts are private. This result is scoped to the tested flag and
workload; it does not reject unrelated scheduling or host-side work.
