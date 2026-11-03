# BlueWake goal loop - v11 (2026-09-14)

Supersedes v10 ([GOAL_PROMPT_V10_2026-09-14.md](GOAL_PROMPT_V10_2026-09-14.md)). The PRD still defines what
finished means. V10 replaced v9's "the intro is the wall" with the measurement that sets the budget: the host
runs the whole machine on one CPU thread at about a quarter of the speed the target needs, with no single
function above 28%. V11 accepts that, and then points out where the loop has been looking.

## The claim this loop is built on

Every profile this project has taken is of the title path or the new-game intro. Both are **pre-play**. The
gate the objective actually names - *controllable Outset gameplay* - is the **play scene**, and the play scene
is the slowest phase measured, 2.76 fps against the intro's 15.65 fps. It is the only phase nobody has
sampled.

So the loop's unit of progress is the play scene, and the first act of this loop is to look at it.

## What v10 got right, and what it never looked at

Right, and kept: the cost is spread across the whole machine; the emulation thread sits at 97-100% of one
core with CoreAudio as the only other live thread; the largest single item anywhere is 28%, and it moves with
the phase. Right: the intro runs at 15.65 fps, not the 5-6 fps an earlier estimate used.

Never looked at: the two samples behind 71/21/5 and 58/28 are the title path and the intro. The 21% item in
the first is `func_803256E0`, the guest GX FIFO writer, feeding the aurora packet path. The play scene is
the render-heavy phase - real 3D sea room, terrain and models instead of menus - and it may be dominated by
exactly that path, or by guest code, or by a device spin. The loop has four hypotheses about the play scene
and no measurement of it. That is the gap this loop closes first.

## Retired by measurement, do not reopen

* **H-SAVE-CONTINUE - killed.** The route card is byte-identical to the user card except 14 header bytes, so
  the route already runs the new-game flow on a card that strings reports as holding save data. There is no
  distinct Outset save to Continue into. V10 listed this as work item 1; it is answered.
* **H-DSP-SHADOW - falsified.** With the LLE DSP adapter off the guest stalls in JAS before the title:
  `title_ready=0`, every milestone zero, `saved_pc=0x80307EAC` waiting on a DSP interrupt that never comes.
  The adapter is load-bearing. Its cost can be parallelized, not deleted.
* **H-INTRO-HOLD - falsified.** A real OS-delivered four-second held START did not end the intro.

## The measurement that sets this loop, and how to take it for free

The gate is play-scene fps, and the profile that explains it is one `sample` of the emulation thread taken
while the guest is in the play scene.

The bench run **is** the play-scene route: its live window is retraces 13910:14100, which is the play scene,
and it runs one `bluewake_host` process. So the profile is taken on that process during that window, with no
second BlueWake process and no violation of the one-at-a-time rule.

    sample $(pgrep -f 'bluewake_host') 6 -file /tmp/bw-playscene-sample.txt

Read the call graph as a decision, not as a status:

| if the play scene is dominated by | then the hypothesis is |
| --- | --- |
| `func_803256E0` and the aurora packet path | **H-RENDER-OFFLOAD** - reused scratch instead of per-draw `assign`/`operator new` |
| `host_sync_cycle_devices_end_turn` reaching `DSP::Interpreter::Step` | **H-DSP-THREAD** - the DSP on its own thread |
| dispatch into guest code, with one `pc` repeating | **H-IDLE-SKIP** if a single `pc` dominates, else **H-GUEST-DISPATCH** |
| everything, like the title path | the same ~4x throughput problem, and say so out loud |

**H-RENDER-OFFLOAD leads on existing evidence.** It is already 21% of the title path, where the guest is
drawing menus; the play scene draws the world. It is a host-only change, so the rebuild is
`cmake --build build/runtime-host-dsp --target bluewake_host -j 8`, and it must not move the digest.

## Pace, measured on the product path 2026-09-14

    boot            1 -> 773        772 frames     84.0 s    9.19 fps
    new-game intro  773 -> 13850  13077 frames    835.8 s   15.65 fps
    play scene    13910 -> 14992   1082 frames    392.3 s    2.76 fps   <- the gate
    whole path      1 -> 14992    14991 frames   1316.1 s   11.39 fps

Time-to-playable 919.8 s, 15.3 min, double-click to `opening-complete`. The product path needs 13,910
retraces, 3.9 minutes of authored content, so under five minutes means 46 fps sustained. The play scene is
where the number is being lost, and it is also the thing the objective asks for by name.

## What a human can see right now, and the honest gap

A human double-clicks **BlueWake.app** and the game boots with video and audio, reaches File Selection, Name
Entry and the new-game intro, and arrives at Outset Island with the island banner over rendered rope bridges,
water, sand and cliffs. Evidence: the retrace-14400 readback in /tmp.

The gap, unchanged and still owed: there is no frame yet of **Link under the control of a real key press on
Outset**. The 14400 capture is the fly-in banner, and the route through the file menu used the synthetic pad
pulse. A synthetic pulse is a route driver, not keyboard evidence. Retiring that gap is work item 3 below,
and it is the first clause of the report this loop owes.

## Harness facts worth not rediscovering

* **Launch.** `open build/runtime-host-dsp/BlueWake.app` is what a double-click does; instrumented is
  `open -n --stdout F --stderr F --env NAME=VALUE ... build/runtime-host-dsp/BlueWake.app`. `open` reaps its
  child when the launching shell exits, so keep the launching shell alive for the whole run.
* **One BlueWake process at a time.** `scripts/bench.sh` refuses to overlap with `bluewake_host`; honour that
  by hand too.
* **A real key press.** `/tmp/bwfocus PID 400` immediately before every press, then `/tmp/bwkey PID <keycode>
  <hold_ms>`. Key codes (macOS virtual keycodes, read by SDL): **38 is J (A), 36 is RETURN (START), 40 is K (B),
  32 is U (X), 34 is I (Y), 16 is Y, 6 is Z**. 40 is
  B, not START - v9 and v10 both carry that error. A press counts only when `[input-probe]` shows non-zero
  `kbFocus` and `[input-chain]` shows the expected `cpad_trig`. Do not gate on `msFocus`, which reads 0x0
  on presses that land.
* **Rate, measured not guessed.** `BLUEWAKE_FRAME_TIMING=1` prints `[frame-timing] retrace=N us=T` on every
  retrace. Do not quote a rate that did not come from that line or from `scripts/bench.sh`.
* **Frame evidence.** `BLUEWAKE_CAPTURE_OPENING_FRAME` with `BLUEWAKE_CAPTURE_RETRACE` reads back the guest
  framebuffer at an exact retrace, and `BLUEWAKE_CAPTURE_INTERVAL` yields a labelled series. Every PPM and PNG
  stays in /tmp.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path, or a human can now see, hear
or do something they could not before. A profile is a cost paid to find the work, never progress by itself.

One hypothesis, one small change, one build, one measurement, and at most one short section appended to
`docs/archive/status/CURRENT.md`. No status document may be longer than the change it describes. Never add a new
tier, slice or patch for a unit that already compiles whole.

If an iteration ends with no change to an observable and no change to a number it **failed**: revert it and
change the mechanism, not the constant. Three failures of the same shape means stop, measure the layer
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
| **time-to-playable** | **15.3 min measured 2026-09-14**, 919.8 s; target under 5 min, 46 fps sustained |

The digest, the turn count and the card are host-independent; fps is not. A/B only against numbers measured
on the host you are standing on. A change that buys fps by moving the digest is a behavior change, not a
result.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git, every
  PPM and PNG stays in /tmp, and `ref/` is gitignored so an edit there ships as a patch under `patches/aurora/`.
* Preserve user data. The save card is at `~/Library/Application Support/BlueWake/GZLE01.card`; drive the
  new-game flow against a copy and keep the /tmp backup current.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. **A synthetic button press is a
  route driver, not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass. Never write that no product gate advanced as a
  successful outcome; that sentence is the failure signature of this project.

## The work, in order

1. **Profile the play scene**, from the running bench process during its 13910:14100 window, and turn the
   result into the one hypothesis in the table above. This decides everything after it.
2. **One change, one build, one measurement** on that hypothesis, against a host-measured A/B with the digest
   holding.
3. **Reach File Selection and the file menu with a real OS-delivered key press, then photograph it**, and
   drive the rest of the route by keyboard. Retires the synthetic route as the only way through the menu, and
   is the first clause of the report.
4. **H-DSP-THREAD**, then the render path if step 1 pointed there. Parallelism and allocation work on a
   measured share; neither may move the digest or the paced-PCM fingerprint.
5. **H-GUEST-DISPATCH / H-IDLE-SKIP.** The long pole. Measure per-instruction cost before touching the
   recompiler.
6. **M1c, then M2, M3 and H1/H2/H3** as recorded in v4: one press by a human hand on a double-clicked window;
   Outset median at least 15 fps then 30 fps with the digest unchanged; Link hair correct at the GX owner with
   a screenshot and no geometry regression; continuous intelligible audio; a physical controller including
   reconnect; a ten-minute unattended soak with bounded RSS; and one-command clean build and launch from a
   fresh clone with the private disc.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS START appears, and the file
menu comes up; the new-game intro costs X minutes and Y reaches Outset; here is the screenshot of Outset with
Link under your control.* Silence is not acceptable; neither is a document about why it cannot be done.
