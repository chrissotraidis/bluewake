# PRD — Lava Room Smooth Motion follow-up

Status: M0 and M1 done; M2 implemented and measured across five rendered scenes; visual qualification remains open\
Source: BlueWake `main` at `a7731853c197cc9763a86d62e731d65ca0b25556`; measured on the [#480](https://github.com/chrissotraidis/bluewake/pull/480) → [#495](https://github.com/chrissotraidis/bluewake/pull/495) → [#501](https://github.com/chrissotraidis/bluewake/pull/501) stack and [RecompCore #49](https://github.com/chrissotraidis/RecompCore/pull/49)\
Scope: why Smooth Motion stalls in the Lava Room on a Steam Deck, and a fix that keeps the picture and the game speed; excluded: Outset's game-thread limit ([#503](https://github.com/chrissotraidis/bluewake/pull/503))

## Motivation

On a Steam Deck in Game Mode, Smooth Motion in the Lava Room either pauses its in-between frames or, forced on, slows the game by about 10%. The overlay only says "paused", the benchmark disables interpolation, and nothing showed which part of the system gave way. The player's settings are render scale 3 with 2× anisotropic filtering, HD look off and no texture pack.

### From symptom to cause

Each step below ruled something out before the next was measured; the evidence is in the [tracker](PROGRESS.md#evidence-ledger).

| Step | What was measured | Result | Conclusion |
|---|---|---|---|
| 1 | Game Mode: Smooth Motion off, automatic pacing, never pause | Off and automatic hold full speed; never pause costs ≈ 10% | Pacing protects the game as designed |
| 2 | Typed overload causes and per-frame outcomes ([RecompCore #49](https://github.com/chrissotraidis/RecompCore/pull/49)) | 0 frames rejected; drops on late drawables and render-worker waits | Not the content verdict |
| 3 | FIFO against Mailbox presentation | Headless: waits vanish. Game Mode: unchanged | Not the present mode (headless misled) |
| 4 | GPU clock fixed at 1600 MHz | No GPU overloads, yet the game slower | Not GPU capacity |
| 5 | Game-thread profile, pinning, synthetic memory load | Same work, lower IPC; a memory load costs 38% | Memory contention is real but small (≈ 3%) |
| 6 | Game thread, power and clocks, same session | IPC unchanged, **clock 3.45 → 3.1 GHz**, chip at its **15 W cap** | **The shared power budget** |
| 7 | GPU time by pass, then by draw range, at the player's settings | One pass is 95% of the frame; ≈ 80 draws are ≈ 70% of it | **The cost is a few draws** |
| 8 | Those draws hidden, captured and compared | Pass 21.3 → 6.5 ms; they are the room's heat haze | **The heat haze** |
| 9 | Haze hidden, Smooth Motion on | 60 FPS, full speed, under the cap | Fixing the haze fixes Smooth Motion here |

### The cause

The Lava Room's heat haze is about 80 large see-through quads drawn over the scene. At the game's own 640×480 they are cheap. At render scale 3, each one blends over millions of pixels, several layers deep:

| Game Mode, scale 3 | Haze drawn | Haze hidden |
|---|---|---|
| Real frame, GPU | ≈ 23 ms | ≈ 8 ms |
| In-between frame, GPU | ≈ 17.5 ms | ≈ 9 ms |
| APU power, Smooth Motion on (cap 15 W) | 14.1–16.0 W | 12.8–13.8 W |
| Game-thread clock, Smooth Motion on | ≈ 3.1 GHz | 3.45 GHz |
| Game speed, Smooth Motion on | 83–87% | ≈ 100% (60 FPS) |

The CPU and GPU share one 15 W budget. Drawing the haze twice per game frame (real and in-between) keeps the GPU at the cap, so the CPU clock gives way and the game thread falls behind. The effect is subtle: the player saw no difference with it hidden, though nearly every pixel changes slightly.

The same pattern is the largest cost in the Shipwreck (≈ 62% of its GPU time at scale 3). In Outset, similar sprites (likely clouds) are interleaved with terrain, and their own cost is not yet separated.

### Proposed solution: Efficient soft effects

Draw each run of these quads into a buffer of half the frame's size, and blend that buffer over the frame once, in the same place in the draw order. The buffer accumulates the quads exactly as they would have blended onto the frame, so the result differs only in resolution: a soft veil loses nothing visible at half resolution. It is an opt-in setting, **Efficient soft effects**, under **Settings → Display**. It is off by default and applies live so it can be compared in one session.

### Result in Game Mode

Same session, Lava Room, render scale 3, Smooth Motion on, the setting toggled from the menu (off, off, on, off, on):

| | Efficient soft effects off | **On** |
|---|---|---|
| Game-thread clock | 3.08–3.18 GHz | **3.44–3.45 GHz** |
| APU power (cap 15 W) | 14.5–15.5 W | **11.7–13.6 W** |
| In-between frames per 15 s | 385–415 | **420–450** (450 = all) |
| Seconds with a dip per 15 s | 15 of 15, at 85–89% | **2–5, at 91–96%** |

Gains, means of 6 windows off and 4 on:

| | Off | On | Change |
|---|---|---|---|
| Game frames per second | 26.1 | 29.0 | +11% (87% → 97% of 30) |
| Game-thread clock | 3.14 GHz | 3.45 GHz | +10% |
| APU power | 14.8 W | 12.7 W | −15% |
| Seconds with a dip, per 15 s | 15.0 | 3.2 | −79% |
| In-between frame, GPU time | ≈ 16.2 ms | ≈ 9.3 ms | −43% |

The picture, compared on identical frames from the save state at scale 3, differs by 0.96/255 on average (none of it over 16/255); the player saw barely any difference, if any.

### Result across five rendered scenes

The same AppImage ran as both sides of an interleaved A/B: render scale 3 and
60 FPS Smooth Motion on both, with only `DOL_AURORA_EFFICIENT_EFFECTS=1` added
to A. Four runs per side loaded all five states and produced identical guest
checkpoints.

| Scene | FPS change | GPU-time change | Picture result |
|---|---:|---:|---|
| Outset | +0.41% | -12.62% | changed |
| Lava Room | **+16.47%** | **-28.36%** | changed |
| Hyrule Castle | -1.24% | +0.33% | similar |
| Butterfly Room | +1.77% | -4.21% | changed |
| Shipwreck | +2.49% | **-32.16%** | increased flicker detected |
| **Five-scene geometric mean** | **+3.84%** | — | review required |

The performance hypothesis holds in the two intended heavy-effect scenes.
The visual acceptance gate does not: Shipwreck reports more blinking pixels,
and three other scenes cross the comparison threshold. The option therefore
remains opt-in and the retain/reject decision remains open pending review.

## Outcomes

- At render scale 3 in Game Mode, Smooth Motion keeps its in-between frames in the Lava Room and the Shipwreck at full game speed.
- The heat haze and similar effects look the same to the player.
- With Smooth Motion off, the same rooms draw less power.
- Automatic pacing still protects game speed under genuine overload.

## Success and constraints

| Metric | Baseline (Game Mode, scale 3, Lava Room) | Target | Validation |
|---|---|---|---|
| GPU per real frame | ≈ 23 ms | ≤ 14 ms (≥ 40% less) | `[gpu-prof]`, setting toggled in one session |
| Game speed, Smooth Motion on | 83–87% | ≥ 98%, in-between frames delivered | `[interp-stats]`, `perf stat` |
| APU power, Smooth Motion on | 14.1–16.0 W (at the 15 W cap) | under the cap | `power1_average` |
| Picture | Five-scene baseline recorded | Differences acceptable to reviewers; no new flicker | Paired captures and amplified differences in `images/` |
| A/B validity | — | the A/B difference exceeds the A/A spread | in-session toggles |

Constraints: off by default until validated; no change to the game's own logic; the setting changes rendering only.

## Clarifications

- **Settings matter.** Headless runs used default settings (scale 1) and understated the real frame's cost about 5×. Final measurements are taken in Game Mode at the player's settings.
- **Corrections made along the way:** the Lava Room has ≈ 930 draws per game frame, not 9,000; no texture pack was active; the survey's Outset share was withdrawn; a headless-only cause (presentation) did not hold in Game Mode.
- **Out of scope:** Outset's game-thread limit, planned in [#503](https://github.com/chrissotraidis/bluewake/pull/503).
- **Naming:** "Efficient soft effects" covers haze, smoke, fog and clouds.
