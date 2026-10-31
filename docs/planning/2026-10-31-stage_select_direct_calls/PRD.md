# PRD — Stage-select direct-call cost

Status: Draft  
Source: BlueWake [`0c8a70d`](https://github.com/chrissotraidis/bluewake/commit/0c8a70dde95d3acca12ce98a99232fc0adc7e3ac); discovery in [#503](https://github.com/chrissotraidis/bluewake/pull/503) at `4806997`  
Scope: remove persistent direct-call overhead while stage selection or a benchmark warp is enabled; excluded: changing stage-select behavior, game logic, or unrelated Outset optimization

## Motivation

The developer stage select is off by default, but enabling it makes every translated direct call ask the BlueWake host whether the boundary may be skipped. On a Steam Deck in Outset, turning it off raised game speed from 23.6 to 26.1 FPS (**+10.9%**) while clock speed stayed unchanged. Warp-based benchmarks also enable the same path for the whole run, making absolute CPU results pessimistic.

The useful outcome is not a faster default build: it is preserving stage selection and benchmark warps without paying a host round trip at unrelated guest addresses.

## Outcomes

- Keep the module's inline direct-call answer active while stage selection is enabled, except at addresses the stage-select integration observes.
- Preserve menu entry, room warps, file selection, return music, save protection, and existing fallback behavior.
- Make warp-based measurements representative after arrival, while recording the stage-select state in every comparison.
- Retain the change only with matching guest checkpoints and an on/off gap within the contemporaneous A/A spread.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---:|---:|---|
| Outset, stage select on versus off | 23.6 / 26.1 FPS; 10.9% gap | Gap no larger than A/A spread | Interleaved Game Mode windows with [#522](https://github.com/chrissotraidis/bluewake/pull/522)'s measurement tool |
| Stage-select host checks | `host_direct_can_skip_stage_select` 4.5%; `bluewake_stage_select_observes_body` 1.7% | Absent from unrelated direct calls; sampled only when an observed boundary is reached | Game-thread `perf record`, path counters, verification mode |
| Guest-visible execution | Same build and state, feature setting apart | Identical checkpoints and rendered pictures outside intentional warps | Rendered warp tour and checkpoint comparison |
| Compatibility | `v2`/`v3` modules can load | Older modules retain the conservative host-query path | Component test with missing newer export |
| Stage-select behavior | Menu and warps work today | No functional regression | File-select R entry, F7 return, warp arrival and music checks |

## Clarifications

- Default players do not pay this cost because the stage select is off by default.
- The measured result is historical evidence from #503, not proof that a proposed fix works.
- The exact fast representation remains an M0 decision: it must cover every observed address and be versioned so host and module revisions cannot disagree.

