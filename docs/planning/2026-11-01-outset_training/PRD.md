# PRD — Train the optimization profile on Outset

Status: Closed: ≤ 0.4%, not consistent; planning record only (builder code removed)  
Source: BlueWake `main` at `6b429fea9e60`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503)  
Scope: what the composite module's profile-guided optimization (PGO) training plays, and whether training on Outset makes Outset faster. Excluded: game logic, runtime code, Smooth Motion.

## Outcome (2026-11-01): closed, planning record only

Training the optimization profile on Outset gives **≤ 0.4% game FPS, not consistent** in the player's setup (16:9, widescreen mod): the candidate led in 2 of 3 launch pairs. Without mods at 4:3 it led in 3 of 3, by +0.2%. That doesn't meet the acceptance rule, so the default training stays as it is.

**The experiment's builder code was removed from this PR.** It added `--train-state FILE` to the Linux builder: a private save state played as an extra training stop, with the mods playback's mods and the state in the training fingerprint, plus builder tests. It stays in this branch's history for anyone repeating the experiment:
- [`b56c94e`](https://github.com/irregularthanis/bluewake/commit/b56c94e9c) — `--train-state`
- [`88b258e`](https://github.com/irregularthanis/bluewake/commit/88b258e4b) — state stops use the build's mods

This PR merges only this planning record.

## Problem

Outset runs at about 26.8 game FPS on the Steam Deck after #503, against the game's 30. Its game-thread time is flat: about 85 M cycles per frame in translated game code, with no function above about 2.3 M ([#503 findings](https://github.com/chrissotraidis/bluewake/pull/503)). A flat profile like this is where code layout matters most. Hot code packed together and branches laid out the way they usually go cost less instruction-cache and branch-predictor work across all functions at once.

That layout comes from the PGO training run in the Linux builder ([`build.py`](../../../scripts/linux/build.py), `TRAINING_TOUR`). What the training plays today:

| Part | What it plays | Outset content |
|---|---|---|
| Opening | The new-game opening, about 23,000 retraces | Outset, early-game and story-driven |
| Tour | 10 stops of 1,500 retraces each, reached by warp: Windfall, Dragon Roost, its cavern, Forest Haven, the Forbidden Woods, the Tower of the Gods, the Forsaken Fortress, the sea near it, Hyrule Castle, Outset | One stop at Outset's arrival point |

What M0 found (2026-11-01):

- **Coverage is good at the chunk level.** Every chunk Outset runs was trained, and all 50 of its hottest chunks are in training's top 100. Nothing Outset needs is compiled cold.
- **Character work is under-weighted.** The chunks that hold skinning, animation and environment lighting take 2–6× more of Outset's time than of training's. Outset has more animated villagers on screen than the tour's stops.
- **The planned warp fix is not needed.** The tour warps with `BLUEWAKE_TEST_WARP` (fast load), which leaves the stage-select logic off; the profile shows the player's inline direct-call path was trained.

So the remaining lever is the balance inside those chunks: which branches and blocks the compiler favours. Expect a small effect.

## Outcomes

- Know how much of Outset's hot code the current profile already treats as hot, before changing anything.
- A training change that adds Outset play with its villagers on screen.
- A measured result in Outset and in the other benchmark scenes.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Outset game FPS | ≈ 26.8 (#503 on `main`, Game Mode, worst-angle state) | Ahead of the control in every interleaved launch, at least six; no minimum size | Game Mode launches alternating control and candidate |
| Other scenes | render_bench five-state results | No scene consistently slower | Rendered A/B/B/A |
| Guest behaviour | Control checkpoints | Identical; training changes layout, not logic | render_bench checkpoints |
| Build time | ≈ 40 min full build | Not much longer (training runs in parallel playbacks) | Builder timings |

Training inputs are private (disc, states), as today; only the builder logic and its tests are public.

## Decisions

- **Measure coverage first:** if the current profile already marks Outset's hot code as hot, the change is unlikely to help, and that is recorded as a finding.
- **Exact by construction:** PGO changes code layout and inlining choices, never what the game does. Checkpoints must still match.
- **Acceptance** follows #503's rule: consistent across interleaved launches, exact, tested; size is not a bar.
