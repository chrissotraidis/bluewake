# PROGRESS — Native extensions for Outset's character work

Status: M0 done; M2 exact; first Deck A/B +0.8% (partial runs on); empty-model fix rebuilt for a second A/B; M1 blocked on porting  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Why skinning declines

- [x] M0-T01: Decline-reason and model-size counters in `native_skin.c` (diagnostic define). `BLUEWAKE_SKIN_CENSUS`: each of the ten decline sites by name (state, guest data, model, overlapping stores, mix counts, joint count, joint tables, joint matrices, turn budget, deadline), and the accepted and timing-declined calls by estimated cycles (< 1k … ≥ 16k), printed through the native's existing exit report. Without the define, `SKIN_DECLINE(n)` is the original `goto decline`.
- [x] M0-T02: Outset, Game Mode, one minute: declines by reason and size; accepted calls by size. **All timing declines are the turn budget** (28,118 of 28,120); big models are refused: 50% at 4–8k cycles, 91% at 8–16k, 100% at ≥ 16k. M2 confirmed as the route.

### M1: The native switches (can run beside M0)

- [ ] M1-T01: `--native-entries`, trained on its own code; Game Mode A/B (≥ 6 interleaved launches) against #503's stack; checkpoints. **Blocked:** the build fails in `native-leaf-gen` (`__ieee754_fmod`: unhandled `bw_write32_ato`, a write form the lean-memory translation emits that the generator predates). Needs the generator updated first.
- [ ] M1-T02: `--native-set4`, the same. **Blocked:** it builds, but links its sources without hooks (`cmake/composite/CMakeLists.txt`: "Hooks stay out until hashes are recorded for this tree"), so nothing calls it and the build behaves like the control. Needs its entry points certified against this tree first.
- [ ] M1-T03: Both together if each qualifies; retain or record.

### M2: Resumable skinning (depends: M0 confirms timing)

- [x] M2-T01: The native runs the envelopes that fit the turn and hands over at the loop's test (`9645667`): code 1 → the translation's epilogue (0x802EE834), code 2 → the loop head (0x802EE6F0). Partial runs are opt-in, `BLUEWAKE_NATIVE_SKIN_PARTIAL=1`.
- [x] M2-T02: Differential test with turn budgets of 100–3,100 cycles: 15,355 cases identical to the translation (2,112 of them partial), 4,645 declined unchanged.
- [x] M2-T04: Local speed check, `native_skin_test MODULE --bench` (x86-64-v3, control module): native + epilogue **1.33–1.43× faster** than the translated function at 4–200 envelopes, three runs ([numbers](TECH_PLAN.md#is-the-native-faster-than-the-translation)).
- [x] M2-T05: Off by default on builds without hardware FMA (x86 below `x86-64-v3`), where the bench measured no gain; `BLUEWAKE_NATIVE_SKIN=1` forces it on. Differential test unchanged: 15,355 identical (2,112 partial).
- [ ] M2-T03: Full build trained on its own code (`t541-m2`, `bench/541-m2` = control + `9645667`, partial runs on during training).

### M3: Qualify (depends: M2)

- [x] M3-T01 (first build): +0.8% with partial runs on, ahead in 6/6; but the build declines empty models, and with partial runs off it is 0.5% behind the control. Fix and re-measure: M3-T03.
- [ ] M3-T03: Empty models handed over natively (prologue, then the epilogue at 0x802EE834). **Done in code** (`ce42138f3`): differential test 17,021 identical (2,112 partial), declines down from 4,645 to 2,979. Rebuild `t541-m2b` (own training) in progress; then the same A/B. Expected: up to ≈ +1.3% (the +0.8% plus the −0.5% gap, part of which may be the hand-back itself).
- [ ] M3-T01 (fixed build): Game Mode A/B (≥ 6 interleaved launches) against the control `a64af0e`, with `BLUEWAKE_NATIVE_SKIN_PARTIAL=1`; the same build with it off shows the hand-over alone.
- [ ] M3-T02: Checkpoints (render_bench, run by the user); retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M3-T01 (first build) | Control `a64af0e` (AppImage `d4e36d76…`) and M2 `412c76c` (`bench/541-m2`, own training, `bf0bb021…`); local `measure-541/m2/` (private) | 6 rounds, rotating order: control, M2 partial on, M2 partial off; 3 × 15 s windows per launch; native counters at exit | Steam Deck LCD, Game Mode, worst-angle Outset state, 16:9, widescreen | Game FPS: **control 26.09, M2 on 26.29 (+0.8%, ahead 6/6), M2 off 25.95 (−0.5%, behind 5/6)**; on vs off +1.3% (6/6). Counters per launch: control ≈ 255k native / 25k declined, 1.40 M joints; M2 on 30k full + 22k partial / 235k declined, **2.77 M joints**; M2 off 27k / 255k declined, 1.40 M joints. **Fault found:** the M2 native declines models with no envelopes (≈ 230k calls per launch, which the control's native ran), the likely cause of the −0.5% |
| M2-T02 | `9645667`; `bluewake_native_skin_test` against the control module `a64af0e` | `native_skin_test gGZLE01_recomp.so 20000`; a partial or full native run is followed by the translation from 0x802EE6F0 or 0x802EE834 and compared with the translation from the entry: every CPU byte (the cycle suffix too) and all of MEM1 | Linux workstation; synthetic models, 1–24 envelopes, budgets 100–3,100 cycles on one case in five | **15,355 identical (2,112 partial), 4,645 declined unchanged.** Also found: the M0 native's epilogue left the cycle suffix at 2, but on the #503 stack the inline `_restgpr` leaves 0, so the committed M0 code failed this test against that module (case 0). Handing over before the epilogue removes that dependence on the build's transforms |
| M1 | Local `bench/541-ctl` (#503 stack, `a64af0e`); full builds with `--native-entries` and `--native-set4` | `build_script` with each option, own training | Linux workstation | **Neither set can be measured as it stands.** `--native-entries`: build fails in `native-leaf-gen` on `__ieee754_fmod` (`unhandled ctx: bw_write32_ato(&bw_ool, ctx, 0x8032EC1Cu, …)`). `--native-set4`: builds (`t541-set4`), but its hooks are not inserted until hashes are recorded for this tree, so its natives are never called |
| M0 | Control `a64af0e` (AppImage `t541-ctl/a64af0e941c4`, own training) and the census build `78a3382` (`--reuse` over it, `BLUEWAKE_SKIN_CENSUS` on locally) | Census symbols confirmed in the module | Linux workstation | Ready for the one-minute Outset run on the Deck |
| M0-T02 | Census build `78a3382` (#503 stack + `BLUEWAKE_SKIN_CENSUS` on, `--reuse` over the control); local `measure-541/census.log` (private) | One Game Mode launch, ≈ 90 s, then SIGTERM; the native's exit report | Worst-angle Outset state, 16:9, widescreen mod, 27 game FPS | native 282,718, declined 28,120 (9%). **Reasons: turn budget 28,118, CPU state 2, every data check 0, deadline 0.** By estimated cost (accepted / budget-declined): < 1k 258,829 / 1,660; 1–2k 7,827 / 929; 4–8k 15,445 / 15,201; 8–16k 617 / 5,950; ≥ 16k 0 / 4,378. Weighted by cost, the declined calls carry roughly half of the function's guest cycles: the big models, refused because what is left of a turn (≈ 8,600 cycles on average, set by the DSP step) is too short |
| Planning | #503 `measure-503/m1-native-counts.txt`, `m1-functions.txt` (private) | Native counters and the game-function mapping from #503's Outset session | Game Mode, Outset, stage select off | `calcWeightEnvelopeMtx`: native 1,080,359 runs, 114,745 declined (≈ 10%), yet 2.33 M cycles/frame still translated against 0.83 M in the native. Translated costs of functions whose natives are switched off: Maya `calcTransform` 0.59, key-frame 0.46 + 0.44, `JMAEulerToQuat` 0.40 M cycles/frame |
