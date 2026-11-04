# PROGRESS — The direct-call gate and FP checks on Windows

Status: **wired and tested on Linux CI; unmeasured on Windows.** fp_checks and the direct-call gate are both Windows builder defaults, following #552. `BLUEWAKE_DIRECT_GATE=0` closes the gate.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · Builds on [#552](https://github.com/chrissotraidis/bluewake/pull/552) and [#553](https://github.com/chrissotraidis/bluewake/pull/553)

## Story

**What we tried.** On Linux, #552 and #553 together made the Steam Deck about 8.5% faster. Their code already compiles into the Windows module, but the Windows builder didn't know their options: it never ran fp_checks, and it trained with the gate closed.

**What we found.** The Windows builder runs the same passes, in the same order, from the same scripts. The gate's host code has nothing Linux-specific in it: the union has byte fields and no bit-fields, the word read has no atomics, and the export is `dllexport`.

**Decision.** Mirror the Linux wiring exactly. fp_checks is on by default, as on Linux. At first the gate stayed opt-in. When #552 made it a Linux default (`ed834a53f`), Windows followed, so the two builders stay one recipe. `BLUEWAKE_DIRECT_GATE=0` is the A/B switch.

**Long-term impact.** All three desktop builders (Linux, Windows, and then macOS) use one recipe for these passes. Any later tuning applies to all of them.

## Milestones

### M1: Wiring

- [x] M1-T01: `scripts/windows/build.py`: the pass, the cache keys, provenance, `--fp-checks`, `--no-fp-checks` and `--direct-gate`.
- [x] M1-T02: `tests/test_windows_perf_options.py` in `audit.yml`. The existing Windows and fp_checks tests still pass.
- [x] M1-T04: The gate on by default, matching #552: `--no-direct-gate`, `-DBLUEWAKE_DIRECT_GATE_DEFAULT`, and the tests updated (8 tests).
- [x] M1-T03: A static portability review of the gate's host code (see the technical plan).

### M2: Windows verification

- [ ] M2-T01: Windows CI build green.
- [ ] M2-T02: Windows checkpoints identical, with and without fp_checks, and with the gate on and off.
- [ ] M2-T03: A Windows A/B, at least six interleaved launches. Keep only consistent gains.

### M3: Decide

- [ ] M3-T01: Keep or revert per platform, recorded here.
