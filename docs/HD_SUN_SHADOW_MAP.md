# The HD look's sun shadow map: how it works and how to keep going

Work in progress (RecompCore#42, BlueWake#398). This page is for whoever picks it up next: what it does, where the
code is, how it was tested, and what's left. [WWHD_LOOK.md](WWHD_LOOK.md) has the player-facing description.

## The problem it solves

The HD look's first sun shadows (RecompCore 0173) were screen-space: for each pixel the shader stepped toward the sun
through the frame's depth buffer. A step that left the screen counted as lit, so anything off screen cast nothing.
pdale-boop's report: "objects that cast shadows have no permanence. As soon as something is off screen, even walking
between buildings, suddenly you're in light again." On open ground the screen version also drew long dark streaks,
which is why #321 took sun shadows out of the HD look switch.

Two things could hide a caster: the screen-space method, and the game culling what the camera can't see. A headless
sweep of Windfall's 24 spawn points (counting what `J3DUClipper::clipByBox` hides) found the room model hides only 0
to 11 of its 42 shapes, usually 2 to 7, because it culls by large joint boxes. So the buildings are nearly always
drawn, and the screen-space method was the main cause. That's why the shadow map came first.

## How it works

Turned on by `BLUEWAKE_HD_SUN_SHADOWS=1` (BlueWake's `runtime/host/src/hd_look.c` calls
`aurora_set_sun_shadow_map(true)` with it). `DOL_AURORA_SUN_SHADOW_MAP=0` keeps the old screen-only shadows.

1. **The sun and camera.** Every retrace, `hd_look.c` `clouds()` reads the game's sun position and camera and hands
   them to Aurora (`aurora_set_sun_direction`, `aurora_set_world_view`).
2. **Drawing the map.** Before each EFB pass, `common.cpp` `encode_sun_map()` draws that pass's casters again from
   the sun into a set of depth maps (the frame's first pass clears them). Every gxcore draw already keeps its full
   transform state in its constants, which is what Smooth Motion relies on to re-encode a frame, so one matrix,
   `sunOrtho × sunView × cameraInvView`, takes each draw from the camera's view space into the map. A few fixed
   pipelines (`gxcore_draw.cpp` `make_sun_caster_pipelines()`: current matrix or indexed positions, with or without a
   texture for cutouts) draw them all with each draw's own constants, so nothing is compiled per draw.
   There are four maps (cascades), each 2048×2048 and four times wider than the one before: 1,250, 5,000, 20,000
   and 80,000 world units from their centres, which sit half their range ahead of the camera. The first has a texel
   about the width of Link's leg, so his shadow starts under his shoes; the last reaches an island you sail toward.
   Each moves by whole texels so its edges don't crawl. The casters are drawn once per cascade. In an in-between
   frame the draws use their blended constants and that frame's camera.
3. **Which draws cast** (`gxcore_draw.cpp`):
   - `sun_caster()`: writes depth and colour, not blended, triangles, and its alpha test keeps every pixel. Scenery
     and characters.
   - `sun_caster_alpha()`: the same but its alpha test drops pixels (leaves, fences, roofs). Its pipeline samples
     the texture and discards below half, so it casts its cutout.
   - Cloth: a scope over draws that index their positions (the sail, flags) casts solid whenever it writes depth,
     even though the game draws it blended.
   - Blended furniture: a blended, depth-writing, indexed draw whose positions stood still since the frame before
     (a chest, Beedle's ship, a Wizrobe). The sea is excluded because it rewrites its grid every frame.
   - Left out: what's blended otherwise (water, clouds, particles), what writes no depth (the sky), lines and points.
4. **The game's own shadows are left out** while the map is on, so they don't darken things a second time. Its
   projected shadows are recognised by their blended re-draw of the ground, and its blob and volume shadows by the two
   fixed position arrays they index (`d_drawlist`'s `l_shadowVolPos` 0x8037C554 and `l_simpleShadowPos` 0x8037C7DC).
   `DOL_AURORA_GAME_SHADOWS=1` keeps them.
5. **Looking it up** (`post_fx.cpp`). The sun shadow pass takes each pixel's world position into the sharpest
   cascade that holds it (blending into the next near its edge) and checks 12 texels in a disc two texels wide,
   turned per pixel by a stable angle, so the edge comes out soft and the texels break into fine noise instead of
   stairs. The result is multiplied into the scene at full resolution.
6. **Keeping surfaces from shading themselves.** A surface compared against its own depth in the map shades itself
   in stripes or rings ("acne"), worst with the sun low over flat ground. Four things keep it lit:
   - a constant bias that follows each cascade's texels: 3, 12, 48 and 192 world units;
   - the lookup moved out along the surface's normal (worked out from the depth buffer's neighbours), half a texel
     near the camera and two far off;
   - a slope term: the bias grows with the angle between the surface and the sun, 1 texel's worth near the camera and
     10 far off (from 1,500 to 5,000 units), because far away the normal from the depth buffer is rough;
   - the casters' own slope bias, one texel (two lifted the shadow off thin casters' bases).
   And nothing at the sea's height or below shades the water or the sand at its edge: the islands' models carry their
   sea floor up to the waterline, and the waves dip below it, so the water read the sea floor as something above it.
7. **Sunset and night.** The map's shadows hold at full strength until the sun is 2.9 degrees up, so the long shadows
   of a low sun show, and fade out by 0.6. Below the horizon the map is drawn from the moon, which the game puts
   opposite the sun (`dScnKy_env_light_c`'s `mMoonPos` is the eye minus the sun's offset), at half the sun's
   strength; its shadows come in once the dusk has gone, from 12 degrees up, full at 27. Only the map's own copy of
   the light changes: the edge light and the other effects keep the sun. Windfall is a good place to watch it: its
   clock runs from 18:00 to 23:00 (about 25 seconds a game hour), so a state saved at sunset plays into the night.
   The
   old screen steps are off (`DOL_AURORA_SUN_SHADOW_MAP_STEPS=1` brings them back for what the map leaves out); when
   on, a step counts only where the map doesn't already hold that surface, which stops a second shadow of Link
   appearing behind him.

## Switches

| Switch | Default | What it does |
|---|---|---|
| `BLUEWAKE_HD_SUN_SHADOWS` | off | Sun shadows on, using the map |
| `DOL_AURORA_SUN_SHADOW_MAP` | follows the above | `0` keeps the screen-only version, for A/B |
| `DOL_AURORA_SUN_SHADOW_QUALITY` | `high` | `high` (map 4096, plus the room and palm culling below), `medium` (2048) or `low` (2048, three cascades, six filter lookups, no gap fill); each switch below still wins. In the desktop menu: Mods › Experimental › Sun shadow quality, which changes it at once (the map is made again, a one-frame hitch) |
| `DOL_AURORA_SUN_SHADOW_MAP_SIZE` | 4096 | Texels a side, every cascade (4096 is 256 MB of video memory for four cascades, 2048 is 64 MB, 8192 is 1 GB) |
| `DOL_AURORA_SUN_SHADOW_MAP_CASCADES` | 4 | How many cascades (1 to 4) |
| `DOL_AURORA_SUN_SHADOW_MAP_RANGE` | 1250 | The first cascade's world units from its centre to its edge |
| `DOL_AURORA_SUN_SHADOW_MAP_SPREAD` | 4 | Each next cascade this many times wider |
| `DOL_AURORA_SUN_SHADOW_MAP_DEPTH` | 20000 | World units toward and away from the sun a cascade holds (at least twice its range) |
| `DOL_AURORA_SUN_SHADOW_MAP_BIAS` | 12 | World units of bias at the 5,000-unit cascade, scaled by each cascade's texels (raise it for acne, lower it for gaps at feet) |
| `DOL_AURORA_SUN_SHADOW_MAP_SLOPE`, `_SLOPE_NEAR` | 10, 1 | Texels of the surface's own slope added to the bias, far from and near the camera |
| `DOL_AURORA_SUN_SHADOW_MAP_NORMAL_OFFSET`, `_NORMAL_NEAR` | 2, 0.5 | Texels the lookup moves out along the surface's normal, far and near |
| `DOL_AURORA_SUN_SHADOW_MAP_FAR_START`, `_FAR_END` | 1500, 5000 | World units from the camera over which "near" turns into "far" |
| `DOL_AURORA_SUN_SHADOW_MAP_SMOOTH` | on | The bias, normal offset, slope term and filter reach grow continuously from one cascade's inner edge to its outer one (4x); `0` steps them at each edge, as before |
| `DOL_AURORA_SUN_SHADOW_MAP_GAP_FILL` | on | Steps toward the sun through the depth buffer over the bias's reach, so shadows meet their casters' feet when zoomed out; `0` turns it off |
| `DOL_AURORA_SUN_SHADOW_MAP_TAPS` | 12 | Lookups in each shadow's filter disc (6 to 12) |
| `DOL_AURORA_SUN_SHADOW_MAP_SEA_LOW`, `_SEA_RISE` | 40, 15 | Up to this far above the sea a surface counts as sea; what stands no higher than this above the water there doesn't shade it (scaled down with the sun's height) |
| `DOL_AURORA_SUN_SHADOW_MAP_FADE_LOW`, `_FADE_HIGH` | 0.01, 0.05 | The sun's height (sine of its elevation) where its shadows are gone, and full (0.6 and 2.9 degrees) |
| `DOL_AURORA_MOON_SHADOWS_STRENGTH` | 0.5 | The moon's shadows, a share of the sun's (`0` leaves them out) |
| `DOL_AURORA_MOON_SHADOWS_FADE_LOW`, `_FADE_HIGH` | 0.2, 0.45 | The same for the moon (12 and 27 degrees) |
| `DOL_AURORA_SUN_SHADOW_MAP_CONTACT` | 300 | Reach of the screen steps, when they're on |
| `DOL_AURORA_SUN_SHADOW_MAP_STEPS` | off | Bring back the screen steps |
| `DOL_AURORA_SUN_SHADOWS_STRENGTH` | 0.35 | How dark the shadow is |
| `DOL_AURORA_GAME_SHADOWS` | off with the map | `1` keeps the game's own shadows |
| `DOL_AURORA_SUN_SHADOW_MAP_DEBUG` | off | `1` shows the shadow term alone (gray outside the maps), `2` to `5` each cascade's map as contour bands, `6` tints the cascades red, green, blue and yellow |
| `DOL_AURORA_SUN_SHADOW_MAP_TRACE` | off | Logs how many draws the map took, and the sun's direction and elevation with the camera |
| `BLUEWAKE_HD_ROOM_CULL` | on with sun shadows on `high` | The room model's culling done by the host: each joint is kept while its box, swept away from the sun by its shadow's length (its height over tan of the sun's elevation, half again plus 500, at most 20,000), reaches the view, so islands and buildings behind the camera keep casting. `0` leaves the game's culling, `1` forces it |
| `BLUEWAKE_HD_PALM_CULL` | on with sun shadows on `high` | Palm trees keep drawing (and casting) while their shadow could still reach the view: the common palm's cull box set each frame as wide as its shadow is long (same rule as the room), Gale Isle's palms widened by 5,000. `0` or `1` overrides |

Debug tools for testers (separate commits in #42): `DOL_AURORA_SUN_SHADOW_MAP_ALL=1` lets
alpha-tested draws cast solid (to see which shadows a rule is costing), `DOL_AURORA_SUN_CASTER_LOG=1` says why each
scope's draws did or didn't cast, `DOL_AURORA_GAME_SHADOW_LOG=1` logs the blended textured draws the game-shadow
rules test, and `DOL_AURORA_FRAME_INTERP_DUMP_KEY=N` with `DOL_AURORA_FRAME_INTERP_DUMP=<folder>` makes F3 dump the
next N game frames (every platform).

## How it was worked on

The loop was short: change RecompCore, build host-only, play, look, repeat.

- **Host-only builds.** All of this is renderer code, so `--host-only` over an existing trained build is enough and
  takes seconds to minutes, including after a `RECOMPCORE_SHA` bump: it checks out the new pin, keeps the last
  build's training, and recompiles only the game-module files the change affects: from none of the 850 to all of
  them, depending on the change. Against main this branch affects none, so only the app is rebuilt. Point the
  builder at a local RecompCore checkout to skip pushing between tries.
- **Playing by hand first.** pdale-boop played each step and described what looked wrong: "Link's shadow is a blob",
  "the sail casts nothing", "a dark patch at his feet inside a building's shade". Each of those became one commit.
  Save states (`BLUEWAKE_LOAD_STATE=<file>.bwstate`) pinned the places where something looked off, so every try
  started from the same frame. Renderer switches like the ones above do apply after a state load.
- **Fixed time of day.** The sun moves, so shadows move. On islands the game's clock holds between 11:00 and 18:00
  (and 23:00 to 6:00), so warp there at a fixed hour: `BLUEWAKE_WARP=sea:11:0 BLUEWAKE_WARP_TIME=16` puts you at
  Windfall at 4 pm ([STAGE_SELECT.md](STAGE_SELECT.md)). A state saved before 11:00 runs forward to 11:00 and then
  holds, which looks like shadows drifting but is the real sunrise.
- **A/B in one build.** `DOL_AURORA_SUN_SHADOW_MAP=0` against the default, and `DOL_AURORA_GAME_SHADOWS=1`
  against the default, separate the map's shadows from the old ones and from the game's.
- **The debug views.** `_DEBUG=1` shows what the map darkens and nothing else; `_DEBUG=2` shows the map's depth as
  bands, which makes a missing caster obvious (no bump where the object is). The caster log names the rule that
  refused a draw.
- **Frame dumps.** `DOL_AURORA_FRAME_INTERP_DUMP=<dir>` with `_FROM`/`_TO` writes every real and in-between frame as
  images, so a flicker can be checked frame by frame. They need a paced run (not `BLUEWAKE_WALL_PACE=0`), and they're
  big: at render scale 2, 90 frames come to about 13 GB.
- **Scripted warps for numbers.** Headless stage-select sweeps measured the culling, and rendered warps measured the
  map's cost: 0.04 to 0.07 ms of GPU time a frame on an RTX 5070 Ti (the first, single map; with four cascades the
  casters are drawn four times, not remeasured yet).
- **Captures from states, compared side by side.** The low-sun work (Outset in the evening, the sun at 20 degrees)
  went like this: pdale-boop saved a state wherever something looked wrong (from the lookout, on the beach, on level
  ground), and each change was checked by loading those states with `BLUEWAKE_LOAD_STATE`, waiting for the frame to
  settle and capturing the window, before and after, cropped to the spot. One switch at a time narrowed each problem
  down:
  - `_DEBUG=1` (the shadow term alone) showed the island and the sea shaded where nothing stood between them and
    the sun; `_DEBUG=6` showed which cascade held the beach, ruling the cascades' edges out.
  - Turning caster rules off one by one, then leaving out the island model's draws by texture, half at a time, found
    that the beach and the ground around it were shading themselves, not something hidden casting on them.
  - The sun's direction from `_TRACE`, checked against the palms' shadows, settled which way the light came from.
  - A sharper map (`_SIZE=8192`) closed the gap under Link's feet where every bias change had not: it was the map's
    resolution, so a smaller first cascade went in instead (16 MB, not 768).
  - The projection the draws really use, logged next to the camera the host reads, matched (aspect 2.24 at 21:9), so
    the shadows aren't shifted by the widescreen camera.

## Questions for reviewers

- **How close to Wind Waker HD should this be?** None of the shadow map work was compared with the Wii U version;
  it was judged by eye. Some of it may go beyond what HD does: moon shadows at night, long shadows down to a sun a
  few degrees up, and surfaces turned away from the sun shaded. If someone can check HD (their own copy, or footage)
  at sunset, at night and between Windfall's buildings, it would tell us which parts to keep as the HD look and which
  to put behind a switch of their own.
- **Should moon shadows follow the moon's phase?** The game shows phases that change from night to night. The
  moon's shadow strength could follow it, none at new moon and full at full moon (a small host change: read the
  phase and scale `DOL_AURORA_MOON_SHADOWS_STRENGTH`). It only makes sense if moon shadows stay at all.

## What's left, with leads

- **Ultra (8192 maps).** Sharper still, but it dropped frames on an RTX 5070 Ti (four cascades drawn at 8192 every
  frame) and showed a wedge of self-shading on the cliff under Link that moved with the camera. Lead: draw the far
  cascades less often than every frame; they barely change.
- **Long shadows fading toward the tip** (sky light filling them in), tried and taken out: faded by the distance to the
  blocker the map holds, a tree's shadow inside a mountain's came out a different shade from the mountain's around
  it. The map keeps only the blocker nearest the sun, so a fade needs something that knows a point is already in
  another shadow.
- **A blue Chuchu's oval.** The game draws it itself (it stays with the HD look off) and it doesn't go through
  either of the paths the map skips (`DOL_AURORA_GAME_SHADOWS` on or off makes no difference). The goal is no
  GameCube-style darkening next to the HD shadows: find what makes that draw distinct (a frame-interp trace on a state
  with a Chuchu) and leave it out while the map is on.
- **A thin caster's shadow far away starts behind its base.** From Outset's lookout, the palm trunks across the
  beach (in the 5,000-unit cascade) have their shadows start a little back from the trunk, from the far slope term
  (10 texels) that keeps the far beach from shading itself. Turning that term down (to 3, or 0) made the far
  shadows weaker or gone altogether instead of stronger, which less bias shouldn't do, so something in the far
  lookups isn't understood yet. Next: log one far beach pixel's numbers (its depth, the map's, each bias term and
  the sea-level rule's answer) rather than tuning by eye. A per-lookup receiver plane (each lookup compared against
  the surface's own depth at that texel, from the screen's derivatives) was tried and changed nothing with the slope
  terms at their defaults.
- **Distant islands with cascades.** The last cascade reaches 80,000 units; islands farther off still cast nothing,
  and how the far cascades look on an island you sail toward hasn't been checked closely.
- **Shadows popping when an actor leaves the view.** The game stops drawing a culled actor, so it leaves the map.
  Buildings are rarely culled; actors are. The fix is on the game side: widen the clipper while the room model and
  then actors draw. Actors with `fopAcStts_NOCULLEXEC` stop updating when they aren't drawn, so widening their culling
  changes what runs off screen; keep that behind its own switch and measure the CPU cost. Walking NPCs already turn the
  flag off themselves.
- **Surfaces facing away from the sun** are shaded by the map, which the screen version never did. It looks right
  to us, but it's a change in the look worth a second opinion.
- **The debug tools stay.** The four debug commits stay in #42 and in patch 0183 as tools for testers, each off
  unless its variable is set (listed under the switches above). `DOL_AURORA_SUN_SHADOW_MAP=0` stays too, as a
  variable for comparing against the screen-only shadows; it isn't in the menu.
- **Platforms:** played on Windows (Direct3D 12) and Linux (Vulkan), both RTX 5070 Ti. Not yet on AMD or Intel GPUs,
  Mac (Metal) or iPad.

## Code map

RecompCore, `GXRuntime/graphics/aurora/`:

- `include/aurora/aurora.h`: `aurora_set_sun_shadow_map()`.
- `lib/gfx/post_fx.cpp`: the map's settings and texture, `sun_map()` (its matrices), `draw_sun_map()`, the lookup
  shader (`in_sun_map`, `fs_sun_map`) and the debug views.
- `lib/gfx/gxcore_draw.cpp`: the caster rules (`sun_caster`, `sun_caster_alpha`, cloth, blended furniture), the
  caster pipelines, `begin_sun_casters()` / `render_sun_caster()`, and the game-shadow skips.
- `lib/gfx/common.cpp`: `encode_sun_map()`, which runs the casters before each EFB pass.
- `lib/gfx/frame_interp.cpp`: `collect_parts()` is shared with the map, so in-between frames cast from their blended
  constants.

BlueWake: `runtime/host/src/hd_look.c` (the switch, sun and camera), `patches/recompcore/0183-*.patch` (the RecompCore
change as one patch), `config/dependencies.lock.json` and `scripts/builder/profiles/bluewake.sh` (the pin).

To move the pin after changing RecompCore: update `RECOMPCORE_SHA` in the profile and the `sha` and
`sun_shadow_map.runtime` entries in the lock, regenerate 0183 from #40's head to the new commit, then run
`scripts/audit_repo.sh` and `python3 scripts/check_attribution.py origin/main..HEAD`.
