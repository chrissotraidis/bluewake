# PROGRESS — Constant-time guest alias lookup

Status: Planned
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Baseline and census
Architecture: [guest-to-host address resolution](../../architecture/GuestState.md#guest-to-host-address-resolution).
- [ ] M0-T01: Add `same_as_last`, page-scan-step and miss counters to the RecompCore census, plus an exported report call
- [ ] M0-T02: Make BlueWake call that report before exiting at `BLUEWAKE_MAX_RETRACES`; check a non-empty `[alias-cost]` line on the Deck
- [ ] M0-T03: Deck A/A of the current stack (#495/#480), 5 states, `--warmup 1 --pairs 5`: record each scene's spread
- [ ] M0-T04: Census in the lava room and Hyrule Castle: calls per frame, last-hit rate, scan steps for 4 KiB pages
- [ ] M0-T05: Check whether the gather path resolves off the game thread during a REL load; record the M1 build mode (lazy or eager)

### M1: Resolver lookup (depends: M0)
Parallel with M2; owns RecompCore `GXRuntime/src/core/cpu.c`; stacked on RecompCore #45; integrates at M3 under the TECH_PLAN contract.
- [ ] M1-T01: Rebase RecompCore #34's last-hit change onto #45
- [ ] M1-T02: Add the page index and stale marking in insert, rebuild, clear and remove
- [ ] M1-T03: Replace the binary search with the page start and forward scan; keep the overlapping path and the over-32 MiB fallback
- [ ] M1-T04: Run the existing guest-memory tests in both `BW_GUEST_MEM1` variants

### M2: Differential test (depends: M0)
Parallel with M1; owns `GXRuntime/tests/guest_alias_lookup_test.c` and its `CMakeLists.txt` registration; integrates at M3.
- [ ] M2-T01: Reference linear-scan resolver in the test; random alias sets (none, overlapping, 1 and 4,096 aliases)
- [ ] M2-T02: Edge cases: page boundaries, sizes 1–8, mirror bit, remove and re-add, alias span over 32 MiB
- [ ] M2-T03: Register it in CTest for both variants; run on Linux and Windows CI

### M3: Integration and Deck acceptance (depends: M1, M2)
- [ ] M3-T01: RecompCore PR to `bluewake-next` with M1 and M2; CI green on Linux and Windows
- [ ] M3-T02: BlueWake build of the current stack with the new pin; census and `render_bench --cpu-profile` in the lava room
- [ ] M3-T03: Deck A/B against the same stack with the old pin, 5 states, `--warmup 1 --pairs 5`: FPS, checkpoints, pictures
- [ ] M3-T04: Record the result against the PRD targets (pass, or no-go with numbers); BlueWake pin PR only on pass

## Evidence ledger
| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
