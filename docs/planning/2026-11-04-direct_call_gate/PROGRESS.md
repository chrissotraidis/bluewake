# PROGRESS — Direct-call gate

Status: In progress. The gate is implemented, opt-in and unit-tested; the candidate build is next.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · Stacked on [#503](https://github.com/chrissotraidis/bluewake/pull/503)

Acceptance (#503's rule): the candidate ahead of the control in every interleaved Game Mode launch (at least six), checkpoints identical, zero unsafe gate answers under verify, focused tests in CI. No minimum gain.

## Milestones

### M0: Attribute the remaining cost

- [x] M0-T01: Profile the #437 build at Outset. The direct-call checks are 8.4% of samples, mostly the call's entry; 840 copies of each helper.

### M1: The gate (depends: M0)

- [x] M1-T01: Implement the gate, opt-in (`BLUEWAKE_DIRECT_GATE=1`), with the builder's `--direct-gate` for training.
- [x] M1-T02: Unit test: the gate against the full check in 1,024 states, verify counters, stale-byte detection, every path that closes it; -O0 and -O2. The host compiles cleanly in the build container.

### M2: Build and verify (depends: M1)

- [ ] M2-T01: Build the candidate: the #437 recipe plus this change, `--clang22 --direct-gate` (local `bench/503-gate`, not on the PR).
- [ ] M2-T02: `BLUEWAKE_DIRECT_SKIP_VERIFY=1` with the gate handed over, in Outset and a scene change: zero unsafe gate answers.

### M3: Measure (depends: M2)

- [ ] M3-T01: Desktop first read: instructions and cycles per frame against the `clang22` control.
- [ ] M3-T02: Steam Deck Game Mode A/B, at least six interleaved launches.
- [ ] M3-T03: render_bench, five scenes: checkpoints identical.

### M4: Decide (depends: M3)

- [ ] M4-T01: Keep opt-in, propose as the default, or no-go (code removed, record kept).

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | #437 build `clang22` (`perf/clang-runtime-matrix@14c558d`, Clang 22.1.8, libc++); local profile and attribution (private) | `perf record -e cycles:u -F 4000`, headless, unpaced, 2,400 retraces from `1-outset.bwstate`; symbols grouped by layer | Desktop PC (Ryzen 7 5800X3D), Linux | Direct-call checks **8.4%** of all samples (translated chunks 58.6%, natives 12.2%, dispatch loop 5.0%); in `bw_direct_call_ready_plain` most samples sit on the entry and the `bw_host_skip_fast` load; 840 copies of each helper in the module |
| M1-T02 | This branch | `tests/direct_calls_test.c` at -O0 and -O2; host `main.c` syntax check in `bluewake-linux-ci:local`; `tests/test_linux_ci_container.py` | Desktop PC | All pass; the host compiles with no warnings |
