# PRD — Profile the GX command stream in Outset

Status: Linux qualified: batching exact, +1.7% Game Mode, +1.2% rendered; on by default everywhere  
Source: BlueWake `main` at `6b429fea9e60`; follows [#503](https://github.com/chrissotraidis/bluewake/pull/503)  
Scope: the game-thread cost of turning the game's GX command stream into renderer work. Excluded: GPU time, Smooth Motion, the renderer's own threads.

## Problem

The game writes its graphics commands through the GX gather pipe ([Graphics: GX command path](../../architecture/Graphics.md#gx-command-path)). On the game thread, each write goes through:

1. the module's gather-pipe check and batching ([`gather_pipe.h`](../../../cmake/composite/gather_pipe.h)), whose alias test [#495](https://github.com/chrissotraidis/bluewake/pull/495) already caches;
2. the host's `aurora_backend_gx_write`, which hands commands to the GX front end.

In #503's Outset profile (Steam Deck, Game Mode, stage select off), this costs about **5.1 M cycles per game frame**, about 4% of the frame:

| Symbol | Side | M cycles/frame |
|---|---|---:|
| `aurora_backend_gx_write` | host | 3.5 |
| `bw_gather_pipe` | module | 1.4 |
| `dol_platform_gx_write` | host | 0.2 |

Outset draws about 1,960 draws per game frame. It is unknown whether this cost scales with draws, with command words, or with specific command kinds (vertex data, display lists, register loads).

## What we found (2026-11-02)

| Finding | Evidence | Consequence |
|---|---|---|
| The GX stream costs ≈ 5.4 M cycles per Outset frame, two thirds of it in the host's `aurora_backend_gx_write` | Stage-visible profiling build, Game Mode, worst-angle Outset | The host side is the target; the module's stages are small (matching the Oct 27 runtime CPU plan's split) |
| The host is called once per written value | Samples sit on the function's flag checks, entry and exit | Per-call overhead, not parsing |
| The batching path already exists but is off | `[chassis] gather-pipe=direct`; batch needs `BLUEWAKE_GATHER_PIPE_BATCH=1` (added Oct 2, `56dc5e5`, never qualified) | Turning it on is the candidate |
| **Batching: +1.7% game FPS, ahead in 6 of 6 pairs** | #503's build with and without the variable, twelve Game Mode launches | Candidate: on by default on Linux |

**Prior work:** the Oct 27 runtime CPU plan ([fork #5](https://github.com/irregularthanis/bluewake/pull/5)) built the first gather-pipe attribution and split the module's stages: alias validation 1.01%, packing 0.14%, drain 0.09%. Its winner, the alias cache, shipped as [#495](https://github.com/chrissotraidis/bluewake/pull/495) (+2.84%, checkpoints identical). It didn't cover the host side or the batch switch; this is that counterpart.

## Platforms

| Platform | Default (this PR) | How to compare | Measured |
|---|---|---|---|
| Linux desktop (Steam Deck) | On | Config → Performance switch, or `BLUEWAKE_GATHER_PIPE_BATCH=0` | **+1.7%** in Outset, 6 of 6 pairs |
| Windows | On | The same switch, or the variable | Pending |
| macOS | On (combined-optimization builds already batched, #292) | The same switch, or the variable | Pending |
| iOS / iPadOS | On (combined-optimization builds already batched) | `BLUEWAKE_GATHER_PIPE_BATCH=0` | Pending |
| Android | On | Display → "Batch Graphics Commands" | Pending |

Batching is on by default on every platform, so testers already run it; each platform records a with/without measurement (PROGRESS M4).

## Outcomes

- A breakdown of the game-thread GX cost by stage (module batching, host hand-off, command parsing) and by command kind, per game frame.
- Words and commands per frame, and cycles per word, for Outset and one lighter scene.
- A bounded candidate (a ceiling build first), or a recorded no-go.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---|---|---|
| Attribution | 5.1 M cycles/frame, by symbol only | ≥ 80% assigned to a stage and command kind | Profile with stage symbols visible, Outset state |
| Candidate gain | #503 stack on `main` | Ahead in every interleaved Game Mode launch (≥ 6); no minimum size | Game Mode A/B |
| Pictures and state | Control | Checkpoints identical; pictures identical or similar in all five render_bench scenes | render_bench |

## Possible follow-up: the module's per-store check

`bw_gather_pipe`, the test each store that leaves plain RAM makes (is it a gather-pipe write?), measured ≈ 1.4 M cycles per Outset frame (≈ 1%) in the profiling build. That figure is inflated: the profiling build keeps the check out of line (`noinline`), so each call pays a call, a return and register saves that the shipped build, where it is inlined, does not. It cannot go to zero either, since every hardware store must still be routed. A realistic gain is about 0.2–0.4%, and the change would touch a helper inlined into every store path, where #503's first candidate lost 2.9% to code layout.

**If picked up:** bound it first with a diagnostic build whose check keeps only the mirror-bit and FIFO-window comparisons (not exact), six interleaved Game Mode launches against batching on. Pursue an exact version only if the bound is consistent.

## Decisions

- **Batch by default on every platform (2026-11-02):** measured on Linux; on everywhere else pending each platform's measurement, so testers already run it. A menu switch (desktop Config → Performance, Android Display) or `BLUEWAKE_GATHER_PIPE_BATCH=0` keeps direct writes for comparison. Qualification: identical checkpoints and identical or similar pictures, the bar #495 met.
- **Profile before changing:** the first deliverable is the breakdown.
- **Make the attribution build public:** #495's attribution used a local-only build that keeps gather stages as profiler symbols. M0 adds that as a builder option, so others can repeat it.
- **Acceptance** follows #503's rule: consistent, exact, tested; size is not a bar.
- **One Performance section (2026-11-03):** the desktop menu's performance options sit together under Config → Performance: "Efficient soft effects" (#504, off by default) and the batching switch. This stacks #531 on #504, which merges first.
