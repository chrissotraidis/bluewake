# Technical plan — A cheaper effects mask for the HD look's shading

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## What the mask does today

`encode_effect_mask` (RecompCore, `gfx/common.cpp`) runs for each scene pass with effects while sun shadows, ambient occlusion or cloud shadows are on:

- **"Effects mask"**: the pass's effect draws (`DrawData::effect`: blended, writing no depth) drawn again with their own pipelines over black, against the pass's depth, into a full-size target (`post_fx::effect_mask_target`).
- **"Effects mask (over white)"**: the same draws once more over white (`effect_white_target`), for their opacity: a dark effect adds little over black but hides what is behind it. `DOL_AURORA_EFFECT_MASK_OPACITY=0` leaves this pass out.
- With an effect at its own depth (`effectOwnDepth`), each of the two is drawn in two steps against a copy of the sea mask's depth.

So a scene's blended effects are rasterized three times at the render size, and with Smooth Motion again for the in-between frame. In the Big Octo state: "IB Effects mask" 1.74, "Effects mask" 0.90 and "IB Effects mask (over white)" 0.58 ms a shown frame. The shading passes read the mask with `textureLoad` (post_fx.cpp, the occlusion's apply, the sun shade and the cloud shadows).

Efficient soft effects (`DOL_AURORA_EFFICIENT_EFFECTS`) already draws the soft effects themselves at a reduced size; the mask still draws them at the full size.

## Candidates, to measure in this order

1. **Half-size mask targets.** Both targets at half the render size, tested against a half-size depth (the farthest of each 2x2, so an effect in front of any of the four counts), read by the shading passes with a linear sample. A mask is a soft weight; its consumers are low-frequency. Expected: about a quarter of the fill for both passes.
2. **One pass for black and white.** The two passes draw the same draws with the same blending over different clears: two colour attachments in one pass, the effect pipelines' fragment output written to both. Halves the vertex and raster work; needs a two-target variant of each effect pipeline, so it costs pipelines (and their first-use compiles).
3. **The soft effects' mask at their own size.** With Efficient soft effects on, draw the mask for those effects into the reduced-size soft target's size instead of the full one.

1 is the first to build: it needs no new pipelines per effect and takes both passes at once. 2 and 3 only if 1 leaves a visible cost or a visible difference.

## The switch

`DOL_AURORA_EFFECT_MASK_FAST` (working name), off by default, read when the renderer starts; in BlueWake a Config switch beside the other Performance switches, with its test entries, and the RecompCore pin.

## Verification

- Speed: the PRD's table again with the switch on, in Game Mode, at least two runs a setting, interleaved.
- GPU profile (`DOL_AURORA_GPU_PROF=1`): the three mask zones before and after.
- Pictures: the Big Octo state, a lava room (effects at their own depth), a door's dust (the opacity case the white pass exists for) and a plain view; switch off against on at 1x, 2x and 3x; PSNR and the pictures in the PR.

## How the scene was measured

As in #655: the owner's Steam shortcut in Game Mode with a save state loaded at boot (`BLUEWAKE_LOAD_STATE`), one variable changed a run through the launcher's env hook, `[perf]` for the retrace rate and `[gpu-prof]` for the GPU. The game thread is busy 87 to 94 % in every run, including those at full speed, so it is not the limit here.
