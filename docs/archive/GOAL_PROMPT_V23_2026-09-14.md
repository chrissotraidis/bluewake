# BlueWake goal loop — v23 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:

> From a double-click of `BlueWake.app` with no exported variables, a human
> reaches controllable Outset gameplay with correct video and audio, drives it
> with the keyboard without a crash, and time-to-playable is under five minutes,
> with the five governing product numbers holding on this host.

**This document supersedes `docs/archive/GOAL_PROMPT_V22_2026-09-14.md`.** v22's own
launch was void (below); the measurement run it predicted is the one this
document now reports, **v23**, and v23 has a verdict.

Run labels. v20 was retired deliberately, after it had already emitted its
complete verdict and was then sitting in a frozen cutscene. v21 was stopped
early by its loop once the probe-latch defect was proved from its own log. v22
was **void**: its harness died seconds after it launched the app and before its
first key press, so the app sat unattended in the title screen's attract demo
for 21 minutes with no input ever delivered — the absence of `control-admitted`
in `run-v22-void.log` measures the dead harness, not the guest. All three runs'
artefacts are kept (`run-v20-final.log`, `run-v21-final.log`,
`run-v22-void.log`, `acceptance-v20.recorded.txt`, `acceptance-v21.recorded.txt`,
`v21-overlap-evidence.txt`). The run this document reports is **v23**, launched
as the foreground process of a persistent session so the script outlives the
shell that starts it.

## v23's verdict: two FAIL, and the first product gate ever to fire

`/tmp/bw-acceptance/acceptance-v23.out`, 523 lines; log
`/tmp/bw-acceptance/run.log`, 38,033 lines; app pid 54811; ceiling 24,000
retraces.

**Passed for real**, in order: title-ready at retrace 333 → file-select 537 →
name-input-complete 785 → new-game-intro 847 → play-scene 13,984. **The
`control-admitted` gate fired — the first time in any run of this project:**

```
[player-milestone] control-admitted retrace=20164 event_mode=0 demo_type=0 demo_mode=0 ovl=6 pos=C83ED280,44CE4000,48992880
```

Then: 492 real key presses landed in the guest's own controller record; the
player-ready frame was written (`/tmp/bw-acceptance/frames-game-225710/outset`,
1920x1440, nonblank 2,130,859 bytes); `[run] stopped: normal after 3795835022
blocks at pc=0x80307ef4`; `gx-core ... rejected=0 failed=0`; `dsp-lle ...
first_nonzero=1`; the user's own save slot byte-identical.

**Failed (2).** The fix for both is written, and v24 is the run that tests it:

```
acceptance: FAIL a real 6 s W hold left the guest's own player position unchanged
acceptance: FAIL time to playable is 495.93 s, over the five minute target
```

The second is now split into two clauses, TTP-A and TTP-B (below). The first is
the clause three loops could not close, and v23 is the run that closed the
question — not by passing it, but by naming who was at fault.

## FAIL #1, corrected: the driver stopped driving, then compared a pinned frame with itself

The prior handoff's reading — "mode-6 does not move the player, and the stick
reads all zero" — was half right and half wrong, and the wrong half is the
important one.

**Right:** the position did not move, and the record the clause measured carried
`demo_type=2 demo_mode=6`.

**Wrong:** the stick did not read all zero. Retrace 20,194, inside the hold,
reads

```
[player-scene-state] ... stick=00000000,3F800000,3F800000 pad_hold=0x08000000
```

so the real W **did** reach the guest's own decoded steering value. The clause's
`pos_before` and `pos_after` reads simply took two different records **from
inside one and the same `demo_type=2 demo_mode=6` sequence**, so a pinned
position was compared with itself while the key was arriving. A pinned position
was compared with itself twice, and the line that proves the key arrived is
neither of the two lines the clause compared.

**The guest's own state timeline**, from 1,199 `[player-scene-state]` lines:

| retraces | demo_type / demo_mode / proc / event_mode | what it is |
| --- | --- | --- |
| 0–294 | 0/0/0/0 | title |
| 336–450 | 1/4/4/2 | attract demo |
| 450–13,997 | 0/0/0/0 | idle through the intro |
| 13,997–14,005 | 0/0/4/0 | play scene |
| 14,005–19,659 | 1/512/169/2 | authored Outset awake cutscene |
| 19,659–19,927 | 1/4/4/2 | cutscene pages |
| 19,927–19,933 | 1/1/4/2 | cutscene tail |
| 19,933–19,935 | 0/0/4/0 | **control window A** (2 retraces) |
| 19,935–20,043 | 2/6/170/1 | post-cutscene message sequence (108) |
| 20,043–20,049 | 0/0/4/0 | **control window B** (6) |
| 20,049–20,157 | 2/6/170/1 | sequence (108) |
| 20,157–20,173 | 0/0/4/0 | **control window C** (16) — carries the 8-retrace hold at 20,159 |
| 20,173–23,990 | 2/6/170/1 | sequence re-arms, position pinned to the end |

The sequence re-arms **9 retraces after `control-admitted`**, because the driver
stopped feeding A the instant that milestone fired. `msg` walked 16→17→18→19
while A flowed (retraces 20,119–20,155) and then **held at 16 for fifty-seven
seconds** once A stopped. So the sequence genuinely needs A, and it cannot
simply be abandoned: the cutscene does not advance itself.

### The decisive cross-reference: the route already moved Link through these exact windows

The route's own oracle, from `docs/archive/GOAL_PROMPT_V19_2026-09-14.md` (lines 45–70):

```
[player-control-admission] moved=1 start=C83ED280,44CE4000,48992880 final=C83EE1CE,44CE4000,4899151C trigger_retrace=19972 final_retrace=20153
```

That `start` position is **exactly** v23's frozen position — the same words the
`control-admitted` line above carries — and its `trigger_retrace=19972 →
final_retrace=20153` range **coincides with v23's own sequence windows**. The
oracle's control lines read `event_mode=0 demo_type=0 demo_mode=0
stick=00000000,3F800000 stick_value=3F800000`, with the position advancing
`C83EE129 → C83EE127` across the same span.

So the route held the stick **through** the same sequence windows and got
displacement; v23's driver, which stopped A at admission and then pressed W for a
fixed 6 s, did not. The measured Euclidean distance between the oracle's start
and final position is **166.77 world units** — the displacement clause's bar is
4.0, so the oracle clears it forty times over. Position changes elsewhere in the
v23 log are only the cutscene's idle micro-bob (y toggling `44CE4000 ↔ 44CE7000`,
a raw-word delta of thousands that is **1.5 units** as a float — deliberately
below the 4.0 bar precisely so that bob cannot pass the clause).

**The product lever is the driver, not the guest.** This is not a reason to
loosen the assertion and it is not a reason to touch `main.c`.

## The fix, built (script-side, `scripts/app_acceptance_test.sh`)

Nothing in `runtime/host/src/main.c` changes for this clause. The probe is kept
exactly as v23 ran it, because v23 proved it right.

* **Real, live readers.** `player_select()` filters the `[player-scene-state]`
  stream for the control tuple by an optional argument — `control` selects
  `demo_type=0 demo_mode=0` with `event_mode=0`, a live player pointer and a
  nonzero position. `player_field`, `player_stick` and `player_pos` take an
  optional `control|any` selector. `player_select` opens with `tail -1` before
  its greps, which is required because the probe only prints **on change**.
* **`in_control()` is live, not a latch.** It compares the newest record with
  the newest control-tuple record, so as soon as any other tuple prints, the two
  differ and the answer flips back. It cannot answer yes forever after one
  control line has ever been printed.
* **`pos_units()` measures units, not words.** The three position words are
  IEEE-754 big-endian floats, so a raw word delta is not a distance: computed in
  Python, the idle bob is 1.5 units and the oracle's real move is 166.77.
* **`measure_stick_displacement()` replaces the fixed 6 s window.** It holds W
  with `stick_hold_keep` and taps A **only while the guest's own record is not
  in the control tuple**, then watches the guest's record inside each control
  window for ≥4.0 units of displacement. On failure it reports which of the two
  hypotheses the log supports — **H-PLAYER-CONSUMES-STICK** (is there a
  `stick=...,3F800000,3F800000` line anywhere) versus **H-KEYBOARD-NEVER-
  ARRIVED** — and prints the last `any` and last `control` records. The old
  `sleep 3` and `sleep 4` are gone, so nothing sleeps over the seam.
* **`stick_hold_keep` / `stick_hold_stop`** re-arm the background W hold each
  iteration and stop it before the RETURN/arrow clause, so a killed key-down
  cannot leave the guest's stick stuck on.

## The timing clause, split: TTP-A and TTP-B

Both read the host's own first `[frame-timing]` stamp as the origin, through the
new `us_to_retrace()` helper (which reproduces v23's reported 495.93 s exactly
from retrace 13,984 — checked). The unused `launch_epoch` is deleted; the
script's own wall clock is not the product's clock.

* **TTP-A — launch to play-scene ≤ 300 s.** The cold-boot cost of walking the
  new-game intro by hand. **Expected to stay red**, and it is the measurement the
  save-state work has to move. v23: **495.93 s** (play-scene at retrace 13,984).
* **TTP-B — launch to interactive ≤ 300 s.** Interactive is the play-scene
  milestone, **or** the first `control-admitted`/player-ready capture when a run
  starts from a save state whose intro is already cleared. The earliest marker
  that exists answers, and the clause **always names which marker answered**, so
  a save-state run can never quietly pass itself off as the cold-boot number.
  In a cold run TTP-B resolves to the play scene and therefore equals TTP-A; in
  v23's cold log the `control-admitted` marker sits at retrace 20,164, i.e.
  **1,627.87 s** — the intro plus the awake cutscene on top of the play scene.
  Both clauses are reported separately and are never silently substituted.

Background: a double-click launch includes a cold object cache. Retraces
537→847 (`new-game-intro`) cost 3,931,313,660 blocks for 310 retraces (~101
retraces/s) against ~300 retraces/s later, so the launch prefix is
**block-bound**, not trace-I/O-bound. v20's 356.96 s and v21's 464.55 s were
traced numbers; H-TTP-TRACE-TAX is still unpaid.

## Hypotheses this loop owns

* **H-DRIVER-STOPS-AT-ADMISSION** — new, and it is the finding above. The gate
  firing was treated as the finish line by the driver, but it is the *start* line
  for the very sequence that consumes the stick. A driver that stops driving the
  moment a gate opens measures the gate, not the gameplay. Retiring this
  hypothesis means the displacement clause passes **inside** a control window
  with A still flowing.
* **H-PLAYER-CONSUMES-STICK** — retired as a *root cause* and kept as a
  *discriminator*. v23's own log contains `stick=y 3F800000`, so the key reached
  the guest's steering value; the clause now says so in exactly those words
  instead of calling it a keyboard failure.
* **H-TTP-SAVE-STATE** — owns TTP-A. The product claim is a *double-click*, and
  the cold path is bound to replay the intro. A save state whose intro is
  cleared reaches control without replaying it, so TTP-B (measured from the
  control marker) is the number that can meet 300 s while TTP-A stays the honest
  cold-boot cost. This is a **product feature** to build, not a test to relabel;
  if it lands, TTP-B falls and TTP-A stays put, which is the whole point of
  splitting them.
* **H-TTP-TRACE-TAX** — carried and still unpaid. The product claim is the
  double-click with **no** exported variables; every timing on record was taken
  with `BLUEWAKE_FRAME_TIMING`, `BLUEWAKE_INPUT_PROBE`, `BLUEWAKE_TRACE_PLAYER`,
  `BLUEWAKE_TRACE_PAD` and `BLUEWAKE_TRACE_ROOM0` set. The clause is owed a clean
  untraced run, with traced numbers reported as traced numbers.
* **H-LOAD** — carried. Every timing here is indicative, not controlled, until a
  clean re-measure on an idle host.
* **H-TURN-COST**, **H-DSP-THREAD**, **H-CORPUS** — carried from v17/v18/v19/
  v20, still unpaid, to be priced as numbers first and changed second, in v17's
  order.

## Falsification on record

* **H-PROBE-LATCH-REACHES-6** — **settled by v23.** The v22 fix (gate reads
  `g_overlap_terminal_phase`, the handler-instant latch the route already trusts)
  worked: the gate emitted for the first time, with `ovl=6 event_mode=0
  demo_type=0 demo_mode=0`. The old probe read is kept and printed as `ovl_live=`
  — v23's line reads `ovl_live=0`, exactly as documented, and it is not the gate.
* **H-KEYBOARD-NEVER-ARRIVED** — falsified positively and repeatedly; v23's log
  carries `stick=00000000,3F800000,3F800000` under a held, real W.
* **H-GUEST-IGNORES-STICK-IN-MODE-6** — retired. It rested on comparing two
  records *inside* the mode-6 sequence; the mode-6 windows carry control tuples
  (windows A, B, C) and the route moved Link through them.
* **H-GATE-STATE** (event_mode 0 && demo_mode 0 is sufficient) — still
  falsified (v20: fired at 13,985, in the seam).
* **H-SHORT-CEILING** — still falsified; v23 stopped normally at pc=0x80307ef4
  after 3,795,835,022 blocks.

## The work, in order

1. **Launch v24 and read it out.** Do not read it live as a verdict; wait for the
   ceiling stop. One BlueWake process at a time.
2. **Read the verdict**, and write it down with the line that carries it.
3. **If the displacement clause passes**, append one short section to
   `docs/archive/status/CURRENT.md` and send the report this loop owes.
4. **TTP-A / TTP-B**: keep both red or both green on their own merits, report
   both, and never silently substitute. TTP-A stays the cold-boot number.
5. **Then performance**, in v17's order: **H-TURN-COST priced as a number before
   any build flag moves**, then H-DSP-THREAD, then H-CORPUS.
6. **Owed residue:** the "one press moves Link N units" clause (`pos_units` now
   makes it nearly free); `scripts/bench_report.py:87` `window_metrics`
   off-by-one (`KeyError: 14097`); `scripts/bench.sh`'s overlap guard, which greps
   only `bluewake_host` and so misses the bundle process `MacOS/BlueWake`; and
   the duplicate player-ready capture path near the `0x80122D30` hook
   (`demo_type==1 && demo_mode==4`). The `[event-control-*]` and `[room0-*]`
   hooks are confirmed **dead paths** — zero lines in v23's log — candidates for
   deletion.

## What not to do

* **Do not loosen the displacement assertion**, and do not touch `main.c` for it.
  The guest is not the fault; the driver is.
* **Do not "fix" the probe's live read by loosening its liveness test.** It
  observes what it observes; the gate's reading is the handler-instant one and
  both are printed.
* **Do not stop driving A while the cutscene or the post-cutscene sequence is
  up.** Neither advances itself. Stopping at `control-admitted` is precisely
  what pinned the position in v23.
* **Do not substitute a synthetic stick**, and do not accept `demo_mode 512/4`
  as control. A synthetic button press is a route driver, not evidence that the
  keyboard works.
* **Do not build the bare `all` target** — `tests/delivery_digest_test.c` has a
  pre-existing, unrelated break. Build `bluewake_host` only:
  `cmake --build build/runtime-host-dsp --target bluewake_host`.
* **One BlueWake process at a time.** The acceptance script refuses to overlap
  and its guard matches the bundle path.
* **Do not read a run that is still live as a verdict.** Wait for the ceiling
  stop.
* **Do not quote a traced timing as the product's**, and do not delete a trace
  to make a timing clause pass.
* **Do not report v21's or v22's FAIL clauses as product failures.** They are
  instrument/harness artefacts and are void; the runs are kept so this can be
  checked.
* **Never write that no product gate advanced as a successful outcome.**

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an iteration**,
and neither is a probe: a probe is a cost paid to find the work. Report to the
user every three iterations in one paragraph. Silence is not acceptable; neither
is a document about why it cannot be done.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source. The
  user-owned GZLE01 card `b0163d86...` stays private and out of git; the walk
  runs against a **copy** and must return byte-identical. Every PPM and PNG stays
  in `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* No disk-heavy work — repo-wide scans, `lldb`, `atos`, `otool` — while a
  measurement run is live. Any guard has to match the bundle path too.
* No silent stubs, no no-op substitutes, no weakening or deleting a test or a
  digest because it exposes a failure.
* Never write that no product gate advanced as a successful outcome.

## Appendix — the constants this loop relies on

Keyboard → pad: arrows are the D-pad, **J = A**, K = B, U = X, I = Y, **W/A/S/D
= the left stick**, H/F/T/G = the C-stick, E/R = L/R, Q = Z, RETURN = START.
Physical keycodes used by the test: 38 = J (A), 36 = RETURN, 13 = W, 123 = left
arrow. SDL scancodes: 80 = W, 13 = J, 40 = RETURN, 26 = W.

The two button layouts disagree and must not be "fixed": on the wire A is
`0x0100` and START is `0x1000`; in the guest's own cpad record at `0x803A4E20`
A is `0x0100` and START is `0x0010`.

Guest records: player pointer `0x803CA74C`; `demo_type +0x304`, `demo_mode
+0x314`, `proc +0x31D8`, position `+0x1F8/+0x1FC/+0x200`; `event_mode
0x803C9EA2`, `event 0x803C9EB8`, `msg 0x803CA7D2`; game pad pointer
`0x803A4DE0` with the button word at `+0x18`; decoded left stick
`0x803A4DF0/+4/+8`; scene-transition overlap object `0x803F6160` (live when
`>= 0x80000000` and `+0x04 == 1`), phase at `+0x1C`.

Route timing on this card: title 333, file-select 535, name-input-complete 712,
opening-complete ~13,850, play-scene ~13,910, cutscene pages 17,868 → 19,773,
control 19,972. v23 actual: 537 / 785 / 847 / 13,924 / 13,984 / control-admitted
20,164.

Env knobs: `BLUEWAKE_RENDERER`, `BLUEWAKE_FRAME_TIMING`, `BLUEWAKE_INPUT_PROBE`,
`BLUEWAKE_TRACE_PLAYER`, `BLUEWAKE_TRACE_PAD`, `BLUEWAKE_TRACE_ROOM0`,
`BLUEWAKE_CAPTURE_PLAYER_READY`, `BLUEWAKE_PLAYER_PROBE`, `BLUEWAKE_MAX_RETRACES`,
`BLUEWAKE_CARD_PATH`.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*

