# PRD — Controller camera zoom while sailing

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Extends [#213](https://github.com/chrissotraidis/bluewake/pull/213)

## Why

#213's controller zoom (hold the right-stick click and move the stick up or down) changes the follow camera's distance on foot. On the boat it does nothing, so the only camera distance while sailing is the game's.

## What changes for players

- With **Hold the right-stick click and move it up/down to zoom** on, the same gesture moves the boat's camera closer or farther while sailing.
- The boat keeps **its own distance**, separate from the one on foot, within the same limits (0.5x to 2x) and at the same speed setting.
- While zooming on the boat, the camera doesn't also turn. Afterwards the right stick is the game's own boat camera again, as today.
- A tap of the click on the boat does nothing, as today. On foot it still enters first person.

## Not in scope

- The mouse wheel on the boat.
- Other cameras the game controls: Z-targeting, cutscenes, fixed cameras.

## Success

- On the boat, the gesture zooms in and out smoothly between 0.5x and 2x, and the boat camera doesn't turn while it does.
- On foot, nothing changes. Off the boat, every boat camera style is back at the game's own distance.
