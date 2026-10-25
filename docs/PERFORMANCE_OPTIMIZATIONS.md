# Performance optimizations

Every change made to speed up Wind Waker Recomp on the Mac from 2026-09-28 to 2026-09-29, with what it
fixed, where it lives, what it measured, and how to turn it off. It started at 25 to 30 frames a second
with the display tied to the game's 30; the game now holds 60 or 120 on screen in the scenes tested,
including the Forsaken Fortress and Adanmae, while its own logic still runs at its 30 steps a second.

Measurements are from an Apple M3 Max MacBook Pro (3456 x 2168 display at 120 Hz) with the user's save,
full screen, the ZWW4K 4K texture pack and 16x anisotropy unless a row says otherwise. The engineering
log with the full reasoning for each is [docs/archive/status/CURRENT.md](archive/status/CURRENT.md); the RecompCore
changes are recorded as patches in [patches/recompcore](../patches/recompcore/README.md) (0098 to 0110)
and are in https://github.com/elliotttate/RecompCore, branch `bluewake`, at 6892947.

Earlier measured work (PGO of the hot chunks, the high-level DSP, the guest alias index, uniform reuse,
the GX worker and FIFO changes, 2026-09-23 and 24) is in [docs/archive/status/PERFORMANCE.md](archive/status/PERFORMANCE.md).

## Where the time goes

The game's PowerPC code is translated ahead of time to arm64 (no JIT), but it is one CPU's worth of
code: every actor, collision check and animation reads and writes shared memory in a fixed order, so it
cannot be split across cores. What runs beside it:

| Thread | Work |
| --- | --- |
| Game thread | the translated game (about 86 percent of it) and the host's interrupt timing |
| GX translation worker | the game's GX command stream into gxcore draws; Smooth Motion's matching |
| Smooth Motion helper | matching and blending each draw for the in-between frames |
| Render worker | encoding each frame (and each in-between frame) for Metal, presenting |
| GPU | drawing every frame, four times a game frame at 120 Hz |

The game waits for the GX worker at the end of every frame, and the render worker waits for drawables
from macOS, so a slow worker or a late drawable slows the game itself. Most of the work below takes
load off those two workers or stops them waiting.

## Summary

| # | Change | Result | Where |
| --- | --- | --- | --- |
| 1 | PGO without the machine outliner | untrained code (486 of 748 chunks) no longer a call every three instructions | `scripts/builder/build.sh` |
| 2 | Smooth Motion: in-between frames | 30 -> 60 frames a second on screen, game unchanged | 0098 |
| 3 | Instanced scenery pairing | grass, bushes, palms blended, not flickering | 0098 |
| 4 | In-between frames through fast turns | 355 of 355 frames interpolated in fast turns (up to a third lost before) | 0100 |
| 5 | Draw-heavy scenes: compare once, reuse, blend only what is read | Forsaken Fortress 49 -> 60; blending 10.0 -> 1.8 ms a frame | 0102 |
| 6 | Matching and blending on a helper thread | taken off the GX worker the game waits for | 0103 |
| 7 | 120 FPS in-between frames | 120 on screen on a 120 Hz display | 0108 |
| 8 | A steady present clock | 120 Hz frames 8.0 ms apart instead of bursts; 60 Hz 16.5 ms instead of 22/12 | 0108 |
| 9 | Pass state set once a pass | encode 3.65 -> 1.60 ms median, 24.7 -> 5.7 ms worst; Adanmae 107-110 (dips to 68) -> 119-120 | 0110 |
| 10 | Draw batching | Adanmae 6,400 -> 3,600 draws a frame; the sea 11,800 -> 4,600 | 0110 |
| 11 | In-between data through mapped staging buffers | 41-55 MB of zero-filled uploads a frame gone; render worker 35 -> 32 % | 0110 |
| 12 | Pacing under overload | the game keeps full speed under GPU overload at 60 Hz (77-85 % before) | 0110 |
| 13 | No drawable while hidden, surface kept | half-second freezes at full-screen switches gone | 0110 |
| 14 | Helper thread spins less, woken in batches | spinning 431 -> 22 samples; helper 34 -> 29 % | 0110 |
| 15 | PC branch picks (RecompCore) | gather-pipe words straight to the batch, draw plans reset in place, a texture cache crash fixed | 0110 |
| 16 | PC branch picks (host) | graphics address resolutions cached, actor search checks collapsed, a crash in 1 of 8 launches fixed | `runtime/host/src/main.c` |
| 17 | Mouse camera check inline | no call at every translated block boundary | `runtime/host/src/mouse_camera.h` |
| 18 | Fast scene changes | a door or exit 2.2 -> 0.63 s | `runtime/host/src/fast_load.c`, 0101 |
| 19 | Quick doors | a door with a knob 5.1 -> 1.9 s | `runtime/host/src/quick_doors.c` |
| 20 | Settings read again at start | saved 120 Hz and 16x anisotropy actually apply at launch | 0109 |
| 21 | GX worker: derived pipeline state cached by a register version | 96 % of draws reuse it; with 22-24, a slower CPU's heavy view 23-26 -> 29-30 game FPS; on an M3 Max the worker 55.4 -> 53.2 % busy | 0115 |
| 22 | GX worker: assembly totals only where read | a walk over every vertex's indices gone from the renderer's consumer | 0115 |
| 23 | GX worker: pipeline and texture bind group lookups remembered | no `std::function` allocation, hash or lock for a draw that repeats the one before | 0115 |
| 24 | Smooth Motion hand-off without a copy for repeated constants | a sequentially consistent fence only when the helper may sleep | 0115 |
| 25 | Graphics threads' CPU time, slow presents and the CPU in the session log | `[fps] busy:`, `[fps-dip] threads:`, `[present-slow]`, `CPU cores:`; `CPU model` from CPUID | 0116 |

Patch numbers are RecompCore changes (`patches/recompcore/NNNN-*.patch`, in RecompCore's
`GXRuntime/graphics/aurora/lib/gfx/` unless noted).

## Build

### 1. PGO without the machine outliner

**Problem.** The default build trains its profile (PGO) on a run from boot to the first moment the
player controls Link. Clang marks code the profile never saw as cold and runs the machine outliner over
it, which pulls repeated instruction sequences into shared snippets it calls. That was 486 of the 748
game chunks: the boat, Bokoblins, Moblins, most NPCs, bosses and dungeon objects. Bokoblin's first
chunk (d_a_bk) compiled to 567 KB with 45,364 calls into snippets, about one every three instructions;
without the profile, or with the outliner off, it is 982 KB with none.

**Change.** `pgo_flags()` in `scripts/builder/build.sh` adds `-mllvm -enable-machine-outliner=never`.
Trained code keeps the profile's layout and inlining; untrained code compiles as plain -O2.

**Result.** The outlined calls are gone (checked in the emitted code). Its effect on the frame rate was
not measured separately on a device; it applies to every chunk outside the training route.

## Smooth Motion (in-between frames)

Wind Waker's logic advances one step per frame and waits 1/30 s between frames. Unlocking that wait
doubles the game's speed (the Meowmaritus 60 FPS hack needs about 50 slow-down patches and lists
softlocks), and the CPU could not run the logic twice as often anyway. So the game keeps its 30 steps,
and the renderer draws frames between each pair: every gxcore draw carries its whole transform state,
so an in-between frame is the finished frame's passes encoded again with each draw's matrices and
lights blended toward the same draw in the frame before. It is real geometry, not image warping.
Switch: `DOL_AURORA_FRAME_INTERP=1` (on), `DOL_AURORA_FRAME_INTERP_STEPS=1` or `3` (60 or 120), or the
options menu's Smooth Motion (Off, 60, 120).

### 2. In-between frames for 60 FPS (0098, `frame_interp.cpp`, `common.cpp`)

A draw is keyed by a hash of its vertex payload, paired with the same draw in the previous frame, and
blended halfway if the pair is plausibly one object a frame apart; 40 percent implausible is a camera
cut, shown without an in-between frame. The blended blocks go to their own uniform area.

| | 30 FPS | Smooth Motion 60 |
| --- | --- | --- |
| Frames on screen a second (title, Outset play) | 29.9-30.0 | 59.8-60.0 |
| Draws blended in a heavy title frame | - | 99.6 % |
| Game logic (new game to control, 6.4 min) | retrace 20,405 | the same |
| CPU time on the same route | 195 s | 245 s (+25 %, on the graphics threads) |
| Peak memory | 1.26 GB | 1.43 GB |

### 3. Instanced scenery pairing (0098)

Grass, bushes and palms are copies of one model that the game culls one by one, so a copy leaving the
view shifted every later copy onto its neighbour: foliage flickered in turns, and one in-between frame
drew a palm that is in neither real frame. Unique draws vote for the camera's motion, and each copy
pairs with the previous copy that motion carries onto it; a copy new in view is drawn unblended. Cost:
the GX worker's matching 3.48 -> 3.68 ms a game frame.

### 4. In-between frames through fast turns (0100)

A mouse turn over about 11 degrees a game frame moved distant scenery past the plausibility bounds: up
to a third of frames were taken for cuts (the counter fell toward 30), and draws that failed alone
jumped against the blended scene. The camera's motion is now taken out of each pair, so plausibility
judges the object's own motion and the in-between frame carries exactly half the camera's turn; copies
are found through a grid of where the camera carries them (a budgeted scan ran out when a turn brought
dozens into view); and copies just come into view do not count toward a cut during a turn.

| Scripted turns, 11-32 degrees a game frame | Before | After |
| --- | --- | --- |
| Frames interpolated | up to a third lost | 355 of 355 |
| Draws shown unblended | 5.9 % | 0.65 % |
| Frames on screen a second | toward 30 | 59-60 |

### 5. Draw-heavy scenes (0102)

The Forsaken Fortress exterior draws about 17,500 times a frame (550 is usual) and fell to 49 frames a
second. The GX worker was 93 percent busy, about 40 ms for each 33 ms game frame, 31 percent of it the
in-between blending, which compared and copied a 2.8 KB constant block per draw (about 50 MB a frame).
96 percent of its draws repeat the vertex constants of the draw before. Now:
- a repeated draw's constants are compared once, in `submit_draw_plan`, instead of three times (the
  uniform de-duplication, the in-between pool, the in-between block's de-duplication);
- a draw that repeats the one before and is matched the same way reuses its in-between block;
- indexed position and normal matrices are blended only for draws that read them.

A Fortress-shaped benchmark: 10.0 ms of blending a frame before, 1.8 ms after, with bit-identical
blended values. In play, the exterior at 17,900 draws a frame holds 60.

### 6. Matching and blending off the GX worker (0103, `gxcore_draw.cpp`, `frame_interp.cpp`)

Made for the Windows PC build (i9-13900KF, RTX 5090), where Smooth Motion held Outset (about 10,000
draws a frame) below 30 because the GX worker, which the game waits for at every frame's end, was
saturated. The Mac build has it too:
- draws are matched and blended on a helper thread, in draw order, while the GX worker goes on;
- the in-between staging is appended instead of cleared and copied, and the per-frame key table keeps
  its size instead of rehashing a dozen times a frame;
- an unchanged texture takes one hash lookup instead of two (the lookup by the whole key was a tenth of
  the worker on Outset);
- a frame with nothing to blend (a menu, a still shot, a cut) keeps the in-between cadence instead of
  presenting early (the counter read 30 in every menu).

### 7. 120 FPS (0108)

Three in-between frames a game frame, at a quarter, half and three quarters, for a 120 Hz display. The
camera's part of each step is the screw motion's power at t, so four quarter steps make the whole. The
in-between uniform area has 32 MB per step: at 120 Hz a beach needed about 55 MB, the blocks past 32
did not fit, and the sea blinked out on the shore in two of three in-between frames.

### 8. A steady present clock (0108, `common.cpp`)

The first in-between frame was presented at once and the rest after it, so at 120 Hz the second came
18 ms late and the third and the real frame were forced out back to back (the sea shimmered), and 60 Hz
alternated 22 and 12 ms. Presents are now held and shown on one clock, a quarter (or half) of a game
frame apart, continuing from the frame before's last, from two alternating sets so a new game frame
never forces the one before's out early; what is due is presented between replays and every 512
commands of a pass.

| | Before | After |
| --- | --- | --- |
| 120 Hz: gap between presents | bursts of 18, 2.5 and 0.1 ms | typical 8.0 ms (ideal 8.3), 9 in 10 within 10.8 |
| 120 Hz: frames forced out early | about half | none outside scene changes |
| 60 Hz: gap between presents | alternating 22 and 12 ms | 16.5 ms (ideal 16.7) |

Switch: `DOL_AURORA_PRESENT_CLOCK=0` keeps the old 60 Hz timing.

## The render worker

At 120 Hz each game frame is encoded four times (the real frame and three in-between frames) within
about 33 ms, so its cost per draw is multiplied by four.

### 9. Pass state set once a pass (0110, `gxcore_draw.cpp`)

Every draw re-set its vertex buffer, index buffer and bind groups even when nothing had changed. Now
the whole vertex and index buffers are bound once a pass (a draw is its base vertex and first index),
and the pipeline and bind groups are set only when they or their offsets change; the cached state is
cleared at each pass and after any other kind of draw.

| Adanmae, 120 Hz, about 6,400 draws a frame | Before | After |
| --- | --- | --- |
| Encode one frame, median / 90th percentile / worst | 3.65 / 6.85 / 24.7 ms | 1.60 / 2.03 / 5.67 ms |
| Present, median | 2.64 ms | 1.43 ms |
| One-second waits for a drawable | 2 | 0 |
| Frames on screen a second, walking | 107-110, dips to 68 | 119-120 |

### 10. Draw batching (0110, `gxcore_draw.cpp`, `common.cpp`)

A draw with the same pipeline, vertex constants, pixel constants and textures as the pass's last
command, whose vertices and indices follow on, extends that command (its indices counted from the
batch's first vertex, up to 65,536 vertices). In-between frames draw a batch as one where its draws'
blended blocks came out the same and split it where they differ, so Smooth Motion is unaffected. It is
done at record time, the way Dusklight's renderer merges draws.

| | Off | On |
| --- | --- | --- |
| Draw calls a frame, Adanmae walk | about 6,400 | about 3,600 |
| Draw calls a frame, at sea | about 11,800 | about 4,570 |
| Encode a frame | 1.57 ms | 1.36 ms |
| GPU use, render scale 6 at sea (median) | about 53 % | about 50 % |

Switch: `DOL_AURORA_GXCORE_BATCH=0`.

### 11. In-between data through mapped staging buffers (0110, `common.cpp`)

The blended blocks (about 41 MB a frame at 120 Hz, 55 MB in a busy scene) and particle vertices were
uploaded with `queue.WriteBuffer`, and above Dawn's 4 MB upload ring that makes and zero-fills a new
upload buffer every call: a fifth of the render worker's time, with the GPU's memory swinging by 350 MB.
They are now written straight into one of three staging buffers mapped for the frame and copied on the
GPU (`CopyBufferToBuffer`). A frame that finds no buffer back from the GPU after a 4 ms grace shows no
in-between frames; one that does not fit shows none and the next buffers are made bigger. The uniform
de-duplication compares against the staged block instead of a copy kept aside. Result: the render
worker 35 -> 32 percent busy, no zero-filling on it.

## Presentation and pacing

### 12. Pacing under overload (0110, `frame_interp.cpp`)

When the GPU could not finish four renders in 33 ms, the render worker waited for drawables, the queue
filled, and the game itself slowed to 55-75 percent speed with its own thread half idle. Overloads (the
GPU a game frame behind, the game waiting on the render worker or the GPU, a late drawable) in 3 of the
last 8 game frames take 60 Hz Smooth Motion to no in-between frames; it comes back after 3 s calm, with
a longer wait after a quick relapse. A single late buffer, texture load or hitch does not count. Forced
overload (render scale 8): the game holds full speed at 60 on screen, against 77-85 percent speed
without it.

120 Hz is not lowered by default: a drop to 60 for a stall already over was felt more than the stall.
`DOL_AURORA_FRAME_INTERP_PACING=1` paces 120 Hz too; `0` turns pacing off everywhere. The log's
`[interp-pace]` lines say each change and its cause.

### 13. No drawable while hidden, surface kept (0110, `aurora.cpp`, `window.cpp`)

About every 20 to 60 seconds the game froze for half a second (520-528 ms, once a full second). These
lined up with switching away from the full-screen game and with the full-screen animation at startup:
`nextDrawable` blocks while the window is out of sight. A window out of sight (another full-screen
space, or ours sliding in or out) now asks for no drawable, and the game keeps running. The surface is
also kept: the Error status of a texture never asked for had dropped a working surface and rebuilt it
on return (the "Surface texture is Error" warnings and a hitch). One stall of about 200 ms can remain
at the moment of switching, before macOS reports the window hidden.

## The GX worker and the helper thread

### 14. The helper thread spins less (0110, `gxcore_draw.cpp`)

The Smooth Motion helper spun 4,096 times before sleeping and was woken for every job. It now spins 256
times and a sleeping helper is woken for 32 jobs at once or at the frame's end. Sampled: spinning 431
-> 22 samples, the helper's load 34 -> 29 percent.

### 15. From the PC branch, RecompCore (0110)

From RecompCore's `native-60hz-pc` branch (d43acc0, fbaae39, f7154b5), whose work took the Windows
build from about 35 frames a second without Smooth Motion to almost 60:
- a gather-pipe word goes straight to the GX worker's batch (`backends/aurora/aurora_graphics.cpp`,
  `shadow_frontend_enqueue_word`), and the worker keeps its batch buffer between batches;
- a reused draw plan is reset in place instead of reallocated (`gxcore/src/gxcore.cpp`);
- the texture layout cache is locked for the pipeline compiler (a crash).

## The game thread (host)

### 16. From the PC branch, host (`runtime/host/src/main.c`)

- The guest-alias registry is locked for the GX worker's reads (290dc27): it crashed about one launch
  in eight.
- Graphics address resolutions are cached until the registry changes (c211003, 21792e3); on the PC
  this was the largest single step of its second jump (51 -> 55 and 57 -> 60).
- The actor search's nine budget checks per node are collapsed into their final bounds (from
  a7ea68f; `actor_search_budget.h`, covered by `tests/actor_search_budget_test.c`).

Adanmae at 120 Hz with these and 9-15: 119.8 frames a second, no slow render items.

### 17. Mouse camera check inline (`runtime/host/src/mouse_camera.h`)

The mouse camera's hook runs at every translated block boundary; its check whether there is anything
to do is now inline (`bluewake_mouse_camera_dispatch`), so the common case costs no call.

## Load times

### 18. Fast scene changes (`runtime/host/src/fast_load.c`, 0101)

A door or an exit took 2.2 seconds, none of it loading: the plain fade counts 26 game frames each way,
and the new scene may not load its sounds until 36 frames after the door. The host shortens a plain
fade to 6 frames as it starts (the scene still swaps only once the screen is fully black), and once the
screen has been black for 6 retraces it runs the game unpaced with `dol_aurora_set_fast_forward`:
frames are not presented and the audio queue is kept at its 100 ms target, so neither the display nor
the audio holds the game to real time.

| Warping into Link's house, windowed, Smooth Motion and sound on | Before | After |
| --- | --- | --- |
| Fade to black | 0.87 s | 0.20 s |
| Black while loading | 0.49 s | 0.25 s |
| Fade back in | 0.87 s | 0.18 s |
| Total | 2.2 s | 0.63 s |

Switches: `BLUEWAKE_FADE_FRAMES=N` (0 keeps 26), `BLUEWAKE_FAST_FORWARD=0`. The options menu has both.

### 19. Quick doors (`runtime/host/src/quick_doors.c`)

Through a door with a knob, once its fade covers the screen the rest (Link's walk behind it, the wait,
the door opening and closing again in the next room) is cut: 5.1 seconds to 1.9.
Switch: `BLUEWAKE_QUICK_DOORS=0`.

### 20. Settings read again at start (0109)

Smooth Motion, its steps, the FPS overlay and forced anisotropy were read by static initialisers before
the host applies the saved options, so a saved 120 came back as 60 and 16x anisotropy as none. The
backend reads them again when it initialises.

## The GX worker on a slower CPU (2026-09-30)

A tester's Ryzen 5 5600X got about 25 frames a second from the Windows release. Measured on the i9-13900KF
pinned to 12 of its efficiency cores (slower than a Zen 3 core one thread at a time), the release stood at
Outset's spawn view (about 14,400 draws a frame) at 23 to 26 game frames a second, the game waiting 190 to 250
ms a second for the GX worker at its frame's end, the worker itself only two thirds busy: the game's logic
and the translation's tail took turns. The details, the profile and the verification are in
[status/SLOW_CPU_2026-09-30.md](status/SLOW_CPU_2026-09-30.md).

### 21. Derived pipeline state cached by a register version (0115, `gxcore.cpp`)

`build_draw_plan_into` derived the whole pipeline key (TEV stages, lit channels, texgens, blending, depth)
from the BP, CP and XF registers at every draw. The key is now cached, keyed by a version `apply()` changes
whenever a BP, CP or VAT register changes value (the host's draw tag registers, which change at nearly every
particle draw and feed no derived state, leave it alone) and by the draw's own XF, channel, light and texture
values; the vertex walk is cached by its VCD and VAT. From the `native-60hz-pc` line (dc81247, f863821).
96 percent of draws hit at Outset.

### 22. Assembly totals only where read (0115, `render_sink.cpp`)

The consumer under the gxcore renderer walked every vertex's indices of every draw for totals that only the
shadow packet sink's frame comparison and the replay tools read, about 2 percent of the worker.

### 23. Pipeline and texture bind group lookups remembered (0115, `common.cpp`, `gxcore_draw.cpp`)

A draw's pipeline lookup built a `std::function` holding a copy of the whole pipeline config (a heap
allocation) and hashed it before `find_pipeline` looked at the last pipeline; with early depth, twice a draw.
The last config of each kind and its reference are remembered until the pipeline cache is shut down (a
generation says so). A textured draw with the view and sampler state of the one before reuses its bind group
within the frame, without the layout's, sampler's and bind group's hash and lock.

### 24. Smooth Motion's hand-off (0115, `gxcore_draw.cpp`)

Each draw's job copied its 2.8 KB constant block and paid for a sequentially consistent store and load (the
store buffer drained behind the copy): 4 percent of the worker. A job whose constants repeat the job before's
carries none (the helper keeps the last it was given), and the fence is paid only when enough jobs wait that
the helper may be asleep.

| E-cores, paced, Smooth Motion 60 | Standing at the spawn view | Game waiting for the worker | Running |
| --- | --- | --- | --- |
| Before | 23-26 game FPS | 190-250 ms a second | 26-27 |
| After | 29-30 | 20-65 ms a second | 30 |

Every frame is the same byte for byte: 19 of 19 captured frames with Smooth Motion off and 62 of 62 dumped
real and in-between frames with it on, against the release; and with `DOL_GXCORE_DERIVED_VERIFY=1` (every
hit derived again and compared) 34,851,442 hits through the whole new-game opening, 0 mismatches.

### 25. The graphics threads in the session log (0116)

`[fps]` ends with `busy: gx=N% interp=N% render=N%` and `[fps-dip]` with `threads:`, the CPU time the GX
worker, Smooth Motion's helper and the render worker used that second; a present that holds the game thread
100 ms or more writes `[present-slow]` with its parts (the worker's batch, the end of the frame, the window's
events, the next frame's begin), and a slow event pump `[events-slow]` (SDL's pump or the host's handlers). The
freezes of 0.4 to 1.2 s seen about one run in five on the test PC are SDL's pump waiting on another program:
when the window's activation changes, Windows waits for the window losing it to answer. The CPU model comes from CPUID (the WMI query failed after the first launch's
disc picker had set up COM security, and the log read `Unknown`), and `CPU cores:` gives cores and threads.

On an M3 Max, where memory copies and hashes are cheap, the gain is small: the worker 55.4 -> 53.2 percent
busy at the Outset spawn with Smooth Motion 60 (sampled 8 s each), 95.6 percent of draws hitting the cache,
and `DOL_GXCORE_DERIVED_VERIFY=1` finding no mismatch in 5.7 million hits on a walk nor in 7.0 million after
a save state load. The Mac line carries 21-25 as its patch 0112 (RecompCore 8ab24da, merged there from
RecompCore windows-release 82607d4).

## A 4-core CPU (2026-10-01)

Measured on 4 of the i9-13900KF's E-cores (affinity `0x000F0000`), the stand-in for a 4-core laptop CPU such as
an i7-8565U. Details in [status/CURRENT.md](archive/status/CURRENT.md), 2026-10-01.

### 26. Frame interpolation gives way to the game (0119, `frame_interp.cpp`, `gxcore_draw.cpp`)

At 60 FPS frame interpolation, the in-between frames' work (the capture on the GX worker, the matching on its
helper, the render worker drawing each frame twice) took cores the game's thread needed, and the game ran in
slow motion. The pacing now also drops in-between frames when game frames end more than 35.5 ms apart on
average. It waits up to 2 minutes before trying again, and a frame without them queues no matching work.

| 4 E-cores, paced, 60 FPS frame interpolation | Standing at the spawn view | Running |
| --- | --- | --- |
| 0.2.2 | 24.4-25.4 game FPS | 25.7-28.1 |
| With it | 29.6-30.0 | 29.7-30.0 |

The 60 FPS dumps (62 real and in-between frames) are byte for byte the release's.

### 27. Guest MEM1 as a global array (0118, `core/cpu.h`, `cmake/composite/guest_cpu.c`)

The Windows module reaches guest RAM through `bw_guest_mem1`, a global array that the host adopts as
`cpu.ram`, instead of through `cpu->ram`. The compiler then knows that a guest load or store cannot touch the
guest CPU's state (another global). The guest registers stay in host registers across guest memory accesses,
instead of being stored before every guest store and reloaded after. Unpaced on the 4 E-cores with
interpolation off, 1-5 percent faster (31.0 and 31.9 game FPS standing and running, now 32.4 and 32.3).

### 28. Up to 7 in-between frames (0119)

Not for speed: **Match the display** shows the display's rate in steps of 30, up to 240. The buffers start
where 60 and 120 used them and grow only as a frame needs. See [WINDOWS.md](WINDOWS.md).

### 29. Guest loads and stores inline (gather_pipe.h, scripts/windows/lean_memory.py)

clang had stopped inlining gather_pipe.h's guest memory wrappers into the chunks (each chunk is one huge
function): every guest load and store was a call into a helper with its own frame, and the guest registers
went back to the CPU state around each. Ordinary MEM1 is now inline and forced, the rest the old wrapper out
of line. In the prepaid copies a plain access no longer stores its pc and cycle suffix first (only an MMIO
handler reads them; the out-of-line path stores them). On 4 E-cores, game-thread CPU per game frame 26.3-26.6
ms before, 23.6-24.2 after; frames identical. Tried and dropped (slower or larger): a one-test block entry,
the return dispatch through a label table, and gating the deadline test on the out-of-line path. Details in
[status/CURRENT.md](archive/status/CURRENT.md), 2026-10-01.

## Finding slow spots

| Tool | What it shows |
| --- | --- |
| `[fps-dip]` (host, `fps_watch.c`, on unless `BLUEWAKE_FPS_WATCH=0`) | each second under 57 on screen: game speed, frames interpolated, rejected and unmatched draws, waits on the GX worker, presents and the GPU, the graphics threads' CPU, stage, room and Link's position |
| `[fps] ... busy:` (Windows) | each second: the GX worker's, Smooth Motion helper's and render worker's CPU time |
| `[present-slow]`, `[events-slow]` | a present that held the game thread 100 ms or more, and which part took the time; a slow event pump, SDL's or the host's |
| `[interp-pace]` | each Smooth Motion pacing change and its cause |
| `[render-slow]` (`DOL_RENDER_SLOW_MS`, default 60) | a render worker item, acquire or present that took that long |
| `[gx-slow]` (`DOL_GX_SLOW_BATCH_MS`, default 20) | a GX batch that held the game that long, with the pipelines and textures it made; `[gx-slow-bytes]` adds its first bytes at 100 ms |
| `DOL_GX_STALL_STACKS=1` | the GX and render workers' stacks when a batch runs 200 ms |
| `DOL_AURORA_PRESENT_LOG=1` | each present, encode and present times, in-between data sizes |
| `DOL_GXCORE_DRAW_DUMP=<game frame>` | every draw of that frame: TEV and indirect stages, texgens and their matrix rows, textures, registers |
| `DOL_AURORA_FRAME_INTERP_TRACE=<first-last>` | each draw's matching outcome over those game frames |
| `DOL_AURORA_FRAME_INTERP_DUMP=dir` | real and in-between images, checked by `scripts/mac/frame_interp_report.py` |
| `BLUEWAKE_TEST_PLACE=retrace:x:y:z` | stands Link at a position, to profile one spot repeatably |

## Measured and not done

- **Dusklight-style particle batching.** Dusklight bakes each particle's matrix, colour and texture
  transform into its vertices so an emitter is one draw. Not built: in Adanmae only about 100 draws a
  frame are tagged particles, batching already brought the area to 3,000-4,000 draws, a trace there
  showed the GPU about 42 percent used (about 3.5 ms a render), and the game was waiting on
  presentation rather than draw calls. A Wind Waker version means a native replacement for its older
  JPA drawing, written from the decomp, keeping the particle tags Smooth Motion pairs with.
- **The PC branch's derived graphics-state cache** (96 percent hit rate on the PC). Needs the particle
  tag registers left out of its key and a pipeline cache version bump.
- **The PC branch's game-code rewrites** (direct calls between chunks, inlined register saves, inline
  FP helpers, native skinning and vector math). The big game-thread gain, but only the Windows builder
  runs them; the Mac needs a builder step, a module rebuild and retraining, a stack-depth check and a
  route check.
- **A submission thread for the GX worker** (2026-09-30). The FIFO parse and the plans on one thread,
  Aurora's recording and the presents on another, a queue of plans between them. Exact, but no faster on the
  E-cores (unpaced 32.2-32.8 game FPS against 33.7 without it) and 20 to 30 percent more CPU for the two
  together: each draw's plan, about 4 KB with its vertices, crossed from one core's cache to the other's.
- **Running the game's code on more cores.** Not possible: it is one CPU's code over shared memory in
  a fixed order.
- **Remaining 120 Hz dips in Adanmae's heaviest views.** The limit there is presentation, not draw
  calls or the GPU's shading.
- **Batching audio pushes** (2026-10-01; 256 frames a push, not each DSP chunk) and **a larger GX FIFO
  hand-off** (4-32 KB): no measurable change on the 4 E-cores.
- **Tried earlier and measured no gain or slower** (CURRENT.md and PERFORMANCE.md): exact inline FP
  fast paths, forced inlining of the paired-single loads and stores, a lower render resolution,
  compiling for M2-class chips, a last-alias lookup cache, native dispatch of __save_gpr and
  __restore_gpr, native cache-flush loops, a higher priority for the graphics workers.

## Switches

| Variable | Default | Effect |
| --- | --- | --- |
| `DOL_AURORA_FRAME_INTERP` | off unless Smooth Motion is on in the menu | Smooth Motion on |
| `DOL_AURORA_FRAME_INTERP_STEPS` | 1 | in-between frames: 1 for 60 Hz, 3 for 120 Hz, up to 7 (240 Hz) |
| `DOL_AURORA_FRAME_INTERP_PACING` | 60 and 90 Hz; the game's own speed at any rate | 1 paces the GPU at 120 Hz and up too, 0 turns all pacing off |
| `DOL_AURORA_PRESENT_CLOCK` | on | 0 keeps the old 60 Hz present timing |
| `DOL_AURORA_GXCORE_BATCH` | on | 0 turns draw batching off |
| `DOL_GXCORE_DERIVED_CACHE` | on | 0 derives the pipeline state at every draw |
| `DOL_GXCORE_DERIVED_VERIFY` | off | 1 derives it again at every cache hit and reports a mismatch |
| `DOL_AURORA_RENDER_SCALE` | display size | render resolution (the options menu) |
| `BLUEWAKE_FADE_FRAMES` | 6 | a plain fade's length in game frames; 0 keeps the game's 26 |
| `BLUEWAKE_FAST_FORWARD` | on | 0 keeps a scene change's black at real time |
| `BLUEWAKE_QUICK_DOORS` | on | 0 keeps the game's doors |
| `BLUEWAKE_FPS_WATCH` | on | 0 turns the `[fps-dip]` lines off |
