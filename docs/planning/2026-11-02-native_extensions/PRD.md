# PRD — Native extensions for Outset's character work

Status: Done. Skinning hand-over kept for exactness; partial runs measured and dropped; the other native sets need porting first  
Source: BlueWake `main` at `ee991d312`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503) and [#530](https://github.com/chrissotraidis/bluewake/pull/530)  
[Technical plan](TECH_PLAN.md) · [Tracker and evidence](PROGRESS.md)

## Problem

Outset runs at about 26 game FPS on the Steam Deck against the game's 30, with the game thread nearly always busy ([#503](https://github.com/chrissotraidis/bluewake/pull/503)). Its largest single game function is skinning, `J3DModel::calcWeightEnvelopeMtx`: 2.33 M cycles per frame still translated, although a native for it is on.

Other natives for character work exist in the tree but are off: `--native-entries` (joint matrices, rotations) and `--native-set4` (key frames), together about 2 M cycles per Outset frame translated.

## What we tried

1. **Find out why skinning declines** (M0): count every decline by reason and by model size, in Outset on the Deck.
2. **Turn on the other native sets** (M1): build and measure `--native-entries` and `--native-set4`.
3. **Let skinning cover big models** (M2, "partial runs"): when a model does not fit the turn, run the envelopes that do fit natively and hand the rest to the translation at the loop's head.

## What we found

| Finding | Evidence |
|---|---|
| **The turn budget refuses the big models.** All timing declines are the turn's cycle budget, none the deadline. Calls costing 8k+ guest cycles decline 91–100% of the time. They are about 9% of calls but roughly half of skinning's cycles | Census on the Deck, one Outset launch |
| **The other native sets cannot run as they stand.** `--native-entries` fails in its generator on the current translation; `--native-set4` builds but has no hooks for this tree | Two full builds |
| **The native is about 1.3–1.7× faster per call than the translation** | Local bench on the game module, at the module's `-march` |
| **The existing native was not exact on the #503 stack.** It replays the function's ending itself and leaves the cycle suffix the plain translation leaves; the #503 stack's inline `_restgpr` leaves another value. No effect was seen in play | Differential test fails on its first case |
| **Partial runs work but gain nothing measurable.** They double the joints skinned natively and are exact, but Game Mode measured +0.05% (3 of 6 rounds) and render_bench +0.55%, inside noise | Deck A/B and render_bench, after a wrapper fix |
| **A wrapper bug made skipped work look like a gain.** The hook's wrapper turned the partial result into "done", so partial runs skipped the rest of the model; that read as +0.8–1.2% until checkpoints exposed it | Every-retrace checkpoints; replay through the module's hook |

## Decision

- **Keep the hand-over at the function's ending:** the native runs the prologue and the whole loop, or declines, and the translation runs its own ending. That makes the native exact for every build, whatever its transforms do to the ending, at no measurable cost.
- **Drop partial runs.** They were the most complex and riskiest code, for a gain inside noise. The extra joints only save the native-vs-translation difference, about 0.1–0.3% of an Outset frame.
- **Keep the tools:** the skinning census, the local speed bench, the replay of cases captured on the Deck, and tests that call the native the way the game does.
- **Leave the other native sets for porting work**, outside this PR.

## Long-term impact

- **Exactness is robust.** Future build transforms that reshape the skinning function's ending no longer break the native; the differential test passes against current builds again.
- **No speed change.** Skinning was 2.1% of the game thread and is now mostly native; what remains for it is small. Splitting it across threads would be exact but is worth about +0.5–1%, against a 13% gap to 30 FPS. It is not pursued.
- **A reusable method for natives:** census, then a local bench, then a Deck A/B, then checkpoints. [#542](https://github.com/chrissotraidis/bluewake/pull/542) (ground collision) starts from it.
- **A testing rule:** test natives through the same wrapper and hook the game calls. Calling the native directly hid a bug that skipped work and looked like a gain.
- **Other hardware:** the native is exact everywhere. It is off by default where the build has no hardware fused multiply-add (x86 below `x86-64-v3`), because there it is no faster than the translation.

## Constraints that applied

| Rule | How it was applied |
|---|---|
| Exact or nothing | Every candidate compared byte for byte with the translation; render_bench checkpoints identical |
| Consistent gains only, no minimum size | ≥ 6 interleaved Game Mode launches per candidate |
| Each candidate trained on its own code | Every measured build had its own optimization profile |
| Private game data stays off GitHub | Modules, states, captures and logs kept local; only hashes and numbers here |
