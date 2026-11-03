# Wind Temple stage-load comparison — October 15, 2026

The opt-in loose-REL wait from #331 fixes the two reported cold stage loads on this Mac, matching the contributor's Windows/Linux outcome comparison in [#318](https://github.com/chrissotraidis/bluewake/issues/318).

## Method

M3 Max, host `14bc288eeb8622fbcbf3b4e261e94cb392635a55`, RecompCore `57123c6c12ae0fa52dcba20742dc7e79866de978`, and an older October 11 prepared module. The comparison explicitly uses headless/HLE controls and an early save, with at most two child processes. Each run has its own card and data folder. `BLUEWAKE_LOOSE_REL_RETRACES` is explicitly 0 or 1. All 468 destinations are run once per mode, checking for 120 frames after arrival.

| Result | Wait off | One retrace |
| --- | ---: | ---: |
| Completed arrival check | 446 | 448 |
| Unmapped-PC stop | 9 | 7 |
| Normal stop without completing the arrival check | 13 | 13 |

Only R04 (`8.6`) and R08 (`8.9`) change outcome: both stop at `PC 80000000 / LR 80027A2C` with the wait off and complete arrival with it on. The other 20 non-completions are the same four test rooms and sixteen story/cutscene entries in both modes. Another 445 pairs differ only in final PC within the compared fields; all differences are retained, so this is not a claim of identical instruction execution.

All 936 children exited. The original card, seed, every run card and tested artifacts have matching before/after hashes. Local records are under `build/wwhd-loose-rel-318/20261014T192208Z`: `comparison-summary.json`, `full-results.tsv`, `differences.json` and `provenance-before.json`, plus per-run logs and commands. Game inputs and cards remain private.

## Rendered checks

Separate Metal sessions with the same host/runtime and older module, at one retrace, render both rooms without the reported unmapped-PC crash. R08 allows movement to the right wall. R04 responds to input while enemies crowd and damage Link; an earlier unattended run reached the death/save screen. All three sessions exited normally and left their copied cards unchanged. This establishes bounded arrival and input, not dungeon completion or normal door transitions.

Fresh-module checks now repeat R04/R08 rendering on M3 Max/Metal with host `14bc288`, runtime `57123c6` and `BLUEWAKE_LOOSE_REL_RETRACES=1`. R08 shows movement; R04 shows bounded input/menu response amid enemies. All four sessions exit 0 with copied cards unchanged. The run named “traversal” does **not** establish traversal or dungeon success. Per-run provenance identifies the fresh module; the 936-run sweep and older rendered summary still describe the older module.

## Remaining

The switch remains off by default. [#350](https://github.com/chrissotraidis/bluewake/pull/350) merged with green Windows/Linux CI and eight ASan/UBSan cases guarding save/load while host-side waits are occupied. The rendered checks above use the earlier host and do not exercise that guard: native F5/F8 during a pending wait and normal traversal remain unchecked. A default change remains separate; these Mac checks do not establish phone/iPad acceptance, performance, or full dungeon completion.
