# PRD — Outset game-thread follow-up

Status: Draft  
Source: BlueWake `main` at `a7731853c197cc9763a86d62e731d65ca0b25556`; measured baseline is the combined [#480](https://github.com/chrissotraidis/bluewake/pull/480) → [#495](https://github.com/chrissotraidis/bluewake/pull/495) → [#501](https://github.com/chrissotraidis/bluewake/pull/501) stack at `2749e6a194dcdba93846ebd925a33c4f95c695ab`.  
Scope: identify and improve the remaining Outset CPU bottleneck on Steam Deck; excluded: Smooth Motion/GPU pacing and unrelated build-recipe changes.

## Motivation

The requested outcome is smoother Steam Deck play in Outset, the slowest remaining scene after the current optimization stack. The controlled rendered benchmark reaches 26.971 FPS, while live Game Mode reports 24–26 FPS with the game thread at about 98% busy. GPU, GX-worker and presentation waits are small, so this is a broad CPU-side game-thread problem rather than the alias lookup fixed by #501.

The profile is flat: `func_802456E0` is about 3.6%, `bluewake_native_game_math_try` 2.4%, `func_802ED6E0` 2.1% and `func_800A56E0` 2.1%. The native game-math, math and vector routing checks total about 4.9%. No single item is yet proven to be the correct optimization.

## Outcomes

- Reproduce Outset from the same private state on the #501 stack and establish A/A noise before selecting a candidate.
- Attribute game-thread time by translated guest range, host/native routing, dispatch, services and GX submission; unknown chunks must be mapped to verified decomp symbols.
- Change one measured cost, preserve guest checkpoints and rendered output, then compare it against the unchanged control.
- Reject an optimization that is inside noise, shifts cost elsewhere, skips work, or regresses another benchmark scene.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---:|---:|---|
| Outset rendered FPS | 26.971 on the combined stack | Improvement greater than both 2% and the measured A/A spread | Interleaved rendered A/B/B/A, at least 5 pairs after warm-up |
| Game-thread cycles/frame | About 129 M | Lower in proportion to FPS; selected cost visibly reduced | Same-window `perf stat` and sampled call graph |
| Guest behavior | #501 checkpoints | Identical at every checkpoint | Fixed-state benchmark comparison |
| Other scenes | #501 five-scene results | No slowdown beyond their A/A spread | Full rendered five-scene tour |
| Player experience | 24–26 FPS in Game Mode | No new visual, input, audio or timing issue | Outset walk/camera tour on first-revision Deck |

The candidate may improve Outset without reaching 30 FPS; relative improvement and correctness decide retention. Builds must use the same compiler, PGO inputs, flags, state, settings and #501 dependency pins. Private disc, state and translated-module artifacts stay off GitHub.

## Clarifications

- **Stage select fix dropped (2026-10-31):** the stage select is a debugging tool, off by default; its cost is not worth optimizing. Baselines use it off.
- **The developer stage select costs about 11% (2026-10-31):** with `BLUEWAKE_STAGE_SELECT_TOGGLE` on, the module's inline direct-call skip is disabled and every direct guest call asks the host; Outset runs 23.6 game FPS with it on and 26.1 off. It is off by default, so a default player's Outset baseline is about 26 FPS and the gap to 30 is about 13%. Keeping the fast path with the stage select on is a cheap candidate for players who use it; every comparison must fix this setting.
- **Game Mode baseline (2026-10-31):** Outset is not power-limited (game thread at 3.45 GHz, APU under its 15 W cap). Smooth Motion costs the game thread about 5% through shared cache and memory (IPC 1.74 off, 1.65 on; 25.1 vs 23.9 game FPS), more than the 2% acceptance threshold, so every A/A and A/B must fix the Smooth Motion setting. Either way the game thread does 17–20% too much work per frame for 30 FPS (≈ 138–145 M cycles against ≤ 115 M).

- Motivation and platform are confirmed by the requested #501 live-game follow-up: first-revision Steam Deck, current SteamOS, Outset below its 30 FPS game target.
- Candidate selection remains open until M0/M1 attribution. The rejected partial `GroundCrossGrpRp` hook from [#436](https://github.com/chrissotraidis/bluewake/pull/436) is evidence against repeating that exact implementation, not evidence against all work in the surrounding chunk.

