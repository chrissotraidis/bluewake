# PROGRESS — Inline GPR frame validation

Status: Candidate measured; upstream integration open
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [parent tracker](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Freeze the contract

- [x] M0-T01: Map inline save/restore, memory resolution, fallback and safety constraints.
- [x] M0-T02: Record the exact control, candidate and current-upstream snapshots.
- [x] M0-T03: Preserve the supplied handoff identity as planning input.

### M1: Add precise alias-range detection (depends: M0)

- [x] M1-T01: Implement and component-test `ppc_guest_alias_range_overlaps` on the candidate RecompCore fork.
- [!] M1-T02: Submitted as [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45); blocked waiting for upstream review and merge.
- [ ] M1-T03: Replace the fork-only pin with the accepted upstream RecompCore revision.

### M2: Validate one complete GPR frame (depends: M1)

- [x] M2-T01: Replace per-word readiness resolution with bounds plus original/canonical alias-range checks.
- [x] M2-T02: Reuse the proven MEM1 frame with fixed-offset big-endian accesses while preserving timing and fallback behavior.
- [x] M2-T03: Preserve Windows symbol export and generated-source integration.
- [x] M2-T04: Match helper-path bytes, values and reservation state for every supported first register and stack form.

### M3: Verify correctness (depends: M1, M2)

- [x] M3-T01: Pass source, builder-cache and compiled memory tests.
- [x] M3-T02: Pass repository audit, diff and AppImage ABI checks.
- [x] M3-T03: Cover ordinary RAM and every identified rejection boundary.

### M4: Qualify on Steam Deck (depends: M3)

- [x] M4-T01: Complete a player-facing Game Mode island tour.
- [x] M4-T02: Run two opposite-order five-scene headless and rendered A/B rounds.
- [x] M4-T03: Confirm every scene loaded and all 31 checkpoints matched.

### M5: Integrate upstream (depends: M1, M2, M3, M4)

- [ ] M5-T01: Rebase onto current BlueWake `main` and pin the upstream RecompCore revision.
- [ ] M5-T02: Rebuild control/candidate with an explicitly matched PGO policy and repeat qualification.
- [ ] M5-T03: Pass Linux and Windows CI; record any required Windows hardware follow-up.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | Handoff SHA `185ccc…`; source inspection | Trace generator, helper and memory API | BlueWake `f9205a5` | Contract recorded |
| M0-T02 | Provenance JSONs | Compare commits, composite and compiler | Control `45e503f`; candidate `f9205a5`; upstream `211abfd` | Snapshots fixed |
| M0-T03 | Handoff SHA `185ccc…` | SHA-256 identity check | Supplied Markdown artifact | Input identity recorded |
| M1-T01 | RecompCore `0ab73ca` | `guest_memory_bounds_test` coverage | Alias registry, overlapping ranges | Candidate API passes |
| M2-T01 | BlueWake `ba302db`, `f9205a5` | Source and generated-flow inspection | Original and mirror spans | Range proof implemented |
| M2-T02 | Contributor commit `043495d`; `inline_gpr.h`; generator | Resolve the proven frame once and use `read_be32` / `write_be32` | Save/restore calls | Direct frame access integrated |
| M2-T03 | BlueWake `f9205a5` | Inspect CMake export and generator tests | Linux and Windows build paths | Integration retained |
| M2-T04 | `inline_gpr_memory_test.c` | Compare helper and direct paths across registers, stack forms and reservations | Disc-free compiled test | Bytes, values and reservation state match |
| M3-T01 | Local test output | Python tests; focused CTest | Disc-free | 4, 21 and 1/1 passed |
| M3-T02 | Private AppImage `cf972f…` | Audit, diff, package ABI check | Ubuntu 24.04 CI container, Clang 18 | Passed; max GLIBC 2.38 |
| M3-T03 | `inline_gpr_memory_test.c` | Run focused compiled component suite | RAM, aliases, bounds, MMIO, journal | Rejection coverage passed |
| M4-T01 | Private Deck session log | Game Mode island tour | First-revision Deck, SteamOS | No issue reported |
| M4-T02 | Private summary JSONs | Five-scene A/B, two reversed rounds | Headless and rendered | +2.447% / +2.422% paired mean |
| M4-T03 | Harness comparisons | `bench_tour_23874.py compare` | 1,800 retraces/scene | All loaded; checkpoints match |
