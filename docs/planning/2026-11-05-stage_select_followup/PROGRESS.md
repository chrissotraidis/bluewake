# PROGRESS — Stage-select cost: follow-up

Status: Draft; nothing measured yet
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Tasks

- [x] T01: Slot filter in `direct_calls.h` and `direct_skip.h`, unit-tested at -O0 and -O2 (from #524's `b7079abce`, whose Deck run was stopped)
- [ ] T02: Deck 4-way bench: this PR on/off, #524 on, control off; keep only if every launch is ahead
- [ ] T03: Stage-select-on profile check: control source with #524's profile vs #524, both on
- [ ] T04: Rendered warp tour and stage-select behaviour (menu, warp, file select, music)
- [ ] T05: Decide: stop if ≤ ≈ 1.5 % remains; otherwise profile what's left

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | Split from #524 so #524 ships only measured changes |
