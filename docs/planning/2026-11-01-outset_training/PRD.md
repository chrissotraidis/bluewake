# PRD — Train the optimization profile on Outset

Status: Draft (planning)  
Source: BlueWake `main` at `6b429fea9e60`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503)  
Scope: what the composite module's profile-guided optimization (PGO) training plays, and whether training on Outset makes Outset faster. Excluded: game logic, runtime code, Smooth Motion.

## Problem

Outset runs at about 26.8 game FPS on the Steam Deck after #503, against the game's 30. Its game-thread time is flat: about 85 M cycles per frame in translated game code, with no function above about 2.3 M ([#503 findings](https://github.com/chrissotraidis/bluewake/pull/503)). A flat profile like this is where code layout matters most. Hot code packed together and branches laid out the way they usually go cost less instruction-cache and branch-predictor work across all functions at once.

That layout comes from the PGO training run in the Linux builder ([`build.py`](../../../scripts/linux/build.py), `TRAINING_TOUR`). What the training plays today:

| Part | What it plays | Outset content |
|---|---|---|
| Opening | The new-game opening, about 23,000 retraces | Outset, early-game and story-driven |
| Tour | 10 stops of 1,500 retraces each, reached by warp: Windfall, Dragon Roost, its cavern, Forest Haven, the Forbidden Woods, the Tower of the Gods, the Forsaken Fortress, the sea near it, Hyrule Castle, Outset | One stop at Outset's arrival point |

Two gaps follow:

- **Outset in play is a small share of training.** The heaviest view (#503's worst-angle state) is never played.
- **Warps keep the stage-select logic live.** That turns off the module's inline direct-call path (#503 measured that state as ≈ 11% slower), so the tour trains a code path players never take.

## Outcomes

- Know how much of Outset's hot code the current profile already treats as hot, before changing anything.
- A training change that adds Outset play as players see it, and does not keep the stage-select logic live during the tour.
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
