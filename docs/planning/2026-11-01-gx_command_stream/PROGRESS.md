# PROGRESS — Profile the GX command stream in Outset

Status: In progress
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Stage-visible profile

- [x] M0-T01: Builder option keeping gather stages visible to the profiler; builder test; build with host line tables.
- [ ] M0-T02: Outset per-frame table by stage (Game Mode, worst-angle state).

### M1: Counts (depends: M0)

- [ ] M1-T01: Opt-in word, command-kind and flush counters at the hand-off; parity test.
- [ ] M1-T02: Outset and a lighter scene; cycles per word and per command.

### M2: Bound (depends: M1)

- [ ] M2-T01: Ceiling for the largest stage; Game Mode A/B against the #503 stack; go/no-go.

### M3: Candidate (depends: M2)

- [ ] M3-T01: Exact change with focused tests.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints and pictures; retain or record no-go.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | [`e2ed31c94`](https://github.com/irregularthanis/bluewake/commit/e2ed31c94) | `python3 tests/test_linux_ci_container.py`; `git diff --check`; `scripts/audit_repo.sh` | Source-only Linux builder contracts; no game data | PASS: 20 tests; diagnostic flag requires gather pipe, default stays inline; audit passed |
| M0-T01 (review) | `e2ed31c` reviewed; fix in this commit | Code review; `python3 tests/test_linux_ci_container.py` | Source only | The option works as described (stages kept as profiler symbols, refused without `--gather-pipe`, off by default). Fixed: the training fingerprint ignored `--gather-pipe-profile`, so a profiling build could reuse a normal build's counts for differently shaped code (#530 measured that mismatch as −4.9%); new test. Noted: line tables cover the module only; the host stage (`aurora_backend_gx_write`) resolves by symbol |
