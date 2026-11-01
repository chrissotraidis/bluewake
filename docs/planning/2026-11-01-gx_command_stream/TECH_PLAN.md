# TECH_PLAN — Profile the GX command stream in Outset

Status: Planning  
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

## Changes (planned)

| File | Change | Why |
|---|---|---|
| `scripts/linux/build.py` | Option to keep gather stages visible to the profiler (attribution only) | Repeatable stage breakdown |
| Host GX entry (`aurora_backend_gx_write` and callees) | Opt-in per-kind counters | Attribution; off in shipped builds |
| `scripts/` (report) | Profile and counters → per-frame table | Repeatable breakdown |
| Tests | Counter on/off parity on a synthetic command stream | The shipped path stays unchanged |
| Candidate files | Decided by M2 | — |

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
- [#495 gather-pipe alias resolution](https://github.com/chrissotraidis/bluewake/pull/495)
- [Graphics architecture](../../architecture/Graphics.md)
