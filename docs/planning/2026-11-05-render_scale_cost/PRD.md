# PRD — Why higher render settings slow the game when the GPU has room

Status: Closing — render settings cost 0 % to ~1 % at Outset in the recheck; the game thread is the limit
Source base: BlueWake `main` at `ee991d3`
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: explain the game's frame-rate loss when higher render settings increase GPU work, then test a narrowly scoped reduction in that work. The first source experiment batches safe staging-buffer copies while preserving texture-upload order. Excluded: changing the picture, shipping a GPU clock cap, reordering texture uploads, and enabling an unqualified experiment by default.

## Recheck (2026-11-09): the settings cost little; the game thread is the limit

A recheck logged the chip's power (slowPPT, 15 W limit), GPU clock and temperature every half second during Outset launches, six per setting, and lined them up with each launch's frame rate.

| Outset, same build | Power | FPS |
|---|---|---|
| 3x | ~14.2 W | 26.05 |
| 4x | ~14.6 W | 26.00 (−0.2 %) |
| 3x, HD look off | ~14.2 W | 26.38 |
| 3x, HD look on | ~15.0 W, at the limit | 26.07 (−1.2 %) |

The earlier −6.3 % at 4x did not reproduce. Both settings ran about 9 % below that day's 3x, a variation between days larger than any render effect. The 1100 MHz cap does not confirm the power explanation either: compared with uncapped runs at the same scale, it is slower in 9 of 10 cases, matching the owner's in-play result. Shared power costs at most about 1 % here.

Outset is below 30 fps unpaced at every setting, and the owner sees it below 30 in play at under 10 W. The limit is the game thread: about 38 ms of work per frame where 30 fps allows 33 ms. Speed work moves there.

## The problem

On the Steam Deck, raising the render scale from 3x to 4x lowers the game's frame rate. That would be expected if the GPU were full, but at 3x it reports only a few milliseconds of work per frame (render_bench's GPU busy: about 2 to 16 ms, against a 33 ms frame at 30 fps). The HD look, and other picture options, seem to slow the game in the same way.

The measurements now identify the missing link: the Deck's CPU and GPU share an APU power budget. At Outset, 4x plus Smooth Motion raises GPU demand enough to lower the busy game thread's effective clock by 7.0 %, accounting for its 6.3 % frame-rate loss. Fixing the GPU at 1100 MHz restores the game-thread clock and reverses the 4x result. The cap is diagnostic, not a proposed player setting; the product direction is to remove avoidable GPU work so the CPU retains more power.

## Why it matters

- **It's a general lead.** One cause could explain several slowdowns: render scale, the HD look, and possibly others such as Smooth Motion or HD textures.
- **It's separate from the CPU work so far.** #503 to #554 and #524 made the game thread's own code faster. This is about time the game thread spends waiting, which those changes can't touch.
- **It decides what players can turn on.** The Deck could hold higher settings at full speed if the wait isn't really the GPU.

## What we know already

| Fact | Source |
|---|---|
| The game thread meets the render worker at one barrier, the draw-done commit, which drains everything appended so far | [`main.c`](../../../runtime/host/src/main.c), `BLUEWAKE_GX_FLUSH_CENSUS` |
| An earlier census found the rendered play window's cost bimodal with a two-retrace period, all of the gap on the retrace that presents | `docs/archive/status/CURRENT.md` (2026-09-22), cited in `main.c` |
| At 3x on the Deck, the render worker's CPU work is high in the all-threads profile (`build_draw_plan_into` ≈ 5.8 %, `submit_packet` ≈ 2.8 %, `fill_render_packet` ≈ 2.1 %) while the game thread is ≈ 50 % of all cycles | render_bench `--cpu-profile`, stack build `afb6c35d8805`, Outset (2026-11-04) |
| render_bench reports game FPS, the game thread's busy %, GPU busy ms and per-thread profiles | [`render_bench.py`](../../../scripts/render_bench.py) |
| Uncapped Outset at 4x does the same game-thread work at 3.06 GHz instead of 3.29 GHz; FPS falls 6.3 % | Six interleaved Deck launches plus `--cpu-profile`, stack `afb6c35d8805` |
| With the GPU fixed at 1100 MHz, 4x holds 3.34 GHz and every 4x Outset launch beats every 3x launch | Six interleaved Deck launches plus `--cpu-profile`, same stack |
| Smooth Motion replay skips ordinary staging-copy operations and reruns render/EFB work; real-frame staging uploads are incremental | RecompCore/Aurora source inspection at #49 head `a4dbaaa8` |

## Questions answered

1. **Why does 4x lose at Outset?** More GPU work changes the Deck's shared-power operating point and lowers the clock of the CPU-bound game thread.
2. **Why does Smooth Motion matter?** It gives the GPU an in-between frame to render. With it off, the GPU does substantially less work and the Outset loss disappears.
3. **What should change?** Reduce the work rerun for Smooth Motion's in-between frame. The first experiment, batching non-texture staging copies, reduced reported GPU work but did not improve frame rate.
4. **What remains outside this pass?** Confirming whether the HD look has the same mechanism is deferred until a concrete source change needs that qualification.

## Outcome and next direction

- The shared-power diagnosis remains in BlueWake PR #563.
- RecompCore #53's safe staging-copy experiment was tested with 12 candidate and 12 baseline launches. It was performance-neutral overall (+0.03 %) and slower in four of five scenes, including Outset (−1.23 %), despite lowering the median reported GPU work from 26.627 to 22.188 ms.
- All 24 measured runs loaded every scene and guest checkpoints matched. The experiment failed performance acceptance, so its BlueWake pin, patches, build-recipe changes and GUI setting were removed. Picture qualification was not run because it could not change the rejection.
- The next isolated source experiment should target render/EFB work repeated for Smooth Motion's in-between frame.

## First experiment result

The staging-copy candidate was measured with 12 launches per setting. It reduced median reported GPU work from 26.627 to 22.188 ms, but overall FPS was effectively unchanged (+0.03 %) and four of five scenes were slower, including Outset (−1.23 %). All 24 measured runs loaded every scene and guest checkpoints matched. **Rejected:** the pin, patches, GUI switch and build recipe are removed from this PR; RecompCore #53 stays as the record. Lower reported GPU work did not free the CPU, so the next target is the in-between frame's repeated render and EFB work.

## Success and constraints

| Measure | Target |
|---|---|
| Attribution | ≥ 90 % of the 3x → 4x frame-time increase assigned to a named cause |
| Repeatability | Six interleaved launches per setting, same build, same save states |
| Performance gate | Candidate ahead of baseline in every one of at least six interleaved Deck launches |
| Exactness of a retained source change | Identical guest checkpoints; identical pictures with Smooth Motion off |
| Picture | No change to what is drawn |
| Rejected experiments | Remove product/build integration and preserve the measured no-go result |
