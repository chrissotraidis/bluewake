# HD look: GPU and CPU cost against main (November 4, 2026)

What #533's HD look costs to draw, measured against the 0.10.0 release, on one PC. This is a record, not a guide.

## Summary

On an RTX 5070 Ti at 2560x1080 (21:9):

- **The whole HD look in #533 costs 0.28–0.66 ms of GPU time per frame** (look on against look off). Main's own HD
  look already costs 0.20–0.63 ms.
- **Against main, #533 adds +0.02–0.08 ms outdoors** (Outset's dock at 10:00, Windfall at night, the open sea) and
  **+0.13–0.48 ms in dungeons and Hyrule**, where main draws no sun shadows and #533 lights the rooms.
- **No dusk spike.** Running Outset's hilltop from 18:01 with the clock going, #533 stays at 1.29–1.37 ms from dusk to
  night. A player can't stay at dusk anyway: past 18:00 the clock runs on to 23:00.
- **The one real cost is on the CPU, in Hyrule.** Sun shadows add 2.4 ms per game frame there (7.82 ms with them,
  5.46 without). Elsewhere the look adds 0.27–0.79 ms of CPU per game frame. Hyrule's is worth a look: probably the
  shadow casters being re-encoded per cascade, or the lit room's caster culling.
  - **Update, November 5:** it wasn't the casters. The GX worker made the sun shadows' smooth normals for the hall's
    66 lit draws (47,000 vertices) again every frame. RecompCore patch 0240 keeps them per draw, and I measured it on
    the i5-6500 below.

The GPU is mostly idle either way: unpaced, the game is CPU-bound at roughly 80–200 FPS here, and GPU busy time stays
under 1.4 ms per frame everywhere.

## How I measured

| | Build |
|---|---|
| A | main: the v0.10.0 Linux AppImage (SHA-256 checked against SHA256SUMS; RecompCore 78c266d2, the same pin as main), its HD look on |
| A0 | A with `BLUEWAKE_HD_LOOK=0` |
| B | #533 at 07ffbd9d (RecompCore 4b98e06c90), with the HD look's launch switches, and a game module rebuilt with the far-cascade hook but without PGO training |
| C | B with `BLUEWAKE_HD_LOOK=0` |

- **Machine:** Ryzen 9 9900X, RTX 5070 Ti, NVIDIA 615.71.09, Arch Linux, Vulkan.
- **Same inputs for all:** my settings (Smooth Motion off, render scale 1), a copy of a late-game card, quest log 1,
  and the same HD pack.
- **GPU time:** `DOL_AURORA_GPU_PROF=1`, its `busy_ms` (the summed pass and zone times from timestamp queries).
- **Unpaced:** no wall pace, no audio throttle, no vsync. Paced at 30 FPS the GPU idles about 85% of the time and
  drops to about 1.3 GHz, and the numbers stop measuring work (paced, look-off read 4.5 ms against look-on's 3.7).
- **Each run:** a stage-select warp to the place, then 1680 frames with the camera still. A, B, C and A0 in alternating
  order, 3 rounds at 2560x1080 and 2 at 1920x1080 (16:9). Run-to-run spread is 0.01 ms or less.

## GPU busy ms per frame, 2560x1080

Median of 3 runs.

| Place | A main | B #533 | C look off | A0 main, look off | B−A | B−C |
|---|---|---|---|---|---|---|
| Outset dock, 10:00 | 0.990 | 1.069 | 0.522 | 0.479 | +0.08 | +0.55 |
| Outset hilltop, from 18:30 ¹ | 1.001 | 1.345 | 0.702 | 0.639 | +0.34 | +0.64 |
| Windfall pier, 21:00 | 1.208 | 1.231 | 0.636 | 0.581 | +0.02 | +0.60 |
| Open sea, 12:00 | 0.827 | 0.851 | 0.452 | 0.407 | +0.02 | +0.40 |
| Dragon Roost Cavern | 0.596 | 1.073 | 0.410 | 0.346 | +0.48 | +0.66 |
| Forbidden Woods, room 5 | 0.691 | 1.053 | 0.560 | 0.481 | +0.36 | +0.49 |
| Tower of the Gods, room 0 | 0.915 | 1.046 | 0.772 | 0.704 | +0.13 | +0.28 |
| Ganon's Tower | 0.825 | 1.147 | 0.681 | 0.625 | +0.32 | +0.47 |
| Hyrule Castle (layer 2) | 0.730 | 1.157 | 0.610 | 0.507 | +0.43 | +0.55 |

¹ The warp sets the clock once and doesn't hold it, so this run covered 18:46 to 21:40. Between about 19:30 and 20:45
main switches its sun shadows off and drops to 0.94 ms; outside that stretch A and B are within 0.04 ms. The higher B
number than at the dock comes from the hilltop view, not the hour.

At 1920x1080 (16% fewer pixels) every build drops only 5–10%: on this card most of the cost is per frame (shadow
cascades, fixed-size passes), not per pixel.

## What the parts cost

B with one part switched off at a time:

| Part | GPU ms per frame |
|---|---|
| HD linear fog's frame copy | 0.044–0.066 |
| Shadow temporal pass | 0.034–0.038 |
| HD bloom (over main's bloom) | 0.02–0.04 |
| HD rim light | under 0.025 |
| SSAO, cloud shadows | about 0 (they run inside the sun pass) |
| Sun shadows as a whole | −0.23 to +0.22 (HD's depth pre-pass that comes with them makes the scene passes cheaper) |

The `hd` shadow quality is 0.2–0.74 ms cheaper than `high`, so it should stay.

## CPU, ms per game frame (unpaced)

Compare B with C: both use the module built without PGO, which by itself makes them 0.5–2 ms slower than the release.

| Place | B | C | B−C |
|---|---|---|---|
| Outset dock | 12.24 | 11.66 | +0.57 |
| Outset hilltop | 14.52 | 13.72 | +0.79 |
| Windfall pier | 11.53 | 10.78 | +0.74 |
| Open sea | 6.03 | 5.77 | +0.27 |
| Dragon Roost Cavern | 7.68 | 7.35 | +0.33 |
| Forbidden Woods | 7.89 | 7.56 | +0.33 |
| Tower of the Gods | 13.73 | 13.08 | +0.65 |
| Ganon's Tower | 8.32 | 8.05 | +0.27 |
| **Hyrule Castle** | **7.77** | **5.34** | **+2.42** |

## Caveats

- **One fast card.** A Steam Deck's GPU is roughly 10–20 times slower, so these fixed costs could become whole
  milliseconds there. They need measuring on a Deck, not scaling from these.
- **The stage select was on for every run** (it's how the warps work). On the Deck it costs about 10% in the host
  code, so the CPU times here are inflated by it. It's the same for every build, and the GPU timestamps shouldn't be
  affected.
- **GPU clocks can't be locked** without root, and NVIDIA clocks a look-off run lower, so B−C is if anything
  understated.
- **The profiler's `frame_ms` is unreliable with sun shadows on:** one timestamp goes stale and the value grows every
  window. I used `busy_ms`. Its per-pass list also gives the fourth cascade more time than it costs (its timer starts
  while earlier work is still running), which is why the parts above come from switching each one off.
- Not measured: Windows, Mac, the Steam Deck.

## Both modules trained, look off (Arch, update)

The #533 module above had no PGO training. With #533's module built by the release recipe (the CI container, clang
18, the builder's own training), and the look off on both builds:

- **CPU:** #533 costs +0.04 to +0.23 ms per game frame more than 0.10.0 (+0.5 to +2.5%, mean +1.7%, 9 places × 3
  rounds). The earlier gap of +0.5 to +2.0 ms was mostly the untrained module.
- **GPU:** +0.04 to +0.09 ms, the same as before. The module doesn't change GPU time.
- The look's own cost (look on against look off) is unchanged, including Hyrule's +2.4 ms of CPU.

## A weaker PC: i5-6500 and GTX 1660 SUPER (Debian 13, November 4)

The same comparison on a weaker machine: an i5-6500 (4 cores, 4 threads), a GTX 1660 SUPER (NVIDIA 550.163.01),
Debian 13, Vulkan, a 1280x720 window with the render scale on "Window" (720 lines). Both builds are AppImages from the
release recipe: 0.10.0, and #533 at 58b72a43 with its own trained module. Headless, the two are within 0.4–2% of each
other on the CPU. 3 unpaced rounds per place, then 1 paced round at 30 FPS, as players run it.

**Paced, 30 FPS (what you'd feel):** every build holds 29.97 FPS at every place, #533 included, and no frame took over
50 ms anywhere. Share of frames over 34 ms:

| Place | main | #533 | #533, look off |
|---|---|---|---|
| Outset dock, 10:00 | 2.0% | 2.0% | 1.9% |
| Outset hilltop, evening | 1.5% | 1.9% | 2.1% |
| Windfall, open sea, Dragon Roost Cavern | 0–0.1% | 0–0.2% | 0% |
| Forbidden Woods | 0.8% | 0.8% | 0% |
| Tower of the Gods | 4.0% | 4.6% | 4.2% |
| Ganon's Tower | 1.1% | 1.7% | 1.0% |
| **Hyrule Castle (layer 2)** | 0% | **5.9%** (p99 34.7 ms, worst 38.6) | 0% |

**CPU, unpaced, ms per game frame:**

| Place | main | #533 | #533, look off | look's cost |
|---|---|---|---|---|
| Outset dock | 24.8 | 25.9 | 24.7 | +1.2 |
| Outset hilltop | 29.4 | 30.4 | 29.2 | +1.2 |
| Windfall pier | 24.0 | 25.1 | 23.8 | +1.3 |
| Open sea | 13.8 | 14.1 | 13.3 | +0.8 |
| Dragon Roost Cavern | 17.6 | 18.4 | 17.7 | +0.8 |
| Forbidden Woods | 17.1 | 17.9 | 17.1 | +0.8 |
| Tower of the Gods | 32.0 | 33.0 | 32.1 | +0.9 |
| Ganon's Tower | 18.9 | 19.7 | 19.0 | +0.7 |
| **Hyrule Castle** | 13.9 | **22.2** | 14.0 | **+8.3** |

- **Where it lands:** from each thread's CPU time, the game's emulation thread carries only +0.6 to +1.3 ms of the
  look. The rest, about +11 to +14 ms of CPU time per game frame summed over threads, is on the rendering and driver
  threads. So the look mostly stays off the game thread, but in Hyrule the rendering side holds the frame back.
- **GPU:** this card moves between power states during the runs (graphics 600–1250 MHz, memory 810–7000 MHz), so the
  raw GPU milliseconds jump around. Normalised to one clock, the look costs about 0.2–1.3 ms per frame, and #533
  against main mostly lands within ±0.7 ms.
- **The one thing to fix:** Hyrule's sun shadows, at +8.3 ms of CPU per game frame on this machine (+2.4 ms on the
  9900X). It still holds 30 FPS, with about 6% of frames slightly late.
  - **Update, November 5:** with the smooth-normals cache (patch 0240), Hyrule Castle layer 2 goes from 22.2 to 15.0 ms
    of CPU per game frame, and paced late frames from 5.8% to 0.1%, the same while turning the camera or walking. The
    frozen castle is still held back by the emulation thread: −0.35 ms still, −1.3 ms turning, and walking is still
    emulation-bound. The picture doesn't change.
- Not covered here: other render scales and Smooth Motion. Those runs are next.
