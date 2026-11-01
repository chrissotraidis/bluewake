# PROGRESS — Train the optimization profile on Outset

Status: M2-T01 done (`--train-state`); M2-T02 builds next  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Coverage of the current profile

- [x] M0-T01: Dump the current build's profile counts and rank functions.
- [x] M0-T02: Join them with #503's Outset samples; report the share of Outset's hot chunks the profile treats as hot. Chunk-level coverage is good; skinning, animation and lighting chunks are under-weighted.

### M1: Train the player's direct-call path (depends: M0)

- [ ] ~~M1-T01/T02~~: Dropped. The tour warps with `BLUEWAKE_TEST_WARP` (fast load), which does not make the stage-select logic live; the profile shows the inline direct-call path was trained (its host-query fallback count is 0).

### M2: Outset play in the tour (depends: M0)

- [x] M2-T01: Add Outset play with the villagers on screen. Done as `--train-state FILE` (Linux builder): a private save state played as one more stop, from its own retrace, with the tour's three runs; mounted read-only in the CI container and part of the training fingerprint. The default training is unchanged, so `TRAINING_VERSION` stays. Three builder tests.
- [ ] M2-T02: Two full builds of one commit (#503's stack with this change): the control with the default training, the candidate adding `--train-state` with #503's worst-angle Outset state. Then the M0 coverage report and the A/B.

### M3: Decide (depends: M2)

- [ ] M3-T01: Game Mode (≥ 6 interleaved launches) and render_bench (checkpoints, five scenes) for the M2 build.
- [ ] M3-T02: Retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01/T02 | Profile `composite-68112e95` from the `main` `6b429fe` build (training fingerprint `a3a61f3f`, 425 executed functions); #503's Outset samples (`measure-503/m1-samples.txt`, control `f4bda45`); local `measure-529/` (private) | `llvm-profdata show --detailed-summary` and per-function `--counts`; chunk totals joined with Outset sample weights | Linux workstation | Hot threshold (99% cutoff): block count ≥ 298,388. **Every chunk Outset runs was trained; 95.8% of Outset's chunk time is in chunks with hot blocks; all 50 of Outset's hottest chunks are in training's top 100 (39 in its top 50).** Under-weighted against Outset: `func_802ED6E0` (Outset 5.3%, training 1.6%: `calcWeightEnvelopeMtx`, key-frame interpolation), `func_8018D6E0__mod_widescreen` (2.0% vs 0.3%) and `func_801916E0` (1.7% vs 0.5%: `dScnKy_env` lighting), `func_802FD6E0` (1.1% vs 0.1%: JMA quaternions), `func_800A16E0` (1.3% vs 0.2%: `dBgS_Acch` ground checks). The direct-call check's host-query fallback has count 0 in every chunk: the tour ran the player's inline path |
| M2-T02 | Local `bench/529` (`bf5dec3`: #503 stack + `--train-state`), two full builds: control (default training) and candidate (`--train-state` with the worst-angle Outset state once); profiles compared offline | Per-chunk training share against #503's Outset samples (`measure-529/`, private) | Linux workstation | **One state stop barely moves the profile**: about 1,500 of ≈ 38,000 trained retraces. `func_802ED6E0` 1.60% → 1.64% (Outset 5.26%); overlap with Outset's distribution 67.6% → 68.3%. No Deck A/B for this build (the profiles are too close to tell apart). Next: the same state 8 times (deterministic playbacks, ≈ 8× the weight) |
