# TECH_PLAN — Train the optimization profile on Outset

Status: M2-T01 done; builds next  
Source: BlueWake `main` at `6b429fea9e60`  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Where training comes from

The Linux builder builds an instrumented module, plays it headless, merges the counts into `composite.profdata`, and compiles the real module with `-fprofile-instr-use` ([`build.py`](../../../scripts/linux/build.py): `TRAINING_RUN`, `TRAINING_TOUR`, `training_tour`, `tour_groups`). The opening plays once and saves a state at `TOUR_START - 100`; the tour stops then play from that state in parallel playbacks, each a warp plus three scripted runs (forward, then right and left turns).

| Fact | Consequence for Outset |
|---|---|
| One tour stop at `sea:44:0`, 1,500 retraces | Outset in normal play is a small share of all training samples |
| Stops are reached with `BLUEWAKE_TEST_WARP` (fast load, `fast_load.c`) | The stage-select logic stays off (only `BLUEWAKE_WARP` makes it live), so the tour trains the player's inline direct-call path; confirmed in M0 |
| Module functions the profile lacks are compiled as cold | #503 saw this cost 2.9% when two renamed helpers lost their profile entries |

## Approach

1. **Coverage report (no build change).** From an existing build's `composite.profdata` (`llvm-profdata show --all-functions`), rank functions by count. Join that with #503's Outset samples (`measure-503/m1-functions.txt`, private), and report what share of Outset's hot chunks the profile already treats as hot.
2. ~~**Warp without the stage select.**~~ Dropped after M0: the tour's warps never made the stage-select logic live.
3. **Add an Outset play stop.** Two options:
   - a second `sea:44` stop whose scripted runs pass the villagers (M0: skinning, animation and lighting are what is under-weighted), the same `TOUR_STOP` length;
   - or, when a private state is available, a state-based stop like #503's worst-angle view (`BLUEWAKE_LOAD_STATE`).

   Measure the M2 effect on the under-weighted chunks with the M0 report (their training share should rise toward Outset's).

   The fingerprint (`TRAINING_VERSION`) changes so old profiles are not reused.
4. **Measure:** a full build of each, the same commit, only the training different.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `scripts/linux/build.py` | `--train-state FILE` (repeatable): `state_retrace`, `state_stop`, an extra playback per state in `train`, the state's hash in `training_fingerprint`, a read-only container mount, and argument checks | Outset play as players see it (the busy view the tour does not reach), without changing the default training |
| `scripts/linux/build.py` `TRAINING_MODS`, `state_stop_mods` | State stops play with the mods playback's mods (widescreen, Better Wind Waker) when the build has mods | Without them, the state trained plain variants the player's mods replace (M2-T02) |
| `tests/test_linux_ci_container.py` | The stop's moves and end from a synthetic state; non-states refused; states mounted read-only; stops' mods follow the build | Builder logic covered in CI (audit workflow) without game data |
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
| ~~M1~~ | — | Dropped after M0 |
| M2 | M0 | Outset play stop with the villagers, with tests |
| M3 | M2 | Full build and A/B; retain or record no-go |

## Gaps

- A state-based training stop needs a private state at build time; the builder must keep working without it.
- Each candidate is a full build with its own training (≈ 40 min); `--reuse` cannot be used because the profile is what changes.

## References

- [#503 Outset game-thread follow-up](https://github.com/chrissotraidis/bluewake/pull/503) — profile, findings, the stage-select cost
- [#524 stage-select direct-call cost](https://github.com/chrissotraidis/bluewake/pull/524)
- [Performance overview](../../performance/Overview.md)
