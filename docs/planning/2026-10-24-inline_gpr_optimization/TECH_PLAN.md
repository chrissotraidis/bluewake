# TECH_PLAN — Inline GPR frame validation

Status: Candidate implemented and measured; integration gates remain
Inspected: tested BlueWake `f9205a53de307caac4e306e2a744609b1565135b`; tested RecompCore `0ab73ca9a673553675b8e511882e6f6b485407eb`; rebased RecompCore companion `47a5229cbfb3387cfd21c2b9d304b303a54a6767`; upstream BlueWake `main` `211abfdecf13beb5b9f2120459a1a3cde50dd84e`

[PRD](PRD.md) · [Tracker](PROGRESS.md) · [guest memory model](../../architecture/GuestState.md#guest-to-host-address-resolution) · [translated-code dispatch](../../architecture/GameModule.md#translated-code-dispatch)

## Code snapshot and inspected facts

| Role | Exact snapshot | Relevant state |
|---|---|---|
| Tested control | BlueWake `45e503fb869215ce3b4cb2490d12a53851268071` | Clang 18, `x86-64-v3`, composite `54f544…`, PGO `7334fd…` |
| Tested candidate | BlueWake `f9205a53de307caac4e306e2a744609b1565135b` | Same compiler, architecture and composite; PGO `880e00…` |
| Tested candidate runtime | RecompCore `0ab73ca9a673553675b8e511882e6f6b485407eb` | Exact commit used by the measured artifact |
| Rebased runtime companion | RecompCore `47a5229cbfb3387cfd21c2b9d304b303a54a6767` | Same API rebased onto current `bluewake-next`; [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45) |
| Current upstream | BlueWake `211abfd`, RecompCore `1446450` | PR branch rebased; dependency must still be integrated before merge |

The generator inserts guarded inline loads/stores through [`inline_save_restore_gpr.py`](../../../scripts/windows/inline_save_restore_gpr.py). The candidate changes only eligibility in [`inline_gpr.h`](../../../cmake/composite/inline_gpr.h); generated `mem_read32` / `mem_write32`, cycle charging, direct-call checks, fallback, and reservation behavior remain intact. The complete control-to-candidate source diff consists only of this GPR work, its RecompCore alias-range API, integration wiring and tests; no unrelated runtime, renderer, service or gameplay source changed.

## Implemented flow

```mermaid
flowchart TD
    Call["Translated save/restore call"] --> Guard["Direct-call and journal guards"]
    Guard --> Range["Compute complete GPR stack span"]
    Range --> Bounds{"Aligned and inside MEM1?"}
    Bounds -->|No| Fallback["Original translated call"]
    Bounds -->|Yes| Alias{"Original or canonical span overlaps an alias?"}
    Alias -->|Yes| Fallback
    Alias -->|No| Inline["Existing per-register memory helpers"]
    Inline --> Continue["Charge cycles and continue translated code"]
```

RecompCore's candidate `ppc_guest_alias_range_overlaps(address, size)` uses its sorted alias index for non-overlapping registrations and a full scan when registrations overlap, preserving registration-order resolution semantics. BlueWake checks both the supplied range and its `0x40000000`-canonical form before proving plain MEM1.

Central implemented contract:

```c
if (offset > cpu->ram_size - size ||
    ppc_guest_alias_range_overlaps(address, size) ||
    (canonical != address &&
     ppc_guest_alias_range_overlaps(canonical, size)))
    return false;
return true; /* existing memory helpers still perform every load/store */
```

## Change and integration map

| State | File / symbol | Practical role | Milestone |
|---|---|---|---|
| Existing candidate | [`cmake/composite/inline_gpr.h`](../../../cmake/composite/inline_gpr.h) | Validate one contiguous frame and reject both alias forms | M2 |
| Existing candidate | [`tests/inline_gpr_memory_test.c`](../../../tests/inline_gpr_memory_test.c) | Cover ordinary, partial-alias, mirror, bounds, MMIO and journal cases | M3 |
| Existing candidate | RecompCore `ppc_guest_alias_range_overlaps` | Answer precise half-open range overlap without resolving storage | M1 |
| Under review | [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45) and BlueWake dependency pins | Land the runtime API upstream, then replace the temporary contributor pin | M5 |
| Integrated | Candidate branch versus current `main` | Rebased so unrelated Dawn patch history no longer appears in the PR | M5 |

## Measurement methodology and findings

The Steam Deck was in Game Mode and the benchmark was launched over SSH. Both private AppImages used Clang 18, `x86-64-v3`, local training, the same optimization switches, and the same composite digest. Candidate SHA-256: `cf972f…`; control: `51c64c…`.

The preserved state `quick-23874.bwstate` was loaded before a five-scene tour: Outset, Windfall, Dragon Roost Cavern, Forest Haven, and sea by the Fortress. Each stop covered 1,800 retraces; the harness excludes its first 300 retraces. A run-local copy of the pending `--setup-state` harness used `START=24000` to match the state's retrace; its SHA-256 is `8424ea…`. Every accepted run loaded all five scenes and produced 31 matching checkpoints. Two rounds reversed order: candidate→control, then control→candidate.

| Mode | Round 1 mean | Round 2 mean | Mean paired change |
|---|---:|---:|---:|
| Headless | `43.9139 → 44.9400` | `43.6836 → 44.8011` | **+2.447%** |
| Rendered | `39.4510 → 40.4019` | `39.9174 → 40.8888` | **+2.422%** |

### Scene-level results

Each cell reports `control → candidate (change)`. These are the accepted comparator outputs, not values reconstructed from the aggregate mean.

| Scene | Headless round 1 | Headless round 2 | Rendered round 1 | Rendered round 2 |
|---|---:|---:|---:|---:|
| Outset | `42.50 → 43.20 (+1.6%)` | `41.70 → 42.91 (+2.9%)` | `38.68 → 39.72 (+2.7%)` | `39.38 → 40.24 (+2.2%)` |
| Windfall | `45.17 → 45.65 (+1.1%)` | `44.36 → 45.75 (+3.1%)` | `40.53 → 41.51 (+2.4%)` | `40.77 → 42.02 (+3.1%)` |
| Dragon Roost Cavern | `60.43 → 62.18 (+2.9%)` | `60.42 → 61.60 (+1.9%)` | `53.70 → 55.04 (+2.5%)` | `54.44 → 55.66 (+2.3%)` |
| Forest Haven | `37.31 → 38.52 (+3.2%)` | `37.50 → 38.60 (+2.9%)` | `33.45 → 34.05 (+1.8%)` | `33.58 → 34.51 (+2.7%)` |
| Sea by the Fortress | `34.15 → 35.15 (+2.9%)` | `34.43 → 35.14 (+2.1%)` | `30.89 → 31.69 (+2.6%)` | `31.42 → 32.02 (+1.9%)` |

All five scenes improved in all four pairs. The rendered rows are the quantitative evidence closest to live play: the game rendered normally while the fixed scene windows were measured. In the separate player-controlled Game Mode island tour, the tester reported that live gameplay felt better and observed no gameplay issue. That assessment is useful anecdotal evidence, but the tour did not record a synchronized control/candidate interval and therefore does not establish an additional numeric “live gameplay” gain.

Invalid attempts using an incompatible setup state or a no-warp `--state` invocation were rejected and excluded. The different PGO profiles do not introduce an unrelated source-code variable: training each otherwise source-matched artifact can legitimately produce different profiles because the GPR optimization changes what training observes. The result therefore measures the complete GPR candidate under its normal training process; a matched-profile qualification remains open to separate the direct source effect from its training-mediated effect.

## Verification

| Layer | Command / method | Required evidence | State |
|---|---|---|---|
| Source contract | `python3 tests/test_inline_gpr.py` | 4 passing tests | Passed |
| Builder contract | `python3 tests/test_windows_prepared_cache.py` | 21 passing tests | Passed |
| Compiled component | CTest `bluewake_inline_gpr_memory_test` | Alias/mirror/bounds/journal cases pass | Passed, 1/1 |
| Repository | `git diff --check`; `scripts/audit_repo.sh` | Clean diff; audit pass | Passed |
| Packaging | CI-container AppImage build and ABI audit | Successful private AppImage; max `GLIBC_2.38` | Passed |
| Hardware | Two headless and two rendered paired rounds | Loaded scenes, matching checkpoints, repeatable positive delta | Passed on tested snapshot |
| Integration | Rebase, upstream runtime pin, Linux/Windows CI | No fork pin; current-main evidence | Rebase passed; dependency, qualification and CI open |

## Implementation order

| Milestone | Depends | Deliverable / gate |
|---|---|---|
| M0 | — | Preserve existing inline behavior and define range proof |
| M1 | M0 | Tested RecompCore query; upstream dependency still required |
| M2 | M1 | Implemented eligibility change with unchanged memory operations |
| M3 | M1, M2 | Focused tests and repository checks pass |
| M4 | M3 | Repeated headless/rendered evidence and player tour |
| M5 | M1, M2, M3, M4 | Rebase, upstream RecompCore pin, matched qualification and CI |

Critical path: [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45) review → M5 upstream pin → current-main qualification. M3 documentation and evidence review can proceed in parallel, but BlueWake cannot merge before its RecompCore API exists upstream.

## References

- [Steam Deck performance issue](https://github.com/chrissotraidis/bluewake/issues/215)
- [Parent game-thread performance tracker](https://github.com/chrissotraidis/bluewake/pull/421)
- [Companion RecompCore API PR](https://github.com/chrissotraidis/RecompCore/pull/45)
- [Kept-state benchmark PR](https://github.com/chrissotraidis/bluewake/pull/450) and [setup validity follow-up](https://github.com/chrissotraidis/bluewake/pull/459)
