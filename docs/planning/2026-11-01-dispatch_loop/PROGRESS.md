# PROGRESS — Dispatch-loop edges in Outset

Status: M1 in progress: turn ends are the DSP step; bounding their cost  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Count

- [x] M0-T01: Opt-in census at loop entry that does not change which path runs; unit test. `cmake/composite/loop_census.h` (compile-time `BLUEWAKE_LOOP_CENSUS`): host entries, each boundary by target address and skipped/asked, and six exit reasons; report on unload. `tests/loop_census_test.c` in Windows and Linux CI; `composite_dispatch_loop_test` unchanged and passing.
- [x] M0-T02: Outset per-frame table by kind and reason (Game Mode, worst-angle state). ≈ 35,800 boundaries per game frame; 27% returns after `bl`, 26% calls, 23% returns after `bctrl`.

### M1: Attribute (depends: M0)

- [ ] M1-T01: Map the top sites to game functions and to the transform that skipped them. Done for returns: ≈ 14,500 per frame (76–89% of returns) follow calls that ran directly and were unwound by a turn end.
- [x] M1-T02: Which device deadlines end the turns (≈ 1,890 per frame), with an opt-in deadline census. The DSP's 12,600-cycle step ends 92%.
- [ ] M1-T03: The ≈ 9,400 calls per frame that arrive through the loop: untransformed sites, watched addresses or readiness false.

### M2: Bound (depends: M1)

- [ ] M2-T01: Ceiling build for the largest group; Game Mode A/B against the #503 stack; go/no-go.

### M3: Candidate (depends: M2)

- [ ] M3-T01: Exact change with focused tests.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T02 | Counting build `0662957` (#503 stack + census on, local `bench/530`; `--reuse` over the full build `0884d97`); Deck log `measure-530/census.log` (private) | One Game Mode launch, ≈ 60 s, then SIGTERM; census printed by the host at exit; targets classified by the instruction before them (from the chunks' `// ADDR: insn` comments) and by decomp function starts | Game Mode, worst-angle Outset state, 16:9, widescreen mod (mask `0x1`), 26 game FPS; ≈ 2,092 game frames | **≈ 35,800 boundaries per game frame** (≈ 120 cycles each against the ≈ 4.3 M); edge service skipped for 82%, asked ≈ 6,300 per frame. Host entries ≈ 1,890 per frame; exits: budget/exception 98%, edge service 2%, no misses or stalls. By what precedes the target: after `bl` (direct-call return) 9,620 (27%); function starts (calls) 9,364 (26%, edge service asked 49%); after `bctrl` 8,088 (23%); after `b` 2,526 (7%); after `mtmsr` 1,131 (3%); REL code 939 (3%). Top calls: `J3DShapeMtxConcatView::loadMtxConcatView` 1,751, `cTgIt_JudgeFilter` 1,018, `cTgIt_MethodCall` 585, `fpcSch_JudgeByID` 557, `OSDisableInterrupts` 513, `OSRestoreInterrupts` 510 |
| M1-T01 | Same census log; the counting build's generated chunks (`composite-src`, private): which return sites belong to calls the builder runs directly (`ctx->lr = RET; … if (bw_direct_call_ready…` for `bl`, `bw_call_translated(…) && ctx->pc == RET` for `bctrl`) | Static match of each counted target against the transformed sources; turn length from the cycle cap and the census's host entries | As M0-T02 | **≈ 40% of the loop's boundaries are returns after a turn ended mid-call.** Of the 9,620 returns after `bl` per frame, 7,347 (76%) are at sites whose call ran directly; of the 8,088 after `bctrl`, 7,161 (89%) are. A direct call keeps the caller's C frame, so its return can only reach the loop if that frame was unwound: a turn ended inside the callee, and every pending return then comes back through the loop (≈ 7–8 per turn end). Turns end ≈ 1,890 times per frame, averaging ≈ 8,600 guest cycles against a cap of 16,384 (`BLUEWAKE_CYCLE_CAP`), so device deadlines, not the cap, decide them. At ≈ 120 cycles per boundary, these returns cost ≈ 1.7 M cycles per frame |
| M1-T02 | #503 stack (`cand-503`, `274b6290a1e2`) with `BLUEWAKE_DEADLINE_CENSUS=1` (existing host census, runtime); Deck log `measure-530/deadline.log` (private) | One Game Mode launch, ≈ 85 s | Worst-angle Outset state, 16:9, widescreen mod, HLE DSP | **The DSP cadence ends 92% of turns**: 3,873,605 deadline calls; DSP 3,580,635 (92.4%), audio DMA 284,463 (7.3%), VI 5,040, decrementer 2,095, clamp 1,372, cap 0. The host steps the DSP every `g_dsp_update_rate` = 12,600 guest cycles (`BLUEWAKE_DSP_RATE`), in HLE mode too; the September census found the same for LLE (94.5%). So each DSP step ends a turn, unwinds the call stack, and sends the pending returns round the loop |
