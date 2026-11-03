# BlueWake goal loop — v19 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of `BlueWake.app` with no exported variables, a human
reaches controllable Outset gameplay with correct video and audio, drives it
with the keyboard without a crash, and time-to-playable is under five minutes,
with the five governing numbers holding on this host.

(Run labels: a `v19` acceptance run already exists from the previous session
and was taken on the *old* binary, before the correction below was written. The
first run of this loop is therefore labelled `v20`; the document governs the
work either way.)

## The failure signature, stated precisely enough to act on

v18's sentence was **"a gate that cannot fire on the product path is not a
gate."** That was right, and v18 fixed one instance of it: the player-ready
capture was moved off a CPU hook at `0x80122D30` that has never fired in any
run — the route's own included — onto a probe that reads the guest's player
pointer directly, so it needs no PC and cannot be skipped. The capture then
fired on every subsequent run.

Then the loop committed the same error one level deeper, and it consumed the
whole of v18: **it gated on a state the probe can see but that the product does
not act in.** The gate was `demo_mode == 4`. That state arrives *mid-cutscene*.
The driver read it as "Link is under your control", stopped feeding the walk,
and the run then sat inside the author's cutscene for its remaining 2,300
retraces while the test measured a keyboard against a cutscene and called it a
keyboard failure.

So the sentence for this loop is:

> **A gate that fires on a state the product does not act in is worse than a
> gate that cannot fire, because the run stops moving and the resulting failure
> reads as the app's fault.**

Every clause below is written to be decidable from the guest's own state, and
every gate is written to be the state the guest *acts* in.

## The root cause of the W-hold failure, proven and not inferred

Two independent probes agree, and they agree on the same three words.

**Our own run's record.** At and after the old capture the guest's record reads
`demo_type=1 demo_mode=4 proc=4 event_mode=2 event=38 pad_hold=0x00000000`, and
the live heartbeat prints byte-identical positions at every stamp from retrace
19,699 through 21,032. Nothing moved, and nothing *could* move: the guest was
still in its cutscene.

**The route's own player trace.** The route oracle at
`local-research/evidence/reorientation-20260828/outset-semantic-cutscene-message-player-control-final.log`
carries `[player-control-admission] moved=1 start=C83ED280,44CE4000,48992880
final=C83EE1CE,44CE4000,4899151C trigger_retrace=19972 final_retrace=20153`, and
its `[room0-player-execute]` lines at and after that retrace read
`event_mode=0 demo_type=0 demo_mode=0` with `stick=00000000,3F800000
stick_value=3F800000` while the position advances `C83EE129 -> C83EE127 -> ...`.
Its cutscene lines read `event_mode=2 demo_type=1 demo_mode=512`.

Read against the distinct tuples in the whole oracle:

| state | tuple | meaning |
| --- | --- | --- |
| authored Outset `awake` cutscene, its subtitle actor live | `demo_type=1 demo_mode=4 event_mode=2` | a held key is eaten; the position is pinned |
| the cutscene's earlier pages | `demo_type=1 demo_mode=512 event_mode=2` | same |
| **player control** | `demo_type=0 demo_mode=0 event_mode=0`, stick y `3F800000` | the stick steers Link |

The oracle's first `event_mode=0` is retrace **19,971** and its first
`event_mode=0 && demo_mode=0` is retrace **19,973** — roughly 6,000 retraces
after play-scene. **`demo_mode 4` is not control. `demo_mode 0` with
`event_mode 0` is control.**

### The instrument that hid it

`probe_hold` is `mem_read32(game_pad + 0x18)` — the guest's **button** word.
SDL scancode 80 (W) maps to `PAD_AXIS_LEFT_Y_POS`, which lands in a button word
as a single bit, `0x08000000`, and never as an axis. So the two readings "the
key never reached the guest" and "the key reached the guest and the scene was
not accepting input" were **literally the same bytes**, and the test could not
tell them apart. The guest's *decoded* stick is a separate record —
`0x803A4DF0` (x), `+4` (y), `+8` (magnitude) — and those are the three words
the route's own player trace reads. The W press **did** arrive; the scene was
not accepting it.

### The third defect, in the measurement rather than the product

The acceptance script parsed the player record with one `sed -E` whose tenth
field was written `\10`, and BSD sed reads that as group 1 followed by a
literal `0`. On a line whose `demo_type` is 1 the tenth field came back as
`10` and the z word was never present at all, so the clause compared two
identically truncated reads. That is a measurement defect and it is fixed in the
loop, but it is not the product defect: the position was genuinely unchanged as
well.

## What this loop changed before its first measurement

`runtime/host/src/main.c`:

* the player-ready capture gate is the **event-mode control gate** —
`player_valid && g_play_scene_reported && event_mode == 0 && demo_mode == 0`
held for **8 consecutive retraces** so a one-retrace flicker during a scene
handover cannot be mistaken for control — the same tuple the route's own
control gate reads (`player_state_ready` further down the same file tests
`0x803C9EA2 == 0`);
* a new `[player-milestone] control-admitted` line names the retrace and the
tuple, so the moment control is admitted is visible in the log rather than
assumed;
* the probe reads, compares and prints the guest's decoded stick
(`0x803A4DF0/+4/+8`), and the change test covers it (the slot array is 14 wide
now, not 11), so a stick that arrives but does not move Link is distinguishable
from a stick that never arrived.

`scripts/app_acceptance_test.sh`:

* `player_field()` is an awk field parse, so the field count cannot depend on
how a sed dialect counts capture groups; a `player_stick()` reader is added;
* the W-hold clause waits for `player-milestone. control-admitted` before it
measures, and its note prints `event_mode`, `demo_type`, `demo_mode` and the
stick words on both sides of the hold, so a future failure explains itself;
* `drive_play_control` now stops on `control-admitted`, not on the old capture,
and its attempt budget is 1,200 rather than 700;
* the retrace ceiling is 24,000. **The cutscene does not advance itself** — it is
fed by the A presses the driver makes, so a driver that stops early freezes the
run rather than finishing it.

## Falsification on record

Two competing readings of the v19 failure were live, and the loop separated
them rather than choosing:

* **H-PARSER-AFTERSHOCK** (the failure was a sed artifact) — *falsified as a
sufficient cause.* The parser was broken, and the position was also genuinely
unchanged. Fixing the parser was necessary and not sufficient; a loop that had
only fixed the parser would have re-run and failed again.
* **H-DROPPED-KEY** (the press never arrived) — *falsified.* The guest's own
record carries `pad_hold=0x08000000` at the press retrace and
`[input-probe] print=835 ... codes=26,0,0,0,0,0,0,0` on the host side. The press
landed in the guest; the guest was not listening.

## Hypotheses this loop owns

* **H-CONTROL-GATE** — the correction above. Passes when the default game pass
shows `control-admitted` with `event_mode=0 demo_type=0 demo_mode=0` at a
retrace at or after ~19,900 and a real 6 s W then moves the guest's own
`pos=`. Falsified if `control-admitted` appears before retrace ~19,900 (a
transition state is being read as control) or if the position still does not move
once it is admitted.
* **H-STICK-DELIVERY** — new, and it is the discriminator this loop added so the
next W failure cannot be ambiguous. A real held W must leave
`stick=y 3F800000` in the guest's own decoded stick record. Zero stick means the
press died above the player layer; a `3F800000` stick with no displacement puts
the fault downstream, in the player code, and that is a different loop's work.
* **H-SHORT-CEILING** — *falsified.* The cutscene was not being cut off by the
22,000 ceiling; it was being starved of A presses by a driver that stopped at the
wrong milestone. The ceiling is raised for headroom, not as a fix.
* **H-TTP-TRACE-TAX** — new, and it owns the second failed clause. Time to
playable was measured at **382.28 s** with `BLUEWAKE_FRAME_TIMING`,
`BLUEWAKE_INPUT_PROBE`, `BLUEWAKE_TRACE_PLAYER`, `BLUEWAKE_TRACE_PAD` and
`BLUEWAKE_TRACE_ROOM0` all set and ~2.2 MB of stderr going to a file. The
product claim is the double-click with **no** exported variables, so the clause
must be decided on a clean run, with the trace tax reported as its own number.
Deleting the traces to make a clause pass would be weakening the test; quoting a
traced number as the product's would be misreporting it.
* **H-TURN-COST**, **H-DSP-THREAD**, **H-CORPUS** — carried from v17/v18, still
unpaid. Measured first, then changed, in v17's order.

## The work, in order

1. **Land the correction and run the default game pass.** It is green when:
   title → file-select → name-input-complete → new-game-intro → play-scene;
   `[player-milestone] control-admitted` with `event_mode=0 demo_type=0
   demo_mode=0`; a real 6 s W that moves the guest's own `pos=` with the
   `stick=` words showing a nonzero y; a real RETURN and a real left arrow
   delivered at Outset; the player-ready frame written; `[run] stopped: normal`
   at the ceiling; `gx-core ... rejected=0 failed=0`; `dsp-lle ...
   first_nonzero=1`; and the user's own card byte-identical.
2. **The clean time-to-playable measurement** (H-TTP-TRACE-TAX), read from the
   host's own `[frame-timing]` stamps on the double-click path.
3. **Then performance**, in v17's order: H-TURN-COST priced as a number before
   any build flag moves, then H-DSP-THREAD, then H-CORPUS.
4. **Owed residue:** the controlled "one press moves Link N units" displacement
   test — the instrument now exists, since `[player-scene-state]` carries the
   guest's own three position words; `scripts/bench_report.py:87`
`window_metrics` off-by-one (`KeyError: 14097`); and `scripts/bench.sh`'s
overlap guard, which greps `bluewake_host` and therefore misses the bundle
process `MacOS/BlueWake`.

## What not to do

* **Do not weaken the W clause, substitute a synthetic stick, or accept
  `demo_mode 4` as control.** A synthetic button press is a route driver, not
  evidence that the keyboard works.
* **Do not stop driving A while the cutscene is up.** It does not advance
  itself.
* **Do not build the bare `all` target** — `tests/delivery_digest_test.c` has
  a pre-existing, unrelated break. Build
  `cmake --build build/runtime-host-dsp --target bluewake_host`.
* **One BlueWake process at a time.** The acceptance script refuses to overlap
  and its guard already matches the bundle path.
* **Do not read a run that is still live as a verdict.** Wait for its ceiling
  stop.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an
iteration**, and neither is a probe: a probe is a cost paid to find the work.
Report to the user every three iterations in one paragraph. Silence is not
acceptable; neither is a document about why it cannot be done.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86...`,
  is hashed before and after every run and must come back byte-identical. Never
  write the user's own slot: the walk runs against a copy of the canonical route
  card, sha256 `6b43aabd...`.
* No disk-heavy work — repo-wide scans, `lldb`, `atos`, `otool` — while a
  measurement run is live. A background app at 100% CPU invalidates a timing
  number, and any guard has to match the bundle path too.
* No silent stubs, no no-op substitutes, no weakening or deleting a test or a
  digest because it exposes a failure.
* Never write that no product gate advanced as a successful outcome.

## Appendix — the constants this loop relies on

Keyboard → pad: arrows are the D-pad, **J = A**, K = B, U = X, I = Y, **W/A/S/D
= the left stick**, H/F/T/G = the C-stick, E/R = L/R, Q = Z, RETURN = START.
Physical keycodes used by the test: 38 = J (A), 36 = RETURN, 13 = W, 123 = left
arrow. SDL scancodes: 80 = W, 13 = J, 40 = RETURN, 26 = W.

The two button layouts disagree and must not be "fixed": on the wire A is
`0x0100` and START is `0x1000`; in the guest's own cpad record at
`0x803A4E20` A is `0x0100` and START is `0x0010`.

Guest records this loop reads: player pointer `0x803CA74C`; `demo_type` at
`+0x304`, `demo_mode` at `+0x314`, `proc` at `+0x31D8`, position at
`+0x1F8/+0x1FC/+0x200`; `event_mode` at `0x803C9EA2`, `event` at
`0x803C9EB8`, `msg` at `0x803CA7D2`; game pad pointer at `0x803A4DE0` with
the button word at `+0x18`; decoded left stick at `0x803A4DF0/+4/+8`.

Route timing from the same card: title 333, file-select 535, name-input-complete
712, opening-complete 13,850, play-scene 13,910, cutscene pages 17,868 → 19,773,
control 19,972. Host environment knobs: `BLUEWAKE_RENDERER`,
`BLUEWAKE_FRAME_TIMING`, `BLUEWAKE_CAPTURE_PLAYER_READY`,
`BLUEWAKE_PLAYER_PROBE`, `BLUEWAKE_TRACE_ROOM0`, `BLUEWAKE_INPUT_PROBE`,
`BLUEWAKE_PAD_*`, `BLUEWAKE_MAX_RETRACES`, `BLUEWAKE_CARD_PATH`.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*
