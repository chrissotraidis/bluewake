# TECH_PLAN — Train the optimization profile on Outset

Status: Planning  
Source: BlueWake `main` at `6b429fea9e60`  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Where training comes from

The Linux builder builds an instrumented module, plays it headless, merges the counts into `composite.profdata`, and compiles the real module with `-fprofile-instr-use` ([`build.py`](../../../scripts/linux/build.py): `TRAINING_RUN`, `TRAINING_TOUR`, `training_tour`, `tour_groups`). The opening plays once and saves a state at `TOUR_START - 100`; the tour stops then play from that state in parallel playbacks, each a warp plus three scripted runs (forward, then right and left turns).

| Fact | Consequence for Outset |
|---|---|
| One tour stop at `sea:44:0`, 1,500 retraces | Outset in normal play is a small share of all training samples |
| Stops are reached with `BLUEWAKE_TEST_WARP` | The stage-select logic stays live, which turns off the inline direct-call path (`main.c`, `want_fast … && !bluewake_stage_select_live`), so the tour trains the slow path |
| Module functions the profile lacks are compiled as cold | #503 saw this cost 2.9% when two renamed helpers lost their profile entries |

## Approach

1. **Coverage report (no build change).** From an existing build's `composite.profdata` (`llvm-profdata show --all-functions`), rank functions by count. Join that with #503's Outset samples (`measure-503/m1-functions.txt`, private), and report what share of Outset's hot chunks the profile already treats as hot.
2. **Warp without the stage select.** Let tour warps skip `bluewake_stage_select_live` once the warp has arrived, so the rest of the stop runs the player's path. This may overlap with [#524](https://github.com/chrissotraidis/bluewake/pull/524), which plans the same release for benchmarks.
3. **Add an Outset play stop.** Two options:
   - a second `sea:44` stop with runs through the village, the same `TOUR_STOP` length;
   - or, when a private state is available, a state-based stop like #503's worst-angle view (`BLUEWAKE_LOAD_STATE`).

   The fingerprint (`TRAINING_VERSION`) changes so old profiles are not reused.
4. **Measure:** a full build of each, the same commit, only the training different.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `scripts/linux/build.py` | `TRAINING_TOUR` gains an Outset play stop; `TRAINING_VERSION` bump | More Outset play in the profile |
| `scripts/linux/build.py` / host warp code | Tour warps release the stage-select logic after arrival | Train the inline direct-call path players use |
| `tests/` (builder tests) | The tour's generated warps and moves for the new stop; fingerprint changes with the tour | Builder logic covered in CI without game data |
| `scripts/profile_coverage.py` (new, optional) | Ranks profile counts against a sample list | Repeatable step 1 |

## Verification

| Layer | Required evidence |
|---|---|
| Builder | Unit tests for the tour, moves and fingerprint |
| Coverage | Before/after share of Outset's hot chunks the profile marks hot |
| Exactness | render_bench checkpoints identical to the control in all five scenes |
| Performance | Game Mode: ahead of the control in every interleaved Outset launch (≥ 6); render_bench five scenes, none consistently slower |

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Coverage report on the current profile |
| M1 | M0 | Warp release for training, with tests |
| M2 | M0 | Outset play stop, with tests |
| M3 | M1, M2 | Full builds and A/B; retain or record no-go |

M1 and M2 are measured separately first, then together.

## Gaps

- A state-based training stop needs a private state at build time; the builder must keep working without it.
- Each candidate is a full build with its own training (≈ 40 min); `--reuse` cannot be used because the profile is what changes.

## References

- [#503 Outset game-thread follow-up](https://github.com/chrissotraidis/bluewake/pull/503) — profile, findings, the stage-select cost
- [#524 stage-select direct-call cost](https://github.com/chrissotraidis/bluewake/pull/524)
- [Performance overview](../../performance/Overview.md)
