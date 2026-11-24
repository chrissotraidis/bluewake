# Technical plan — A cheaper effects mask for the HD look's shading

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## What the mask does today

`encode_effect_mask` (RecompCore, `gfx/common.cpp`) runs for each scene pass with effects while sun shadows, ambient occlusion or cloud shadows are on:

- **"Effects mask"**: the pass's effect draws (`DrawData::effect`: blended, writing no depth) drawn again with their own pipelines over black, against the pass's depth, into a full-size target (`post_fx::effect_mask_target`).
- **"Effects mask (over white)"**: the same draws once more over white (`effect_white_target`), for their opacity: a dark effect adds little over black but hides what is behind it. `DOL_AURORA_EFFECT_MASK_OPACITY=0` leaves this pass out.
- With an effect at its own depth (`effectOwnDepth`), each of the two is drawn in two steps against a copy of the sea mask's depth.

So a scene's blended effects are rasterized three times at the render size, and with Smooth Motion again for the in-between frame. In the Big Octo state: "IB Effects mask" 1.74, "Effects mask" 0.90 and "IB Effects mask (over white)" 0.58 ms a shown frame. The shading passes read the mask with `textureLoad` (post_fx.cpp, the occlusion's apply, the sun shade and the cloud shadows).

Efficient soft effects (`DOL_AURORA_EFFICIENT_EFFECTS`) already draws the soft effects themselves at a reduced size; the mask still draws them at the full size.

## What was built (RecompCore #68)

Both under one switch, `DOL_AURORA_EFFECT_MASK_FAST` (read when the mask is first drawn, after the host's settings are in the environment):

1. **Half size.** The mask's two targets are made at half the render size (`effect_mask_size`). The effects are drawn into them with the viewports and scissors halved the way a run of soft effects' are (`g_softScale`), against a half-size depth made for them (`effect_mask_small_depth`): the farthest of each 2x2 of the pass's depth, so an effect in front of any of the four is drawn. For effects at their own depth the half-size depth is made from the sea mask's instead, in place of the full-size copy, and made again for the second target; otherwise it is made once. The cover (`effect_cover_view`) reads the half-size targets between their texels (four `textureLoad`s, weighted); the shading's depth reads the texel over each pixel. With the switch off the shaders' `mask_small` constant is false and the reads are the same `textureLoad`s as before.
2. **One pass.** The mask and the pass over white are the same draws with the same blending over two clears. Each effect gets a two-target variant of its pipeline (`DrawData::maskPipeline`; `PipelineConfig::softTarget` 2, a free value of an existing field, so the pipeline cache keeps what it has): the generated fragment entry becomes a function and a new entry writes its colour to both targets. A pass whose effects all have their variant ready is drawn once into both targets ("Effects mask (both)"); any other keeps the two passes, so the result is the same either way. No variant for an effect with destination alpha (dual-source blending takes one target), a depth texture or an early depth pass. `DOL_AURORA_EFFECT_MASK_SINGLE=0` keeps the two passes, for measuring.

Passes with a stencil keep the full-size path.

Not built: the third idea of the first plan, the soft effects' mask at their own reduced size. With both targets at half size already it has little left to give.

## The switch (BlueWake)

`DOL_AURORA_EFFECT_MASK_FAST`, "Faster effects mask": a Config switch after "Faster Light distance blur", off by default, taking effect at the next start, with its place in the settings test and its own assertions, and the RecompCore pin (#68's head). The renderer reads the variable itself. This branch sits on #655's: both add a switch at the same place in the table.

## Verification, as done and still to do

- Speed, done: the Big Octo state in Game Mode, three runs a setting, interleaved (PROGRESS.md): 50.1 to 53.0 retraces a second, 24.5 to 22.0 ms.
- GPU profile, done: the mask's zones, and "Effects mask (both)" in place of the two passes in that scene.
- Pictures, done: seven scenes on a desktop with the Deck's settings, PSNR 57 dB or better, and the one-pass frames identical to the two-pass ones. A render bench on the Deck over ten states (the eight of the earlier render work, the rain state and the Big Octo fight) with the switch on against off replaces these as the PR's pictures.
- Still to do: a state with effects at their own depth (a lava geyser, a particle's dust), and one with the dark-effect case the pass over white exists for (a door's dust in Dragon Roost Cavern); a look for a hitch at a variant's first use.

## How the scene was measured

As in #655: the owner's Steam shortcut in Game Mode with a save state loaded at boot (`BLUEWAKE_LOAD_STATE`), one variable changed a run through the launcher's env hook, `[perf]` for the retrace rate and `[gpu-prof]` for the GPU. The game thread is busy 87 to 94 % in every run, including those at full speed, so it is not the limit here.
