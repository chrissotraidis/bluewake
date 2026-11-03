# Historical RecompCore patches

These patches record BlueWake's RecompCore changes as they were made. They are history, not a build
input: the series starts at 0008 (0001-0007 were never exported), so it does not apply to the
upstream base 5c3611e, and the local head it led to (3476998) was never published.

The build uses a fork instead. BlueWake's is https://github.com/chrissotraidis/RecompCore, branch
`bluewake`, commit 2d6063614a9bc899f6b4d11c7e7b3cd66e4d96f3: it contains the changes here through 0097
(some were revised by later ones), the files that were never committed on the development Mac, and the
DolRecomp submodule pointing at https://github.com/chrissotraidis/DolRecomp (5c91d6e). Wind Waker Recomp
builds from its own copy, https://github.com/elliotttate/RecompCore, branch `bluewake`, commit
8ab24da: that tree plus 0098 to 0112, with DolRecomp at https://github.com/elliotttate/DolRecomp
(b8b5345, 5c91d6e plus patches/dolrecomp/0019). Its `windows-release` branch adds 0113 (9618e9d,
the render worker paused while the swapchain changes). BlueWake now builds from
https://github.com/chrissotraidis/RecompCore, branch `bluewake-next`: 9618e9d plus 0114 (one DSP
interpreter table layout under the Microsoft ABI, so Exact/LLE audio no longer calls address 0
on Windows), with the same DolRecomp. The Builder fetches it at the commit pinned in
`scripts/builder/profiles/bluewake.sh`; see docs/DEVICE_BUILD.md.

Patches 0115-0125 add the ordered save, shutdown and audio fixes, opt-in display
timing, and a render-worker identity fix reproduced with ThreadSanitizer. The stability baseline
pin was `99e4748002d42c1a86fdcb33a47cd0e97292acff`. Tests and hardware limits are in
[the local stability ledger](../../docs/archive/status/LOCAL_STABILITY_2026-10-01.md).

Patch 0126 preserves the published in-memory card contents after a directory-sync
error, while continuing to report the error. Evidence is in
[the overnight ledger](../../docs/archive/status/OVERNIGHT_2026-10-02.md).

Patch 0127 resumes a paused, already buffered output before the overflow/drop
path can prevent recovery; its actual SDL dummy-device regression covers the full queue.

Patch 0128 keeps FIFO translation on the caller for the whole armed trace,
including frames before capture starts. Its synthetic regression compiles the
production start decision and preserves normal repeated worker starts/joins.

Patch 0129 makes the cross-thread frontend failure flag and submitted/rejected
draw counters relaxed atomics. ThreadSanitizer reproduces all three original
races with the actual linked globals; the same bounded probe passes afterward.

Patch 0130 rejects truncated declared vertex spans before attribute reads or
decoded-vertex allocation. ASan reproduces the original direct-float overread;
38 direct/u16-indexed one/two-vertex truncations and valid trailing bytes pass
in the existing runtime conformance test after the fix.

Patches 0131-0135 reconcile the optional global-memory and interpolation work,
including span and extended-alias safeguards. Patch 0136 directly imports
Elliott Tate's dual-texture post-transform correction. Patch 0137 preserves those
matrices in a versioned frontend save-state extension, accepts legacy states,
and tests direct/indexed FIFO capture and malformed-state rejection. They first
shipped in pin `18ba3b642588a33b9e8eac4aba7f713bb8d3d778`; the current pin is in the profile and dependency
lock. New post-texture save states require this or a newer
runtime; regular memory-card saves are unchanged. BlueWake lava-scene acceptance
is still required; donor scene results are not transferred.

Patch 0140 changes session logging only: GX batches between 20 and 50 ms are summed
into one `[gx-slow-sum]` line every ten seconds, and batches of 50 ms or more keep
their own `[gx-slow]` line. A player's 104-minute Windows log had 33,712 of the old
lines. Evidence is in [the stability plan](../../docs/archive/status/STABILITY_PLAN_2026-10-03.md).

Patch 0141 is Elliott Tate's slow-game detector for Smooth Motion (his RecompCore
`0bb1fef`, Wind-Waker-Recomp patch 0122), with his authorship: the median of the last
60 frame gaps, gaps of 150 ms or more left out, two slow medians in a row. A single hitch no
longer drops the in-between frames.

Patch 0142 counts the time the host holds the guest (a menu, the app in the background):
`DolAuroraFrameTiming.held_us` and `dol_aurora_held_us()`, so per-second diagnostics
leave it out. Logging only.

Patches 0152-0155 are Elliott Tate's runtime commits of October 3 from his RecompCore
`windows-release` (`ef3e17f`, `201e909`, `7310b79`, `e6559e0`), with his authorship: a draw's
transform state copied only when it changed; Smooth Motion for the Mac's water and HUD (indexed meshes
blended with their UVs, screen sprites matched by their artwork and bounds); HD replacements sampled at
their own mip levels, so HD packs stop shimmering while the camera turns; and vertex-by-vertex blending
kept to meshes the game wrote, so the water path costs what it did. They replace Wind-Waker-Recomp's
working-tree patches 0113, 0140, 0160 and 0170; its lava patch 0120 is BlueWake's 0136.

Patch 0156 resolves the textures a linked module's own display list names at the module's address.
SETIMAGE3 keeps only 24 bits of a texture address, so a texture in a module's data (linked at
`0xC0xxxxxx`) pointed at unrelated MEM1, and Molgera's sand floor (`d_a_bwdg`, issue #126) drew
scrambled. The HLE `GXCallDisplayList` now passes the list's guest address down
(`call_display_list_guest`). The floor's vertex arrays are BlueWake's host fix in the same pull request.

Patch 0157 gives gxcore the hardware's eight texgens and sixteen TEV stages (they were capped at 5 and
8), so the dungeon map draws its grid and rooms (issue #74). The fixed vertex layout keeps five raw
texture coordinates; a vertex with TEX5..7 and no normal, as the map's quads are, carries them in the
normal, binormal and tangent slots (`ShaderKey::raw_tex_hi_in_nbt`). The key grew, so the pipeline config
is version 13, and BlueWake's bundled pipeline seeds were converted to it row by row (same pipelines, new
layout) in the same pull request.

Patch 0158 fixes two shutdown bugs the Linux port surfaced. Three wgpu::Device-bound statics in
gxcore_draw.cpp were function-locals, so their destructors ran at exit() after webgpu::shutdown()/
window::shutdown() had freed the Vulkan instance and XCB connection; the last device ref then aborted
in xcb_send_request -> realloc ("double free or corruption (!prev)"). The statics are now file-scope
and gxcore::shutdown() releases them in the right order. Fixing that unmasked the second bug: two
detached background threads (texture_replacement's decoder_main and gxcore's interp_helper_main)
waited forever with no stop signal, so exit() hung on the still-live threads. Each now has a stop flag
signalled from its module's shutdown(), and the process exits cleanly. By James Koehler-Killeen
(RecompCore pull request #16, from BlueWake pull request #107).

Patch 0159 scales a controller's sticks to a GameCube stick's travel when Aurora's dead-zone cutoff is
off (`gamecube_axis`: full travel is 100, where a GameCube stick's gate stops it), so the game's own
`PADClamp` is the only dead zone. BlueWake turns the cutoff off for player 1's controller
(`runtime/host/src/controller_ports.h`, issue #138). With it on, the first value the game saw was 22% of
its range and full tilt came at two thirds of the travel. Number 0158 is the Linux port's shutdown fix above.

Patch 0160 lets a host choose Aurora's user folder with `DOL_AURORA_USER_DIR`, as `DOL_AURORA_CACHE_DIR`
already chooses its cache folder. That folder holds `imgui.ini`, controller button remaps (`*.controller`),
keyboard bindings and `controller_ports.dat`. BlueWake's Windows host points it at the player's data folder,
so portable mode no longer writes them to `%APPDATA%\BlueWake` (issue #64), and copies any the portable
folder doesn't have yet. In normal mode the data folder is the one SDL picks, so nothing moves.

Patch 0161 adds `window_pos_x` and `window_pos_y` to `AuroraBackendConfig`, passed to Aurora's `windowPosX` and
`windowPosY`, so the host creates the window where it should be instead of moving it after it appears (pull
request #89, saulob). Left at zero they give the screen's corner, as before; the Windows host passes the
player's last spot or `SDL_WINDOWPOS_CENTERED`.

Patch 0162 is saulob's FPS overlay position (his RecompCore #12, carried onto `bluewake-next` as #20): the counter
at the top center as before, or in a corner, set with `aurora_set_fps_overlay_position` or `DOL_AURORA_FPS_POSITION`.
BlueWake's menus use it in pull request #106.

Patch 0163 adds `dol_aurora_backend_name()`, the graphics API Aurora actually chose. The host logs it as
`[renderer] NAME`, and on Linux and Windows says when it fell back to OpenGL, which is much slower than Vulkan (#56).
Patch 0164 keeps Smooth Motion's in-between frames when the game runs slow on a CPU with 8 threads or more, where
they don't compete with the game thread (#137); GPU overloads still drop them, and smaller CPUs keep the old rule
(RecompCore #21).

Patches 0165 to 0168 are Elliott Tate's October 4 runtime from his `windows-release` (RecompCore #22): the present
log's per-frame bytes and draws, **draw fusion** (a display list's strips as one draw: over an Outset run 8.8 million
draws became 0.87 million, and on four E-cores Forest Haven went from 30.6 to 40.5 game frames a second), and the GPU
profiler (`DOL_AURORA_GPU_PROF=1`). Fusion is on for Windows, where he tested it, and for Linux, played on a Steam Deck
(#215); the host turns it off on the Mac, iPhone and Android (`DOL_GX_FUSE=1` turns it on). His later commits (the compact vertex layout and the
upload changes) conflict with patch 0157 and are not here yet.

Patch 0177 is jslay88's RecompCore #36: the Picto Box reads its actor ids from the current EFB instead of
receiving zero for every pixel. It needs BlueWake's host MMIO handler from #334. Cold-cache alpha-only
pipeline compilation remains a separate follow-up (RecompCore#39), not part of this pin.
Mac recognition and remaining checks: [Picto validation](../../docs/status/PICTO_RECOGNITION_2026-10-15.md).

Patch 0178 is pdale-boop's RecompCore #38, for BlueWake #325: on Vulkan, a resized window keeps its surface and keeps
drawing. A lost surface is configured again (up to 30 times in a row) instead of released at once, which on Linux
made the replacement surface fail with `VK_ERROR_NATIVE_WINDOW_IN_USE_KHR`; between the game's frames the surface
follows the window's size, so the picture no longer trails behind a window being dragged. The size following is
Vulkan only. On every backend a suboptimal texture is now drawn and the surface configured again after it.

Patch 0180 is pdale-boop's RecompCore #37, for BlueWake #338: `aurora_set_copy_crop(width, height, fraction)`. A
texture copy of exactly that size takes `fraction` of its source's width about the frame's middle and is scaled back
to its full size, as a 4:3 camera would have taken it; `GXCopyTex` and gxcore's EFB copies both use it. Nothing
changes until a game calls it: BlueWake does for the Picto Box's 152x104 copy, behind `picto_crop=1` (off by default).

Patch 0182 is RecompCore #41: the HD look's cloud shadows take the sea's place from its plane, since positions from
depth far out on the sea made straight-edged steps (the "square" shadows on Vulkan, a hard band on Metal), with gradient
noise turned per octave. The cloud, sun-shadow, ambient-occlusion and bloom passes add half an 8-bit step of dither so
their smooth shades don't band on the sea. 0181 is taken by draft #342.

Patch 0179 is pdale-boop's RecompCore #40, for BlueWake #136: Smooth Motion without the flicker in the HD look, the
clouds and the wave crests. Every in-between frame gets the HD look's effects (only the first of three at 120 FPS did,
and the first of five at 180); their shadows and lights come from the in-between frame's camera, moved as
`frame_interp` moves the scenery, instead of the latest one the game set; and copies drawn with the view matrix and
their own world positions (the sky's clouds, the wave crests at sea) are paired by the centre of their vertices
instead of their place in the list, which shifted whenever one faded or came into view and blended a cloud toward its
neighbour's alpha, with their colours counting too (each cloud is drawn twice, once with alpha 1). The water around a
swimming Link follows him as well: a ripple's growing rings, the trail of rings behind him and his wake, which stepped
at 30 FPS or jumped back a ring whenever a new one was born. It sits on `bluewake-next` after #41, so this pin carries
#37, #38 and #41 as well.

Patch 0183 is pdale-boop's sun shadow map (`aurora_set_sun_shadow_map`). BlueWake turns it on with its sun shadows
(`BLUEWAKE_HD_SUN_SHADOWS=1`, still off by default); `DOL_AURORA_SUN_SHADOW_MAP=0` keeps the screen's version alone. The HD look's sun shadows came from the screen, so something off screen cast nothing.
With the map, each EFB pass's solid draws are drawn again from the sun into a depth map before the pass, with two
fixed pipelines and each draw's own constants (no pipeline per draw), and each pixel's place in the world is looked up
in it with a turned disc of lookups, at full resolution and without the screen's steps. Alpha-tested scenery (leaves,
fences) casts its texture's cutout, the sail and flags cast, and blended furniture casts when its vertices stand still;
the game's own shadows are left out while the map is on (`DOL_AURORA_GAME_SHADOWS=1` keeps them). In an in-between
frame the draws take their blended constants and the map that frame's camera. The map is four cascades (1,250 to
80,000 units; 4096 texels each on the high shadow quality, the default, 2048 on medium and low), with a bias for a low sun that grows with the surface's angle to it and nothing at
the sea's height shading the water. Its long shadows hold until the sun is 3 degrees up, and at night it is drawn
from the moon (opposite the sun) at half strength, from 12 degrees up. It keeps debug tools for testers, each off
unless its variable is set (the F3 frame dump, `_DEBUG` views, `DOL_AURORA_SUN_SHADOW_MAP_ALL`,
`DOL_AURORA_SUN_CASTER_LOG`, `DOL_AURORA_GAME_SHADOW_LOG`); docs/HD_SUN_SHADOW_MAP.md lists them.

Patch 0184 preserves jslay88's RecompCore #43: vendored Dawn advances to `1155e0ed`, whose bundled
Abseil includes `immintrin.h` correctly when the Linux AppImage builder enables BMI2 through
`-march=x86-64-v3`. Cache callbacks use Dawn's setters; the prebuilt package tag stays unchanged.
Patch 0185 keeps those callbacks compatible with the pinned Apple headers by omitting `noexcept`
on their lambdas (RecompCore #44). The existing header's callback traits do not accept that qualifier.
Focused Mac, iOS, tvOS and Android NDK source checks and a callback-forwarding fixture pass; full application
builds and the Sunday gameplay/visual checks are separate. 0183 is reserved for #398's sun shadows.

Patch 0186 is pdale-boop's RecompCore #47, the HD look's fixes for #491, in one commit. The ambient occlusion is
blurred and stretched back to the scene's size only along each surface. Both steps mixed in neighbours whatever their
distance, so Link's own shade (under his hat, around his shield) spread about 30 pixels onto the sea behind him, a
dark outline around him. Each neighbour now counts by how near its distance is to the pixel's, and where there is no
depth the scene is left as it is. Sun shadows keep the shared blur and apply. It also fades out from 1,000 to 2,500
units away (`DOL_AURORA_AO_FADE_START`, `_FADE_END`): further away its ring is a few pixels across, and the sea's
swells darkened a band from about 1,500 to 8,000 units, right up to Link's outline on Outset's pier.

The bloom is screened onto the scene (glow * (1 - scene) + scene) instead of added. Added, it pushed Wind Waker's
near-white clouds past white: at Outset's pier a fifth of the clouds were flat white, with no shading left. Screened,
it brightens what is darker as before and tends to white without clipping.

Two fixes go with the HD look's sky left sharp under the far blur. The far blur eases off within 24 pixels of the
sky, where it stopped at the horizon's edge in a hard band. A lightning strike's invisible model (depth only, about
100,000 units out) writes no depth while the HD look's effects are on: in the sky it turned the far blur and cloud
shadows into grey columns from the sea to the clouds.

The HD look's effects go in before a copy of the frame laid back over the scene. Wind Waker does that while the
camera shakes (`motionBlure`, as when Link's sword hits a tree): the previous frame's copy, which already holds the
effects, is blended over the scene, and post_fx ran after it, so the shadows, ambient occlusion and cloud shadows
were multiplied in a second time. Shadowed ground went from 0.65 of its unshadowed brightness down to 0.41 and back
with each game frame of the blur (#491). A flat draw that samples an EFB copy is marked as such, and the effects go
in before the first one after the scene's perspective draws.

The ambient occlusion is limited to what is within reach in the scene. Each pair of samples around a pixel occluded
it when their average was nearer, with the radius measured on screen; across a floor seen at a low angle a short step
on screen is a long way along the floor, so Link's legs darkened a wide patch of floor in front of him in a test
room, and long grass at Outset left a dark halo (#491). Each sample now gets a place in the scene from the depth, and
the pixel a surface normal from the depth around it; a sample shades the pixel only when it is above that surface and
within the radius of it, each read at a whole texel of the depth so a sample never mixes two distances across an edge
(bands across the screen otherwise). `DOL_AURORA_AO_DEBUG=2` tints what the occlusion darkens red over the scene
instead of showing it alone.

The sun shadow map gets the same bias near the camera as far off: two texels along the surface's normal and ten of
its slope, where near had half a texel and one. On a rock wall the sun only grazes (Outset), the map's texels showed
through as light and dark vertical stripes that shifted with the camera (#491). And it reads each pixel's distance
from the depth with the camera's far plane (`aurora_set_world_view_far`, from `hd_look.c`): read as if it were
infinitely far, an island 8,000 units ahead came out hundreds of units too far, and shaded itself in blocks from the
waterline up.

Patch 0187 leaves a texture pack's top-level `optional/` folder out of the pack and loads the folders that
`DOL_AURORA_TEXTURE_PACK_OPTIONAL` names (comma-separated) over it, at a higher priority. The HD importers put Wind
Waker HD's own sky clouds there (`optional/hd-clouds`), for the HD look's **HD's clouds in the sky** switch. A
missing folder is reported in the log and not created.

Patch 0188 times the sun shadow map's shadows as Wind Waker HD does. It keeps the sun's shadows while the game draws
its sun disk (they had stopped well before the sun set) and starts the moon's from 25 degrees up, and keeps the
moon's shadows at dawn until it is down to 8 degrees, where they had gone as soon as the sun's began.

Patch 0189 is sun shadows on the sea, as in Wind Waker HD. It leaves the sea's surface out of the sun shadow map (it
takes shadows and casts none): a swell's crest had shaded the trough behind it, specks along the horizon, and a tower
standing in the sea lost the foot of its shadow. It drops the slope and normal bias on the sea, which can't shade
itself, so the far platforms' legs meet their shadows. And the gap fill counts only what the map holds, one-sided, so
wave crests no longer cast through it (sunrise specks, the bow spray's wings).

Patch 0190 shapes sun shadows as Wind Waker HD's. It holds the sun at 24 degrees, its direction too, below which HD's
shadows stop growing, and thins them out from there to half by -7 degrees and none by -8 (measured in Cemu at the
Private Oasis). It softens long shadows toward their tips by the caster's distance, adds two debug views
(DOL_AURORA_SUN_SHADOW_MAP_DEBUG=7, how surfaces face the sun; 8, the shadow in pink), and makes the shadow a colour,
the shade lit by the sky: HD's on Windfall's dock is about (0.21, 0.39, 0.56) of what is lit. The map has three
cascades, out to 20,000 units: a fourth held only islands deep in the haze, and shaded Windfall in light-blue blocks
from across the sea. aurora_set_sun_shadow_strength sets how dark its shadows are (BlueWake's menu), and the slope's
share of the bias far off is 20 texels, against acne on walls edge-on to the sun.

Patch 0191 draws the three vertices left over at the end of a GX_QUADS draw as a triangle, as the hardware does. The
frontend dropped quads with fewer than four vertices, and the game draws each night star as a three-vertex quad
(dKyr_drawStar), so no stars showed at night. It also keeps them steady in Smooth Motion: each star stands at the
camera's eye plus its place on a small dome, and the in-between frames paired stars by their places in the world, so
they jumped with the camera or half way to a neighbour; a three-vertex quad is now paired from the camera's eye and
moved with each in-between camera's eye.

Patch 0192 adds aurora_set_exposure: the scene brightened by the game's factor after the HD look's effects and before
its bloom, where there is depth and not the sea. BlueWake sends Wind Waker HD's own factor for the hour's palettes,
its sunlight 3 times at noon on actors and scenery, 1.65 after gamma. HD brightens the sun's light, not the sea or the
sky. The sea is a mask, from the scene's draws drawn again: those that sample the sea's textures (d_a_sea's water,
the island rooms' ring of sea and their beaches' surf, by the texture packs' hash) and the sea rooms' untextured
square of sea, covered by what writes depth in front of them. A height band above the sea had left beaches below a
swell's height unlit and lit the swells' crests. Pools, rivers and falls count as the sea, and the brightness fades out
from 10,000 to 25,000 units away (`DOL_AURORA_EXPOSURE_FAR`), where islands are the haze's colour, and tapers off on what is already bright
(`DOL_AURORA_EXPOSURE_HIGHLIGHTS`, ice and white rock). `DOL_AURORA_SEA_MASK_DEBUG=1`
shows the mask.

Patch 0206 keeps the HD look's shading off the effects drawn over the scene. The sun shadows, the occlusion and the
cloud shadows are worked out after the scene from its depth buffer, and an effect drawn blended without writing depth
(the spin attack's rings and burst, the Master Sword's glow, sparks) leaves the depth there the scenery's behind it:
Link's cast shadow was drawn across the glowing rings. Wind Waker HD draws such effects after its lighting. The scene's
effects are drawn again with their own pipelines into a mask, over black and against the finished depth, and the
shading takes away as much of its shade as they make up (their brightest channel times 2,
`DOL_AURORA_EFFECT_MASK_GAIN`), within 3,000 units. Left out of the mask: the sea's surf, the GameCube's own shadows,
what has no depth test (the sky's clouds) and a character's eyes and mouth; the effects are drawn 5% of their distance
further back (`DOL_AURORA_EFFECT_MASK_PUSH`), so what lies on a surface keeps its shade.
`DOL_AURORA_EFFECT_MASK_DEBUG=1` shows in red what the sun shadows leave.

Patch 0206 also treats a blended draw that writes depth as an effect when it is one of the game's particles (the host's
tag) or a glow model (its colour from neither a texture nor the rasterized colour). A particle's colour draw over
its own depth draw, and an effect that writes depth, go into the effects' mask at their own depth, against a copy
of the sea mask's depth. Where such an effect covers a pixel, the occlusion, sun shadows and cloud shadows shade
from the sea mask's depth, the scenery under it, as Wind Waker HD lights before it draws its effects. A small pass
makes that depth only in frames with such effects.

Patch 0206 also finishes Dragon Roost Cavern's effects. A glow model may take the rasterized colour when its textures
only cut its shape (the geysers' middle layer, C1 + 0.5 x RASC, was shaded as scenery); an effect drawn at its own
depth keeps its share near or far (the cover's fade from 3,000 units went by the far wall's depth behind it). The
effects are drawn into the mask a second time over white, so a blended effect's share is its opacity: a dark
dust no longer lets the floor's shadows show through. `DOL_AURORA_EFFECT_MASK_OPACITY=0` keeps the colour alone.

Patch 0207 adds Wind Waker HD's facing term to the sun shadows (off by default, `DOL_AURORA_SUN_SHADOW_FACING=1`):
what faces away from the sun is in shade too, from the scenery's smooth normals made on the CPU while it is on,
then HD's terrain curve. Characters, marked in the sea mask, keep their own shading. `aurora_set_sun_palette` gives
the shade HD's light for the hour: its ambient (the palette's shadow colour from above, BG0 C0 from the side) over
its sunlight. The gap fill is off (it shaded Link's own front) and the slope term is 20 near the camera as far off.

Patch 0208 reads a table of Wind Waker HD's normals keyed by the GameCube's exact vertex positions (`BWHN`;
`DOL_AURORA_SUN_SHADOW_FACING_HD_NORMALS` for tests), for the facing term. It also tries the depth's own facing
(`DOL_AURORA_SUN_SHADOW_FACING_DEPTH_MUL`, `_BLUR`), which HD turned out not to have; off by default.

Patch 0209 adds Wind Waker HD's shadow bias and terrain shadow mix (test switches), `aurora_set_sun_hd_light` (HD's
light rule: the sun by day, the moon's side by night, its day/night fade) and `aurora_set_hd_normals` (HD's normals
for the stage in play, from the HD pack's `optional/hd-normals/<stage>.bin`).

Patch 0209's pre-pass takes its grazing term from the edge-aware depth normal (`depth_normal()`), and characters
none: its neighbours lie four pixels apart, and across a silhouette their normal came out edge-on, so Link's outline
grazed and the blur spread that onto the ground around him, a lit hole in his shape in the soft shadow.

Patch 0210 adds Wind Waker HD's bloom chain, SSAO, rim light and cloud shadows (each behind its own switch, off by
default; the cloud texture from the host, `aurora_set_hd_cloud_texture`), and HD's own shadow pass as a fourth
shadow quality (`hd`, `set_sun_shadow_quality(3)`): three 1024 maps, a half-size pre-pass with one filtered lookup
per pixel in the cascade its distance picks, blurred.

Patch 0211 blends the shadow pre-pass with the last frames' by distance (`DOL_AURORA_SUN_SHADOW_TEMPORAL=1`, off by
default): at sea the clock turns the sun every frame and shadow edges jumped by the map's texels.

Patch 0212 shades and lights the palms as Wind Waker HD does (test switches, off by default): skinned draws that
aren't toon-shaded are scenery to the sun pass, with smooth normals lit from either side; a table of HD's vertex
colours by position (`BWHC`) gives the draws that mix their shade colour to their light colour by the vertex colour
HD's light: texture x HD's colour x the light, the sun pass making the light.

Patch 0213 adds `aurora_set_sun_cascade_splits`: the HD shadow quality's two cascade splits and the first two
cascades' sizes each frame, as Wind Waker HD sets them from its camera; with fixed ones, a camera zoomed out past
HD's left the ground around Link in the far cascade, where grass shadows crawled as the sun turned.

Patch 0214 adds `aurora_set_hd_object_tables`: Wind Waker HD's normals and vertex colours for the game's object
models (the HD pack's `optional/hd-normals/objects.bin` and `optional/hd-colours/objects.bin`), looked up after the
stage's; and HD's light on the scenery (`DOL_AURORA_SUN_SHADOW_HD_TERRAIN_LIGHT=1`, off by default): HD's terrain
material is texture x vertex colour x light, where the GameCube's mix never goes below the shade colour.

Patch 0214 also gives all scenery with a smooth normal Wind Waker HD's hemisphere per vertex
(`DOL_AURORA_SUN_SHADOW_SMOOTH_AMBIENT=1`, which the HD look sets): the mask pass normalizes each vertex's normal
and writes the blend across the triangle as it is, and the hemisphere (0.5 + 0.5 N.y between the ambient and the
sky's light) takes its y without renormalizing, as HD's terrain shader does. Before, the ground took N.y from the
depth's face (about 1: the sky's colour alone), bluer in the shade than HD's where its normals lean. Palms keep
theirs; `=skinned` keeps the old rule.

Patch 0214 also keeps the sun shadows' smooth normals and Wind Waker HD's vertex colours per draw from frame to frame
(`smooth_normal_cache.hpp`). They were made again for every lit scenery draw every frame, a weld map and a lookup
per vertex in HD's tables: 66 draws and 47,000 vertices a game frame in Hyrule Castle's hall. A draw that comes
again with the same bytes and the same tables and light colours takes the vertices made the first time; it is
compared in full, so the picture is the same. Lava draws always make them. Entries unused for 120 game frames go.
`DOL_AURORA_SUN_SHADOW_NORMAL_CACHE=0` makes them every frame, as before.

Patch 0215 adds Wind Waker HD's actor light (`DOL_AURORA_ACTOR_LIGHT_HD=1`, off by default): characters drawn with
the GameCube's toon pattern (C0, K and the ramp, then the albedo) take HD's toon shader, read pixel for pixel from
Cemu captures: the ramp as a lit/dark switch on the lit channel, its coordinate times the blurred sun shadow (this
frame's, as HD's); the shade a hemisphere from actor C0 to the palette's shadow colour; the lit side actor
K0 times its factor; HD's rim; the albedo in linear light, clipped before the gamma. The host sends the blended
palette each frame (`aurora_set_actor_light_hd`). The GameCube's effect-light stage (K1) is left out, as HD drops it
for its point lights. Matched draws mark the sea mask's alpha, and the sun pass, the HD rim and the exposure leave
them alone. `DOL_AURORA_ACTOR_LIGHT_DEBUG` (1-5) shows its terms, `DOL_AURORA_ACTOR_LIGHT_LOG` each toon shader's TEV.
The characters take this frame's sun shadow, as Wind Waker HD does (depth, then its shadow pre-pass,
then the characters): for scene passes with HD's actor light, the sea mask and its depth are drawn before the scene,
a characters' pre-pass is built from that depth with HD's blur, and the characters sample it. Before, they took the
last frame's pre-pass, whose edge lagged a frame on a moving camera. `DOL_AURORA_ACTOR_LIGHT_SAME_FRAME=0` keeps the
last frame's, to compare.

Patch 0216 finishes the sea for the HD look. The sea rooms' untextured square of sea counts as the sea at the size
it is drawn (`sea_shape()` scales its extent by the position matrix), so the sea mask takes the open sea and the
HD look's brightness no longer turns it pale cyan with a seam. Blended draws that write depth (a bush faded near
the camera) are drawn into the mask by their cut-out and clear the character, bloom and actor-light marks behind
them. On Wind Waker HD's light the sea takes HD's own term, 0.5 + 0.5 x clamp(cloud + floor) x the sun shadow in
linear light (its sea quad and wave grid), in place of the scenery's shade.

Patch 0217 dims the characters' sunlight under Wind Waker HD's cloud shadows, as HD does: mix(shade, lit, ramp x
cloud), the cloud read from the same texture, drift and floor as the scenery's cloud shadows on HD's rule. The floor
is HD's from the clock, 1 - max(0.25, -0.9 cos(time - 15 degrees)), which the host gives
(`aurora_set_cloud_hd_floor`); unset, it comes from the light's height. `DOL_AURORA_ACTOR_LIGHT_DEBUG=6` shows the
characters' cloud term.

Patch 0218 makes the HD shadow quality's cascades follow the camera, as Wind Waker HD's: each cascade is fitted to
its slice of the view between the splits, a far fade to lit from 12,000 to 14,000, and the facing term no longer
fades with distance. Each cascade is HD's own fit (`DOL_AURORA_SUN_SHADOW_CASCADES_HD=1`, the default), read from
Cemu captures of a turn at Outset's pier: the tight box round its slice of the view (the first from 30 units to the
first split), in a frame whose y is the camera's forward made flat to the sun, refitted every frame. At 21:9 the
square about the sphere round the slice wasted most of its texels: from the boat at Outset the first cascade was
4096 units square where HD's fit is 3194 x 1038. `=2` takes HD's frame only, the sizes held in steps and the centre
snapped to whole texels. `=0` gives stable cascades: each in the frame fixed to the sun, its box the square about the
sphere round its slice, so the grid doesn't move as the camera turns; with it,
`DOL_AURORA_SUN_SHADOW_CASCADES_STABLE=0` turns the frame with the camera instead (and `_TURN_STEP` applies only
then). Past the second cascade only the stage's BG lists cast, as Wind Waker HD's drawShadowCascade draws mpModelList
into every cascade and the other lists into the first two: the host marks the game's draw list in BP register 0x7A
(0 or never written: a BG list, 1: another), kept until written again, and a game module that doesn't write it keeps
the old rule (`DOL_AURORA_SUN_SHADOW_FAR_CASTERS=all` keeps every caster in every cascade).
`DOL_AURORA_SUN_SHADOW_CASCADES_FOLLOW=0` keeps the cascades fixed to the sun, to compare;
`DOL_AURORA_SUN_SHADOW_TEMPORAL_DEBUG=2` shows debug view 12 unblurred, with the pre-pass shadow in red.

Patch 0219 lets alpha-tested draws of more than one texture cast into the sun map with the cut-out of the texture
their alpha is from (`gxc::cutout_alpha_texmap()`: a Windfall girl's body, colour from texmap 0 and its toon ramp
from texmap 1, cast no shadow before), and still scenery the game draws blended and alpha-tested while writing
depth (Outset's grass) casts through the same cut-out, as Wind Waker HD casts it. Additive glows and what the game
writes each frame stay out.

Patch 0220 gives three more toon patterns Wind Waker HD's actor light, right after the albedo stage, with the
stages after it run on the lit colour as the GameCube ran them: the colour-register albedo (the Mirror Shield's
face, the Master Sword's grip, ice, Chus; no rim, and the glint added in linear light as HD adds it), a register
added to stage 1 (the bomb's flash; no rim, as HD's material table has for all 28 of those materials), and the main
pattern then other stages (the chests' decal). The GameCube's effect-light stage (PREV + K1 x the ramp: the
sword-hit flash, the bombs' flash, the item-get light) runs on HD's colour too, until a light buffer takes it
(`DOL_AURORA_ACTOR_LIGHT_HD_EFFECT=0` leaves it out, to compare). `DOL_AURORA_ACTOR_LIGHT_HD_EXT=0` keeps the main
pattern only.

Patch 0221 adds Wind Waker HD's far blur ramp (`DOL_AURORA_FAR_BLUR_HD=1`): clamp(level x (distance - start) /
(end - start)) with HD's 25,000 and 50,000, in place of the smoothstep from 8,000 to 30,000, times
`DOL_AURORA_FAR_BLUR_AMOUNT` as before. The level is HD's for the hour (1.2 by day, 0.65 at night, blended at
dawn and dusk), from the host (`aurora_set_far_blur_level`).

Patch 0222 adds Wind Waker HD's light buffer (`DOL_AURORA_POINT_LIGHTS_HD=1`): the point and effect lights the
host sends on HD's rule (`aurora_set_point_lights`, up to 32) add colour x sat(N.L) x sat(1 - d / r)^1.5 to the
characters' HD light (x 1.75) and to the lit scenery's stage-0 light (x 1.25) in their own draws, and the sun
shadows' shade leaves that light as HD's does (1.25 x AO x the lights after its shade). With it on, the torch
glow and the GameCube's effect-light stage are left out, so the lights aren't counted twice.
