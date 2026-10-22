# PROGRESS — Native GroundCrossGrpRp reimplementation

Status: In progress

[Requirements](PRD.md) · [Technical plan](TECH_PLAN.md)

This tracker decomposes [PR #421](https://github.com/chrissotraidis/bluewake/pull/421)
runtime milestone M1. Completion of this local M0–M3 closes that parent
milestone; it is not a parallel candidate track.

## Milestones

### M0: Map the translated contract

Architecture: [translated-code dispatch](../../architecture/GameModule.md#translated-code-dispatch).

- [x] M0-T01: Record every block, cross-chunk edge, call, guest field, register, flag, and cycle/deadline effect for `0x802491F4–0x80249368`. (FR-01)
- [x] M0-T02: Record callback and host-observed boundaries plus the exact child traversal order; identify every condition that must decline. (FR-01, FR-02)
- [x] M0-T03: Freeze private fixture identities and certification fragments without committing generated code or game data. (FR-03)

### M1: Build correctness tests first (depends: M0)

- [ ] M1-T01: Add public synthetic guard cases for success, bad pointers, aliases, exceptions, journals, deadlines, callbacks, and watched edges. (FR-02, FR-04)
- [ ] M1-T02: Extend the private native-versus-translated oracle across shallow, recursive, filtered, hit/no-hit, interruption, and callback-mutated cases. (FR-01, FR-04)
- [ ] M1-T03: Prove accepted-call activation, unchanged decline, explicit native-disabled fallback, and Linux/Windows oracle execution. (FR-02, FR-03, FR-04)

### M2: Implement and certify the native entry (depends: M1)

- [ ] M2-T01: Add the guarded serial routine using local computation and a single guest-visible commit. (FR-01, FR-02)
- [ ] M2-T02: Register `0x802491F4`, accepted/declined counters, and the existing global opt-out. (FR-03)
- [ ] M2-T03: Certify every replaced fragment and verify changed or watched translations refuse the hook. (FR-03)
- [ ] M2-T04: Run public guards, the private differential oracle, existing native suites, repository checks, and Linux/Windows CI. (FR-01, FR-02, FR-03, FR-04)

### M3: Qualify on Steam Deck and decide (depends: M2 and current control)

- [ ] M3-T01: Build matched native-off/native-on AppImages from one current revision and record provenance, hashes, settings, and A/A noise. (FR-04)
- [x] M3-T02: Run five interleaved target-scene pairs, confirm accepted calls, compare checkpoints, and re-profile the game thread. (FR-03, FR-04)
- [x] M3-T03: Complete the collision-heavy gameplay route and record any crash, visual, physics, audio, save, or state difference. (FR-01, FR-04)
- [x] M3-T04: Retain the opt-in path only if it is equivalent and faster beyond noise; otherwise reject it or return the discovered fix to M2. (FR-04)

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
| M0-T01 | [`GROUND_CROSS_CONTRACT.md`](GROUND_CROSS_CONTRACT.md) | Inspect the private generated fragment and pinned decomp; enumerate blocks, calls, fields and observable CPU/timing effects | Source `2ab8118`; GZLE01 private composite | Contract mapped; routine remains within one DOL chunk and calls virtual `ChkGrpThrough`, translated `GroundCrossRp`, recursion and save/restore helpers |
| M0-T02 | [`GROUND_CROSS_CONTRACT.md`](GROUND_CROSS_CONTRACT.md) | Trace child/sibling control flow and host/call boundaries; compare with current native decline contract | Same private composite and public direct-call interfaces | Depth-first child order frozen; unsafe speculative translated-call path identified and made an explicit decline/implementation gate |
| M0-T03 | Contract [fixture identity](GROUND_CROSS_CONTRACT.md#private-fixture-identity) | Canonicalize the routine with `native_entries.py` rules and SHA-256 the private metadata/chunk | Base composite generated from source `2ab8118` | Fixture and canonical fragment hashes recorded; generated source and game data remain untracked |
| M3-T02 | Private Deck evidence bundle; AppImage SHA-256 `6f903d88...bee8d31` | Five interleaved control/candidate pairs in both headless and rendered modes, six checkpoint comparisons per run, then matched 30-second `perf` captures | Steam Deck LCD, Game Mode, same AppImage and `quick-23874.bwstate`; control used `BLUEWAKE_NATIVE_GROUND_CROSS=0` | All checkpoints matched. Rendered median was 25.4759 FPS control and 25.5724 candidate (+0.38%); headless median was 27.4352 and 27.5667 (+0.48%). Profiles had no lost samples and remained game-thread-bound. |
| M3-T04 | Same Deck evidence and native summary counters | Compare paired distributions against observed run-to-run noise and inspect candidate activation/fallback ratio | Candidate rendered profile: 1,139,546 native executions, 2,677,657 declines and 183,003 cross-chunk cases | Return to M2: correctness is positive, but the measured gain is within noise and about 70% of attempts decline to translation after entering the candidate path. Do not retain this increment as a performance optimization. |
| M3-T03 | Private Game Mode session log | Player completed the collision-heavy island route with the candidate through the Steam shortcut, then exited normally | Steam Deck LCD, same candidate AppImage; 3 minutes in `sea/44` and `sea_T/44` | No player-reported visual, physics, audio, input, save-state or crash issue. `ground-cross-return=220,972`; native `6,175,937/13,206,369` accepted/declined; normal renderer teardown warnings only. |

The POSIX native-test builder now builds `native_bg_test`, so the collision
oracle and its GroundCross cases run against a private Linux `.so`. Recursive,
hit-producing and full hooked-module activation coverage remain open; the
current evidence does not complete M1-T02 or M1-T03.

## Current implementation increment

- `0x802491F4` is hash-certified and routed to `native_bg.c`.
- The native accepts complete early-return paths where `ChkGrpThrough`
  filters the group or the first bounds comparison rejects it.
- Every later bounds, tree-hit and child-recursion path declines unchanged;
  no translated mutating callee is executed speculatively.
- The source-matched Clang 18 container produced the private control module.
  The Linux oracle compared 60,000 GroundCross cases: 19,112 identical native
  executions, 40,888 unchanged declines, and zero mismatches. The two existing
  collision natives also completed 60,000 cases each with zero mismatches.
- Hook certification reported the expected `0ac24f...a7d` fragment and the
  full `--native-entries` module built with Clang 18 in the Linux CI container.
  The builders retain the established native-entry pass before behavior timing,
  then certify only GroundCross after that final rewrite; the focused ordering
  regression also proves the later pass preserves earlier hooks.
  The replay generator now accepts the current always-prepaid, lean-deadline,
  address-aware direct-call and guarded inline-GPR source forms, with focused
  source-shape regressions.
- A 1.4-minute desktop gameplay run reached the play scene and shut down
  cleanly. GroundCross reported 3,050,936 native executions, 5,935,727 safe
  declines and 409,422 cross-chunk executions. This proves positive runtime
  activation; matched Steam Deck correctness and performance comparisons
  remain open.
- `BLUEWAKE_NATIVE_GROUND_CROSS=0` now bypasses only the candidate hook, so
  Deck control and candidate runs can use the same AppImage without disabling
  unrelated native entries or introducing build drift. A paired 1,000-case
  module check reported `310/0` with the default and `0/0` with the switch,
  with zero oracle mismatches in both modes. The shared AppImage passed the
  SteamOS 2.39 ABI/private-file audit and extraction smoke test.
- Steam Deck qualification used the same AppImage and state for both modes.
  Five rendered pairs produced a +0.38% median and +0.40% mean candidate
  difference; five headless pairs produced a +0.48% median and -1.65% mean
  difference. All checkpoint hashes matched. Both rendered profiles stayed at
  22–25 game FPS with the game thread 96–98% busy; GPU waits remained about
  2–6 ms. The candidate's high decline rate makes this partial coverage an
  implementation target, not a retained optimization.

PR #421 is the temporary source for candidate selection and the broader Deck
procedure. No task above is complete merely because that PR contains an older
profile or artifact.
