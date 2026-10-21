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
- [ ] M3-T02: Run five interleaved target-scene pairs, confirm accepted calls, compare checkpoints, and re-profile the game thread. (FR-03, FR-04)
- [ ] M3-T03: Complete the collision-heavy gameplay route and record any crash, visual, physics, audio, save, or state difference. (FR-01, FR-04)
- [ ] M3-T04: Retain the opt-in path only if it is equivalent and faster beyond noise; otherwise reject it or return the discovered fix to M2. (FR-04)

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
| M0-T01 | [`GROUND_CROSS_CONTRACT.md`](GROUND_CROSS_CONTRACT.md) | Inspect the private generated fragment and pinned decomp; enumerate blocks, calls, fields and observable CPU/timing effects | Source `2ab8118`; GZLE01 private composite | Contract mapped; routine remains within one DOL chunk and calls virtual `ChkGrpThrough`, translated `GroundCrossRp`, recursion and save/restore helpers |
| M0-T02 | [`GROUND_CROSS_CONTRACT.md`](GROUND_CROSS_CONTRACT.md) | Trace child/sibling control flow and host/call boundaries; compare with current native decline contract | Same private composite and public direct-call interfaces | Depth-first child order frozen; unsafe speculative translated-call path identified and made an explicit decline/implementation gate |
| M0-T03 | Contract [fixture identity](GROUND_CROSS_CONTRACT.md#private-fixture-identity) | Canonicalize the routine with `native_entries.py` rules and SHA-256 the private metadata/chunk | Base composite generated from source `2ab8118` | Fixture and canonical fragment hashes recorded; generated source and game data remain untracked |

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
  privately generated hooked chunk compiled with Clang 22. Full hooked-module
  activation remains open because `--native-entries` currently reaches the
  unrelated `native-leaf-gen` failure tracked separately from this routine.

PR #421 is the temporary source for candidate selection and the broader Deck
procedure. No task above is complete merely because that PR contains an older
profile or artifact.
