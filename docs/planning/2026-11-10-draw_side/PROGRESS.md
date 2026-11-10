# PROGRESS — The draw side of the game thread

Status: In progress — #451's generator blockers fixed; round 7 passes its comparison test on today's module (19/19, 0 mismatches); re-certifying so the builder hooks it
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Measured on #592
- [x] M0-T01: Busiest view against the light one, by game system (#592 M1-T03): draw side 19.9 → 26.0 M cycles a frame, flat (no function above 0.95 M)
- [x] M0-T02: Single natives priced (#592 M3-T02): the key-frame native's coverage worth ~0.15–0.25 %; no single draw-side function is measurable

### M1: Unblock the native sets (#451)
- [x] M1-T01: Reproduced on the current tree (prepared source of a default build, `f89aeea05`): the `_ato` forms are handled by now and the libm group generates, but both generators stop on `inline_save_restore_gpr.py`'s frame form (`u8* const slots = bw_inline_gpr_frame(ctx, N, save)` with `write_be32`/`read_be32`): `cBgW::MakeBlckBnd` (leaf) and `J3DGDSetFog` (GX)
- [x] M1-T02: The frame form replayed on the run's memory (`2bbf00252`: the frame at r11 − 4·(32 − N), a save's stores as `gx_stp32`, a restore's loads as `gx_ld32`), and the inline form's host gates (`d98e7f540`: `gx_silent` at the routine and the return, as `bw_direct_call_ready`; the comparison test caught the missing gate in `MakeBlckBnd` with the host busy). Both generators run to the end: round 7 (libm, bgblk, rot, calc, geom, jas) and round 5 (GX FIFO writers). The "not a block leader" errors #451 saw don't occur on this tree
- [x] M1-T03: A build with `--native-entries --native-gx --native-set4` hooked **0 of 34** entries: every recorded hash predates lean memory, lean deadlines and inline registers ("not the certified translation")
- [x] M1-T03b: Round 7's comparison test (`tests/native_leaf_test.c`, built on Linux in the build container against `libgxruntime.a`) against today's module: **all 19 natives, 20,000 random cases each, 0 mismatches** (`__ieee754_fmod`, `fmod`, `cM_rad2s`, `cM_rnd`, `cM_rndF`, `cM_rndFX`, `sin`, `cos`, `tan`, `cBgW::MakeBlckMinMax`, `cBgW::MakeBlckBnd`, `JMAEulerToQuat`, `cM_atan2s`, `cM3d_CalcPla`, `cSPolar::Val`, `dKyw_pntwind_get_info`, JASystem's `TOscillator::getOffset`, `TOscillator::calc`, `TChannel::updateEffectorParam`)
- [ ] M1-T03c: Re-certify round 7: the builder step's hash for each entry (it runs before behaviour timing, so the final source's `--hashes` differ; the step now logs them), recorded in `native_entries.py`; a build hooks them
- [ ] M1-T04: Round 5 (GX FIFO writers): build, measure (`8-busiest` + bench scenes, 6 launches each way, checkpoints identical), keep or remove
- [ ] M1-T05: Round 4 (animation, actor collision, environment light): the same
- [ ] M1-T06: Round 7 (libm, rotations, geometry, collision blocks, JASystem): the same

### M2: The API-level option (only if M1 leaves a gap)
- [ ] M2-T01: Draw-side profile with M1's sets on; pick the largest remaining path
- [ ] M2-T02: Prototype plan for that path, for the owner's decision

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T03b | `native_leaf_test` output (scratchpad `ntests/leaf-run2.txt`) | `tests/native_leaf_test.c` on Linux, 20,000 cases a function, bench 200,000 | Module of `2bbf00252`, natives from `d98e7f540`'s generators | 19/19, 0 mismatches |
| M0-T01 | #592 `scripts/guest_profile.py` / `guest_profile_systems.py` outputs | Named perf profiles, headless | Build `eaa85f0f1`, `6-play` and `8-busiest` | Draw side 19.9 → 26.0 M cycles a frame |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: plan the structural draw-side work separately from #592. Phase 1 is #451 (Elliott's existing, exact native sets, blocked on the current tree); drawing at the API level only if a gap remains |
