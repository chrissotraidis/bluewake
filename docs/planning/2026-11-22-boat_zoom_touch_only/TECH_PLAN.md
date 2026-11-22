# Technical plan — Boat zoom: leave the game's data alone while it's off

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Follow-up to [#633 review](https://github.com/chrissotraidis/bluewake/pull/633#pullrequestreview-6011551306)

## Change

- `mouse_camera.c`: `g_boat_style_wrote[]`, one flag per boat style, like Sail on Z's `g_sail_z_wrote`. `boat_zoom_frame` skips a style that isn't scaled now and wasn't scaled last frame; a style it scaled is put back to the table's distance once when the zoom stops (left the boat, zoom back at 1x, gesture off), and the flag clears.
- A save state taken while zoomed keeps the scaled value until the zoom is next used on that style (before, until the next boat frame).

## Verification

- CI builds; on the Deck: zoom on the boat in and out, step off and back, check the camera returns to the game's distance.
