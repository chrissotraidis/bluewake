# PRD — Stage-select cost: follow-up

Status: Draft; nothing measured yet
Builds on: [#524](https://github.com/chrissotraidis/bluewake/pull/524) (stage select's cost on the Deck −10.0 % → −3.6 %)
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: the stage select's remaining ≈ 3.6 %, and the checks #524 left open. Excluded: speed with the stage select off.

## Where #524 left off

#524 cut the slowdown from having the stage select enabled from −10.0 % to −3.6 % on the Steam Deck (the same build, on vs off). With it off, nothing changes. Its last profile puts the remaining cost at ≈ 8.8 M instructions a game frame at Outset:

| Where | Share | Can it go? |
|---|---:|---|
| The module: the five hook compares on every direct call | ≈ two-thirds | Mostly: this PR's slot filter skips them for 59 in 64 targets |
| The host: per-edge stage-select calls in the edge service | ≈ a quarter | Little: #524's attempt (`cde980cc3`) was within noise |
| The hooks running, small per-frame work | the rest | No: that's the stage select doing its job |

**Realistic floor: ≈ 0.5–1 %.** If the slot filter removes most of the module's part, the cost would fall to ≈ 1–1.5 %.

## Outcomes

1. Measure the slot filter; keep it only if it's ahead in every one of six interleaved launches with identical checkpoints.
2. Measure #524's true gain with the stage select on, with the same optimization profile on both sides (#524's ≈ +5.8 % compares two separately trained builds, which differ by ≈ 1.75 %).
3. Run the rendered warp tour and check the stage select's behaviour: menu open and return, a named warp, file selection, music.
4. Stop after the slot filter unless more than ≈ 1.5 % is left.
