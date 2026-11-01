# PROGRESS — Dispatch-loop edges in Outset

Status: M0 done; M1 (attribution by call site) next  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Count

- [x] M0-T01: Opt-in census at loop entry that does not change which path runs; unit test. `cmake/composite/loop_census.h` (compile-time `BLUEWAKE_LOOP_CENSUS`): host entries, each boundary by target address and skipped/asked, and six exit reasons; report on unload. `tests/loop_census_test.c` in Windows and Linux CI; `composite_dispatch_loop_test` unchanged and passing.
- [x] M0-T02: Outset per-frame table by kind and reason (Game Mode, worst-angle state). ≈ 35,800 boundaries per game frame; 27% returns after `bl`, 26% calls, 23% returns after `bctrl`.

### M1: Attribute (depends: M0)

- [ ] M1-T01: Map the top sites to game functions and to the transform that skipped them. First: why ≈ 9,400 calls and their `bl` returns per frame still go round the loop when #234/#235 run such calls directly (sites left untransformed, watched addresses, or readiness false at the call).

### M2: Bound (depends: M1)

- [ ] M2-T01: Ceiling build for the largest group; Game Mode A/B against the #503 stack; go/no-go.

### M3: Candidate (depends: M2)

- [ ] M3-T01: Exact change with focused tests.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T02 | Counting build `0662957` (#503 stack + census on, local `bench/530`; `--reuse` over the full build `0884d97`); Deck log `measure-530/census.log` (private) | One Game Mode launch, ≈ 60 s, then SIGTERM; census printed by the host at exit; targets classified by the instruction before them (from the chunks' `// ADDR: insn` comments) and by decomp function starts | Game Mode, worst-angle Outset state, 16:9, widescreen mod (mask `0x1`), 26 game FPS; ≈ 2,092 game frames | **≈ 35,800 boundaries per game frame** (≈ 120 cycles each against the ≈ 4.3 M); edge service skipped for 82%, asked ≈ 6,300 per frame. Host entries ≈ 1,890 per frame; exits: budget/exception 98%, edge service 2%, no misses or stalls. By what precedes the target: after `bl` (direct-call return) 9,620 (27%); function starts (calls) 9,364 (26%, edge service asked 49%); after `bctrl` 8,088 (23%); after `b` 2,526 (7%); after `mtmsr` 1,131 (3%); REL code 939 (3%). Top calls: `J3DShapeMtxConcatView::loadMtxConcatView` 1,751, `cTgIt_JudgeFilter` 1,018, `cTgIt_MethodCall` 585, `fpcSch_JudgeByID` 557, `OSDisableInterrupts` 513, `OSRestoreInterrupts` 510 |
