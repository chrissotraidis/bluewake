# Technical plan — Automatic render scale (opt-in)

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## What exists

- `aurora_set_frame_buffer_scale(float)` (RecompCore, `window.cpp`) already changes the scale while the game runs: the Display tab calls it. It takes a fraction, though the setting (`DOL_AURORA_RENDER_SCALE`) and the menu are whole numbers (`env_int`).
- A change goes through `request_frame_buffer_resize()`: the same deferred resize a window resize takes, which rebuilds the frame buffer and what hangs off it (`resize_swapchain_internal`: `g_frameBuffer`, the resolved copy, and by size the post-effects' targets, the EFB copy cache and Smooth Motion's history).
- The host already has, once a second and for nothing: the retrace rate and the game's speed (`[perf]`, `[fps-dip]`), the game thread's busy share, the present and GPU waits.

## Open questions, answered by measurement before any design is fixed

1. **What does one live change cost?** Time a scale change in play on the Deck (a test hook that changes it at a retrace): the frames around it, pipelines or targets made, the longest frame. If it is a hitch, changes must be rare and placed at cuts (below), or the resize path needs a cheaper lane for a pure scale change.
2. **What does Smooth Motion do across a change?** Its history is the old size. Expected: one in-between frame skipped. To check: no garbage frame, no flash.
3. **Which signal says "the GPU is the limit"?** `[fps-dip]` named `cause=game-thread` in both scenes above while the GPU was the limit (busy 100 %, full speed at 2x with the game thread as busy). So the existing cause line cannot drive this. Candidates: one timestamp pair a frame (the frame's GPU time, not the per-pass profiler); the render worker's and present's waits; on Linux, `gpu_busy_percent` read once a second. To measure: each candidate in the two scenes and in a game-thread-bound one, at 3x and 2x.
4. **How small can a step be before it is seen?** Pictures at 3x, 2.75x, 2.5x, 2.25x and 2x on a 1280x800 panel, and the owner's eye in play.

## The controller (to be tuned with the answers)

A small state machine in the host, run once a second:

- **Down:** the GPU signal over budget for N seconds in a row (start: 2) → one step down (start: a quarter of a scale). Not below the floor.
- **Up:** the signal under budget with a margin (start: 15 % headroom at the *current* scale, scaled to what the next step up would cost by the pixel ratio) for M seconds in a row (start: 20) → one step up. Not above the player's scale.
- **Hysteresis:** the up margin is wider than the down trigger, and after any change no opposite change for a hold time (start: 30 s). A step up that is followed by a step down inside the hold time doubles the hold for that place.
- **Cuts first:** when a stage or room load, a fade to black or a door is under way, pending changes are applied there; a step down may also be taken mid-scene if the cost measured in question 1 allows, a step up waits for a cut unless the scene has been steady for a long time.
- **Memory:** the scale that held, by stage and room, for the session (and later in the settings folder): entering a place starts at its remembered scale, applied during the load.
- **Left alone:** when the game thread is the limit (the signal says the GPU has room), while a menu is open, during a save state load, and with the setting off.

Every change is logged (`[auto-scale] from to reason place`), so a session can be checked for flicker by reading the log.

## Settings

`BLUEWAKE_AUTO_RENDER_SCALE` (working name), off by default, and its floor; in the Display tab under the render scale. Its test entries. Fractional scales stay internal: the player still picks whole numbers.

## Order of work

1. The test hook and the four measurements above; results in PROGRESS.md. If a live change is a hitch that cuts cannot hide, stop and report: the alternative (keeping the targets at their largest size and drawing into a part of them) is a larger renderer change and its own plan.
2. The signal and the logging, with the controller only reporting what it would do; a play session's log checked for flicker on paper.
3. The controller acting, behind the opt-in; the owner's play test.

## Verification

- The PRD's success list, in Game Mode on the Deck.
- A scripted check on the controller alone (no renderer): fed recorded signal traces from the two scenes and from a steady one, it makes no opposite changes inside the hold time and settles.
