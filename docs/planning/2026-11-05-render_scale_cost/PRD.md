# PRD — Why higher render settings slow the game when the GPU has room

Status: In progress; mechanism confirmed, opt-in source experiment next
Source base: BlueWake `main` at `ee991d3`
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: explain the game's frame-rate loss when higher render settings increase GPU work, then test a narrowly scoped reduction in that work. The first source experiment batches safe staging-buffer copies while preserving texture-upload order. Excluded: changing the picture, shipping a GPU clock cap, reordering texture uploads, and enabling an unqualified experiment by default.

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
3. **What should change?** Reduce GPU work, beginning with an opt-in staging-copy experiment and then the in-between frame itself if the copy experiment is insufficient.
4. **What remains outside this pass?** Confirming whether the HD look has the same mechanism is deferred until a concrete source change needs that qualification.

## Planned outcome

- Keep the diagnosis, implementation, dependency pin, and validation together in BlueWake PR #563.
- Stack RecompCore #53 on #49. #53 keeps ordered texture uploads in place and batches only vertex, uniform, index, and storage-buffer staging copies into a command buffer submitted before the already-encoded render work.
- Expose the experiment as an experimental GUI switch backed by `DOL_AURORA_BATCH_STAGING_COPIES=1`. It is off by default, persists through the shared settings file, and takes effect on the next launch while it is being qualified.
- Keep the source change only if it wins every one of at least six interleaved Deck launches, preserves guest checkpoints, and produces identical pictures with Smooth Motion off. Otherwise revert the pin/change in #563 and record the no-go result.

## Success and constraints

| Measure | Target |
|---|---|
| Attribution | ≥ 90 % of the 3x → 4x frame-time increase assigned to a named cause |
| Repeatability | Six interleaved launches per setting, same build, same save states |
| Performance gate | Candidate ahead of baseline in every one of at least six interleaved Deck launches |
| Exactness of the source change | Identical guest checkpoints; identical pictures with Smooth Motion off |
| Picture | No change to what is drawn |
| Rollout | Experiment remains opt-in until it passes the performance and exactness gates |
| Product access | Experimental GUI switch on every desktop platform; default off and next-launch behavior covered by Linux and Windows CI settings tests |
