# PRD — Edge smoothing (FXAA) for lower render resolutions

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

## Why

On a Steam Deck the screen is 1280×800 and a 16:9 picture is shown at 1280×720. Render resolution 2x and 3x both draw more than that:

| Render resolution | Drawn at (16:9) | Per axis against the 1280×720 shown | Drawn pixels per screen pixel |
|---|---|---|---|
| 1x | 853×480 | 0.67× | 0.44 |
| 2x | 1707×960 | 1.33× | 1.78 |
| 3x | 2560×1440 | 2.00× | 4.00 |

(From `aurora::window::get_window_size()`: the game's 480 lines times the scale, the width from the aspect.)

So 3x draws 2.25 times the pixels of 2x, and on the Deck's own screen the extra only smooths edges when the picture is shrunk. The owner plays at 3x with the HD look and Smooth Motion, where the GPU is the limit in many scenes ([#655](https://github.com/chrissotraidis/bluewake/pull/655), [#658](https://github.com/chrissotraidis/bluewake/pull/658)); at 2x the game runs much better and the owner sees barely any difference.

What 2x gives up is edge smoothing. The renderer has no anti-aliasing a player can turn on: multisampling is in Aurora but has no setting, and the sea mask, the sun shadows and parts of the HD look are written to leave themselves out when it is on.

## What changes for players

- A new chooser under Display, after Texture filtering: **Edge smoothing (FXAA)**, with Off (the default), Low, Medium and High.
- With it on, jagged edges in the scene are smoothed in one pass over the finished picture. The HUD and the menus are drawn afterwards and stay as sharp as before.
- The levels set how faint an edge is smoothed and how far along a nearly level or upright edge the pass looks. Higher levels cost a little more.
- It takes effect at once, so it can be compared while playing.
- Nothing changes with it Off.

## Not in scope

- Changing any default, the render resolution's included.
- The Android and iOS menus.
- An automatic render resolution ([#659](https://github.com/chrissotraidis/bluewake/pull/659)): related, apart.

## Possible follow-ups

- **SMAA:** three passes and two lookup textures; sharper than FXAA on textures, for more GPU time. A second choice in the same chooser if FXAA's softness shows at 2x.
- **Multisampling (MSAA):** needs the sea mask, the sun shadows, the monotone and the HD look's depth passes to work on a multisampled frame, and does nothing inside transparent textures (grass, leaves).
- A sharpening step after the smoothing, if fine detail softens.

## Success

- On the owner's Deck, 2x with edge smoothing against 3x as played today, over the twelve bench scenes of #655 and #658: game frames a second and GPU time reported for 2x, 3x, and 2x with the smoothing at Low, Medium and High, with pictures of each as the Deck shows them (1280×720).
- The smoothing pass costs less than 1 ms of GPU time a shown frame at 2x on the Deck.
- The owner prefers 2x with the smoothing to plain 2x in play, and sees no blurred HUD text.
