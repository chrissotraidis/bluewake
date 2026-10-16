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
