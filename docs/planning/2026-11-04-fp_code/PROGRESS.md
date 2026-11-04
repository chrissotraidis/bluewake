# PROGRESS — Floating-point code in the translated game

Status: M1 implemented, opt-in and unit-tested; the measurement build is next.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

Acceptance: the candidate ahead of the control in every interleaved Game Mode launch (at least six), checkpoints identical, focused tests in CI. No minimum gain.

## Milestones

### M1: One FP-available test per stretch of a prepaid copy

- [x] M1-T01: Attribute the FP cost: 36% of the hottest chunks' time; one `ps_madd` is about 117 x86 instructions; FP loads and stores aren't lean.
- [x] M1-T02: `fp_checks.py`, the builder's `--fp-checks`, the unit test (in the audit job).
- [x] M1-T03: On the #437 prepared source: 296,188 of 368,745 tests removed; the hottest J3D chunk compiles cleanly.
- [ ] M1-T04: Build the candidate on the #437 recipe with `--fp-checks` (local bench branch, not on the PR).
- [ ] M1-T05: Desktop first read against the `clang22` control.
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
