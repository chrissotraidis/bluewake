# PROGRESS — Train the optimization profile on Outset

Status: Closed: ≤ 0.4%, not consistent; planning record only (builder code removed)  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Outcome (2026-11-01): closed, planning record only

Training the optimization profile on Outset gives **≤ 0.4% game FPS, not consistent** in the player's setup (16:9, widescreen mod): the candidate led in 2 of 3 launch pairs. Without mods at 4:3 it led in 3 of 3, by +0.2%. That doesn't meet the acceptance rule, so the default training stays as it is.

**The experiment's builder code was removed from this PR.** It added `--train-state FILE` to the Linux builder: a private save state played as an extra training stop, with the mods playback's mods and the state in the training fingerprint, plus builder tests. It stays in this branch's history for anyone repeating the experiment:
- [`b56c94e`](https://github.com/irregularthanis/bluewake/commit/b56c94e9c) — `--train-state`
- [`88b258e`](https://github.com/irregularthanis/bluewake/commit/88b258e4b) — state stops use the build's mods

This PR merges only this planning record.

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

- [x] M3-T01: Game Mode (≥ 6 interleaved launches) for the M2 build, at 4:3 and at 16:9 with the widescreen mod. render_bench skipped: no gain to qualify.
- [x] M3-T02: Retain or record no-go. **No-go**: the default training is unchanged and the builder code is removed; this plan merges as a record.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01/T02 | Profile `composite-68112e95` from the `main` `6b429fe` build (training fingerprint `a3a61f3f`, 425 executed functions); #503's Outset samples (`measure-503/m1-samples.txt`, control `f4bda45`); local `measure-529/` (private) | `llvm-profdata show --detailed-summary` and per-function `--counts`; chunk totals joined with Outset sample weights | Linux workstation | Hot threshold (99% cutoff): block count ≥ 298,388. **Every chunk Outset runs was trained; 95.8% of Outset's chunk time is in chunks with hot blocks; all 50 of Outset's hottest chunks are in training's top 100 (39 in its top 50).** Under-weighted against Outset: `func_802ED6E0` (Outset 5.3%, training 1.6%: `calcWeightEnvelopeMtx`, key-frame interpolation), `func_8018D6E0__mod_widescreen` (2.0% vs 0.3%) and `func_801916E0` (1.7% vs 0.5%: `dScnKy_env` lighting), `func_802FD6E0` (1.1% vs 0.1%: JMA quaternions), `func_800A16E0` (1.3% vs 0.2%: `dBgS_Acch` ground checks). The direct-call check's host-query fallback has count 0 in every chunk: the tour ran the player's inline path |
| M2-T02 | Local `bench/529` (`bf5dec3`: #503 stack + `--train-state`), two full builds: control (default training) and candidate (`--train-state` with the worst-angle Outset state once); profiles compared offline | Per-chunk training share against #503's Outset samples (`measure-529/`, private) | Linux workstation | **One state stop barely moves the profile**: about 1,500 of ≈ 38,000 trained retraces. `func_802ED6E0` 1.60% → 1.64% (Outset 5.26%); overlap with Outset's distribution 67.6% → 68.3%. No Deck A/B for this build (the profiles are too close to tell apart). Next: the same state 8 times (deterministic playbacks, ≈ 8× the weight) |
| M2-T02 | Same commit, candidate with the Outset state 8 times (deterministic playbacks, 12 tour playbacks at once, 15 min) | Same offline comparison | Linux workstation | **Partly moves toward Outset**: overlap 67.6% → 71.0%; `func_802ED6E0` 1.60% → 1.81%, `func_801916E0` 0.48% → 0.57%. But `func_8018D6E0__mod_widescreen` fell 0.28% → 0.19%: the state stops ran without mods, so they trained the plain variants of chunks the player's widescreen mod replaces. Fix `88b258e`: state stops play with the mods playback's mods. No Deck A/B for this build |
| M2-T02 | Candidate `512472d` (Outset state ×8 with the mods playback's mods; AppImage `8ba18a05ef37`): overlap with Outset 71.9% (control 67.6%), `func_8018D6E0__mod_widescreen` 0.28% → 0.94% | Same offline comparison | Linux workstation | The profile now reflects Outset with the widescreen mod; the skinning chunk stays under-weighted (1.81% vs Outset's 5.26%) |
| M3-T01 | Control (`bf5dec3`, `5022bd1301f8`) vs candidate (`8ba18a05ef37`); local `measure-529/ab/` (private) | `gamemode_measure.py` 3 × 15 s, six launches alternating | Game Mode, worst-angle Outset state, the Deck's settings (4:3, no mods) | 26.42 → 26.48 game FPS (+0.2%), ahead in 3/3 pairs; IPC 1.648 → 1.651 |
| M3-T01 | Same builds; local `measure-529/ab-16x9/` (private) | Same, with `BLUEWAKE_MODS=widescreen` and `BLUEWAKE_ASPECT=16:9` for the test runs (the player's setup; log mask `0x1`) | Game Mode, worst-angle Outset state | 26.24 → 26.36 (+0.4%), but **ahead in only 2 of 3 pairs** (26.33/26.43, 26.03/26.33, 26.37/26.30); the mean is lifted by one low control launch |
