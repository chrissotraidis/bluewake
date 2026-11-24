# PRD — A cheaper effects mask for the HD look's shading

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

The Big Octo fight (sea room 17) runs at about 83 % speed on a Steam Deck at 3x with the HD look and Smooth Motion: 50 retraces a second, 25 game frames. The GPU is the limit, about 24.6 ms of work for each shown frame against a 16.7 ms budget. Unlike the rainy Windfall of #655, no single effect is the gap.

Steam Deck Game Mode, the owner's shortcut and settings (with #655's faster Light blur on), a save state loaded at boot, about 25 s a run, build `128888488` (main with #642 and #643, #655, RecompCore #67):

| Run | Retraces a second (60 = full speed) | GPU busy per shown frame |
|---|---|---|
| As played (3x, HD look, Smooth Motion) | 49.9, 49.9, 50.0, 50.3 | 24.6 ms |
| Render scale 2 | 60.0 | 15.8 ms |
| Smooth Motion off | 60.0 | (27.1 ms a game frame) |
| Every HD effect below off together | 60.0 | 17.3 ms |
| Ambient occlusion off | 52.6 | 22.9 ms |
| Far blur off | 51.9 | 23.9 ms |
| Distance blur: Default | 51.2 | 23.9 ms |
| Bloom off | 51.1 | 23.9 ms |
| Cloud shadows off | 50.6 | 23.8 ms |
| Edge light off | 50.1 | 24.3 ms |
| **The mask's second pass off** (`DOL_AURORA_EFFECT_MASK_OPACITY=0`) | **52.8, 52.5** | **22.6 ms** |
| Efficient soft effects off | 38.7 | 34.1 ms |

The effects together cost about 7.3 ms a frame, but one at a time they come to about 4.6 ms. Much of the rest is the **effects mask**: every blended effect of the scene (here the octopus's sprays and the sea's splashes) drawn again over black and again over white at the full render size, so the sun shadows, the occlusion and the cloud shadows leave those pixels alone. In this scene its zones read 3.2 ms a shown frame; its second pass alone is 2.1 ms of GPU time and 2.5 retraces a second.

## What changes for players

- A new opt-in switch under Config's Performance section, off by default (working name: **Faster effects mask**).
- With it on, scenes with many particles and the HD look's shading cost less GPU time; the picture should be the same to the eye.
- Nothing changes with the switch off.

## Not in scope

- Holding this scene at full speed at 3x: about 8 ms has to go and the mask is 2 to 3 of them. That needs the render scale to follow the GPU's load, which is its own plan.
- Changing any default.

## Success

- In the Big Octo state on the Deck at 3x, at least 2.5 retraces a second more with the switch on, and a lower `busy_ms`.
- Pictures with the switch on against off, in a particle-heavy scene and a plain one: PSNR reported, and no difference the owner sees in play.
