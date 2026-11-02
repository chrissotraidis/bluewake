# TECH_PLAN — Profile the GX command stream in Outset

Status: Candidate under qualification (batch by default on Linux)  
Source: BlueWake `main` at `6b429fea9e60`
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## The path

| Stage | Where | Known |
|---|---|---|
| Gather-pipe check | `cmake/composite/gather_pipe.h` `bw_gather_pipe`, `bw_gather_pipe_put` | Batched into a buffer and flushed at `BW_GATHER_PIPE_BATCH`; alias test cached (#495) |
| Hand-off | `bw_gather_pipe_flush` → host `aurora_backend_gx_write` | 3.5 M cycles/frame in Outset |
| Front end | GXRuntime / Aurora GX command parsing | Not yet separated in the profile |
| Platform | `dol_platform_gx_write` | 0.2 M cycles/frame |

[#495's attribution](../2026-10-27-runtime_cpu_optimization/GATHER_ALIAS_CACHE.md) measured byte packing (0.14%) and the drain check (0.09%) as small. The open question is the host side, which has not been broken down since.

## Approach

1. **Stage-visible build.** Add a builder option (`scripts/linux/build.py`) that keeps the gather stages as profiler symbols, as #495's local attribution build did. Add line tables for the host's GX entry points, as #503 M1-T02 did for chunks.
2. **Outset profile.** Game Mode, worst-angle state, `perf record` on the game thread; normalize per game frame.
3. **Counters.** An opt-in counter for words, commands by kind and flushes per frame at the hand-off (a diagnostic build only).
4. **Second scene.** The same in a lighter scene (Hyrule Castle runs about 61 FPS rendered), to see what scales with draw count.
5. **Bound.** For the largest stage, a ceiling build (for example a batched hand-off, or skipping a redundant copy) A/B against the #503 stack.
6. **Candidate.** Only if the bound is consistent: an exact change with focused tests; pictures compared directly.

## Candidate: batch GX writes by default on Linux

**Problem.** In play, the module's gather pipe hands every written value to the host on its own: `bw_gather_pipe_put` calls `bw_gather_pipe_write`, which is `dol_platform_gx_write` → `aurora_backend_gx_write`. Each call checks seven flags before enqueuing the value for the GX worker. That's 3.4 M cycles per Outset frame, the GX stream's largest cost.

**What already exists.**
- The module can collect a run of bytes and hand it over in one call: `bw_gather_pipe_bytes` → `aurora_backend_gx_write_bytes` → `shadow_frontend_enqueue_bytes`.
- It drains the run before any hardware access and at every chassis boundary (`bw_gather_pipe_drain`), so the GX worker receives the same bytes in the same order.
- The host only installs this when `BLUEWAKE_GATHER_PIPE_BATCH=1` (`gather_pipe_bridge.h`), which nothing sets.

**Change.** On Linux, an unset `BLUEWAKE_GATHER_PIPE_BATCH` now means batch (`bluewake_gather_pipe_batch_option`); `=0` keeps direct writes. Windows and Mac are unchanged until measured there. Everything else that already turns the gather pipe off still does: FIFO traces, edge-census builds, `BLUEWAKE_GATHER_PIPE=0`, and modules without `--gather-pipe`.

**Why it should be exact.** The guest-visible FIFO stream is the same bytes in the same order; only how many host calls deliver them changes. The drain points keep anything that reads GX state (hardware registers, chassis boundaries) behind a delivered stream. Qualification still has to show it: identical guest checkpoints and identical or similar pictures in all five scenes, the bar #495 met.

**Measured.** #503's build, `BLUEWAKE_GATHER_PIPE_BATCH` unset against `=1`, twelve alternating Game Mode launches: **26.70 → 27.16 game FPS (+1.7%), ahead in 6 of 6 pairs**.

## Changes

| File | Change | Why |
|---|---|---|
| `scripts/linux/build.py` | `--gather-pipe-profile` adds module line tables and retains gather check, packing and drain boundaries; the host already builds with `-g` | Repeatable stage breakdown without changing normal builds |
| `cmake/composite/gather_pipe*.h` | Compile the selected stages `noinline` only in the diagnostic build | Give `perf` distinct symbols for validation, packing and drain work |
| Host GX entry (`aurora_backend_gx_write` and callees) | Opt-in per-kind counters | Attribution; off in shipped builds |
| `scripts/` (report) | Profile and counters → per-frame table | Repeatable breakdown |
| Tests | Counter on/off parity on a synthetic command stream | The shipped path stays unchanged |
| [`runtime/host/src/gather_pipe_bridge.h`](../../../runtime/host/src/gather_pipe_bridge.h), [`main.c`](../../../runtime/host/src/main.c) | `bluewake_gather_pipe_batch_option`: on Linux an unset `BLUEWAKE_GATHER_PIPE_BATCH` means batch | One host hand-off per run of bytes instead of one call per value: +1.7% |
| [`tests/gather_host_test.c`](../../../tests/gather_host_test.c), `linux/CMakeLists.txt`, `linux-host.yml` | The default per platform and the opt-out; the test now also runs in the Linux host job | Covers the Linux default in CI |
| `docs/WINDOWS.md` | Notes the Linux default and the opt-out | User-facing setting |

The diagnostic boundary work is implemented by
[`e2ed31c94`](https://github.com/irregularthanis/bluewake/commit/e2ed31c94).
It is opt-in and does not alter the default or release build path.

## Verification

| Layer | Required evidence |
|---|---|
| Profile | Stage symbols present; two runs of the same state within noise |
| Counters | Off by default; checkpoints identical with counters on |
| Candidate | Game Mode ≥ 6 interleaved launches ahead of the #503 stack; render_bench checkpoints identical and pictures identical/similar; unit tests in CI |

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Stage-visible Outset profile and per-frame table |
| M1 | M0 | Word and command counts; second scene |
| M2 | M1 | Ceiling for the largest stage; go/no-go |
| M3 | M2 | Exact candidate, tests and A/B |

## Gaps

- The renderer's own threads also consume the commands. A change that moves work off the game thread must not add waits elsewhere, so the GX worker and present waits in `[fps-dip]` must be checked alongside the frame rate.
- `DOL_AURORA_GPU_PROF` timestamps are unreliable with Smooth Motion's pass split (seen in #504 and #503), so they are not used here.

## References

- [#503 Outset game-thread follow-up](https://github.com/chrissotraidis/bluewake/pull/503) — the 5.1 M figure and method
- [#495 gather-pipe alias resolution](https://github.com/chrissotraidis/bluewake/pull/495) and its plan, [fork #5](https://github.com/irregularthanis/bluewake/pull/5): the module-side stage split this builds on
- `56dc5e5`: the host's batch opt-in, Oct 2
- [Graphics architecture](../../architecture/Graphics.md)
