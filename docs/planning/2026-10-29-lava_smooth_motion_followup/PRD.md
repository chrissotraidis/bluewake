# PRD — Lava Room Smooth Motion follow-up

Status: Draft  
Source: BlueWake `main` at `a7731853c197cc9763a86d62e731d65ca0b25556`; measured baseline is the combined [#480](https://github.com/chrissotraidis/bluewake/pull/480) → [#495](https://github.com/chrissotraidis/bluewake/pull/495) → [#501](https://github.com/chrissotraidis/bluewake/pull/501) stack at `2749e6a194dcdba93846ebd925a33c4f95c695ab`.  
Scope: diagnose and correct Lava Room Smooth Motion loss under live Steam Deck GPU/presentation pressure; excluded: Outset's CPU bottleneck and unconditional removal of overload protection.

## Motivation

The requested outcome is stable Smooth Motion in the Lava Room without slowing the 30 FPS game. On a first-revision Steam Deck in Game Mode, the #501 stack held 99–100% aggregate GPU load in 11/12 one-second samples with Smooth Motion enabled. Disabling it changed the same view to burst/idle GPU behavior while BlueWake CPU stayed near 144%. The controlled benchmark does not reproduce this because it disables interpolation unless `--smooth` is supplied.

The overlay's `paused` label is ambiguous. It can mean the pacing controller reduced its in-between-frame budget, or that the frame-content verdict rejected interpolation. Existing logs name pacing transitions, but the test must classify both paths before changing policy. [#470](https://github.com/chrissotraidis/bluewake/pull/470) exposes “Never pause” as an opt-in workaround; [#473](https://github.com/chrissotraidis/bluewake/issues/473) tracks the root cause.

## M0 findings (2026-10-30)

Same Lava Room view, same build (#501 alias-lookup AppImage `6fe3d1a7868b`), 5 × 10 s windows per mode in Game Mode, then 3 × 30 s runs per mode headless with `DOL_AURORA_GPU_PROF=1`.

| Mode | Game speed | In-between frames | Present wait | GPU |
|---|---|---|---|---|
| Smooth Motion off | ≈100% | — | 6–11 ms/s | 69% busy at 1040 MHz; 5.26 ms GPU per frame headless |
| Automatic pacing | ≈100% | paused nearly throughout; 3 retries each dropped within 0.25 s | 6–29 ms/s | 66% busy |
| Never pause | 74–98% (mostly 84–92%) | nearly all delivered | 10–156 ms/s | 96% busy, clock up to 1.5 GHz |

- Automatic pacing is working as intended here: it trades the in-between frames for full game speed. The premise of a false overload is not supported in this view.
- The content verdict is not involved (0.2–0.3% rejected in every mode), nor the game's own GX wait (≈5 ms everywhere).
- Retries fail on "a drawable came late" (a ≥ 20 ms swapchain acquire) and "the game waited for the render worker".
- Headless reproduces the cost: present waits of 41–87 ms/s and present gaps up to 362–498 ms with Smooth Motion on. The game frame's own GPU work (EFB ≈ 4.6 ms) fits easily into 33 ms, and the GPU profiler does not time the in-between replay at all, so its cost is still unmeasured.

M1 attribution (headless, #504 build with RecompCore #49):

- **Content verdict ruled out:** 0 rejected across 6 runs; nearly every frame is delivered when pacing allows.
- **Main trigger:** the game waiting on the render worker (42–112 overloads per 30 s run, up to 1.7 s of waiting), then late drawables. The GPU-staging trigger never fires.
- **In-between replay:** about 5 ms of GPU per game frame, roughly the real frame's EFB cost.
- **Staging copies grow about 10× with Smooth Motion on** (≈ 0.4 ms → 3.4–5.5 ms per profiled frame, for the same bytes). This is the leading suspect: the copies then wait or contend instead of transferring. The zone covers the copy commands' GPU time, so contention with the replay is not yet separated from the copies themselves.

Headless result (M1-T04, later withdrawn for Game Mode): with Mailbox presentation (`BLUEWAKE_VSYNC=0`) instead of the default FIFO, the render-worker waits disappear (42–101 per run → 0) and late drawables fall to 1–3, with the same frames delivered. Pausing is driven by **blocking FIFO presents on gamescope's 3-image swapchain**, the same class of problem BlueWake already avoids under Wayland (KWin), not by GPU capacity. The extra staging-copy cost remains as a secondary, smaller item.

In **Game Mode** the same Mailbox setting does not help: automatic pacing still pauses, and never pause still runs at 75–93% game speed, with 178 late drawables (4.8 s) in 50 s. The live compositor holds swapchain images in a way headless gamescope does not, so the cause is GPU and compositor capacity on the real display path, still to be attributed.

**GPU fixed at 1600 MHz (Game Mode, never pause):** every GPU-side overload disappears (0 late drawables, 0 render-worker waits, 0 GPU behind, GPU 66% busy), yet game speed is 74–82% with the game thread 98% busy. The GPU has headroom, which fits Wind Waker HD running a steady 30 FPS on the Deck in Cemu with heavier lighting. The binding limit is the **game thread**: in this room it is about 97% busy with Smooth Motion off (≈ 10% headroom), and Smooth Motion's per-draw work on that thread exceeds it. The late drawables at automatic clocks are a secondary effect of the GPU governor holding 1040–1500 MHz.

**Game-thread CPU profile (headless, off vs never pause):** the game thread runs the same instructions (115.5 G vs 114.7 G) but needs 8.3% more cycles at the same 3.35 GHz (IPC 1.89 → 1.73), and its busy share rises from 91% to 99%. No Smooth Motion function appears on it. Smooth Motion adds about 5.8 s of CPU per 20 s on other threads (GX worker, render worker, Smooth Motion helper), which most likely share the game thread's core (SMT sibling) or its cache. The working hypothesis for M2 is to keep that work off the game thread's core.

The goal therefore narrows: make the in-between frames cheap enough (GPU replay or presentation path) that automatic pacing keeps them, rather than loosening pacing.

## Outcomes

- Reproduce the same Lava Room view with automatic pacing, pacing disabled and Smooth Motion off.
- Attribute missing in-between frames to pacing budget, content verdict, staging exhaustion, render-worker wait, GPU wait, late drawable or true GPU backlog.
- Correct only the misclassified or unnecessarily expensive path while preserving overload protection when interpolation reduces game speed.
- Demonstrate the result in repeatable rendered measurements and a live Game Mode player test.

## Success and constraints

| Metric | Baseline | Target | Validation |
|---|---:|---:|---|
| Game rate | About 30 FPS in the reported live view | No worse than control by 2% | Fixed-view paired Game Mode captures |
| Smooth Motion delivery | Automatic mode can report `paused`; exact cause not yet classified | Fewer missing in-between frames for the proven false-overload path | Pacing/verdict counters and displayed/interpolated frame ratio |
| Pacing transitions | Existing `[interp-pace]` reason strings | Every budget drop attributable to a typed cause and measured wait | Session log plus focused test |
| GPU/presentation | 11/12 samples at 99–100% with Smooth Motion | Lower wait/backlog or more delivered frames for equal game speed | GPU busy/clock, present, slot and staging waits |
| Correctness | #501 rendered output/checkpoints | No guest, camera-cut or interpolation-content regression | Frame-interpolation tests, pictures and checkpoints |

A continuously busy GPU is not itself a failure if it delivers the requested frames and leaves game speed intact. The plan does not assume the heuristic is wrong; a true sustained overload may correctly reduce interpolation. Private states, disc content and translated modules stay off GitHub.

## Clarifications

- “GPU-bound with Smoothness on” means the live Lava Room presentation path described in #501, not the headless or interpolation-disabled benchmark.
- The fix belongs in RecompCore if it changes Aurora pacing or interpolation. BlueWake then pins the reviewed RecompCore commit and carries its exported patch according to repository policy.

