# PRD — A cheaper "Light" distance blur

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

In a rainy Windfall at night, a Steam Deck at 3x with the HD look and Smooth Motion runs the game at about 92 % speed (27–28 game frames a second). The GPU is the limit: about 18 ms of GPU work for each shown frame against a 16.7 ms budget.

One setting is most of the gap. **Distance blur: Light** (`DOL_AURORA_GAME_DOF=1`) costs 4 to 5 retraces a second in this scene; every other step of the same setting, Off included, holds full speed.

Measured in Steam Deck Game Mode through the owner's shortcut and settings, a save state loaded at boot, about 25 s each, build `23eb4da30` (main at 0.13.0 + #642 + #643):

| Distance blur | Retraces a second (60 = full speed) | GPU busy per shown frame |
|---|---|---|
| Off (0) | 59.8 | 18.4 ms |
| **Light (1)** | **55.4, 54.6** | 18.2, 18.5 ms |
| Default (2) | 59.7 | 17.4 ms |
| Strong (3) | 58.8 | 17.3 ms |

The other HD effects, one off at a time in the same scene with Light on: ambient occlusion 59.6 and 58.7, far blur 57.4, cloud shadows 56.2, bloom 56.0, edge light 55.2 (baseline 55.2). Render scale 2 instead of 3: 60.0. Smooth Motion off: 59.9.

## What changes for players

- A new opt-in switch under Config's Performance section, off by default: **Faster Light distance blur**.
- With it on, Distance blur: Light looks the same to the eye and costs about what Default does.
- Nothing changes with the switch off, or at any other Distance blur step.

## Not in scope

- The far blur (second in the table above); a follow-up if this one works out.
- Changing any default.
- Ambient occlusion, the other effect this scene is sensitive to (off: 59.6, 58.7); #586's packed AO is already on here.
- An intermittent crash seen in 2 of about 21 measurement runs, near the end of the run (ImGui's draw in the deferred present on the render worker): reported apart.

## Success

- In the rain scene on the Deck at 3x, Light with the switch on holds within 1 retrace a second of Default.
- Pictures with the switch on against off: PSNR reported, and no difference the owner sees in play.
