# PRD — Automatic render scale (opt-in)

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

Some scenes cost more GPU time at the player's render scale than a frame has, and no cheaper effect closes the gap. Measured on a Steam Deck in Game Mode at 3x with the HD look and Smooth Motion (#655, #658):

| Scene | At 3x | At 2x | GPU busy per shown frame, 3x / 2x |
|---|---|---|---|
| Windfall at night in the rain | 92 % speed | full speed | 18.2 / 14.8 ms |
| The Big Octo fight | 83 % speed | full speed | 24.6 / 15.8 ms |

Most of the game holds full speed at 3x on the same Deck. Today the player picks one scale for the whole game: 3x and slow scenes, or 2x everywhere. A scale that follows the GPU's load keeps 3x where it fits and gives way only where it does not.

## What changes for players

- A new opt-in setting, off by default (working name: **Automatic render scale**), with a floor the player chooses (for example "never below 2x").
- With it on, the chosen render scale is the ceiling. While the GPU cannot keep up, the scene is rendered a step smaller; when it has had room to spare for a while, a step larger again.
- The HUD, text and menus stay at their own size and sharpness.

## What it must not do

The setting is only worth having if the changes go unnoticed. So:

- **No flicker.** The scale never goes back and forth. A step down and the step back up are separated by a long, deliberate gap, and a step up is only taken with clear room to spare.
- **No jolt.** A change is a small step, not a jump from 3x to 2x, and never costs a hitch the player feels. If a change cannot be made without a hitch, it waits for a moment the picture changes anyway (a room load, a fade, a door).
- **No hunting in a place it already knows.** Where a scale was needed before, it starts there next time.
- **No cost when idle.** Deciding takes no GPU profiler and no measurable frame time.

## Not in scope

- Upscalers or temporal reconstruction (FSR and the like).
- Changing any default; lowering the scale below the player's floor.
- Game-thread-bound slowdowns: a smaller picture does not help them, and the setting must leave the scale alone there.

## Success

- The two scenes above hold full speed on the Deck with the setting on (ceiling 3x, floor 2x), and return to 3x after leaving them.
- In a play session through those scenes the owner cannot point at the moment of a change, and the log shows no pair of opposite changes less than the gap apart.
- With the setting on in scenes that already hold full speed: the same retrace rate and GPU time as with it off.
