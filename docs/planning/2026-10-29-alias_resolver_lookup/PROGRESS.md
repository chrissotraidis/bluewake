# PROGRESS — Constant-time guest alias lookup

Status: In progress (accepted: lava room +10.0 %, resolver 9.36 % → 2.52 %, identical checkpoints and pictures; next: the merge order (#45, #48, #480, #495), then rebase onto main)
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Baseline and census
Architecture: [guest-to-host address resolution](../../architecture/GuestState.md#guest-to-host-address-resolution).
- [x] M0-T01: Add `same_as_last`, page-scan-step and miss counters to the RecompCore census, plus an exported report call
- [x] M0-T02: Make BlueWake call that report before exiting at `BLUEWAKE_MAX_RETRACES`; check a non-empty `[alias-cost]` line on the Deck
- [!] M0-T03: Deck A/A of the current stack (#495/#480), 5 states, `--warmup 1 --pairs 5`: record each scene's spread (blocked by decision: the owner skipped it on 2026-10-29, confident Hyrule Castle's −0.77 % is noise; covered by later testing)
- [x] M0-T04: Census in the lava room and Hyrule Castle: calls per frame, last-hit rate, scan steps for 4 KiB pages
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
- [x] M2-T03: Register it in CTest for both variants; run on Linux and Windows (RecompCore has no PR CI: run by hand; CI coverage through BlueWake's host CI with the pin, M3)

### M3: Integration and Deck acceptance (depends: M1, M2)
- [ ] M3-T01: RecompCore PR to `bluewake-next` with M1 and M2; tests passing on Linux and Windows (opened as RecompCore [#48](https://github.com/chrissotraidis/RecompCore/pull/48), draft, stacked on #45; RecompCore has no PR CI, tests run by hand)
- [x] M3-T02: BlueWake build of the current stack with the new pin; census and `render_bench --cpu-profile` in the lava room
- [x] M3-T03: Deck A/B against the same stack with the old pin, 5 states, `--warmup 1 --pairs 5`: FPS, checkpoints, pictures
- [x] M3-T04: Record the result against the PRD targets (pass, or no-go with numbers); BlueWake pin PR only on pass
- [x] M3-T05: With the pin (after #45, #48, #480, #495 land): register `bluewake_guest_alias_lookup_{plain,global}_test` in `runtime/host/CMakeLists.txt` from RecompCore's `GXRuntime/tests/guest_alias_lookup_test.c`, linked to `gxruntime` like the gather-memory tests, and add them to the build targets in `linux-host.yml` and `windows-host.yml`; CI then runs it on Linux and on Windows (MSVC)

## Evidence ledger

Builds: A = `alias-lookup` `4a2158d` (RecompCore `abb3e31`; #48 later added `2e4e599`, an MSVC-only path that leaves other compilers' code unchanged), B = `alias-base` `8d6fd5e` (RecompCore `caeedd8`), both BlueWake #495 `82371eb` plus host commit `172a787`. Deck = Steam Deck, offscreen gamescope 1280×800. Numbers are in [Results](#results).

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | RecompCore `abb3e31` | New census counters and `ppc_guest_alias_cost_report_now()` | GXRuntime, Linux | Builds and links |
| M0-T02 | BlueWake `172a787` | `BLUEWAKE_TRACE_ALIAS_COST=1`, run stopped at `BLUEWAKE_MAX_RETRACES` | Deck, builds A and B | `[alias-cost]` printed in 4/4 runs |
| M0-T04 | Builds A and B | 1,800 retraces from each state, unpaced | Deck, lava room and Hyrule Castle | [Census](#census): 99.5 % last hits in the lava room |
| M0-T05 | RecompCore `abb3e31` | Source review: mutators rebuild the page index, as they do the sorted index | `cpu.c` at `caeedd8` | Eager build; no reader builds it |
| M1-T01 | RecompCore `abb3e31` | RecompCore #34's last-hit check carried onto #45 | `cpu.c` | Done |
| M1-T02 | RecompCore `abb3e31` | `ppc_guest_alias_pages_rebuild()` in add, remove, clear | `cpu.c` | Done |
| M1-T03 | RecompCore `abb3e31` | Page start and forward scan replace the binary search | `cpu.c` | Done; overlap and over-32 MiB paths kept |
| M1-T04 | RecompCore `abb3e31` | `ctest -R 'guest_alias_lookup\|guest_memory\|gxruntime_tests'` | GXRuntime, Release, Linux | 5/5 passed |
| M2-T01 | `guest_alias_lookup_test.c` | Differential test against registration order | Same CTest run | 11,249,145 accesses matched |
| M2-T02 | `guest_alias_lookup_test.c` | Edge cases; page start +1 mutation | Same CTest run | Edges pass; mutation caught |
| M2-T03 | RecompCore `abb3e31` | Linux: CTest, both variants. Windows: `x86_64-w64-mingw32-gcc -static` (GCC 12), run under Wine 8 in `debian:bookworm-slim` | Linux (GCC), Windows (MinGW, Wine) | Both variants pass on both: 11,249,145 accesses matched. Not yet built with MSVC |
| M3-T02 | Build A; report `2026_10_29-145102` | `render_bench --cpu-profile` | Deck, 5 states | [Profile](#profile): resolver 9.36 % → 2.52 % |
| M3-T03 | Builds A and B; same report | `render_bench --warmup 1 --pairs 5 --cpu-profile` | Deck, 5 states, 10 runs per build | [A/B](#ab): lava room +10.00 %, overall +2.97 % |
| M3-T04 | Report `2026_10_29-145102` | PRD targets against the A/B | — | Pass: resolver 2.52 % (≤ 3 %), lava room +10.00 % beyond run spread, checkpoints and pictures identical; "no scene slower" accepted by the owner without the A/A (M0-T03 skipped) |
| M3-T05 | BlueWake `523a333` (#501), RecompCore `2e4e599` | Upstream CI on #501: [Linux host](https://github.com/chrissotraidis/bluewake/actions/runs/40971122162/job/123787822952), [Windows host](https://github.com/chrissotraidis/bluewake/actions/runs/40971122244/job/123787824304) | Ubuntu 24.04 (GCC); Windows (Clang, MSVC mode) | `bluewake_guest_alias_lookup_{plain,global}_test` passed on both; 41/41 and 75/75 tests. The first run found the missing MSVC atomics (fixed in `2e4e599`) and the Linux registration in the wrong project (fixed in `523a333`) |

## Results

### Census

Game module's resolver, 1,800 retraces from each state (M0-T04).

| Scene | Build | Calls | Search steps | Last hits | Page lookups | Steps per page lookup |
|---|---|---:|---:|---:|---:|---:|
| Lava room | B | 233.5 M | 2.55 G | — | — | — |
| Lava room | A | 233.5 M | 0 | 231.0 M (99.5 % of hits) | 1.12 M | 2.3 |
| Hyrule Castle | B | 4.6 M | 34.9 M | — | — | — |
| Hyrule Castle | A | 4.6 M | 0 | 2.4 M (76 % of hits) | 0.75 M | 1.6 |

Both builds ran the same blocks: 1,305,946 (lava room) and 1,306,926 (Hyrule Castle). 1,900 aliases, 7,041 pages.

### A/B

Game frames a second, median of 10 runs per build (M3-T03).

| Scene | B | A | Change |
|---|---:|---:|---:|
| Outset | 26.695 | 26.909 | +0.80 % |
| Lava room | 30.465 | 33.511 | **+10.00 %** |
| Hyrule Castle | 55.277 | 54.854 | −0.77 % |
| Butterfly room | 41.607 | 42.069 | +1.11 % |
| Shipwreck | 29.882 | 30.262 | +1.27 % |
| Overall | 35.347 | 36.398 | +2.97 % |

Checks: 20/20 runs loaded every scene; guest checkpoints identical in every run; pictures identical in all 5 scenes. Hyrule Castle's −0.77 % is within its run spread (sd ≈ 1.5 %); the A/A that would measure the noise floor was skipped by the owner's decision (M0-T03), to be covered by later testing.

### Profile

Lava room, game thread, over the scene's FPS window (M3-T02).

| Measure | B | A | Change |
|---|---:|---:|---:|
| `ppc_guest_alias_resolve` share | 9.36 % | 2.52 % | |
| Cycles per game frame | 114.4 M | 104.9 M | −8.30 % |
| Instructions per game frame | 234.3 M | 193.0 M | −17.62 % |
