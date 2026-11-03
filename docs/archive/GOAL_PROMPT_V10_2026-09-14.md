# BlueWake goal loop - v10 (2026-09-14)

Supersedes v9 ([GOAL_PROMPT_V9_2026-09-14.md](GOAL_PROMPT_V9_2026-09-14.md)). The PRD still defines
what finished means. This loop replaces the central claim of v9 - that the new-game intro is the wall -
with the measurement that actually sets the budget: **the host executes the whole machine on one CPU
thread at about a quarter of the speed the target needs, and no single function is responsible.**

## What v9 got right, and what it got wrong

Right: the product path reaches Outset Island; the intro is authored content that runs to its end; START
and A do not skip it (H-INTRO-SKIP, answered, not supported).

Wrong: v9 called the intro "the wall" and left open that a path-specific slowdown explained it. It does
not. A profile of the live product path shows the cost spread across the whole emulated machine, and the
intro is simply the longest stretch of it.

## The measurement that sets this loop

`sample` on the live product process, title and opening phase, 6 s, 4228 main-thread samples:

| share | where |
| --- | --- |
| 71% | `bluewake_composite_dispatch_until_boundary` then recompiled guest code |
| 21% | `func_803256E0`, the guest GX FIFO writer, plus the aurora path it feeds |
| 5% | `DSP::Interpreter::Step()` |

The same profile taken during the new-game intro, 7603 samples:

| share | where |
| --- | --- |
| 58% | `bluewake_composite_dispatch_until_boundary` then recompiled guest code |
| 28% | `host_sync_cycle_devices_end_turn` then `DSP::Interpreter::Step()` |
| 5% | other main-thread work |

Threads: the emulation thread sits at 97-100% of one core, and the only other live thread is CoreAudio at
about 10% of a second core. **There is no idle capacity and no dominant function.** The largest single
item in either sample is 28%, and it moves with the phase.

Pace, taken from the `[frame-timing]` line the app prints on every retrace:

    retrace=415 us=181175276353
    retrace=419 us=181176055605      4 retraces / 779 ms = 5.13 fps on the title path

## The arithmetic that defines the target

The product path needs **13,910 retraces** to reach Outset. At the 60 Hz VI those frames are **3.9
minutes** of authored content.

* under 5 minutes means **46 fps sustained for the whole path**
* today, measured on the product path 2026-09-14: boot 9.19 fps, new-game intro **15.65 fps** over its
  13,077 frames, play scene at Outset **2.76 fps**, whole path 11.39 fps; time-to-playable **15.3 min**,
  which is 919.8 s from double-click to opening-complete. The intro is the longest stretch but the play
  scene is the slowest phase, and 2.76 fps matches the 3.10 fps bench median

So time-to-playable is a **throughput problem worth roughly 4x**, and it is the same problem whether the
intro can be skipped or not. Skipping the intro would remove 13,077 of the 13,910 frames and would meet
the target on its own, but the game does not offer a skip and inventing one in the port is a stub, which
the fences forbid.

There are two honest ways to move the number.

1. **Make the port faster** - parallelism plus hot-path work, below.
2. **Make the product fast path the default.** A human who already holds a save reaches Outset through
   Continue, not through a new game, and that path is short. This is a product decision rather than an
   emulation result, and it has to be demonstrated, not assumed.

## New falsifiable hypotheses, cheapest first

* **H-SAVE-CONTINUE.** The Continue entry on a card that holds a save boots at Outset, so a human who
  already has a save meets the five-minute target today with no emulation work. Falsified if that path to
  controllable Outset exceeds five minutes, needs an exported variable, or stops short of gameplay.
* **H-IDLE-SKIP.** The guest spends long stretches in wait loops that the host still executes instruction
  by instruction. The block trace repeats `pc=0x80307EF4` across millions of blocks while the visible
  retrace counter barely moves, which is what a device poll loop looks like. If that address is a poll
  loop, advancing the emulated clock to the next device deadline instead of executing the loop is a
  large, digest-preserving win. Falsified if emulated time advances through that stretch at the same rate
  as the block count.
* **H-DSP-THREAD.** The DSP is separate hardware. It costs 28% of the intro phase on the same thread the
  CPU uses. Moving it to its own thread turns that 28% into parallelism, so the intro phase should gain up
  to about 1.4x with the route digest and the paced-PCM fingerprint unchanged. Falsified if either moves.
  The adapter is **load-bearing, not a shadow** - decided 2026-09-14 by switching it off, whereupon the
  guest stalls in JAS before the title (title_ready=0, submitted=0, saved_pc=0x80307EAC). This cost can be
  parallelized but not deleted.
* **H-RENDER-OFFLOAD.** The aurora packet path allocates per draw: `build_topology_indices` reaches
  `operator new` and `free`, and `build_draw_plan` reaches `std::vector<float>::assign`, `memmove` and
  `memset`. That is 21% of the title phase spending part of its time in the allocator. Falsified if
  removing per-draw allocation in favour of reused scratch does not move fps on a title-path run.
* **H-GUEST-DISPATCH.** The remaining 50% or so is the recompiled corpus itself, with no function above a
  few percent. This is the long pole and it needs better per-instruction codegen, memory access and cycle
  accounting at the 256-cycle boundary, not a hot-spot fix. Measure before touching the recompiler.

## What a human can see right now

A human double-clicks **BlueWake.app** and the game boots with video and audio. Real keyboard presses,
delivered by the OS, drive it forward. Every screen renders correctly (v9 table, unchanged):

| screen | reached at retrace | press that caused it |
| --- | --- | --- |
| opening cutscene | 333 onwards | none, it is a movie |
| File Selection | 1324 | J at 1175 |
| quest-log detail panel | ~1900 | J at 1697 |
| Name Entry | ~3000 | J at 2945 |
| new-game legend intro | 773, ends 13850 | authored, not skippable |
| play scene, Outset | 13910 | follows the intro |
| Outset Island banner | 14402 | the fly-in |

File Selection takes two presses: the first selects the quest log and reveals Start, Copy, Erase and
Return, and the second takes Start. That is how the game is designed.

## Harness facts worth not rediscovering

* **The product path is the app bundle.** `open build/runtime-host-dsp/BlueWake.app` is what a double-click
  does. Instrumented: `open -n --stdout F --stderr F --env NAME=VALUE ... build/runtime-host-dsp/BlueWake.app`.
* **`open` reaps its child when the launching shell exits.** Keep the launching shell alive for the whole
  run, or the run dies mid-flight and looks like a crash.
* **One BlueWake process at a time.** `scripts/play.sh` refuses to overlap; honour that by hand too.
* **Frame rate, measured and not guessed.** `BLUEWAKE_FRAME_TIMING=1` prints `[frame-timing] retrace=N
  us=T` on every retrace, so fps is delta retrace over delta us. Do not quote a rate that did not come
  from this line or from `scripts/bench.sh`.
* **Where the time goes.** `sample $(pgrep -f 'BlueWake.app/Contents/MacOS/BlueWake') 6 -file /tmp/s.txt`
  and read the call graph. The emulation thread is `com.apple.main-thread` and the recompiled corpus
  symbolises as `func_XXXXXXXX`.
* **Delivering a real key.** `/tmp/bwfocus PID 400` immediately before every press, then
  `/tmp/bwkey 0 38 700`. A pid of 0 is a global HID post. Key codes: 38 is J (A), 40 is RETURN (START),
  K is B, U is X, I is Y. A press counts only when `[input-probe]` shows `pressed=1` with non-zero
  `kbFocus` **and** `[input-chain]` shows `cpad_trig=0x0100`. Do not gate on `msFocus`, which reads
  `0x0` on presses that land. Needs `BLUEWAKE_INPUT_PROBE=1`.
* **Frame evidence.** `BLUEWAKE_CAPTURE_OPENING_FRAME` with `BLUEWAKE_CAPTURE_RETRACE` reads back the
  guest framebuffer at an exact retrace, and `BLUEWAKE_CAPTURE_INTERVAL` yields a labelled series. Every
  capture stays in /tmp.
* **Synthetic pulses are route drivers, not keyboard evidence.** `BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1`
  arms a latched new-game sequence. `PULSE_RETRACE`, `PULSE2_RETRACE` and `PULSE3_RETRACE` all share the
  one button in `BLUEWAKE_PAD_BUTTONS`, and the name-END pulse is hard-coded to START.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path, or a human can now see,
hear or do something they could not before. A profile is a cost paid to find the work, never progress by
itself.

One hypothesis, one small change, one build, one measurement, and at most one line appended to
`docs/archive/status/CURRENT.md`. No status document may be longer than the change it describes. Host-only
rebuild: `cmake --build build/runtime-host-dsp --target bluewake_host -j 8`, about two minutes, which also
re-assembles and re-signs the bundle. Never add a new tier, slice or patch for a unit that already
compiles whole; new code must remove cost, not add diagnostics.

If an iteration ends with no change to an observable and no change to a number, it **failed**: revert it
and change the mechanism, not the constant. Three failures of the same shape means stop, measure the layer
underneath, and say so out loud. Report to the user every three iterations in one paragraph.

## The governing numbers

`scripts/bench.sh` runs the shipping configuration on the Outset route and reproduces the 2026-09-01
baseline.

| number | baseline |
| --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b |
| host turns | 820,034,526 |
| median fps | 3.10 fps on this host |
| p99 frame time | 512.35 ms (1.95 fps) on this host |
| peak RSS | 262,799,360 bytes on this host |
| **time-to-playable** | **about 20 min**, 13,910 retraces at roughly 11.6 fps; title path 5.13 fps measured 2026-09-14; target under 5 min, which is 46 fps sustained |

fps is host-dependent, while the digest, the turn count and the card are not. A/B only against numbers
measured on the host you are standing on. A change that moves fps but moves the digest is not a result,
it is a behavior change.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git,
  and every PPM and PNG from this work stays in /tmp. `ref/` is gitignored, so an edit made there ships
  as a patch under `patches/aurora/`, never as committed vendored source.
* Preserve user data and unrelated work. The save card is at
  `~/Library/Application Support/BlueWake/GZLE01.card`; drive the new-game flow against a copy, never
  against the save slot, and keep the /tmp backup current.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, and no captured emulator state as authentic initialization. **A synthetic button
  press is a route driver, not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass. Never write that no product gate advanced as a
  successful outcome; that sentence is the failure signature of this project.

## The work, in order

1. **H-SAVE-CONTINUE.** Does Continue reach controllable Outset in under five minutes on a card that
   holds a save? One run, one answer, and it decides whether the target is already met on the normal
   product path.
2. **Reach File Selection with a real key press, then photograph it,** and drive the rest of the menus by
   keyboard. This is the first clause of the report this loop owes, and it retires the synthetic route as
   the only way through the file menu.
3. **H-IDLE-SKIP.** Identify `pc=0x80307EF4` and decide whether the guest is waiting there on a device.
   It is the only place the trace shows millions of blocks with little progress.
4. **H-DSP-THREAD**, then **H-RENDER-OFFLOAD**. Both are parallelism and allocation work on a measured
   share, and neither may move the digest or the paced-PCM fingerprint.
5. **H-GUEST-DISPATCH.** The long pole. Measure per-instruction cost before touching the recompiler.
6. **M1c, then M2, M3 and H1/H2/H3** as recorded in v4: one press by a human hand on a double-clicked
   window; Outset median at least 15 fps then 30 fps with the digest unchanged; Link hair correct at the
   GX owner with a screenshot and no geometry regression; continuous intelligible audio; a physical
   controller including reconnect; a ten-minute unattended soak with bounded RSS; and one-command clean
   build and launch from a fresh clone with the private disc.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS START appears, and the
file menu comes up; the new-game intro costs X minutes and Y reaches Outset; here is the screenshot of
Outset with Link under your control.* Silence is not acceptable; neither is a document about why it
cannot be done.
