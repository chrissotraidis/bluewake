# BlueWake goal prompt — 2026-09-14 (v3)

This replaces v2 at this path, and §12 of [GOAL_LOOP.md](../GOAL_LOOP.md), as the durable goal given to the implementation agent. The PRD still defines what "finished" means. This document defines what counts as a day's work.

## Why v2 was also going to fail

v2 was honest and it fixed the right three things: it made a build the unit of progress, it froze Route B, and it refused "no product gate advances" as an outcome. It still had two faults, and they are the same two faults as v1.

1. **Its first milestones were not observable.** M0 was "the benchmark reproduces the baseline" and M1 was "15 fps". No human can see a benchmark or watch a frame rate. A loop that optimizes toward M1 can run for weeks without a human ever being shown anything, while remaining fully compliant with the letter of the contract.
2. **It never asked what the shipping binary does when a human launches it.** v2 asks for a one-command launch at M5, last. That question was asked for the first time on 2026-09-14, and the answer is: *nothing happens at all.*

## The finding that reorders the work

Measured on 2026-09-14, from the tree at 0e86899:

- [BlueWake.app](build/runtime-host-dsp/BlueWake.app) exists, is signed, and has a correct Info.plist. Its Contents/MacOS/BlueWake is the host binary. Launching it from Finder or with `open` runs it with an empty environment.
- main.c:3027 aborts with BLUEWAKE_DOL not set before boot. Double-click therefore fails on the first line of real work.
- Even given the paths, main.c:3190 installs the **headless** backend unless BLUEWAKE_RENDERER=aurora. Headless means: no window (main.c:3210), no platform input (g_live_pad_enabled stays false), and no audio capture at all, because BLUEWAKE_CAPTURE_AUDIO_WAV is unset (main.c:3239).
- The visible, audible, playable path already exists and is one environment variable away. With BLUEWAKE_RENDERER=aurora: a 960×720 vsync window, live pad input merged at SI (main.c:3206), and real sound — aurora_backend.cpp opens SDL_OpenAudioDeviceStream on the default playback device, and the DSP PCM reaches it through dol_platform_audio_push (audio_dma.c:217 → aurora_backend.cpp:180).

So the plain explanation of *"we've tried and tried and tried and a human has never seen or heard anything"* is not that the emulator does not work. It is that the emulator was never delivered to a human. For ten days the project profiled, digitized and hardened a simulation whose **shipped configuration is a headless, silent, input-less process that cannot even start unless an engineer exports six variables and passes an absolute path as argv[1].**

The work in the ledger is real. The last mile was never walked, and v1 and v2 both put it last.

## The one defining outcome

**One command — scripts/play.sh — opens BlueWake, and a human can play it.**

From a clean shell, with no exported variables, using the user's own private GZLE01 image, and with developer tracing compiled out:

1. a 960×720 window opens and shows the game;
2. sound comes out of the default output device;
3. a keyboard and a physical pad move, jump and steer Link;
4. the same thing happens when a human double-clicks BlueWake.app.

Until all four hold at once, the project has shipped nothing, no matter what any number in the ledger says.

## The iteration contract

An iteration counts only if a human can now see, hear or do something they could not before, **or** one of the five governing numbers moved.

A clearer screen, a louder speaker, a working button and a faster frame are all progress. A new tier, a new oracle, a new instrument, a longer ledger, a reduced and explained failure, and a better-understood blocker are **costs you pay to make a measurement trustworthy** — not progress.

If an iteration ends with no change to an observable and no change to a governing number, it FAILED. Revert it and change the mechanism, not the constant. One failure is a signal. Three failures of the same shape means stop, measure the layer underneath, and say so out loud.

Never write "no product gate advances" as a successful outcome. That sentence is the failure signature of this project.

## The five governing numbers

scripts/bench.sh runs the shipping configuration on the Outset route and reports all five. It is trustworthy as of 2026-09-14 and reproduces the 2026-09-01 baseline exactly.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps, retraces 13,910–14,100 | 6.0 fps on the 2026-09-01 host, **3.10 fps on this host** |
| p99 frame time | 2.5 fps tail on the 2026-09-01 host, **512.35 ms (1.95 fps) on this host** |
| peak RSS | 262,799,360 bytes on this host |

**fps is host-dependent and the 6.0 fps reference came from a faster Mac.** The same binary measures 3.10 fps median and 24.5% of real speed here. A/B only against numbers measured on the host you are standing on, and record the host identity with every timing. The digest, the turn count and the card are host-independent, and those must reproduce everywhere. A change that moves fps but moves the digest is not a result; it is a behavior change.

## What is already done — do not rebuild it

- Route A boots retail GZLE01 to controllable Outset gameplay, headless, deterministically, in about 16 minutes of wall time.
- The hybrid-O2 composite with the 256-cycle cap is the promoted configuration.
- The RecompCore donor DSP adapter runs authentic DSP LLE with interprocedural optimization.
- The Aurora backend already provides Metal video, SDL3 audio output, and live pad input.
- scripts/route_digest.py and scripts/cycle_invariance.py are the correctness oracles. BLUEWAKE_CAPTURE_AUDIO_WAV is the paced-PCM oracle.
- The default-inert edge service ABI already exists in the composite (edge-service-inert-v1-20260901, ACCEPT infrastructure).
- scripts/bench.sh is the correctness-bound benchmark, and two census instruments are implemented and in the tree: BLUEWAKE_DEADLINE_CENSUS and BLUEWAKE_RETURN_CENSUS.

Read the ledger before you measure. Do not re-derive a fact that is already recorded.

## Where the speed is

The 2026-08-31 return census made the one-cycle interrupt clamp look like the project's largest lever. It is not, and that was measured on 2026-09-14 rather than argued. With BLUEWAKE_DEADLINE_CENSUS attributing every deadline to the term that actually bounded the budget, over the 700-retrace boot route:

    162,441,352 deadline evaluations
      97.90%  cap (no device deadline constrained the budget)
       1.93%  DSP LLE update cadence
       0.20%  audio work chunk
       0.0071% one-cycle interrupt clamp   (11,533 evaluations)

The 84.8% clamp figure described only the first ~50 ms of boot, under the ungated clamp that commit a87b403 already fixed. **H1 as v2 wrote it was aimed at a stale premise.**

The corrected turn map, same route, 62,096,587 dispatch returns:

    56.6%  cross-chunk return  (control returns to the host between chunks)
    31.2%  budget exit         (the 256-cycle cap expired)
    11.9%  same-chunk return
     0.22% outside translated code
     0.09% guest exception

Read those two tables together and the mechanism is unambiguous. **97.9% of the time no device needs anything, and the host is still re-entered every 256 guest cycles anyway.** The cost is not the clamp and it is not the DSP cadence. The cost is the per-turn host loop that runs at every 256-cycle chunk boundary — which the 2026-09-01 O2 profile independently corroborates: after O2, generated dispatch is 56.0% of main-thread samples, but there is a 1,269-sample unsymbolized main leaf, 16.2%, spanning exactly that per-turn loop.

Two facts constrain the fix. Host rebuilds take about 30 seconds; **composite rebuilds take hours**, so prefer host-side mechanisms and treat every composite change as expensive. And chaining has been tried three times and has failed identically every time:

| attempt | turns | outcome |
| --- | --- | --- |
| composite-dispatch-loop-v1 | 62,038,491 → 22,048,210 (−64.5%) | REJECT; loop bypassed host interception/HLE |
| edge-yield-qualified-v1 | 22,321,565 | REJECT; first external delivery diverged |
| edge-service-pe-finish-v1 | 62,038,491 → 20,096,479 (−67.6%) | REJECT; matched every milestone and the first 1,024 deliveries, then the aggregate delivery hash diverged |

Each of those cut turns 61–68% and each bought only 10–13% CPU, because servicing every edge cost about what the saved turn cost. Each failed exactness in the *same place*: the aggregate delivery hash, after the retained 1,024-record window.

### Work these in order: one hypothesis, one build, one measurement

**H1 — find the first divergent delivery, then decide.** The three failures do not say chaining is impossible; they say no one has looked at the divergence. Install the existing default-inert edge service ABI, **widen the retained delivery history window** until the first divergent delivery is captured, and explain it in one sentence. Until that delivery is in hand, any further chaining attempt is a guess, and three guesses of the same shape have already failed. This is a measurement, not a rewrite. Falsifier: the 1,050-record digest or the card changes while the service is inert.

**H2 — make chunk boundaries cost nothing when no device needs service.** This is the untried shape and the one the 97.9% cap finding points at. The host currently re-enters every 256 cycles because 256 is a conservative bound. Instead, compute the cycle of the *next actual device deadline*, set the dispatch budget to that cycle, and skip the host return entirely when a pure predicate says the host has nothing to do. bluewake_scheduler_interrupt_requires_host() (runtime/host/src/scheduler_contract.c) is already a compact pure predicate of (pc, msr, external_pending) that can be evaluated without flushing cycles. Note the circularity you must break: the predicate needs the next external event cycle, which is exactly what the deadline census already computes. Both halves of this are host-side, so it is a 30-second rebuild. Falsifier: turn count does not fall by an order of magnitude with the digest unchanged, or CPU time does not follow the turn count down.

**H3 — only then the GX frontend/sink (12%) and DSP (12–31%).**

The guest-visible cycle contract, the route digest, the card outcome and the paced-PCM fingerprint must survive every promotion. If a speedup breaks them, the mechanism is wrong. Never weaken, skip or re-record a digest to pass.

## The second outcome

Link's hair is visibly wrong and it is a first-class defect, not cosmetic debt. Diagnose it at the owning GX contract by comparing the submitted state against the retail display list — not by special-casing Link or the model. Add a repeatable screenshot checkpoint and a state-level regression. Exit when the accepted view renders hair credibly with no regression to opaque geometry, transparency, or depth ordering.

## Fences (unchanged, non-negotiable)

- Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures, device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git.
- Preserve user data and unrelated work. One BlueWake process at a time.
- No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it exposes a failure, no captured emulator state as authentic initialization.
- Never add a new tier, slice or patch for a unit that already compiles whole. Any new tier needs a named concrete compile blocker and a runtime cost it removes. New code must remove cost, not add diagnostics.
- Do not re-derive facts already in the ledger. Read first; measure second.
- Keep Route A's accepted artifacts and the dependency lock intact, and keep the private-data audit green.

## Budget and cadence

You have twenty iterations. An iteration is one hypothesis, one small change, one build, one measurement, and at most one line appended to CURRENT.md. No new status document may be longer than the change it describes. Report to the user every three iterations, in one paragraph: what a human can now see, hear or do that they could not before, plus the five governing numbers. If a line of work has not moved a number after three iterations, kill it and say so.

## Milestones — each one is something a human can check

**M1 — DELIVER IT. Do this before any optimization.** scripts/play.sh and a double-clickable BlueWake.app open a window, play sound, and accept live input, with no exported variables and no absolute path arguments. Evidence: a screenshot of Outset with Link in it, a captured WAV with nonzero samples, and a recorded button press that moves Link. This is assembly work, most of it in main.c's startup path, and it is the single thing the user has been asking for. Nothing below is allowed to delay it.

**M2** Outset median ≥ 15 fps with p99 ≥ 10 fps, digest unchanged, **measured through play.sh** so the number is a number a human can feel.

**M3** Outset median ≥ 30 fps with p99 ≥ 20 fps, pacing stable, digest unchanged. This is where the game starts to feel like the game.

**M4** Link's hair correct at the GX owner, with a screenshot checkpoint and no regression to opaque, transparent or depth-ordered geometry.

**M5** Continuous intelligible audio under human review; physical controller works including reconnect; ten-minute unattended soak with bounded RSS and no crash.

**M6** One-command clean build and launch from a fresh clone plus the user's own disc, signed, no Nintendo data in the artifact.

## Start with exactly this

1. Build the shipping configuration and check whether the pending census-14100 run finished; record its gameplay-window census numbers if it did. If the tree at 0e86899 does not build or its regressions fail, fixing that is iteration 1 and nothing else proceeds.
2. **Write scripts/play.sh, and make the app default to the visible, audible, input-enabled path.** Default the renderer to aurora when no environment is set, default the composite, DOL, REL and disc paths so that launching the bundle works, and keep every existing headless behavior and digest exactly as it is when the bench asks for it. Screenshot it. Capture a WAV. Press a button. This is M1 and it is the whole point.
3. Commit the two census instruments and the corrected status.
4. Then H1: install the default-inert edge service, widen the retained delivery history, and name the first divergent delivery in one sentence.
5. Then H2, then H3.

If you find yourself producing a long honest document explaining why a subsystem cannot yet advance, stop and treat that as the failure signal it is. Ship something a human can see, hear or press.

## Note on scope

Nothing in v3 relaxes the PRD, the legal boundary, or the evidence rules that made this project trustworthy. It changes two things: it moves the delivery step from last to first, and it makes "a human can see it" the literal definition of the first milestone. The Route B freeze and its reopening trigger are unchanged.

