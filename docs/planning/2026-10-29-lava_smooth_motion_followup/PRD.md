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

**Pinning test:** with the game thread alone on its core (sibling idle), the per-instruction gap halves (−8.4% → −4.6%), but game speed does not recover (29.84 → 28.86 FPS). SMT sharing is not the main factor. The remaining suspects are shared L3 and memory traffic from Smooth Motion's other threads, and spin-waiting on frame slots in `camera_steps`.

**Memory bandwidth is the shared resource.** With Smooth Motion off, a synthetic memory-streaming load on CPUs 4–7 (≈ 12 GB/s) cuts the game thread's IPC by 38% (1.89 → 1.18) for the same instructions, and game speed to 19 FPS. On the Deck, the CPU and GPU share LPDDR5, so Smooth Motion's traffic counts on both sides: about 9.5 MB of vertex staging copies per game frame, the in-between data, the in-between replay, and twice the presents. This also explains why fixing the GPU at 1600 MHz made the game slower, not faster. Spin-waiting is ruled out: the frame-slot, staging and pacing waits sleep (`wait_for_gpu_progress`).

**Attribution by isolating parts of Smooth Motion (M2-T01):** the vertex bytes copied are the same with Smooth Motion on and off (9.5 MB per game frame); the copy zone's longer time is queue wait. The in-between data is about 3.2 MB per game frame. With `DOL_AURORA_FRAME_INTERP_ABLATE`, the game thread loses as much with matching and blending alone (IPC −5.3%) as with full Smooth Motion (−4.7%); adding the upload or the in-between redraw changes nothing measurable.

**Confirmed over 5 interleaved rounds with warm caches:** game-thread IPC versus off is −2.0% with matching only and −3.3% with full Smooth Motion, in that order in every round. Matching and blending are about 60% of the loss; the upload and the replay about 40%. Headless, that ≈ 3% fits within the game thread's headroom; in Game Mode, the compositor's and the display's memory traffic add to it and the game loses about 10%.

**Sizing (per-draw counters):** ≈ 930 helper jobs per game frame, ≈ 720 full constants copies, ≈ 930 blended blocks: about 0.5 GB/s of traffic in all, spread across stages. No single copy is a large lever.

**Game Mode cause (same session, Smooth Motion toggled in the menu):** the game thread's IPC is unchanged (≈ −1%), but its clock falls from 3.45 to ≈ 3.13 GHz (−9%). The Deck shares a 15 W budget between CPU and GPU; with Smooth Motion on it runs at the cap (13–16 W) with the GPU 99–100% busy, and the CPU clock gives way. The lever in Game Mode is the **power** Smooth Motion costs, mostly the GPU's in-between replay and presentation, not memory traffic.

**Charger and settings ruled out:** on the retail charger (AC online, battery full) the result is the same, since the 15 W cap is the APU's own limit. The Game Mode settings are already light: 1280×720 frame buffer, render scale 1, every HD-look effect off.

**Reframed question (2026-10-30):** a GameCube game at 720p should not need a second full render to show an in-between frame. The Lava Room has about 930 draws per game frame, and Smooth Motion redraws all of them, so it costs about as much power as rendering at a true 60 FPS. The goal becomes an in-between frame that costs much less than a real one.

**Per pass:** the in-between replay's four EFB passes cost about the same as the real frame's (≈ 1.24 vs 1.16 ms each), and nothing else in it is significant. A cheaper in-between frame therefore has to draw less or draw more cheaply.

**Player settings (Game Mode, 2026-10-30):** render scale 3 and 2× anisotropic filtering (no texture pack is active) make the real frame cost ≈ 22 ms of GPU at the governor's clock; the in-between replay adds ≈ 17.5 ms at a higher clock. Headless used default settings and understated this about 5×. The GPU and power cost of Smooth Motion scales with these settings.

**Render scale is the driver (Game Mode):** at render scale 2, Smooth Motion costs no measurable game speed. The in-between frame takes ≈ 9.4 ms, the chip stays at 13.2–13.8 W under its 15 W cap and the game thread keeps 3.45 GHz. At scale 3 the in-between frame takes ≈ 17.5 ms, the chip reaches the cap and the game loses ≈ 10%. The room is pixel-bound at these settings; no texture pack was active.

**One pass holds the cost (Game Mode, scale 3):** EFB pass 1, the main scene with 776 draws, takes 20.7–22.0 ms, 95% of the real frame. The next step locates which of its draws hold the time.

**The cost is concentrated (chunked timing, Game Mode, scale 3):** four ranges of pass 1 hold ≈ 19 of its ≈ 21 ms: commands 700–749 (≈ 9 ms), 750–780 (≈ 4.7 ms), 50–99 (≈ 3.8 ms) and 650–699 (≈ 1.7 ms). About 180 of the 776 draws cost 90% of the pass, so the favourable case of the estimate below applies; the next step identifies those draws. The in-between frame has the same hot ranges (≈ 15 of its ≈ 17.5 ms), so one fix serves both frames.

**The cost is the room's heat haze (Game Mode, scale 3):** pass 1's commands 700–780 are 81 large alpha-blended quads drawn without depth testing, the haze/smoke veil over the room. Hiding them cuts pass 1 from ≈ 21.3 to ≈ 6.5 ms and the frame's GPU time from ≈ 23 to ≈ 8 ms. The effect is subtle (the player saw no difference) but real: nearly every pixel changes slightly, about a quarter of them clearly. At the game's 640×480 these sprites are cheap; at scale 3 each blends over millions of pixels, several layers deep. Keeping the effect while drawing it into a smaller buffer and compositing it once is the standard remedy for soft particles.

**Confirmed in Game Mode:** with the haze hidden, scale 3 with Smoothness on delivers every in-between frame (450 of 450 per 15 s) at full game speed (game thread 3.45 GHz, 0–1 dip seconds), at 12.8–13.8 W under the cap; the player saw 60 FPS. At these settings, the Lava Room's Smoothness problem is the haze.

**Estimated potential (before measuring it):** for scale 3 with Smooth Motion to behave like scale 2 does today, the real and in-between frames must drop from ≈ 39 ms to ≈ 20 ms per game frame, about half of pass 1. If one or two full-screen layers or shaders dominate, fixes such as tighter depth testing, cheaper shading or not redrawing an unchanged layer could save 30–50% of pass 1 (6–10 ms per real frame, as much per in-between frame), and less power with Smooth Motion off too. If the cost is spread evenly over the 776 draws, it is plain pixel work at about 3.7 million pixels and little (≈ 0–10%) is available without a broader renderer change. The per-chunk timing decides which case applies.

**Direction (agreed 2026-10-30):** (1) a short Game Mode check of how much of the 10% the compositor and display add; (2) document automatic pacing as correct protection in this room, with the remaining effort on the game thread's own headroom; (3) then remove the intermediate constants copies as a measured, modest improvement.

**Earlier M2 direction:** reduce the memory traffic of Smooth Motion's per-draw matching and blending on the CPU (the helper thread and the capture on the GX side), not the GPU replay.

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

