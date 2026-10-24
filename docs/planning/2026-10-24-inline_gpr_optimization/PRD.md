# PRD — Inline GPR frame validation

Status: Implemented candidate; upstream integration open
Source: user handoff SHA-256 `185ccc3b6575a46f11fb4d460993513409d33fb0f392d82d83f5d8a761bb94bb`; tested BlueWake `f9205a5`, RecompCore `0ab73ca`
Scope: inline `_savegpr_N` / `_restgpr_N` eligibility; excluded: renderer, GPU work, new native routines, and direct replacement of memory helpers

[Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · [Steam Deck report](https://github.com/chrissotraidis/bluewake/issues/215) · [parent performance tracker](https://github.com/chrissotraidis/bluewake/pull/421)

## Motivation

Steam Deck gameplay is limited by one saturated game thread while the GPU has headroom. BlueWake already rewrites 5,728 translated calls to shared PowerPC register save/restore helpers, but each inline call validated every stack word separately before the unchanged memory operations resolved those addresses again.

The goal is to remove redundant translated-CPU bookkeeping without changing guest-visible memory, timing, fallback, or reservation behavior. Stable 30 FPS game execution and smoother 60 FPS presentation remain the broader player goal; this focused change is worthwhile when it produces a repeatable improvement rather than claiming to solve the entire Deck bottleneck.

## Outcomes

- Prove an inline GPR stack span is ordinary MEM1 with one range-level eligibility decision.
- Reject misaligned, out-of-bounds, journaled, aliased, mirror-aliased, MMIO, and unsupported register ranges.
- Preserve the generated `mem_read32` / `mem_write32` operations and original translated fallback.
- Demonstrate a repeatable improvement over a matched control with all benchmark scenes loaded and guest checkpoints equal.
- Keep private AppImages, states, cards, logs, and disc-derived data outside the repository.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Guest correctness | Per-word readiness preserves aliases and special memory | No contract regression | Focused BlueWake and RecompCore component tests; matching tour checkpoints |
| Headless Deck throughput | Control artifact mean across five scenes | Repeatable positive change beyond run-order noise | Two paired rounds, opposite order |
| Rendered Deck throughput | Control artifact mean across five scenes | Repeatable positive change | Two paired rounds, opposite order |
| Player behavior | Existing island route | No observed gameplay or visual regression | Game Mode tour and session log |
| Upstream readiness | Candidate pins a fork-only RecompCore API | Current `main` base, upstream RecompCore dependency, CI | Open integration milestone |

## Clarifications

- The supplied handoff described an unmeasured hypothesis. The implementation and Deck evidence now exist, so this trio records the observed candidate rather than redesigning it.
- Aside from the GPR optimization, its RecompCore alias-range API, integration wiring and tests, the control and candidate source are identical. There is no unrelated source-code change confounding the comparison.
- The measured candidate artifact is faster, but its local PGO profile differs from the control. That profile can itself change because the GPR code changes what training observes, so the result qualifies the complete GPR candidate and its normal training outcome rather than isolating the direct source-level contribution.
- Runtime log counters printed `inline=0 fallback=0` during the player tour. Their relationship to build-time-generated inline sites remains unresolved and is not used as activation proof.
