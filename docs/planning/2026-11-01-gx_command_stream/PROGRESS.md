# PROGRESS — Profile the GX command stream in Outset

Status: M3: batch by default on every platform (`d9556a4`); Linux checkpoints and pictures pending; other platforms pending measurement  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Stage-visible profile

- [x] M0-T01: Builder option keeping gather stages visible to the profiler; builder test; build with host line tables.
- [x] M0-T02: Outset per-frame table by stage (Game Mode, worst-angle state). The host's per-value `aurora_backend_gx_write` is two thirds of the cost; the module's batching path is installed but off (`[chassis] gather-pipe=direct`).

### M1: Counts (depends: M0)

- [ ] ~~M1-T01/T02~~: Skipped: M0 showed the cost is per-call overhead, and an existing path removes it.

### M2: Bound (depends: M1)

- [x] M2-T01: Ceiling for the largest stage; Game Mode A/B against the #503 stack; go/no-go. The existing batch path itself: +1.7%, ahead in 6 of 6 pairs.

### M3: Candidate (depends: M2)

- [x] M3-T01: Exact change with focused tests. Batch by default on Linux (`4abfb50`); `gather_host_test` covers the default and opt-out and now also runs in the Linux job.
- [ ] M3-T02: Game Mode (≥ 6 interleaved launches), render_bench checkpoints and pictures; retain or record no-go. Game Mode done (+1.7%); render_bench checkpoints and pictures pending.

### M4: Other platforms (on by default everywhere, pending measurement)

Batching is on by default on every platform, so testers already run it. Each platform records a with/without A/B (the menu switch, or `BLUEWAKE_GATHER_PIPE_BATCH=0`), Outset from the worst-angle state, at least six interleaved launches, pictures compared.

- [ ] M4-T01: Windows.
- [ ] M4-T02: Android (also confirms the new Display toggle builds and works).
- [ ] M4-T03: macOS and iOS.

### Possible follow-up (not scheduled)

- [ ] The module's per-store check (`bw_gather_pipe`, ≈ 1.4 M cycles/frame in the profiling build, inflated by `noinline`; realistic ≈ 0.2–0.4%): bound it with a diagnostic build first ([technical plan](TECH_PLAN.md#possible-follow-up-the-modules-per-store-check)).

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | [`e2ed31c94`](https://github.com/irregularthanis/bluewake/commit/e2ed31c94) | `python3 tests/test_linux_ci_container.py`; `git diff --check`; `scripts/audit_repo.sh` | Source-only Linux builder contracts; no game data | PASS: 20 tests; diagnostic flag requires gather pipe, default stays inline; audit passed |
| M0-T01 (review) | `e2ed31c` reviewed; fix in this commit | Code review; `python3 tests/test_linux_ci_container.py` | Source only | The option works as described (stages kept as profiler symbols, refused without `--gather-pipe`, off by default). Fixed: the training fingerprint ignored `--gather-pipe-profile`, so a profiling build could reuse a normal build's counts for differently shaped code (#530 measured that mismatch as −4.9%); new test. Noted: line tables cover the module only; the host stage (`aurora_backend_gx_write`) resolves by symbol |
| M0-T02 | Profiling build `ab39a11` (#503 stack + #531 with `--gather-pipe-profile`, full build with its own training); Deck `~/m531/perf.data` (private) | `perf record -F 2999 -t <game thread>` 30 s, `perf stat` 10 s; symbols from the build's own module and host | Game Mode, worst-angle Outset state, 16:9, widescreen mod; ≈ 26 game FPS, ≈ 131 M cycles per frame | **GX stream ≈ 4.1% of the game thread, ≈ 5.4 M cycles per frame:** host `aurora_backend_gx_write` 2.61% (3.4 M); module `bw_gather_pipe` (the per-store check) ≈ 1.05% (1.4 M); `bw_gather_pipe_put` ≈ 0.20%; `bw_gather_pipe_drain` ≈ 0.08%; `dol_platform_gx_write` 0.17%. Inside `aurora_backend_gx_write` the samples sit on its flag checks and prologue/epilogue, not on parsing: it is called **once per written value**. The host log shows `[chassis] gather-pipe=direct`: the batching path (`aurora_backend_gx_write_bytes`, one enqueue per run of bytes) exists but needs `BLUEWAKE_GATHER_PIPE_BATCH=1`, which nothing sets; it was never qualified (`docs/WINDOWS.md`) and never measured on the Deck |
| M2 (bound) | #503 stack (`cand-503`, `274b6290a1e2`) with and without `BLUEWAKE_GATHER_PIPE_BATCH=1` (runtime only); local `measure-531/batch/` (private) | `gamemode_measure.py` 3 × 15 s, twelve launches alternating (six pairs); host log confirms `gather-pipe=batch` / `direct` for every launch | Game Mode, worst-angle Outset state, 16:9, widescreen mod | **Batch +1.7% (26.70 → 27.16 game FPS), ahead in 6 of 6 pairs** (by 0.24–0.57 FPS). The existing path, switched on. Not yet qualified: pictures and guest checkpoints must match before it can be the default |
