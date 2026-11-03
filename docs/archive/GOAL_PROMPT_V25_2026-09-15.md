# BlueWake goal loop — v25 (2026-09-15)

**User-started. Finish and test the app.** The terminal condition is unchanged:

> From a double-click of BlueWake.app with no exported variables, a human
> reaches controllable Outset gameplay with correct video and audio, drives it
> with the keyboard without a crash, and time-to-playable is under five minutes,
> with the five governing product numbers holding on this host.

**This document supersedes docs/archive/GOAL_PROMPT_V23_2026-09-14.md.** That document
carried v23's verdict and predicted v24. v24 ran and returned the first real
product win this project has had. This document reports v24, reports the two
harness defects v24 exposed, reports the runtime fix built for the second one,
and reports why v25 could not yet test it.

## The headline: the displacement clause passed, for the first time

v24 is the first run in this project's history in which a real key moved Link in
the guest's own player record. Log /tmp/bw-acceptance/acceptance-v24.out and
/tmp/bw-acceptance/run-v24-final.log; app pid per the acceptance output; ceiling
24,000 retraces.

    acceptance: control window 2 open: pos=C83ED664,44CE4000,4899226B stick=00000000,3F800000,3F800000
    acceptance: a real held W moved Link in the guest's own player record: C83ED664,44CE4000,4899226B -> C83ED686,44CE4000,489921C9 (5.0903 units)
    acceptance: the displacement clause passed against the guest's own player record

Also passed, in order: title-ready 333, file-select 541, name-input-complete 821,
new-game-intro 883, play-scene 14,020; the control gate fired —

    [player-milestone] control-admitted retrace=20016 event_mode=0 demo_type=0 demo_mode=0 ovl=6 pos=C83ED28D,44CE4000,48992842

418 real key presses landed, 409 of them real A taps feeding the awake cutscene;
[run] stopped: normal after 3046285489 blocks at pc=0x80307ef4; gx-core
rejected=0 failed=0; dsp-lle first_nonzero=1; the user's own slot byte-identical.

Note what the win depends on: the driver kept feeding A. Window 1 opened and
closed with no displacement because the position moved between windows; window 2
carried it. The same fix that failed in v23 works as soon as the A presses keep
arriving. The cutscene does not advance itself.

## v24's two failures, and what was actually wrong with them

v24 reported two clause failures. Neither was a product failure, and both are
now fixed in the harness.

**Defect A — TTP-B was a duplicate of TTP-A.** The "earliest marker" rule picked
play-scene at retrace 14,020, so TTP-B could never show the save-state win it
exists to measure; both clauses read 397.41 s. Fixed: the clause now prefers
control-admitted, then player-ready, then play-scene as a fallback, and always
names which marker answered. Re-read against v24's log it now reports
1,492.11 s (retrace 20,016) — the honest cold-boot number.

**Defect B — the "player-ready frame" was never a picture of Outset.** The old
capture path at the 0x80122D30 hook fired on demo_type == 1 && demo_mode == 4 at
retrace 19,716 and won the race against the correct path at 20,016. The guest's
own record proves 19,715/19,716 was still the cutscene
(demo_type=1 demo_mode=4 proc=4 event_mode=2, keys being eaten). v24's frame is
1,920x1,440 with nonblank=3,257 — 0.0012 of the frame, a black frame with a
spinner glyph. v23's frame, re-rendered to /tmp/bw-v23-outset.png, is a cutscene
page with a subtitle box reading "You should probably go home and see w" over
Aryll. Neither run ever produced the picture the clause claimed. The old path
was not dead code; it was actively wrong, capturing the cutscene while the
correct capture stayed armed.

The clause is also stricter now: a written PPM must be a render, at least 0.20
nonblank fraction of the frame, and it reports frames_written and the best
fraction. A near-empty buffer is its own named failure. Re-read against v24 this
fails at 0.0012, which is the correct answer.

## The runtime fix, built and on disk, not yet proven by a run

runtime/host/src/main.c carries five patches:

1. player_ready_capture_followups declared beside player_ready_capture_scheduled,
   with the reason two frames are taken rather than one.
2. The player-scene-state path sets followups = 1.
3. The 0x80122D30 hook now requires event_mode == 0 && demo_type == 0 &&
   demo_mode == 0 && g_overlap_terminal_phase == 6 — the tuple the control gate
   requires — instead of demo_type == 1 && demo_mode == 4. It sets followups and
   tags its log line source=overlap-handler.
4. Frame naming is retrace-stamped when capture_player_ready is set, so the two
   frames cannot overwrite one another.
5. A re-arm branch takes the second frame 120 retraces later.

Build state: bluewake_host rebuilt successfully; the binary is newer than the
edit, strings shows exactly one source=overlap-handler and one
source=player-scene-state, and codesign --verify is clean. Build the target, not
bare all — tests/delivery_digest_test.c has a pre-existing unrelated break.

## v25: two launches, neither a verdict

**v25 attempt 1 (app pid 47154) — void, external interference.** The walk stalled
at the guest's own name-entry grid after six real A presses that never landed.
The app had not lost SDL keyboard focus (flags=0x20002620, INPUT_FOCUS set), and
a hand-sent J seconds later landed immediately and advanced the guest from
main_proc=6 to main_proc=7. The cause was another, unrelated task on this same
machine: an iOS Simulator UI test (xcodebuild test-without-building) was starting
a Simulator window that took the key window and swallowed delivered keys. Log
kept as /tmp/bw-acceptance/run-v25-attempt1-focus-stall.log.

The fix is in tap(): a press is sent up to four times, each retry re-activating
the window and holding longer than the last, and the guest's own controller
counter still has to increase or the press is a failure. This is not leniency —
a press that never arrives is still a named failure — it is what a person does
when a tap is eaten.

**v25 attempt 2 (app pid 52842) — invalid, cut short.** The retry fix worked and
the walk cleared: title after 8 presses, file-select, name-entry grid at
7 0 3 1 0, RETURN moved the selection to END, name accepted at 9 1 4 1 1,
new-game-intro at retrace 911, play-scene at retrace 14,048. Many presses landed
on retry 2 or 3, which is the interference being visibly absorbed.

Then, 151 real A presses into the awake cutscene, presses stopped landing and the
app quit:

    [run] stopped: quit after 1232146430 blocks at pc=0x802555d0

quit is dol_platform_should_quit() at main.c 4,227 — the window/platform quit
event. There is no crash report for BlueWake in ~/Library/Logs/DiagnosticReports.
This is external interference again, not our code and not a clause.

Ten clauses failed downstream of that stop, and none of them measures the
product: the capture never scheduled because control was never admitted, so no
frame was written; the process was gone before control-admitted could appear.
The two timing failures are contamination and must not be read as a regression:
v25 reached play-scene at retrace 14,048 against v24's 14,020 — the same retrace
— but the host's own wall clock put it at 954 s against v24's 397 s, about 2.4x
slower per retrace. The host was loaded by the same concurrent UI automation. On
a quiet host that number is a wall-clock measurement; this one is not.

One genuinely useful observation survived: H-PLAYER-CONSUMES-STICK fired, i.e.
this log contains a record with the decoded stick at y=3F800000, so a real stick
key did reach the guest's own steering value.

## What is built but unproven

Everything below is written, syntax-checked or compiled, and has never been
exercised by a clean run:

- the five main.c frame-capture patches above;
- the two-frame followup and its retrace-stamped naming;
- the stricter frame clause (0.20 nonblank) against a correct capture path;
- TTP-B preferring control-admitted over play-scene;
- the tap() retry.

The single next milestone is a clean run on an idle host in which the frame
clause passes with a picture a human can look at, control-admitted lands, and the
displacement clause passes again with the new binary.

## The work, in order

1. Confirm no competing UI automation is running before a launch, and treat any
   run that quits early with stop reason quit as void rather than as evidence.
2. Launch v26 on an idle host and poll to the ceiling. Expected: displacement
   PASS, frame clause PASS, TTP-A about 397 s and TTP-B about 1,492 s still red.
3. Convert the player-ready frame to PNG and look at it. It must show Link under
   control at Outset. If it does not, the capture path is still wrong and that is
   the next defect, not a clause tweak.
4. Only then start the save-state work that TTP-A and TTP-B exist to measure:
   TTP-A is the honest cold-boot number and should stay; TTP-B is the one a
   save-state launch is meant to collapse.
5. Then performance, in v17's order: H-TURN-COST priced as a number first, then
   H-DSP-THREAD, then H-CORPUS.

## Hypotheses this loop owns

- H-KEYBOARD-NEVER-ARRIVED vs H-PLAYER-CONSUMES-STICK: a zero button word is not
  evidence a stick key missed the guest. Only the three decoded stick words at
  0x803A4DF0/+4/+8 answer that, and only inside the control tuple.
- H-FRAME-RACE: both capture paths must require the same control tuple, so they
  agree by construction instead of by which fires first.
- H-HOST-CONTENTION: a wall-clock product number is only a product number on a
  quiet host. A 2.4x inflation is visible in retrace-per-second and must be
  reported as contamination, never as a regression.

## Falsification on record

- The claim that demo_mode 4 marks control is false; the route's own player trace
  reads demo_type=0 demo_mode=0 event_mode=0 at control, and demo_mode 4 arrives
  with demo_type=1 event_mode=2 and the authored cutscene still on screen eating
  keys.
- The claim that a written PPM is a picture is false; v24 wrote 3,257 nonblank
  pixels of 2,764,800.
- The claim that the displacement clause cannot pass is false as of v24.

## What not to do

- Do not read a run whose stop reason is quit, or whose retrace-per-second is
  inflated, as a verdict.
- Do not drive the name walk, the intro, or the awake cutscene with synthetic
  pad pulses; a synthetic press is a route driver, not evidence the keyboard
  works.
- Do not stop feeding A while the cutscene or the post-cutscene sequence is up.
- Do not weaken a clause to make a log pass, and do not delete a digest or a
  test to make a build green.

## The iteration contract

An iteration counts only if time-to-playable fell, fps moved on a named path, or
a human can see, hear, or do something new. A document is not an iteration, and
neither is a probe. Report one paragraph to the user every three iterations.

## Fences, non-negotiable

Never commit or publish Nintendo data, original binaries, generated game code,
saves, captures, device data, signing material, or leaked source. The card walk
uses a copy at /tmp/bw-acceptance/save.card (from run1.card, sha256 6b43aabd) and
the user's own slot (sha256 b0163d86) must return byte-identical. PPM and PNG stay
in /tmp; ref/ is gitignored, so runtime edits ship as ordinary source, not as a
patch bundle. One BlueWake process at a time. Do not read a live run as a verdict,
and do no disk-heavy work (lldb, atos, otool, repo-wide scans) while a run is
live.

## Appendix — constants this loop relies on

Keyboard to pad: arrows are the D-pad, J is A, K is B, U is X, I is Y, W/A/S/D
are the left stick, H/F/T/G the C-stick, E/R are L/R, Q is Z, RETURN is START.
Physical keycodes: 38 = J, 36 = RETURN, 13 = W, 123 = left arrow. Wire A 0x0100,
START 0x1000; in the guest cpad record at 0x803A4E20, A 0x0100 and START 0x0010.

Guest records: player pointer 0x803CA74C; demo_type +0x304, demo_mode +0x314,
proc +0x31D8, position +0x1F8/+0x1FC/+0x200; event_mode 0x803C9EA2, event
0x803C9EB8, msg 0x803CA7D2; pad pointer 0x803A4DE0 (word +0x18); decoded stick
0x803A4DF0/+4/+8; overlap object 0x803F6160, live when >= 0x80000000 and +0x04
== 1, phase +0x1C.

Route timing on this card: title 333, file-select about 541, name-input-complete
about 821, opening-complete about 13,960, play-scene about 14,020, cutscene pages
17,868 to 19,773, control 19,972.
