# PROGRESS — The draw side of the game thread

Status: Planned — follows #592
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Measured on #592
- [x] M0-T01: Busiest view against the light one, by game system (#592 M1-T03): draw side 19.9 → 26.0 M cycles a frame, flat (no function above 0.95 M)
- [x] M0-T02: Single natives priced (#592 M3-T02): the key-frame native's coverage worth ~0.15–0.25 %; no single draw-side function is measurable

### M1: Unblock the native sets (#451)
- [ ] M1-T01: Reproduce #451 on the current tree (both generators, default build)
- [ ] M1-T02: The `_ato` forms in `native_leaf_gen.py` and `native_gx_gen.py`, with the out-of-line flag's accounting reproduced
- [ ] M1-T03: Leaf boundaries from the current prepared-block layout; the comparison tests pass against the current module
- [ ] M1-T04: Round 5 (GX FIFO writers): build, measure (`8-busiest` + bench scenes, 6 launches each way, checkpoints identical), keep or remove
- [ ] M1-T05: Round 4 (animation, actor collision, environment light): the same
- [ ] M1-T06: Round 7 (libm, rotations, geometry, collision blocks, JASystem): the same

### M2: The API-level option (only if M1 leaves a gap)
- [ ] M2-T01: Draw-side profile with M1's sets on; pick the largest remaining path
- [ ] M2-T02: Prototype plan for that path, for the owner's decision

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M0-T01 | #592 `scripts/guest_profile.py` / `guest_profile_systems.py` outputs | Named perf profiles, headless | Build `eaa85f0f1`, `6-play` and `8-busiest` | Draw side 19.9 → 26.0 M cycles a frame |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-10 | Owner: plan the structural draw-side work separately from #592. Phase 1 is #451 (Elliott's existing, exact native sets, blocked on the current tree); drawing at the API level only if a gap remains |
