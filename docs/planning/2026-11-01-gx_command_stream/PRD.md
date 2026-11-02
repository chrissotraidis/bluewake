# PRD — Profile the GX command stream in Outset

Status: Candidate under qualification: batch GX writes by default on Linux (+1.7% in Outset)  
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

## Decisions

- **Batch by default on Linux (2026-11-02):** only where it was measured; `BLUEWAKE_GATHER_PIPE_BATCH=0` keeps direct writes; Windows and Mac unchanged until measured there. Qualification: identical checkpoints and identical or similar pictures, the bar #495 met.
- **Profile before changing:** the first deliverable is the breakdown.
- **Make the attribution build public:** #495's attribution used a local-only build that keeps gather stages as profiler symbols. M0 adds that as a builder option, so others can repeat it.
- **Acceptance** follows #503's rule: consistent, exact, tested; size is not a bar.
