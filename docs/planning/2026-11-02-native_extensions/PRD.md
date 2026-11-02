# PRD — Native extensions for Outset's character work

Status: Draft (planning)  
Source: BlueWake `main` at `ee991d312`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503) and [#530](https://github.com/chrissotraidis/bluewake/pull/530)  
Scope: the game-thread cost of skinning and animating Outset's characters, through natives that already exist in the tree. Excluded: collision (a no-go of this shape in [#436](https://github.com/chrissotraidis/bluewake/pull/436)), the dispatch loop (#530), GX ([#531](https://github.com/chrissotraidis/bluewake/pull/531)).

## Problem

Outset runs at about 26 game FPS on the Steam Deck against 30, with the game thread busy and no single hot spot ([#503](https://github.com/chrissotraidis/bluewake/pull/503)). Its biggest game cost is its characters: skinning and animating the villagers.

Natives for most of that work already exist in `cmake/composite/`, certified against the translation. Yet in #503's Outset profile, the translated versions still carry the cost:

| Function | Translated cost in Outset (M cycles/frame) | Native | State in the measured builds |
|---|---:|---|---|
| `J3DModel::calcWeightEnvelopeMtx` (skinning) | **2.33**, the biggest single game function | `native_skin.c` | **On**, but declines the expensive calls (below) |
| `J3DMtxCalcMaya::calcTransform` (joint matrices) | 0.59 | `native_mtxcalc.c` | Off (`--native-entries`) |
| `J3DGetKeyFrameInterpolationS`, `J3DAnmTransformKey::calcTransform` (key frames) | 0.46 + 0.44 | `native_anim.c` | Off (`--native-set4`) |
| `JMAEulerToQuat` (rotations) | 0.40 | `native_rot.c` | Off (`--native-entries`) |
| `cM_atan2s` and other leaves | ≈ 0.3 | `native_calc.c` and others | Off (`--native-entries`) |

**Why skinning declines.** The skinning native runs only when the whole function fits inside the current turn's cycle budget and before the next device deadline (`native_skin.c`). Its cost grows with the model: 58 cycles, plus 23 per envelope, plus 60 per joint. #530 found that turns average about 8,600 guest cycles, because the DSP steps every 12,600. So a big model rarely fits, and those are exactly the calls that decline:
- In #503's Outset session, the native ran 1,080,359 times and declined 114,745 times (≈ 10% of calls).
- The translated function still cost 2.33 M cycles per frame, against 0.83 M for the native itself.

The natives cover the many cheap calls; the expensive ones stay translated.

**Why the other sets are off.** `--native-entries` (#361) and `--native-set4` (#366, #381) were linked behind switches, off by default, and never measured on the Deck.

## Outcomes

- Know why each skinning call declines, by reason and by model size, before changing anything.
- A skinning native that also covers big models, stopping exactly where the translation would stop for a deadline, so it stays exact.
- A Deck measurement of `--native-entries` and `--native-set4` in Outset, and a decision whether to turn them on.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Outset game FPS | ≈ 26.3 (#503's stack, worst-angle state, 16:9 + widescreen) | Ahead of the control in every interleaved Game Mode launch, at least six; no minimum size | Game Mode launches alternating control and candidate |
| Skinning declines | ≈ 10% of calls, the expensive ones | Most big models run natively | Decline-reason counters |
| Guest behaviour | Control checkpoints | Identical in every scene | render_bench, or the differential native tests |
| Native tests | `native_skin_test.c`, `native_anim_test.c`, `native_entries_test.c` | Extended for each change; in CI where they need no game data | CI |

**Training:** a candidate that changes chunk code or native routing is trained on its own code. #530 measured a reused profile as −4.9% on the same change.

## Decisions

- **Measure first:** decline reasons and the switch flags' effect before any native change.
- **Exact or nothing:** a native stops, declines or resumes exactly where the translation would; anything else is out.
- **Acceptance** follows #503's rule: consistent across interleaved launches, exact, tested; size is not a bar.
