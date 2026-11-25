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

- A new chooser under Display, after Texture filtering: **Edge smoothing**, with Off (the default), FXAA Low, Medium and High, and SMAA Low, Medium and High.
- With it on, jagged edges in the scene are smoothed over the finished picture: FXAA in one pass, the cheapest; SMAA in three, which keeps fine detail sharper and costs more. The HUD and the menus are drawn afterwards and stay as sharp as before.
- The levels set how faint an edge is smoothed and how far along a nearly level or upright edge the pass looks. Higher levels cost a little more.
- It takes effect at once, so it can be compared while playing.
- Nothing changes with it Off.

## Not in scope

- Changing any default, the render resolution's included.
- The Android and iOS menus.
- An automatic render resolution ([#659](https://github.com/chrissotraidis/bluewake/pull/659)): related, apart.

## Possible follow-ups

- **SMAA:** asked for by the owner on 2026-11-24 as the next step, and written in this PR as a second method in the same chooser. To be built and measured after FXAA's bench.
- **Multisampling (MSAA):** needs the sea mask, the sun shadows, the monotone and the HD look's depth passes to work on a multisampled frame, and does nothing inside transparent textures (grass, leaves). A probe is in this PR (2026-11-25, the owner's choice of "probe first"): a startup setting, 4x only, with those effects off while it is on.
- A sharpening step after the smoothing, if fine detail softens.

## Success

- On the owner's Deck, 2x with edge smoothing against 3x as played today, over the twelve bench scenes of #655 and #658: game frames a second and GPU time reported for 2x, 3x, and 2x with the smoothing at Low, Medium and High, with pictures of each as the Deck shows them (1280×720).
- The smoothing pass costs less than 1 ms of GPU time a shown frame at 2x on the Deck.
- The owner prefers 2x with the smoothing to plain 2x in play, and sees no blurred HUD text.

## Where it stands (2026-11-25)

- **Frames a second and GPU time:** measured for both methods on the owner's Deck, its own screen. 2x takes about a third off 3x's GPU time and reaches full speed in every scene; neither method costs a game frame a second.
- **Under 1 ms:** every level of both methods is, a pass (FXAA 0.48 to 0.63 ms, SMAA 0.79 to 0.95, medians); with Smooth Motion on it runs twice a game frame, so only FXAA Low stays under 1 ms a game frame (0.96 ms).
- **Pictures:** all twelve scenes with the owner's HD texture pack, each level against 2x Off: 42 to 62 dB, changes on edges only. Small at the Deck's screen size.
- **The owner's eye:** "barely any difference on the screen for every filter", in play on the Deck.
- **MSAA:** a probe; it changes the picture far more than either filter, but turns the HD look off while on and costs 2.6 ms of GPU time a frame on the Deck at 2x (median; game frames lost in two Outset scenes even with the HD look off). Recommended: stop at the probe; the owner decides.
