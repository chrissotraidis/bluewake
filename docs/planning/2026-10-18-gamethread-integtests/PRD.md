# PRD — Native work-pool test foundation

## Contents

- [Goal and scope](#goal-and-scope)
- [Requirements](#requirements)
- [Acceptance](#acceptance)
- [Review sequence](#review-sequence)

[Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Goal and scope

Give contributors a small, source-only regression suite for the existing native
work pool. This PR contains both the scoped plan and its implementation: shared
native-math registration, standalone pool tests, and Linux integration. It does
not require the complete optimization/qualification stack to merge.

No new offload, worker default, game algorithm, timing mode, or Windows worker
backend is introduced. Public tests use synthetic inputs; personal modules,
discs and saves are neither required nor uploaded.

## Requirements

The retained identifiers refer to the foundation portions of the earlier plan.

| Contract | Foundation requirement | Evidence |
|---|---|---|
| FR-02 — Partitions | Execute each requested item exactly once; cover empty, uneven and changing ranges | Exact visit counts, output and boundary sentinels |
| FR-04 — Publication | Return only after outputs from every participant are available; repeat 1,000 submissions | Synchronous output checks after each call |
| FR-05 — Configuration | Exercise invalid/zero, 1/2/3-worker and clamped configurations in separate processes | Expected participant count and range ownership |
| FR-06 — Execution evidence | Distinguish real worker execution from successful serial fallback | Worker/batch assertions, not CPU utilization |
| Build integration | Preserve existing native-math target names and flags; build tests before CTest | Standalone and Linux-integrated runs; Windows compatibility checks |

## Acceptance

- The standalone suite compiles production pool code without a renderer or disc.
- Each registered test runs, with bounded timeouts and actionable failures.
- Linux builds the guard and pool targets explicitly before running CTest.
- Existing Windows serial behavior and guard compilation remain unchanged.
- Record commands, compiler/platform, selected cases and results; missing
  hardware or private evidence is not a pass.

## Review sequence

1. Review this scoped plan and its implementation together in this PR.
2. Review [#424's consolidated coverage](https://github.com/chrissotraidis/bluewake/pull/424) only as a concrete optimization
   needs it: boundaries/oracle, creation failure, DSO lifecycle, diagnostics and sanitizers.
3. Submit the optimization separately, measured with the
   [benchmark tour](../../BENCHMARK_TOUR.md) and the
   [performance strategy](../../PERFORMANCE.md), with gameplay checks.

[#396](https://github.com/chrissotraidis/bluewake/pull/396) and
[#400](https://github.com/chrissotraidis/bluewake/pull/400) remain independent.
Qualification/reporting tools (#412–#417) and the broader optimization plan
[#421](https://github.com/chrissotraidis/bluewake/pull/421) are deferred, not prerequisites.
