# PROGRESS — Constant-time guest alias lookup

Status: In progress (M1, M2 implemented; M0 and M3 measurements pending)
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Baseline and census
Architecture: [guest-to-host address resolution](../../architecture/GuestState.md#guest-to-host-address-resolution).
- [x] M0-T01: Add `same_as_last`, page-scan-step and miss counters to the RecompCore census, plus an exported report call
- [ ] M0-T02: Make BlueWake call that report before exiting at `BLUEWAKE_MAX_RETRACES`; check a non-empty `[alias-cost]` line on the Deck (host call in `81e8020`; Deck line pending)
- [ ] M0-T03: Deck A/A of the current stack (#495/#480), 5 states, `--warmup 1 --pairs 5`: record each scene's spread
- [ ] M0-T04: Census in the lava room and Hyrule Castle: calls per frame, last-hit rate, scan steps for 4 KiB pages
- [x] M0-T05: Check whether the gather path resolves off the game thread during a REL load; record the M1 build mode (lazy or eager)

### M1: Resolver lookup (depends: M0)
Parallel with M2; owns RecompCore `GXRuntime/src/core/cpu.c`; stacked on RecompCore #45; integrates at M3 under the TECH_PLAN contract.
- [x] M1-T01: Rebase RecompCore #34's last-hit change onto #45
- [x] M1-T02: Add the page index and stale marking in insert, rebuild, clear and remove
- [x] M1-T03: Replace the binary search with the page start and forward scan; keep the overlapping path and the over-32 MiB fallback
- [x] M1-T04: Run the existing guest-memory tests in both `BW_GUEST_MEM1` variants

### M2: Differential test (depends: M0)
Parallel with M1; owns `GXRuntime/tests/guest_alias_lookup_test.c` and its `CMakeLists.txt` registration; integrates at M3.
- [x] M2-T01: Reference linear-scan resolver in the test; random alias sets (none, overlapping, 1 and 4,096 aliases)
- [x] M2-T02: Edge cases: page boundaries, sizes 1–8, mirror bit, remove and re-add, alias span over 32 MiB
- [ ] M2-T03: Register it in CTest for both variants; run on Linux and Windows CI (registered; Linux run locally; Windows CI pending)

### M3: Integration and Deck acceptance (depends: M1, M2)
- [ ] M3-T01: RecompCore PR to `bluewake-next` with M1 and M2; CI green on Linux and Windows
- [ ] M3-T02: BlueWake build of the current stack with the new pin; census and `render_bench --cpu-profile` in the lava room
- [ ] M3-T03: Deck A/B against the same stack with the old pin, 5 states, `--warmup 1 --pairs 5`: FPS, checkpoints, pictures
- [ ] M3-T04: Record the result against the PRD targets (pass, or no-go with numbers); BlueWake pin PR only on pass

## Evidence ledger
| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | RecompCore `abb3e31` (`perf/alias-lookup`) | `last_hits`, `page_lookups`, `page_steps` in `[alias-cost]`; exported `ppc_guest_alias_cost_report_now()` | GXRuntime build, Linux | Builds and links; the printed line is checked in M0-T02 |
| M0-T05 | RecompCore `abb3e31` | Design: the page index is rebuilt by the alias mutators, as the sorted index already is ("never rebuilt lazily by a reader"), so no reader builds it and no thread question arises | Source review of `cpu.c` at `caeedd8` | Eager build; lazy mode dropped |
| M1-T01 | RecompCore `abb3e31` | RecompCore #34's last-hit check (registry index, relaxed atomic) carried onto #45 | Stacked on `caeedd8` | Applied |
| M1-T02 | RecompCore `abb3e31` | `ppc_guest_alias_pages_rebuild()` after add, remove and clear | `cpu.c` | Applied |
| M1-T03 | RecompCore `abb3e31` | Page start plus forward scan replaces the binary search; overlapping scan and the over-32 MiB fallback kept | `cpu.c` | Applied |
| M1-T04 | RecompCore `abb3e31` | `ctest -R 'guest_alias_lookup\|guest_memory\|gxruntime_tests'` | GXRuntime standalone, Release, Linux (Gentoo, GCC) | 5/5 passed |
| M2-T01 | RecompCore `abb3e31`, `GXRuntime/tests/guest_alias_lookup_test.c` | Reference: first alias in registration order holding the access | Same CTest run | 11,249,145 accesses matched, both variants |
| M2-T02 | Same test | Page edges, sizes 1-8, removes and re-adds, 4,096 aliases, span over 32 MiB, overlapping sets; a deliberately wrong page start (+1) failed at `0xC0414000` | Same CTest run; mutation run by hand | Edges covered; the mutation is caught |
