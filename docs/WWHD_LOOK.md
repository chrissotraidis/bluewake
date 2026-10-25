# The Wind Waker HD look

BlueWake is rebuilding the look of *The Wind Waker HD* (Wii U, 2013) on top of the GameCube game, one piece at a
time. Each piece is an option, off by default, and one switch turns on every piece that exists so far:

- **Mac, Windows and Linux:** F1 › Display › **Wind Waker HD look**, or **F2** in the game to switch between it and the GameCube look. It applies straight away and is kept.
- **iPhone and iPad:** Mods › **Wind Waker HD Look**, then restart BlueWake.
- **Android:** Mods › **Wind Waker HD Look**, then restart BlueWake (experimental own-disc builds, [ANDROID.md](ANDROID.md)).
- **From a terminal:** `BLUEWAKE_HD_LOOK=1`.

Each piece also has its own variable, which wins over the switch (`=1` on, `=0` off), so a piece that costs too
much on a small GPU can be left out.

**Known issues** (October 17): with Smooth Motion above 60 FPS, the effects dropped out of most in-between frames
and the shadows slid against the ground; pdale-boop's draft #371 (RecompCore#40) fixes that. Fixed in `main`: the
softer default (#321: sun shadows off, gentler ambient occlusion, cloud shadows, bloom and blur), and cloud shadows
that looked square and banding on the sea (RecompCore#41: the sea's place from its plane, dithered shades). The last
is checked on the Mac; Vulkan, where jslay88 saw it, still needs a look.

On Mac, Linux and Windows, safe startup turns the HD look off and clears the eight `BLUEWAKE_HD_*`
per-effect overrides listed below. F2 or the menu can turn the whole look back on during that session.
The reset is saved only after the earlier settings file is backed up; if the backup fails, or Mac/Linux
uses `BLUEWAKE_SETTINGS=none`, startup resets only the session. Normal startup keeps the usual overrides.

## Turning one piece off

Each piece has its own switch, which wins over the main one: `=0` turns it off, `=1` on. The names are
`BLUEWAKE_HD_AO` (ambient occlusion), `BLUEWAKE_HD_SUN_SHADOWS`, `BLUEWAKE_HD_CLOUD_SHADOWS`, `BLUEWAKE_HD_BLOOM`,
`BLUEWAKE_HD_FAR_BLUR`, `BLUEWAKE_HD_EDGE_LIGHT`, `BLUEWAKE_HD_TORCH_LIGHT` and `BLUEWAKE_HD_DISTANT_ISLANDS`.

- **Mac and Linux:** add a line such as `BLUEWAKE_HD_AO=0` to the settings file
  (`~/Library/Application Support/BlueWake/settings.ini` on the Mac, `~/.config/BlueWake/settings.ini` on Linux)
  while BlueWake isn't running. The menu keeps lines it doesn't manage.
- **Windows:** set it in PowerShell before starting the game: `$env:BLUEWAKE_HD_AO="0"; .\BlueWake.exe`.
- Menu switches for each piece are planned with the new desktop menu (#368).

## Where this comes from

Elliott Tate showed most of the HD look running on the GameCube recompilation in a video on October 3, 2026
("I Backported All The Zelda Wind Waker HD Features to the Gamecube Recompile"). His code isn't in any public branch,
so we're rebuilding it from what the video shows, from the decompilation ([zeldaret/tww](https://github.com/zeldaret/tww))
and from a Wind Waker HD dump for reference. Nothing from the Wii U disc is copied into BlueWake: the HD textures
come from the player's own disc through [WWHD_TEXTURES.md](WWHD_TEXTURES.md).

## The pieces

| Piece | In the video | Status | Variable |
| --- | --- | --- | --- |
| HD textures and HUD | HD menus and HUD with Wii U button labels | Imported from your own Wii U dump ([WWHD_TEXTURES.md](WWHD_TEXTURES.md)) and chosen in **Mods › Wind Waker HD textures**, or made in the app with **Import from Wind Waker HD…** (#315) | `DOL_AURORA_TEXTURE_PACK` |
| Distant islands | Distant islands +100,000 units | **In.** Off by default | `BLUEWAKE_HD_DISTANT_ISLANDS` |
| Bloom | Bloom | **In**, with the switch (RecompCore 0169). Off by default | `BLUEWAKE_HD_BLOOM` |
| HD's far-distance blur | HD depth of field | **In**, with the switch (RecompCore 0170). Off by default | `BLUEWAKE_HD_FAR_BLUR` |
| Ambient occlusion | Ambient occlusion | **In**, with the switch (RecompCore 0171). Off by default | `BLUEWAKE_HD_AO` |
| Cloud shadows on the sea | Cloud shadows | **In**, with the switch, outdoors (RecompCore 0172). Off by default | `BLUEWAKE_HD_CLOUD_SHADOWS` |
| Sun shadows | Sun shadows (resolution, strength) | **In**, outdoors, from a shadow map of the whole scene (RecompCore 0173, 0183). Left out of the HD look switch: from the screen alone, on open ground it drew long dark streaks. Only its own variable turns it on | `BLUEWAKE_HD_SUN_SHADOWS` |
| Edge light | Character edge light | **In**, with the switch, outdoors (RecompCore 0174). Off by default | `BLUEWAKE_HD_EDGE_LIGHT` |
| Torch light | Torch and fire light | **In**, with the switch, everywhere (RecompCore 0175). Off by default | `BLUEWAKE_HD_TORCH_LIGHT` |

### Distant islands

Every island on the Great Sea has a simple far-away model (the `d_a_lod_bg` actor, process name `0x1C3`). The game
shows it while you're within the actor's `scale.x` (its placed scale times 20,000 units: 150,000 for most islands,
250,000 to 350,000 for landmarks such as Tower of the Gods) and unloads it 20,000 units past that. HD draws them
further out. With the option on, the host adds 100,000 units to both distances on every island model once a frame
(`runtime/host/src/hd_look.c`). It changes only those two numbers in the actor, so the game loads, fades and draws
the models with its own code, and turning the option off puts the numbers back.

**Only as many as fit (#308).** The game keeps every far-away model in one 128 KB heap of its own
(`d_a_lod_bg` makes it with `JKRCreateExpHeap(0x20000, …)`), sized for its own distances. A model that doesn't fit
takes the largest free block of the archive heap instead, the heap that later holds stage and boss archives, and
keeps it. With every island shown 100,000 units further, sea room 20 had 22 models loaded and 5 of them in the
archive heap; pdale-boop's game, after a ten-minute sail, later waited forever for Puppet Ganon's archive. So the host
now extends only as many islands as fit: the game's own first, then the nearest of the others, up to 15 models
loaded (17 fitted at the busiest place measured). If a model is ever found outside the island heap anyway (its data
heap's parent, from JSystem's heap tree, is not the 128 KB one), the budget drops below the count at which it
happened, for the rest of the session. Measured from the stage select at sea rooms 11, 14, 20, 32 and 44: 13 to 15
models loaded (7 to 13 without the option), none outside the island heap.

The game also lowers a far-away model below the sea by a tenth of its distance past 150,000 units, so islands that
are now visible sit low on the horizon and rise as you sail closer, like the curve of the earth. That part is kept.

Checked on an M3 Max from an Outset save state (before the budget above): 15 island models loaded instead of 7, a second island on the
horizon in a captured frame, no lookup failures. `BLUEWAKE_HD_TRACE=1` logs each island model it changes and,
every 300 retraces, the ones within reach with their distance and whether they're loaded.

Costs: more island models in memory at once (the game's own small models, loaded from `LODALL.arc` in ARAM) and a
walk over the actor list once a frame while the option is on. Nothing runs while it's off.

### Bloom

Wind Waker draws its 3D scene, copies the finished scene out at full size, and then draws the HUD flat on top. Bloom
runs in Aurora just before that copy: whatever is brighter than a threshold is blurred at a quarter of the size and
added back, so clouds, foam and white highlights glow while the HUD stays sharp. It's six small GPU passes a frame
and nothing while it's off. `DOL_AURORA_BLOOM_THRESHOLD` (default 0.72) and `DOL_AURORA_BLOOM_STRENGTH` (BlueWake starts it at 0.3; Aurora's default
0.35) tune it; `DOL_AURORA_PASS_TRACE=1` logs one frame's passes.

Checked on an M3 Max from an Outset save state: the clouds glow and the HUD is pixel for pixel the same as without
bloom; Smooth Motion runs with it. Not yet checked on Windows, Linux, a Deck or a phone.

### Far blur

HD softens the distance. At the same point as bloom, the scene is blurred at a quarter of the size and blended back
by distance, before bloom. Aurora's depth gives the distance; Wind Waker draws the sky, the sea toward the horizon and
the islands' far models without depth, so those get a lighter amount of their own. At Outset's pier, Link is about
240 near-plane units away and the island's cliffs about 3,700, so the blur starts at 8,000 and is full at 30,000:
the island stays sharp and the horizon softens. `DOL_AURORA_FAR_BLUR_START`, `_END`, `_AMOUNT` (BlueWake starts it at 0.4; Aurora's default 0.8) and
`_BACKGROUND` (0.35) tune it, and `DOL_AURORA_FAR_BLUR_DEBUG=1` shows the distance as brightness. It needs the
renderer's full feature set, so it stays off in compatibility mode.

Checked on an M3 Max from an Outset save state: the horizon, far islands and sky soften, the island and the HUD stay
sharp. The switch alone gives the same frame as turning on each part by hand.

### Ambient occlusion

HD darkens creases and the ground where things meet it. Before the far blur and bloom, from the depth buffer alone, at
a quarter of the size: around each pixel, six pairs of samples on opposite sides, in a ring that shrinks with
distance. A pair whose average is nearer than the pixel (a crease, a contact) darkens it; on a flat floor the two
samples straddle the pixel, so it stays as it was. Where there is no depth (the sky, the horizon) and across big
steps (a silhouette in front of the background) nothing is darkened. `DOL_AURORA_AO_RADIUS` (default 30) and
`_STRENGTH` (BlueWake starts it at 0.3; Aurora's default 0.6) tune it, and `DOL_AURORA_AO_DEBUG=1` shows the occlusion alone.

Checked on an M3 Max from an Outset save state: darker under Link's feet, round the pier's posts and in the cliffs'
creases; the pier's deck and the sea unchanged.

### Cloud shadows

Soft shadows drift over the sea and the islands, as in HD. Outdoors (the stages the game marks as outdoors or the sea), the host reads the game's camera once a
retrace (the current view's inverse view matrix, field of view, aspect and near plane) and passes it to Aurora. After
ambient occlusion, each pixel's place in the world comes from that camera and the scene's depth, or, below the horizon
where there is no depth, from where the view ray meets the sea. Smooth noise in world X/Z, drifting slowly, darkens it
and fades out with distance. `DOL_AURORA_CLOUD_SHADOWS_STRENGTH` (BlueWake starts it at 0.15; Aurora's default 0.28) and `_SCALE` (world units across a cloud,
7,000) tune it; `DOL_AURORA_CLOUD_SHADOWS_DEBUG=1` paints a 1,000-unit world grid to check the camera.

Checked on an M3 Max from an Outset save state: the grid sits square on the pier and the cliffs, and the shadows show
on the open sea and the cove. Because the camera is read once a retrace, the pattern may trail by a frame in a fast
turn; not checked by hand.

### Sun shadows

HD casts sun shadows across the whole scene. Outdoors (the stages the game marks as outdoors or the sea), the host also passes the direction to the sun
(`g_env_light.mSunPos`, which the game keeps at the camera's eye plus that direction). After ambient occlusion, from each
pixel's place in view space, 24 steps go toward the sun; a step that lands behind a surface in the depth buffer is in
shadow. It's drawn at a quarter of the size, blurred and multiplied in, and fades with distance and when the sun is low.
Because it works from the screen, something off screen casts nothing. `DOL_AURORA_SUN_SHADOWS_STRENGTH` (0.35) and
`_REACH` (world units, 1,500) tune it.

Since RecompCore 0183 they come from a shadow map instead, so a building beside or behind
the camera still puts Link in its shade. Before each pass of the scene, its solid draws are drawn again from the sun
into depth maps, four of them each 2048×2048 and four times wider than the last (1,250 to 80,000 units ahead of
the camera, so near shadows stay sharp and an island ahead still casts; long shadows hold until the sun is 3
degrees up, and at night the moon casts at half strength); each pixel's place in the world is
then looked up there. The game's own culling still leaves out what is far off screen, but at Windfall it hides only a
few of the town's 42 room pieces, so the buildings are nearly always in the map. Leaves, fences and grass (alpha-tested)
cast their texture's cutout, the sail and flags cast, and blended furniture (a chest) casts while its vertices stand
still. The map's shadows are filtered at full resolution over a turned disc of lookups, without the screen's steps
(`DOL_AURORA_SUN_SHADOW_MAP_STEPS=1` brings them back), and the game's own shadows are left out while the map is on
(`DOL_AURORA_GAME_SHADOWS=1` keeps them). Surfaces turned away from the sun are in shade too, which the
screen's version never did. `DOL_AURORA_SUN_SHADOW_MAP=0` goes back to the screen's version alone, for comparing the two;
`DOL_AURORA_SUN_SHADOW_MAP_SIZE`, `_RANGE`, `_BIAS`, `_CONTACT` tune it, `_DEBUG=1` shows
its shadow alone (gray outside the map), `_DEBUG=2` to `5` each cascade's map as contour bands, `6` the cascades tinted, and `_TRACE=1` logs how many draws it took. With one map it cost 0.04 to 0.07 ms of GPU time
a frame on an RTX 5070 Ti. [HD_SUN_SHADOW_MAP.md](HD_SUN_SHADOW_MAP.md) has how it works, how it was tested and
what's left, for whoever works on it next.

Checked on Arch (i5-12600KF, RTX 5070 Ti) from the stage select, headless warps to Windfall rendered at noon and 4 pm:
at noon Link, the boat and the tower cast shadows on the pier and the water; at 4 pm the town square is in the shade of
a building behind the camera, which the screen's version left lit.

On an iPad Pro (M2), walking across Outset's grass, it drew long dark diagonal streaks across the ground, so the HD look
switch leaves it off (October 13). `BLUEWAKE_HD_SUN_SHADOWS=1` still turns it on. With the shadow map, pdale-boop played
Outset on Arch (RTX 5070 Ti, Vulkan) with sun shadows on and saw no streaks (October 17); not yet checked on Metal.

Checked on an M3 Max from an Outset save state with the sun high: Link casts a soft shadow on the pier away from the
sun, and the ladder's post and the pier's edges darken; the HUD is unchanged.

### Edge light

HD rims characters with light on their sunlit side. After sun shadows, for anything within 1,500 units of the camera,
the renderer looks a few pixels toward the sun on screen: where the depth there jumps far behind or there is none, the
pixel is on a sunlit outline and gets a warm rim. It isn't limited to characters; anything near with sky or something
far behind it on the sun's side gets it. `DOL_AURORA_EDGE_LIGHT_STRENGTH` (0.4) tunes it.

Checked on an M3 Max from an Outset save state: Link's arm, sleeve and shirt on the sun's side, and the ladder's posts,
get a warm rim; the HUD is unchanged.

### Torch light

On the GameCube, a torch lights the models near it but not the walls and floor. With the switch, the host reads the
game's own point lights each retrace (`g_env_light.mpPLights`: torches, fires, lanterns, bombs) and passes the 16 nearest
the camera to Aurora, which lights every surface around them from the depth, with a smooth falloff and a slight flicker
for the lights the game marks as fluctuating. `DOL_AURORA_TORCH_LIGHT_STRENGTH` (0.35) and `_REACH` (the glow's radius,
times the game's own, 3) tune it.

Checked on an M3 Max from a Dragon Roost Cavern save state: five lights near the camera, and the wall around the torch
glows warm; the floor and the HUD are unchanged.

