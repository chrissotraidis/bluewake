# TECH_PLAN — Native extensions for Outset's character work

Status: M0 ready to measure; M1 blocked on porting both native sets  
Source: BlueWake `main` at `ee991d312`  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## What exists

| Native | Source | Switch | Covers |
|---|---|---|---|
| Skinning | [`native_skin.c`](../../../cmake/composite/native_skin.c), `scripts/windows/native_skin.py` | `--native-skin` (on in the Deck builds) | `J3DModel::calcWeightEnvelopeMtx` (0x802EE67C–0x802EE874) |
| Joint matrices | [`native_mtxcalc.c`](../../../cmake/composite/native_mtxcalc.c) | `--native-entries` (off; **does not build**: its generator predates the lean-memory write forms) | `J3DMtxCalcBasic/Softimage/Maya::calcTransform` |
| Rotations, leaves | `native_rot.c`, `native_calc.c`, `native_libm.c`, `native_geom.c`, … | `--native-entries` (off) | `JMAEulerToQuat`, `cM_atan2s`, … |
| Key frames | [`native_anim.c`](../../../cmake/composite/native_anim.c) | `--native-set4` (off; **no hooks**: linked but never called until its entry hashes are recorded for this tree) | `J3DGetKeyFrameInterpolation(S)`, `J3DAnmTransformKey::calcTransform` |

## Why skinning declines: the budget and deadline checks

`bluewake_native_skin` declines, changing nothing, unless the result is certain. Most conditions are about data: plain RAM, finite floats, no overlapping stores. Two are about time:

```c
/* The translation stops at no block (the turn's budget) and prepays every
 * block with no refund (the deadline, further than any block's suffix). */
if (cpu->downcount - (s64)cycles <= -cpu->cycle_budget) goto decline;
if (cpu->cycle_deadline_budget > 0 &&
    (cpu->cycle_deadline_budget < 60 || cpu->cycle_deadline_budget + cpu->downcount < (s64)cycles))
    goto decline;
```

`cycles` = 58 + Σ(23 + 60 × joints per envelope). #530's deadline census showed turns ending at the DSP step 92% of the time, averaging about 8,600 cycles. A model with, say, 40 envelopes and 80 joints needs about 5,800 cycles, which only fits early in a turn. Bigger models almost never fit, so the native covers small models and the translation keeps the expensive ones.

## Approach

1. **M0, decline reasons (counting build).** Count each decline by reason (data checks, turn budget, deadline) and by model size (envelopes, joints), plus the accepted calls by size. Outset, Game Mode, one minute. This confirms or rejects the timing explanation before any design work.
2. **M1, the switches (found not to be switches).** Both need porting first: `--native-entries`' generator must handle the lean-memory write forms, and `--native-set4`'s entry points must be certified against this tree. Originally: Full builds with `--native-entries`, with `--native-set4`, and with both, each trained on its own code. Six-launch Game Mode A/B against #503's stack; checkpoints. Retain each set that is consistent and exact.
3. **M2, resumable skinning (if M0 confirms timing).** Let the native run envelope by envelope and stop at the same envelope-loop block boundary where the translation would stop for the budget or deadline. It hands back the translated loop's exact state there (loop counter, pointers, the FP registers in use, `pc` at the loop head), and the translation finishes the call. Big models then run mostly natively.
4. **M3, qualify.** The differential test (`native_skin_test.c`, every register and byte against the translation) extended to stops at every envelope; Game Mode A/B; checkpoints.

## Changes (planned)

| File | Change | Why |
|---|---|---|
| [`cmake/composite/native_skin.c`](../../../cmake/composite/native_skin.c) | `BLUEWAKE_SKIN_CENSUS`: decline counts by reason (the ten decline sites) and cycle bins for accepted, budget-declined and deadline-declined calls, in the exit report | M0: which model sizes the timing checks refuse |
| `scripts/linux/build.py` (or the local build helper) | Builds with `--native-entries` / `--native-set4` for the A/B | M1; default unchanged unless retained |
| `cmake/composite/native_skin.c` | Resumable envelope loop: stop at the translation's boundary with its exact state | M2 |
| `tests/native_skin_test.c` | Partial runs compared with the translation at each possible stop | M2 exactness |

## Verification

| Layer | Required evidence |
|---|---|
| M0 counts | Decline reasons sum to all declines; two runs agree |
| Switches | Native tests pass; Game Mode ≥ 6 interleaved launches; checkpoints identical |
| Resumable native | Differential test: every register and byte equal to the translation for every stop point; Game Mode A/B; checkpoints identical |

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Skinning decline reasons and sizes in Outset |
| M1 | — (can run beside M0) | `--native-entries` / `--native-set4` measured; retain or record |
| M2 | M0 (timing confirmed) | Resumable skinning native with tests |
| M3 | M2 | Qualified on the Deck; retain or record no-go |

## Gaps

- The resumable stop must match the translation's block boundaries in the envelope loop exactly; the translated loop is generated code (private), so the stop points are derived from it, as `native_skin.py` already certifies the body.
- `--native-entries` and `--native-set4` need `--direct-calls`, `--gather-pipe` and `--native-vec`, all on in the Deck builds.

## References

- [#503](https://github.com/chrissotraidis/bluewake/pull/503): Outset profile, game functions, the `native-skin` counts
- [#530](https://github.com/chrissotraidis/bluewake/pull/530): turn ends at the DSP step; turn length
- [#436](https://github.com/chrissotraidis/bluewake/pull/436): a native that was slower than the translation (fixed checks on small calls)
- [#361](https://github.com/chrissotraidis/bluewake/pull/361), [#366](https://github.com/chrissotraidis/bluewake/pull/366), [#381](https://github.com/chrissotraidis/bluewake/pull/381): the native sets behind their switches
- [Native routines architecture](../../architecture/NativeRoutines.md)
