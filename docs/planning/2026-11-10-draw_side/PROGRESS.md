# PROGRESS — The draw side of the game thread

Status: Phase 1 done — #451's generators fixed and round 7 re-certified (exact; no speed gain on the Deck, −0.19 %); rounds 4 and 5 priced below measurable (≤ 0.9 %). Phase 2 (drawing at the API level) needs the owner's decision
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
- [x] M1-T03c: Round 7 re-certified (`d770dd2c2`): the builder step now logs each uncertified entry's hash for this tree (`ba94975d5`; it runs before behaviour timing, so the final source's `--hashes` differ); the 19 that passed M1-T03b recorded. A build with `--native-entries` hooks **19/34** (the rest: rounds 2–3, whose comparison tests haven't run on this tree)
- [x] M1-T04: Round 5 (GX FIFO writers) priced before porting its test: its 32 GX SDK functions are **2.82 M cycles a frame at the busiest view, 2.2 % of the game thread** (`J3DGDSetFog` 0.31 M, `GXSetArray` 0.27, `GXLoadTexObjPreLoaded` 0.26, `GXCallDisplayList` 0.21, `GXSetCurrentMtx` 0.20, …). At the exact natives' 1.3–1.7× that saves 0.5–0.9 %, the size round 7 measured as noise. Not pursued
- [x] M1-T05: Round 4 (animation, actor collision, environment light) priced the same way: its functions are of the same small size in the busiest view's profile (key-frame interpolation 0.62 M, environment-light TEV setup ~0.6 M, actor collision ~0.8 M in total). Not pursued
- [x] M1-T06: Round 7 on the Deck (build `d770dd2c2`, headless, eight states, natives on vs `BLUEWAKE_NATIVE_ENTRIES=0`, 4 launches each way): **guest checkpoints identical in all 16 runs** (exact), but **−0.19 %** overall (25.33 vs 25.38); per scene −2.0 % (shipwreck) to +1.5 % (busiest), noise. The natives run a lot (one Outset launch: `sin` 1.03 M calls natively, `cM3d_CalcPla` 0.82 M, `JMAEulerToQuat` 0.45 M, `cSPolar::Val` 0.34 M, `cM_atan2s` 0.29 M; the two JASystem oscillators mostly leave their calls to the translation), but each saves little per call and together they are a small share of the frame on the Deck's cores. Elliott's +3.8 points were at Forest Haven on four slow E-cores. Not a gain here: `--native-entries` stays off by default; the generator fixes and the re-certification stay (exact, and they unblock the other sets)

### M2: The API-level option (only if M1 leaves a gap)
M1 leaves the whole gap: on the Deck, exact natives of the draw side's functions save under 1 % each round. What remains is structural.
- [ ] M2-T01: Draw-side profile with M1's sets on; pick the largest remaining path
- [ ] M2-T02: Prototype plan for that path, for the owner's decision

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T06 | `render-n7-sanity/report.md` | render_bench headless, paced, 8 states × 4 launches each way, interleaved | Build `d770dd2c2` (`--native-entries`, round 7 hooked), #586 not included | 25.33 vs 25.38 (−0.19 %); checkpoints identical |
| M1-T03b | `native_leaf_test` output (scratchpad `ntests/leaf-run2.txt`) | `tests/native_leaf_test.c` on Linux, 20,000 cases a function, bench 200,000 | Module of `2bbf00252`, natives from `d98e7f540`'s generators | 19/19, 0 mismatches |
| M0-T01 | #592 `scripts/guest_profile.py` / `guest_profile_systems.py` outputs | Named perf profiles, headless | Build `eaa85f0f1`, `6-play` and `8-busiest` | Draw side 19.9 → 26.0 M cycles a frame |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Stop phase 1 after round 7: on the Deck the exact natives save too little per call; rounds 4 and 5 priced at ≤ 0.9 %. Keep the generator fixes and round 7's certification (exact, off by default). Phase 2 waits for the owner |
| 2026-11-10 | Owner: plan the structural draw-side work separately from #592. Phase 1 is #451 (Elliott's existing, exact native sets, blocked on the current tree); drawing at the API level only if a gap remains |
