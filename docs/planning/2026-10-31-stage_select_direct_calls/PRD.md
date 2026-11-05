# PRD — Stage-select direct-call cost

Status: Done (2026-11-05). Deck-qualified; the remaining work is in the follow-up draft PR
Source base: BlueWake [`ee991d3`](https://github.com/chrissotraidis/bluewake/commit/ee991d312d97e1c3e23168e00f1c67443465623e); discovery in [#503](https://github.com/chrissotraidis/bluewake/pull/503)
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: the speed cost of having the developer stage select enabled. Excluded: what the stage select does, game logic, and speed with it off.

## Overview

The stage select is BlueWake's warp menu between stages. It's off by default, but benchmarks and testers often enable it, and so does every warp-based measurement. Enabling it made the whole game slower, even when the menu was never opened: **−10 % on the Steam Deck**, the same build with it on against off.

This PR keeps the stage select working exactly as before and removes most of that cost: **−10.0 % → −3.6 %**. With the stage select off, nothing changes.

## The story

### What we started from

[#503](https://github.com/chrissotraidis/bluewake/pull/503) found the cost. With the stage select on, the host switched off the module's inline answer for direct calls (one translated function calling another). Every one of those calls then asked the host whether it could go ahead: two host functions, 6.2 % of the game thread.

### What we tried, and what we found

| Step | What changed | Stage select's cost (same build, on vs off) | Kept? | Why |
|---|---|---:|---|---|
| Control | — | −10.0 % | — | — |
| `f6bbf27a6` | The module learns the stage select's five hooks (v4), so the inline answer stays on | −6.4 % | Yes | +2.6 % with the stage select on, every launch ahead |
| `cde980cc3` | Test the address inline before the host's per-edge stage-select calls | — | **No** | +0.3 %, launches overlap: the host was never the cost |
| `8f7442ade` | Use v3 when the stage select is off | — | **No** | No effect |
| `f9d926907` | The five hooks become constants in the module | −2.3 % | Yes | +3.1 % with the stage select on, every launch ahead |
| `c48e83f0e` | The hooks ride in bits of a word every call already loads | −3.6 % | Yes | Keeps `f9d926907`'s gain without its cost with the stage select off |
| `b7079abce` | One bit test rules out most targets before the hook compares | not measured | Moved | To the follow-up PR, with the other remaining work |

Three findings shaped the result:

1. **The cost was in the game module, not the host.** After the first change, the profile showed the host's share of the game thread the same with the stage select on and off. The extra time was in the module, where every direct call walked the list of five watched addresses.
2. **The direct-call check isn't inlined.** `bw_direct_call_ready` is a function of its own in the module, so nothing about a call's target folds away at build time. Every instruction the check runs, every direct call pays: millions a frame. The winning changes make the check do less, not move it.
3. **Two builds trained separately differ by about 1.75 %.** Every round showed this PR about 1.5 % behind the control with the stage select **off**. Building the control's own source with this PR's optimization profile reproduced that gap (−1.7 %), and with the same profile this PR's code was level (+0.4 %). So it was training noise, not this PR.

### Decision

Keep `f6bbf27a6`, `f9d926907` and `c48e83f0e`. The stage select's five hooks are constants in the module, switched on by bits the host sets in the fast-path word every direct call already reads. There's no run-time watch list: a host that ever needs a watch the module doesn't know switches the inline path off, and every call asks it, which is exact.

## Impact

| Who | Before | After |
|---|---|---|
| Testers and benchmarks with the stage select on | −10.0 % for having it enabled | **−3.6 %** |
| Players with it off (the default) | — | Unchanged; this PR's code costs nothing there |
| Warp-based measurements | Pessimistic by ≈ 10 % | Pessimistic by ≈ 3.6 % |

**Long term:**

- **Measurements:** compare two builds only with the same optimization profile. Otherwise ≈ 1.75 % of training noise looks like a real gain or loss.
- **New stage-select hooks:** a new hook needs a new constant and bit (`direct_skip.h`); the host's static assertion and `test_direct_calls.py` fail if one is missing.
- **Other features:** the same pattern, fixed hooks carried in an already-loaded word, fits any feature that watches a few fixed guest addresses.

## Success and constraints

| Metric | Target | Status |
|---|---|---|
| Stage select's cost (same build, on vs off) | Well below the control's −10 % | **−3.6 %** (iteration 4) |
| Stage select off | No loss with the same profile | **+0.4 %**, within noise |
| Guest-visible execution | Identical checkpoints | Identical in every run of every round |
| Kept changes | Ahead in every one of six interleaved launches | Yes for all three |
| Older modules | Keep working | A module without v4 keeps the host query while the stage select is live |
| Stage-select behaviour | No regression | Hooks still return to the host; the rendered warp tour is still to run |
