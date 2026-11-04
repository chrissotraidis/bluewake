# PROGRESS — Floating-point code in the translated game

Status: M1 desktop first read **−3.1% cycles per frame, rate +3.2%**; guest state identical (save states byte-identical; the checkpoint's CPU hash differs only in block-scratch `cycle_observation_suffix`, as #481 documents). M2 (`lfs` zeros) implemented. Deck render_bench next.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

Acceptance: the candidate ahead of the control in every interleaved Game Mode launch (at least six), checkpoints identical, focused tests in CI. No minimum gain.

## Milestones

### M1: One FP-available test per stretch of a prepaid copy

- [x] M1-T01: Attribute the FP cost: 36% of the hottest chunks' time; one `ps_madd` is about 117 x86 instructions; FP loads and stores aren't lean.
- [x] M1-T02: `fp_checks.py`, the builder's `--fp-checks`, the unit test (in the audit job).
- [x] M1-T03: On the #437 prepared source: 296,188 of 368,745 tests removed; the hottest J3D chunk compiles cleanly.
- [x] M1-T04: Build the candidate on the #437 recipe with `--fp-checks` (local `bench/fp-checks` `6079b278f687`, not on the PR): 296,188 tests removed, and lean memory now makes 708,394 accesses lean (531,009 in the control: **+177,385 FP loads and stores**).
- [x] M1-T05: Desktop first read against the `clang22` control: **−2.6% instructions, −3.1% cycles per frame, rate +3.2%**, all three runs ahead. Guest state identical: save states byte-identical at retrace 11700 (the first checkpoint that differs) and 12300; guest RAM and cycles equal at every checkpoint; the CPU hash differs only because FP accesses no longer store the block-scratch `cycle_observation_suffix` (#481: scratch inside one block, not guest state; lean memory's own acceptance recorded the same difference).
- [ ] M1-T06: Steam Deck render_bench (five scenes, checkpoints) and Game Mode A/B (at least six launches).
- [ ] M1-T07: Decide: keep opt-in, propose as the default, or no-go.

### M2: Zeros through the hardware's widening in `lfs` (depends: M1)

- [x] M2-T01: Check every pattern: under the host's FTZ and DAZ (set in non-IEEE mode) the hardware gets all 16,777,214 denormals wrong, so only zeros join the fast path.
- [x] M2-T02: The change and `tests/f32_from_bits_test.c` with DAZ off and on: 0 mismatches in 2³² patterns each.
- [ ] M2-T03: Build M1 + M2 and measure against M1 (desktop, then Deck render_bench).

### M3–M5

Planned after M1 and M2 ([PRD](PRD.md#steps)).

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M1-T01 | #437 build `clang22` (`14c558d`, Clang 22.1.8, libc++); local profile, six hottest chunks recompiled with `-gline-tables-only` (function sizes identical to the module's) | `perf record -e cycles:u -F 4000`, headless, unpaced, 2,400 retraces from `1-outset.bwstate`; samples resolved to guest instructions | Desktop PC (Ryzen 7 5800X3D) | FP arithmetic 22.4%, FP loads and stores 14.0% of the hottest chunks' time; top FP opcodes `lfs` 7.1%, `fcmpo` 6.3%, `ps_madds0` 5.0%; one `ps_madd` ≈ 117 x86 instructions |
| M1-T03 | This branch, on the `gate503` build's prepared source | `fp_checks.transform` over every chunk; the J3D chunk `chunk_0187_text1_802ED6E0.c` compiled with the build's command in `bluewake-linux-ci:local` | Desktop PC | 296,188 of 368,745 tests removed in 811 chunks; compiles with no errors or warnings |
| M2-T02 | This branch | `tests/f32_from_bits_test.c` (its header's command, with the `clang22` build's prepared headers), default MXCSR and FTZ + DAZ | Desktop PC | 4,294,967,296 patterns each, 4,261,412,866 on the fast path, **0 mismatches** in both modes; a standalone check found 16,777,214 denormals the hardware widens to zero under DAZ |
| M1-T04 | `fp553` (`bench/fp-checks` `6079b278f687` = `14c558d` + M1; Clang 22.1.8, libc++, `fp_checks: true`, trained with it) | `build_script fp553 bench/fp-checks … --clang22 --fp-checks` | Desktop PC | `FP-available tests removed: 296188 in 811 chunks`; `lean memory accesses: 708394 in 842 chunks` (control: 531009) |
| M1-T05 | Control `clang22` (A) against `fp553` (B); local `measure-553/` (private) | `perf stat`, headless, 2,400 retraces from `1-outset.bwstate`, A B B A A B; `BLUEWAKE_SAVE_STATE=…@11700` and `@12300` from each build, compared with `cmp` | Desktop PC | Per game frame: A 76.05 / 76.00 / 75.82 M cycles, 210.35 M instructions; B 73.61 / 73.74 / 73.48 M cycles, 204.82 M instructions (**−3.1% cycles, −2.6% instructions**); rate 117.2–117.4 → 120.8–121.3 (**+3.2%**). Checkpoints: cycles and guest RAM equal at all eight, CPU hash different from retrace 11700; save states **byte-identical** at 11700 and 12300 |
