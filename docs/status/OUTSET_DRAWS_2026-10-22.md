# The draws at Outset, and the cheapest fusion left

Measured October 22, 2026, on a Steam Deck (VanGogh, 4 c / 8 t), the AppImage built from `main` at
`7382708`, rendered at `sea` room 44 from a save state, Smooth Motion on. Record: jkoehler11.

This is the groundwork for horizon 3 in [PERFORMANCE.md](../PERFORMANCE.md#the-plan) ("drawing at the
GX/J3D API level"), starting from the one frame the dump already gives us rather than from a rewrite.

## What a frame is made of

`DOL_GXCORE_DRAW_DUMP=110` lists every draw of one frame. That frame at Outset:

| | |
| --- | --- |
| draws in the dump | 662 |
| vertices | 40,512 (mean 61 a draw) |
| distinct pipeline signatures | 50 |
| runs of consecutive draws with the same state | 113 |
| runs of two or more | 51 |
| smallest plans | 465 plans of 4 vertices each |

Draw fusion (#225, patch 0166) has already taken the display lists from 8.8 M draws to 0.87 M over the
whole of Outset, and draw batching (0102) pairs what it can. What is left is 662 draws a frame, and two
thirds of them are 4-vertex quads: sprites, HUD pieces, particles, and the sea's strips.

Two limits on every number here. The dump records the plans the GX core builds, not the draw calls the GPU
receives: `dump_draw` runs before batching, and the batching that exists already combines compatible plans
and rebases their indices, so how many GPU calls these plans become is not measured in this record. And the
dump is lossy: it rounds matrix values and does not carry the complete viewport, scissor, constants and
resource state, so equal pipeline signatures and equal position matrices are a way to find candidates, not
a compatibility argument.

## How much of each frame could fuse without touching a vertex

The same dump at the tour's other stops (`--mode headless` produces no draws: the dump is on the rendered
path, so these are paced runs of the same build on a desktop, warped with `BLUEWAKE_TEST_WARP`). "same
transform" counts runs of two or more consecutive draws with identical state **and** identical position
matrix, which is the case that needs no vertex baking:

| Stop | Plans | Verts | Signatures | Runs | Runs 2+ | Plans in same-transform runs | After fusing those | All runs (ceiling) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Outset | 531 | 40,871 | 44 | 85 | 30 | 328 | 220 (-59%) | 85 |
| Windfall | 663 | 41,350 | 54 | 106 | 50 | 365 | 318 (-52%) | 106 |
| Dragon Roost Cavern | 583 | 70,933 | 47 | 105 | 40 | 340 | 265 (-55%) | 105 |
| Forest Haven | 646 | 37,251 | 38 | 243 | 21 | 226 | 427 (-34%) | 243 |

So the potential reduction in the plans the GX core builds is not a third of one frame: it is between a
third and three fifths of every frame measured, and Forest Haven is the weakest because its state changes
243 times a frame. These are plans, before batching: the GPU calls that would remain are a separate count
that has not been taken.

The largest same-transform runs at each stop, which is where a first change would show up:

| Stop | Runs |
| --- | --- |
| Outset | 142 plans / 568 verts, 84 / 336, 57 / 228 |
| Windfall | 143 / 572, 90 / 360, 57 / 228 |
| Dragon Roost Cavern | 130 / 520, 90 / 360, 33 / 132 |
| Forest Haven | 92 / 368, 72 / 288, 50 / 200 |

Those are runs of 4-vertex quads: sprites, HUD pieces, particles, and the sea's strips.

The run count is the upper bound if the transform were baked into the vertices, which is what the HD
project's batching does and what Dusklight-style particle batching would need: 531 plans down to 85 at
Outset, 663 to 106 at Windfall, 583 to 105 at Dragon Roost, 646 to 243 at Forest Haven. That changes the
vertex data and every plan's position matrix, so it is the second step, not the first.

## Why the plan count is worth attacking on four cores

From the GX worker's profile at the same place ([PROFILING.md](../PROFILING.md)), the worker costs 35 to 43%
of one core and its three biggest self-time entries are per-plan work:

- `GxCoreState::build_draw_plan_into` 3.2%
- `ConsumingAuroraRenderSink::submit_packet` 2.0%
- `GxCoreSink::submit_packet` 1.7%

plus `RetailGxFrontend::emit_new_packets` 1.6% and `fill_render_packet` 1.5% on the way in. Those are
per-plan costs, which makes the plan count the place to look, but this record measures no speedup and none
should be read out of the percentages: how much of the worker's share fewer plans would take, and whether
the GPU's own per-draw cost matters at all after batching, are both unmeasured here.

## The change, concretely

In the GX core's plan builder, when a draw's pipeline key and its position-matrix rows are identical to the
plan already open, append its vertices to that plan instead of submitting another. The conditions are the
ones the dump shows directly: same pipeline signature, same transform, and nothing between them that
changes render state.

Two things to keep whole:

- **Draw order.** Concatenation preserves the order of the original draws, and a fused plan must not reorder
  across a state change; the run boundary in the dump is that invariant. It does not follow that the depth and
  blend results are unchanged — a merge that pairs the wrong two plans changes them without reordering
  anything — so that is a result to show with the frame comparison below, not a property of concatenation.
- **Smooth Motion's pairing.** The in-betweens pair draws by their key, and a fused draw is one key where
  there were several. The particle tags the interp path relies on (`draw_tag`) have to survive into the fused
  plan, or the sprites that pair by tag stop pairing.

## What to measure

Same five stops as the benchmark tour, headless and rendered, with `scripts/bench_tour.py` and one kept
state, against the build without the change:

- plans a frame from the dump, and the draws a frame the renderer reports (`draws/frame` in `[fps-dip]`), so
  the post-batching count is in the record instead of being inferred from the dump;
- the GX worker's share (`workers: gx=` in `[fps-dip]`, and `perf report --tid` for the worker);
- game frames a second, and the guest checkpoints identical (a drawing change must not touch guest state);
- rendered frames of both builds at each stop, from the same state and the same camera, compared side by side:
  the original frames and the in-between frames Smooth Motion makes, which is where a pairing mistake shows
  before the profile does;
- the checks that already decide whether two draws may be merged — uniforms, textures, depth, interpolation
  state and index ranges — still passing on every fused plan. Equal pipeline signature and equal position
  matrix is not enough on its own, for the reasons at the top of this record.

This record holds only the measurement and the shape of the change. The change itself belongs in
RecompCore; jkoehler11 cannot push there, so it is written up for whoever takes it.
