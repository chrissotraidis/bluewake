# TECH_PLAN — Steam Deck Zen 2 tuning decision

Status: Complete; option parked
[Product brief](PRD.md) · [Tracker](PROGRESS.md) · [Parent M6](https://github.com/chrissotraidis/bluewake/pull/421)

## Method

| Control | Candidate | Workload | Decision rule |
|---|---|---|---|
| Source `56a9537`; identical options | Same build, plus module-only `-mtune=znver2` | Steam Deck; five fixed scenes; headless and unpaced; four interleaved rounds per build | Implement only for a repeatable gain beyond observed variation |

`-march=x86-64-v3` remained unchanged. The test deliberately isolates the
scheduling hint, not a compiler-version or PGO comparison.

## Milestones

### M0: Measure the isolated flag

- M0-T01: Build matched module-only tuned and untuned variants from source `56a9537`.
- M0-T02: Run four interleaved headless, unpaced rounds across five Steam Deck scenes.

### M1: Record the decision

- M1-T01: Compare scene medians and reject the option when no gain exceeds noise.
- M1-T02: Preserve `x86-64-v3` and the portable default without adding a builder flag.

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Isolated, fixed-source Deck comparison |
| M1 | M0 | Recorded reject / park decision |

## Result and disposition

The mean median difference was +0.0% across the five scenes; individual
differences ranged from -0.7% to +0.4%. That is not a measurable improvement
for this workload. No source, builder, test, or documentation change is needed.

The earlier cross-commit observation is excluded from the decision because it
did not isolate the flag. Details and per-scene medians are retained in
[PR #438's measurement report](https://github.com/chrissotraidis/bluewake/pull/438#issuecomment-6357949110).

## Boundaries

- Do not add `--mtune`, alter `-march`, or change the portable default.
- Do not generalize this result to host-side scheduling, other compiler flags,
  other PGO recipes, or other workloads.
- Keep private performance artifacts on the test device.

## References

- [M6 tracker](https://github.com/chrissotraidis/bluewake/pull/421)
- [Steam Deck performance issue #215](https://github.com/chrissotraidis/bluewake/issues/215)
- [Linux builder](../../../scripts/linux/build.py)
