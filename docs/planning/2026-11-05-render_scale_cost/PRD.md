# PRD — Why higher render settings slow the game when the GPU has room

Status: Planning; nothing measured yet
Source base: BlueWake `main` at `ee991d3`
[Technical plan](TECH_PLAN.md) · [Tracker](PROGRESS.md)
Scope: the game's frame rate, not the picture's quality. Find out where the time goes when the render scale goes from 3x to 4x, or the Wind Waker HD look is turned on, while the GPU still reports idle time. Excluded: making the GPU itself faster, and changing what anything looks like.

## The problem

On the Steam Deck, raising the render scale from 3x to 4x lowers the game's frame rate. That would be expected if the GPU were full, but at 3x it reports only a few milliseconds of work per frame (render_bench's GPU busy: about 2 to 16 ms, against a 33 ms frame at 30 fps). The HD look, and other picture options, seem to slow the game in the same way.

If the GPU has room, the slowdown has to come from somewhere else. Something on the CPU side grows with the picture's size or detail, and the game thread ends up waiting for it.

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

## Questions to answer

1. **Is the GPU really not the limit at 4x?** GPU busy per frame against the frame's time budget, at 3x and 4x.
2. **Where does the game thread wait?** Time at the draw-done barrier, in present, and in any other sync with the render worker or the GPU, at 3x against 4x.
3. **What grows on the render worker?** Its CPU time per frame, and which functions grow: copies and resolves at the larger size, draw-plan building, uploads.
4. **Does the HD look slow things down the same way?** The same measurements with the look on and off at a fixed scale.

## Outcomes

- A clear attribution, per setting, of where the extra frame time goes: game thread's own work, waits on the render worker, waits on present, or the GPU.
- One or more follow-up PRs with a concrete change, each measured the usual way: ahead in every one of six interleaved Deck launches, with identical checkpoints.
- Or, if it really is the GPU, a record that says so, with the numbers.

## Success and constraints

| Measure | Target |
|---|---|
| Attribution | ≥ 90 % of the 3x → 4x frame-time increase assigned to a named cause |
| Repeatability | Six interleaved launches per setting, same build, same save states |
| Exactness of any later change | Identical guest checkpoints and pictures at the same setting |
| Picture | No change to what is drawn |
