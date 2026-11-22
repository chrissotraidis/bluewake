# Technical plan — Sail on Z

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## How the game keeps X, Y and Z

There are two copies:

- The save's `dSv_player_c` `mSelectItem`: one inventory slot per button, at 0x803C4C11 (X), 12 (Y), 13 (Z). The memory card keeps these.
- The play state's copy of the item numbers: 0x803CA7DB (X), DC (Y), DD (Z). Link reads these when a button is pressed (`mouse_camera.c`'s test-item notes).

Checked on a save state from the owner's Deck: the save's X/Y/Z slots were 0x0E/0x00/0xFF, and the play copy was 0x50/0x20/0xFF, which is `mItems[0x0E]`, `mItems[0]`, none. The sail is `mItems[1]` = 0x78.

## When Link is on the boat

`camera_draw`'s style engine is 8 (`rideCamera`) with the player in control. In GZLE01's `dCamParam_c::styles`, every engine-8 style is a boat style: BW07, BP07, BN07, BP08, BN08, BE08 and BE07.

## Change

All in `mouse_camera.c`, which already runs at `camera_draw` and at the player's update on every platform, so no build files and no new dispatch compares on the hot path:

- `camera_frame` sets `g_on_boat`.
- `sail_on_z_frame`, at the player's update before `aim_frame`: with the option on, on the boat, and with the sail owned, it writes 0x78 to the play copy's Z. Otherwise, if the play copy's Z is the sail and the save's Z slot isn't, it writes back the item in the save's Z slot (0xFF for none). It only reads or writes once the option has been on in this run.
- `BLUEWAKE_SAIL_ON_Z` in the settings table (Gameplay, group "Sailing", off by default). The menu reloads it with the stick and mouse settings.

## Risks

- If the game rewrites the play copy between the player's update and Link reading Z in the same frame, Z would be the player's item on that frame. The write happens at every update on the boat, so at worst a single press would be missed. Check it by hand.
- The HUD's Z icon may show the sail on the boat. That's intended; check it.
- A save state taken on the boat holds the sail in the play copy. Loaded with the option on, the next update fixes it either way. Loaded with the option off in a run where it was never on, the play copy keeps the sail until the game rebuilds it (a scene change). The memory card's Z is never affected.

## Verification

- CI builds.
- By hand: put a bottle on Z, board the boat, and Z raises and lowers the sail. Step off and Z is the bottle. Save on the boat, reload the save, and Z is the bottle. Turn the option off and Z is the bottle on the boat too.
