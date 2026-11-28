# Technical plan — A cheaper "Light" distance blur

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## What Light does today

Wind Waker's depth of field (`drawDepth`) copies the EFB at half size and draws it back over the distance. In `gxcore_draw.cpp` (RecompCore), the step from `aurora_set_game_dof` shapes that copy:

| Step | Copy size | Blur pass (`tex_copy_conv::blur`) |
|---|---|---|
| 0 Off | full render size | none |
| 1 Light | **full render size** | **sigma 1, mixed half in** |
| 2 Default | half render size | none |
| 3 Strong, 4 GameCube | half render size | sigma from the render scale |

So Light is the only step that runs the separable blur on a full-size target: at 3x, two 7-tap passes over the whole render target and a `CopyTextureToTexture` back, in the real frame and again in Smooth Motion's in-between frame. Off makes the same full-size copy without the blur and holds full speed, so the blur passes are the cost, not the copy.

## Change (RecompCore #67), as built

Behind `DOL_AURORA_GAME_DOF_FAST` (`aurora_set_game_dof_fast`; read at the first blur, after the host's settings are in the environment), in `tex_copy_conv::blur`:

- **One half-size pass:** the copy is taken down to half its size with the linear sampler (a 2x2 box), plus four side taps for what the box and the stretch back leave of the sigma. In the copy's texels the box has a variance of 1/4 and the linear stretch back 2/3, so they already make a sigma of about 0.96; Light's is 1, so the side taps weigh about 0.01.
- **One full-size pass:** that half is stretched back over the copy itself with the linear sampler and blended in by `mix` through the blend constant. No scratch target at the full size and no copy back.
- Against two full-size passes of 7 texel loads each and a `CopyTextureToTexture`.
- It only stands in where it fits: a sigma from about 0.96 to 1.25, a `mix` above 0, and a copy that can be a render attachment. Everything else (steps 3 and 4, a converted copy) takes the existing path. The GPU profiler zone is "EFB copy blur (fast)".

This differs from the first plan (two blur passes at half size and a composite reading the sharp copy): the box and the stretch turned out to be nearly the whole of a sigma-1 blur, and blending into the copy removed the scratch and the copy back.

## Change (BlueWake), as built

- `settings_table.h`: `DOL_AURORA_GAME_DOF_FAST`, "Faster Light distance blur", a switch in Config after the other performance switches, off by default, taking effect at the next start. It does not depend on another setting in the table: at the other Distance blur steps it does nothing, as its note says.
- `settings_table_test.cpp`: its place in the file order and its own assertions.
- The RecompCore pin (#67's head).
- The renderer reads the variable itself, as it does `DOL_AURORA_AO_PACKED`; the menu does not pass it on.

## Verification, as done

- Speed in the rain state, Game Mode, two runs a setting, interleaved (PROGRESS.md): 54.5 to 58.5 retraces a second.
- GPU profile: the blur's zone 0.85–0.95 ms to 0.24 ms.
- The render bench on the Deck in Game Mode over ten states (the eight of the earlier render work, this rain state and the Big Octo fight), three ways with #658's switch, four runs each: +4.0 % game frames a second overall with this switch, 4 to 7 % in seven scenes. Two later states (a lava geyser, dark ash) the same way: +6.1 % and +6.7 %.
- Pictures from the bench's picture runs on the Deck at 3x, the worst same-frame pair of each of the twelve scenes: 53.9 dB or better, largest difference 22 of 255. The rain state on a desktop: 62.5 dB.
- Not checked: 1x and 2x, and other platforms.
- Against the PRD's first success line: Light with the switch on reached 58.2 to 58.9 retraces a second against Default's 59.8, so 0.9 to 1.6 short of Default, not within 1.

## How the scene was measured

- The game's session log named the game thread (`cause=game-thread`, busy 91–96 %), but the guest pc sampler (`BLUEWAKE_PC_SAMPLE`) found no hot spot on the Deck: GX 13 %, math 10 %, rain and environment 7 %, particles 4.5 %, actor search 4 %, the rest under 1.5 % each.
- At render scale 2 the game thread is as busy (90 %) and the game holds full speed, so the GPU sets the pace: `gpu_busy_percent` 100, `[gpu-prof] busy_ms` 18.2 at 3x and 14.8 at 2x.
- GXSetDrawDone's flush, timed with `BLUEWAKE_GX_FLUSH_CENSUS` (a local change): 510 ms over 300 frames, 13 of them over 5 ms, up to 51 ms. A symptom of the GPU running over, not a cause.
- `[gpu-prof]` note: the "Staging copies" zone reads 8 ms a frame at 3x and 5.5 ms at 2x for the same 4.8 MB of vertex data. It wraps four buffer copies between render passes, so it is taken to hold the tail of the pass before it rather than the copies.
