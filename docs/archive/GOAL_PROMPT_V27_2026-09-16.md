# BlueWake goal loop — v27 (2026-09-16)

**User-started. Finish and test the app.** The terminal condition is unchanged:

> From a double-click of BlueWake.app with no exported variables, a human
> reaches controllable Outset gameplay with correct video and audio, drives it
> with the keyboard without a crash, and time-to-playable is under five minutes,
> with the five governing product numbers holding on this host.

**This document supersedes docs/archive/GOAL_PROMPT_V25_2026-09-15.md.** v25 reported
v24's win and the two harness defects it exposed. Those are now closed and one
more run has been taken. This document reports where the product actually stands
and defines the only work left.

## The state: two clauses remain, and both are timing

v26b is the first run in this project to clear the awake cutscene on real keys
and reach control. Log /tmp/bw-acceptance/run-v26b-latch-fixed.log. It failed
exactly two clauses:

    acceptance: TTP-A launch to play-scene: 327.11 s, over the five minute target
    acceptance: TTP-B launch to interactive: 1206.22 s (control-admitted at retrace 20026)

Everything else passed in the same run, including the two clauses v25 left open:

- the control gate fired: control-admitted retrace=20026 event_mode=0 demo_type=0
  demo_mode=0 ovl=6;
- the player-ready capture came from the control-gated path (source=overlap-handler)
  and wrote two frames that are real renders, nonblank 2,342,451 and 2,731,346 of
  1,920x1,440 — 0.847 and 0.988 of the frame, against v24's 0.0012;
- the displacement clause holds: Link walks under a real held W inside the guest's
  own control tuple, position C83ED280 to C83ED942 across retraces 20019 to 20033
  with the decoded stick at y=3F800000;
- normal stop at pc=0x80307ef4, gx-core rejected=0 failed=0, dsp-lle
  first_nonzero=1, 344 real key presses landed, user's save slot byte-identical.

So the product half of the objective is met on this host, and the remaining
requirement is the objective's own numeric one: **time-to-playable under five
minutes.**

## Why the last three runs died, and what that cost the loop

It is worth recording because it was misdiagnosed three times as machine
interference. This runtime was dropping short key presses. PADRead samples the
keyboard with SDL_GetKeyboardState once per guest frame, while Aurora drains the
whole SDL queue once per rendered frame, so on a slow frame a press and its
release are drained together and the sampled state ends already released. The
awake cutscene renders at 7-9 fps, one sample every 110-136 ms, so a normal tap
is exactly the shape that is lost. Measured directly: a 40 ms J press produced no
sample at all; a 400 ms press was seen cleanly.

The fix is patches/recompcore/0049-aurora-latch-keyboard-presses-for-the-guest-pad.patch.
window.cpp hands every SDL key event to the pad layer, pad.cpp holds a key-down
until a pad read has observed it, and the sample is still consulted so a held key
behaves as before. The same 40 ms press now lands as port0_button=0x0100.

Read this as a general lesson for the next loop: three runs were spent treating a
product defect as harness noise. When a driver fails, the app's own view of the
input is a separate instrument from the guest's, and the two disagreeing is
information, not noise.

## The work left, in order

1. **TTP-B first, because it is the far number.** TTP-B spans launch to the first
   moment a human can act, which on a cold new game is the whole intro plus the
   cutscene drive: 1,206 s at retrace 20,026. TTP-A, launch to the play scene, is
   327 s and load-sensitive — the same clause read 285.82 s on an idle host
   minutes earlier — so treat TTP-A as a number to hold, not the target.
2. **Speed, not skipping — the PRD forbids the fast-start this loop first
   proposed.** A save-state taken at control and restored on launch would
   collapse TTP-B, and an earlier draft of this document said to build it. Do
   not. The PRD makes it out of scope in four separate places: definition of
   done 3 requires the platforms to "boot normally without runtime PPC JIT or
   captured initialization state"; gate P4 passes only "without captured state,
   forced scene construction, or runtime PowerPC JIT"; section 2 forbids
   reaching behavior that is "not a renderer demo, synthetic scene,
   memory-seeded snapshot"; and section 9.3 explicitly refuses "snapshot warm
   start". The PRD's own speed requirement is G1 and definition of done 9:
   "sustains original 30 FPS game speed".
3. **So the five minute clause is a throughput clause, and that is the work.**
   At authentic 60 retraces per second the intro's 13,850 retraces take 231 s,
   which is inside the bar — this is what V12 meant by "target under 5 min,
   which is 46 fps sustained over 13,850 retraces". Measured on v26b the same
   intro ran at 42.8 fps, about 71% of authentic speed. Work the overhead and
   the guest path until the whole route runs at original speed; do not work
   around the route.
4. **Price the per-turn cost first.** See the profile in the status log: the
   dispatch-overhead frames are roughly a quarter of the main thread. H-TURN-COST
   as a number, then H-DSP-THREAD, then H-CORPUS. The Outset cutscene at 6.9 fps
   is the largest single authenticity failure on the route and should be the
   benchmark scene, because it is 11% of original speed.

## Hypotheses this loop owns

- H-COLD-BOOT-FLOOR: TTP-A is bounded by work the intro does regardless of host,
  and the honest number is the idle-host one. Measure it idle, report it with the
  host's own retrace rate beside it, and never report a loaded-host figure as the
  product number.
- H-FAST-START (rejected): a save-state launch would move TTP-B by an order of
  magnitude, and it is prohibited by the PRD. Recorded so the idea is not
  re-proposed.
- H-TURN-COST (priced, and the wall-time half is unresolved): raising the
  dispatch granularity cuts the host turn count exactly and reproducibly -
  521,121,740 at the dynamic cap, 459,027,099 at 2048, 433,268,760 at 4096 - but
  six bench runs show the wall clock does not move outside this host's noise
  band, about 7 points of real-speed ratio. An earlier reading of "+6.1% on the
  intro" from one pair of samples was inside that band and should not be quoted.
  The digest was UNCHANGED at every cap, so the cap stays a free variable, but it
  is not a lever for the metric. See the status log for the table.
- H-DSP-THREAD (next, and the one with enough headroom): the intro profile puts
  the LLE DSP interpreter at 41% of the intro's main thread, reached through the
  per-turn device sync, so getting it off the main thread is worth up to about
  1.7x on the segment the metric is read from. The five-minute bar needs this
  route, 235 s of guest time, at about 78% of authentic speed; the build is at
  55.6-60.6% depending on host load, so the gap is about 1.3x to 1.4x.
- H-INTERRUPT-CHURN (applied): host_mmio_read and host_mmio_write both call
  bluewake_cycle_domain_observe first, and observe already recomputes the dispatch
  budget through the deadline function; the device sync that follows refreshed the
  interrupt lines and derived the deadline a second time. The fix publishes only
  when the three-boolean source set actually changes and rebuilds the budget only
  then. Deadline derivations fell from 1,322,711,331 to 615,959,770 (-53.4%) with
  the route digest UNCHANGED across 1,050 records and host turns moving by one
  part in a hundred thousand. The wall-clock effect is about 2% by the profile and
  is below this host's noise floor, so the change is kept because it is strictly
  less work, not because it is a measured speedup.
- H-CENSUS-AS-METRIC (the instrument this loop should have been using): every fps
  comparison here fights a ~7-point host spread, while BLUEWAKE_DEADLINE_CENSUS
  counts calls deterministically. Where the thing being removed is host-side work,
  the census is the evidence and fps is corroboration at best.
- H-GUEST-DISPATCH (the long pole, needs a number): V12 measured guest dispatch at
  58% of the intro and 62% of the play scene and called it a recompiler problem.
  The route is 235.00 s of guest time for 14,100 retraces, 8.1M guest cycles per
  retrace, so authentic speed is 486M guest cycles per second; at 55.6-62.7% the
  runtime executes about 300M, which on a ~3.5 GHz core is about 11 host cycles
  per guest cycle. That is the number to move before touching the recompiler.
- H-GX-FLUSH (falsified): the guess was that the guest's per-write GX frontend
  flush is fixed per-call overhead and should be batched to a boundary. Built and
  measured, it is a 19% regression - 6.78 fps against 5.72 fps on the same 222
  cutscene retraces - so the flush is load-bearing incremental work, not waste.
  Reverted. Do not re-propose it.
- H-INPUT-LATCH (now retired): the runtime no longer loses a press shorter than a
  frame. If a drop reappears, check the latch before blaming the host.

## The A/B protocol, since it is easy to skip

scripts/bench.sh is the admission test and it is cheap: a headless route to
14,100 retraces in about six minutes. It reports the five governing numbers -
median fps, p99 frame time, wall versus guest seconds, peak RSS, and the route
digest - and only the digest and the host turn count are host-independent. A
change that buys fps but moves the digest is a behaviour change and is not a
result; believe a change only when the digest reads UNCHANGED and the turn count
and RSS have not moved. Do not A/B fps measured while something else runs on the
host: the same build read 60.6% on a quiet host and 55.6% with profiling beside
it. The noise floor is about 7 points of real-speed ratio, so a single pair of
readings cannot resolve anything smaller than that.

## Falsification on record

- The claim that the last three cutscene failures were host interference is
  false; they were this runtime dropping sub-frame presses, proven by a 40 ms
  press producing no sample while a 400 ms press was seen.
- The claim that the player-ready frame could not be a real picture is retired;
  two real renders were written in one run, from the control-gated path.
- The claim that the displacement clause cannot pass is retired; Link walks
  under a real held W in the guest's own control tuple.

## What not to do

- Do not weaken TTP-A or TTP-B to make a log pass, and do not let a save-state
  run quietly answer the cold-boot clause. The marker must always be named.
- Do not drive the name walk, the intro, or the awake cutscene with synthetic
  pad pulses; a synthetic press is a route driver, not evidence the keyboard
  works.
- Do not stop feeding A while the cutscene or the post-cutscene sequence is up.
- Do not read a run as a verdict while a competing UI test or automation is on
  the host; that is what voided two v25 launches, and it inflates the wall-clock
  numbers TTP is read from.

## The iteration contract

An iteration counts only if time-to-playable fell, fps moved on a named path, or
a human can see, hear, or do something new. A document is not an iteration, and
neither is a probe. Report one paragraph to the user every three iterations.

## Fences, non-negotiable

Never commit or publish Nintendo data, original binaries, generated game code,
saves, captures, device data, signing material, or leaked source. The card walk
uses a copy at /tmp/bw-acceptance/save.card (from run1.card, sha256 6b43aabd) and
the user's own slot (sha256 b0163d86) must return byte-identical. PPM and PNG stay
in /tmp. Edits under ref/ ship as patches: patches/aurora applies to ref/aurora,
patches/recompcore to ref/recompcore, and every patch must be registered in
config/dependencies.lock.json. One BlueWake process at a time. Do not read a live
run as a verdict, and do no disk-heavy work while a run is live.

## Appendix — constants this loop relies on

Keyboard to pad: arrows are the D-pad, J is A, K is B, U is X, I is Y, W/A/S/D
are the left stick, H/F/T/G the C-stick, E/R are L/R, Q is Z, RETURN is START.
SDL scancodes: J 13, W 26, RETURN 40, LEFT 80. Physical keycodes: 38 J, 36 RETURN,
13 W, 123 left arrow. Wire A 0x0100, START 0x1000; guest cpad at 0x803A4E20 has
A 0x0100 and START 0x0010.

Guest records: player pointer 0x803CA74C; demo_type +0x304, demo_mode +0x314,
proc +0x31D8, position +0x1F8/+0x1FC/+0x200; event_mode 0x803C9EA2, event
0x803C9EB8, msg 0x803CA7D2; pad pointer 0x803A4DE0 (word +0x18); decoded stick
0x803A4DF0/+4/+8; overlap object 0x803F6160, live when >= 0x80000000 and +0x04
== 1, phase +0x1C.

Two of those are no longer guesses, because the matching decompilation for this
same revision names them. The local zeldaret/tww checkout at
~/GitHub/bluewake-tww-latest-census has, at include/d/actor/d_a_player.h:499,
"/* 0x304 */ daPy_demo_c mDemo;", with getDemoType() returning mDemoType and
getDemoMode() returning mDemoMode, a u32 exactly as the probe reads it. So
demo_type +0x304 is daPy_demo_c::mDemoType and demo_mode +0x314 is its mDemoMode.
Its Type_e enum names our observed values - 0 TYPE_NONE_e, 1 TYPE_TOOL_e,
2 TYPE_SYSTEM_e - and its Mode_e names the control value DEMO_UNK00_e = 0.
The gate this loop derived empirically is the game's own test:
d_a_player_main.h:1913 is "checkPlayerDemoMode() { return mDemo.getDemoType() !=
daPy_demo_c::TYPE_NONE_e; }", and our control tuple is its negation. The other
offsets in this table should be cross-referenced the same way; they are all named
fields or class members in that checkout.

The run environment: card copy /tmp/bw-acceptance/save.card, ceiling 24,000
retraces, and the harness scripts/app_acceptance_test.sh launched as the
foreground process of a persistent session. The app bundle is
build/runtime-host-dsp/BlueWake.app; build the target bluewake_host, never bare
all (tests/delivery_digest_test.c has a pre-existing unrelated break).
