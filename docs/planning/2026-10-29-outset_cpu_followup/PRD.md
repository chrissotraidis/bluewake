# PRD — Outset game-thread follow-up

Status: Stack 1 retained (+1.4–1.8% Game Mode, +1.6% rendered, exact); M4 (interrupt test) closed as no-go; ready for review; follow-ups #529–#531  
Source: BlueWake `main` at `a7731853c197cc9763a86d62e731d65ca0b25556`; measured baseline is the combined [#480](https://github.com/chrissotraidis/bluewake/pull/480) → [#495](https://github.com/chrissotraidis/bluewake/pull/495) → [#501](https://github.com/chrissotraidis/bluewake/pull/501) stack, re-measured on a control build of [#504](https://github.com/chrissotraidis/bluewake/pull/504) + `main` 0.10.0.  
Scope: the CPU cost of Outset's game thread on the Steam Deck. Excluded: Smooth Motion and GPU pacing ([#504](https://github.com/chrissotraidis/bluewake/pull/504)), and build-recipe changes.

## Problem

Outset is the slowest remaining scene on the Steam Deck. It runs at **about 26 game FPS against the game's 30**, with the game thread busy 98–99% of the time. The GPU, the GX worker and presentation all wait very little, so the limit is the CPU work the game thread does in each frame:

| | Game-thread cycles per frame | Game FPS |
|---|---:|---:|
| Outset today (default settings, Game Mode) | ≈ 129–131 M | ≈ 26 |
| What 30 FPS allows at the measured 3.44 GHz | ≈ 113 M | 30 |
| **Gap** | **≈ 16–18 M (about 13%)** | |

Nothing obvious is wrong:

- **Not power-limited:** the game thread holds 3.44 GHz, unlike the Lava Room in #504.
- **No single hot spot:** the largest game function is ≈ 2.3 M cycles per frame.
- **More of everything:** compared with the open ocean at the same settings (a steady 30 FPS), Outset's extra ≈ 28 M cycles are spread over collision, skinning, actor checks and the runtime's own per-call costs.

So the 13% has to come from costs every scene pays, removed one at a time without changing what the game does.

## What we found

| Finding | Evidence | Consequence |
|---|---|---|
| Outset is not power-limited | Game thread at 3.44–3.45 GHz, APU under 15 W | CPU work, not clocks, is the lever |
| Smooth Motion costs ≈ 5% of game-thread IPC | 25.1 → 23.9 game FPS with it on | Every comparison fixes this setting |
| The developer stage select costs ≈ 11% | 23.6 → 26.1 game FPS with it off | Off by default; baselines use it off. Its fix is out of scope (handoff written) |
| No game function dominates | Top: `calcWeightEnvelopeMtx` 2.1%, then collision checks ≤ 1.5% | Hooking one game function cannot close the gap; [#436](https://github.com/chrissotraidis/bluewake/pull/436) showed a collision hook can even be slower |
| Outset is "more of everything" than the ocean | 131 M vs 103 M cycles per frame | Target costs every scene pays |
| The runtime's shared costs are ≈ 44 M per frame, ≈ 8–12 M of it removable | Survey of runtime and host symbols | Worth 6–9%: several changes, not one |
| The direct-call check is the largest removable piece | Ceiling test: +3.1% (Game Mode), +4.5% (rendered) | First candidate |
| An exact smaller check gains +0.7%; with the actor-search lookup inlined, +1.8% | Game Mode A/B/A against the control | Retained; most of the ceiling's gain was the interrupt test it skipped |

Details and raw numbers are in the [tracker's evidence ledger](PROGRESS.md#evidence-ledger).

## Outcomes

- A fixed Outset control and a measured noise level, before any candidate is judged.
- Game-thread time attributed to game functions, runtime costs and host work.
- One measured cost removed at a time, with guest checkpoints and pictures unchanged.
- A no-go recorded for any candidate that is inside noise, moves cost elsewhere, skips work, or slows another scene.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---:|---:|---|
| Outset game FPS | ≈ 26.4 (worst-angle state, Game Mode) | A consistent gain: the candidate ahead of the control in every interleaved launch (at least six). No minimum size; small gains are kept when exact | Game Mode launches alternating control and candidate, plus rendered A/B/B/A where available |
| Game-thread cycles per frame | ≈ 129 M | Lower in proportion; the targeted cost visibly smaller | Same-window `perf stat` and samples |
| Guest behaviour | Control checkpoints | Identical at every checkpoint, every scene | Fixed-state benchmark comparison |
| Other scenes | Five-scene results | No slowdown beyond their spread | Full rendered five-scene tour |
| Player experience | ≈ 26 FPS in Game Mode | No new visual, input, audio or timing issue | Outset walk on a first-revision Deck |

A candidate may help Outset without reaching 30 FPS: consistency and exactness decide retention, not size (2026-11-01: gains under 1% are kept when they are consistent, exact and covered by tests). Control and candidate share compiler, optimization profile, flags, state and settings. Private disc, state and module artifacts stay off GitHub.

## Decisions

- **Stage select fix dropped:** a debugging tool, off by default. Its handoff is kept outside this PR.
- **Candidates must be exact:** the unsafe ceiling changed when interrupts are taken, so its guest state drifted. A shippable change keeps every runtime test.
- **Retain at +1.8% (2026-10-31):** slightly under the 2% target, but exact and beyond noise. The interrupt test is the next, separate change.
- **One PR:** the code is stacked on this PR, as #504 did. [#528](https://github.com/chrissotraidis/bluewake/pull/528) was closed.
- **M4 (interrupt test) closed as no-go:** even a perfect host word gained +0.3%, inconsistently. The first ceiling's gain came mostly from taking interrupts late, which no exact change can keep ([outcome](TECH_PLAN.md#m4-the-interrupt-test-specification-closed-as-no-go)).
- **Follow-ups (2026-11-01):** [#529](https://github.com/chrissotraidis/bluewake/pull/529) training on Outset, [#530](https://github.com/chrissotraidis/bluewake/pull/530) dispatch-loop edges, [#531](https://github.com/chrissotraidis/bluewake/pull/531) GX command stream.
- **Platform:** first-revision Steam Deck, current SteamOS, Game Mode, render scale 3.
