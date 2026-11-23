# Technical plan — Controller camera zoom while sailing

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Extends [#213](https://github.com/chrissotraidis/bluewake/pull/213)

## Why it didn't work

`mouse_camera.c` makes the gesture available only when the camera's style uses the follow camera (engines 1 and 18), and scales that camera's distance limits. On the boat the camera is the game's `rideCamera` (engine 8). The right stick goes to the game's C-stick there, and the gesture is off.

## The boat's camera styles

From GZLE01's `dCamParam_c::styles` in the DOL's data (0x84 bytes each: name, engine, 30 parameters, flags), the engine-8 styles are BW07, BP07, BN07, BP08, BN08, BE08 and BE07. Their parameter 10 is the distance: 800, 900, 800, 1000, 1000, 500 and 750. The follow camera's parameter 10 is also its far distance.

## Change

- `kBoatStyles`: those seven style addresses with the game's distances.
- `boat_zoom_frame`, at `camera_draw` every frame: when the style in use is one of them, the player is in control, and the zoom switch is on, it advances `g_boat_zoom` by the gesture and writes the game's distance times `g_boat_zoom` into that style's parameter 10. Every other boat style is put back to the game's distance. It writes the target the camera eases to, from a constant each time, so nothing compounds, and the game's own easing makes it smooth.
- The gesture is available on the boat (`g_stick_boat_zoom`). A tap doesn't start the first-person hold there. While the gesture is zooming, and until the stick is back at the centre, the C-stick is zeroed so the boat's camera doesn't also turn.
- Controls table and the setting's note say it works on foot and on the boat.

## Risks

- The style table is game data, shared by every boat camera. Only parameter 10 of the style in use changes, and it is reset to the game's value whenever that style isn't in use or the zoom is at 1x.
- A save state taken while zoomed keeps the scaled value. The next boat frame here writes the right value again.
- The addresses are GZLE01 revision 0, the only disc BlueWake accepts.

## Verification

- CI builds.
- By hand: sail, zoom in and out with the gesture, check the camera doesn't turn while zooming, a tap does nothing, and the distance on foot is unchanged; leave the boat and come back, and the boat keeps its own zoom.
