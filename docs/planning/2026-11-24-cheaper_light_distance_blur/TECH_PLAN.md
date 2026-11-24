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

## Change (RecompCore)

Behind `DOL_AURORA_GAME_DOF_FAST` (`aurora_set_game_dof_fast`), step 1 only:

- Run the two blur passes on a half-size scratch target (a bilinear downsample of the copy as the first pass's source), with sigma halved so the blur covers the same picture distance.
- Compose into the full-size copy in one pass: the sharp full-size copy and the half-size blurred one, mixed half in as today. This replaces the second full-size pass and the copy back.
- Cost: two half-size passes (a quarter of the pixels each) and one full-size composite with two taps, against two full-size 7-tap passes and a full-size copy.

To settle while building: whether the composite can write the copy in place or needs the scratch and a copy back; if the half-size blur shows on thin bright edges at 1x and 2x, keep full size below a render scale of 3.

## Change (BlueWake)

- `settings_table.h`: `DOL_AURORA_GAME_DOF_FAST`, a switch in Config's Performance section, off by default, needing `DOL_AURORA_GAME_DOF`; `desktop_menu.cpp` passes it on; `settings_table_test.cpp` lists it.
- The RecompCore pin.

## Verification

- Pictures: the rain state and a clear-day distance view, switch off against on, at 1x, 2x and 3x; PSNR and the pictures in the PR.
- Speed: the table in the PRD again with the switch on, in Game Mode, 3 launches a setting.
- GPU profile (`DOL_AURORA_GPU_PROF=1`): the "EFB copy blur" zone before and after.

## How the scene was measured

- The game's session log named the game thread (`cause=game-thread`, busy 91–96 %), but the guest pc sampler (`BLUEWAKE_PC_SAMPLE`) found no hot spot on the Deck: GX 13 %, math 10 %, rain and environment 7 %, particles 4.5 %, actor search 4 %, the rest under 1.5 % each.
- At render scale 2 the game thread is as busy (90 %) and the game holds full speed, so the GPU sets the pace: `gpu_busy_percent` 100, `[gpu-prof] busy_ms` 18.2 at 3x and 14.8 at 2x.
- GXSetDrawDone's flush, timed with `BLUEWAKE_GX_FLUSH_CENSUS` (a local change): 510 ms over 300 frames, 13 of them over 5 ms, up to 51 ms. A symptom of the GPU running over, not a cause.
- `[gpu-prof]` note: the "Staging copies" zone reads 8 ms a frame at 3x and 5.5 ms at 2x for the same 4.8 MB of vertex data. It wraps four buffer copies between render passes, so it is taken to hold the tail of the pass before it rather than the copies.
