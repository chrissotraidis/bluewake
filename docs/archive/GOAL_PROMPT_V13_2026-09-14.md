# BlueWake goal loop - v13 (2026-09-14)

Supersedes v12 ([GOAL_PROMPT_V12_2026-09-14.md](GOAL_PROMPT_V12_2026-09-14.md)). The PRD still defines what
finished means. V12 said the objective has two gates in two phases, and that the loop compounds one measured
share at a time with the digest held. This session took the three measurements V12 asked for. One clause of
the report is retired, one hypothesis is dead before it was built, and the one substitution this project
already had - turning the cycle cap up - is now measured: it is real, it is large, and it is not free.

## The three measurements, in the order they landed

**1. The keyboard clause is retired.** A human double-click of the built app, launched with
`BLUEWAKE_INPUT_PROBE=1` and with `BLUEWAKE_PAD_PULSE_ON_TITLE_READY` never set, so only the keyboard could
move it, reached File Selection on real key presses with no synthetic pulse anywhere in the process.

    [input-probe] print=13 read=769 kbFocus=0xc14e68a00 pressed=1 J=1 port0_button=0x0100
    [input-chain] retrace=381 cpad_hold=0x0100 cpad_trig=0x0100 jut_hold=0x0100 jut_trig=0x0100
    [boot-milestone] name-scene-create retrace=693
    [boot-milestone] file-select retrace=779

Frames `EVIDENCE-title-press-start.png`, `EVIDENCE-name-entry-721-by-keyboard.png` and
`EVIDENCE-fileselect-781-by-keyboard.png` show the title, the name-entry keyboard and the File Selection
pane with Quest Log 1/2/3 and New Game. Run kb1, one press, is the negative control: `cpad_trig=0x0100`
arrived and the title did not advance in 300 retraces.

Two harness facts came out of it and are load-bearing for every later input test. **A single press at the
title does not reliably take, because the title's confirm is polled and the press must be held across the
poll**; thirteen real presses 0.32 s apart worked. And the synthetic schedule never shows this, because the
pulse is triggered by the guest's own confirm poll (`title-confirm-ready`, from `title_proc+0x30 == 1` at
`host_canonical_linked_pc == 0x81E01BA4`) and therefore always lands inside one.

**2. H-INTERRUPT-CHURN is dead, and it died before any code was written.** The host already carries the
counter V12 wanted: `host_cycle_deadline_distance` attributes every dispatch budget to the term that bound
it. On the product path with `BLUEWAKE_DEADLINE_CENSUS=1` and `BLUEWAKE_DEADLINE_CENSUS_WINDOW=800`, over
2,200 retraces, 356,778,130 budget evaluations split **cap 97.9%**, dsp 1.85%, audio 0.20%, clamp 26,707 (all
one-cycle), vi 0.0024%, decrementer 3,923. In the intro window alone, 165,054,345 evaluations: **cap 98.0%**,
dsp 1.76%, audio 0.20%, clamp 14,153, vi 3,309.

The one-cycle churn signature - a deadline re-derived at distance 1 - is 14,153 / 165,054,345 = **0.009%** in
the intro. The deadline is not churning. It is computed unconditionally once per turn and in 98% of cases
reports that no device deadline is inside the cap at all. Coalescing a value that is not changing buys
nothing, so that lever is retired. What the census establishes instead is the shape of the budget: **host
turns are paid at cap granularity, so the lever is the number of turns, not the value the deadline
returns.**

**3. The cap experiment: minus 36.5% of host turns, and exactly one record moves.** The census pointed at
the cap, and the cap is the one place where a measured, already-built, delivery-invariant substitution
existed rather than had to be invented: `BLUEWAKE_CYCLE_CAP` accepts `dynamic`, `main.c` sets
`bluewake_cycle_domain_set_dynamic_cap(&g_cycle_domain, 256, 1024u)`, and the 2026-08-31 decision record
promoted that mode as the short-route cycle contract with the sentence *"Long-route qualification remains
required."* That qualification had never been done. It has now been done, on the same route, same card
(`6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`), same host, same harness, one
variable:

| | cap 256 (baseline) | cap dynamic(256/1024) |
| --- | --- | --- |
| host turns | 820,034,526 | **521,121,740 (-36.5%)** |
| peak RSS | 262,799,360 B | 262,914,048 B (+0.04%) |
| clock summary | cycles=114210000002 timebase=9517500000 vi_pending=1 ai_remainder=105348 title_retrace=333 | **identical** |
| final stop | pc=0x80307EF4 normal | pc=0x80307EF4 normal |
| route digest | 2b7a1fa5... (1050 records) | 50f8f154... **DIVERGED** |

Everything except the digest looks like a win. The digest is where the information is. **Exactly one of the
1,050 records differs**, and it is the delivery-history hash:

    external=146755 hash=04CEB93422F05CB3   (cap 256)
    external=146755 hash=FF6A24536836303D   (dynamic)

The counts, the ages and every other field are identical: `external=146755`, `history=1024`,
`history_overflow=145731`, `dsp=125471`, `first_dsp_valid=1`, `first_dsp_cycle=14353612`,
`first_dsp_cause=0x00010040`, `first_dsp_pc=0x8030464C`, `first_dsp_context=0x804211E0`. Every
`boot-milestone`, every `dvd-lifecycle`, `aram-dma` and `dsp-lle` summary, the collision and `fp`
records, and the card are byte-identical; the first 1,024 external deliveries are byte-identical, down to
their cycles and prefix hashes. The divergence is later than delivery 1,024 and it is a **cycle** change,
not a state change.

## What the cap result means

The turn length is observable in the emulation, and the cap was never a free knob: it is the interrupt
latency knob. The mechanism is in the deadline function. An external interrupt is noticed **at a turn
boundary**: `host_cycle_deadline_distance` returns distance 1 (`SOURCE_CLAMP`) when `MSR[EE]` is set and
an external is pending, but that expression is evaluated while the *next* turn is being prepared. An
interrupt that becomes pending in the middle of a turn waits for the turn to end, and device synchronisation
happens at `host_sync_cycle_devices_end_turn`. So a 256-cycle turn delivers a device interrupt up to 256
cycles late and a 1,024-cycle turn delivers it up to 1,024 cycles late. The census is the other half of this:
the DSP term owned the budget in only 1.76% of intro evaluations, which is what you would see if the DSP
adapter is advanced at turn boundaries rather than at its own next event.

This is why the digest moved. `external=146755` deliveries are dominated by DSP interrupts (125,471 of them,
about ten per retrace), and the hash covers each delivery's exact cycle. A longer turn shifts where inside a
retrace each one lands. Guest-visible behaviour is untouched - that is what the 1,049 identical records say -
but the emulation's interrupt timing is not, and interrupt timing is exactly what the cycle domain exists to
model.

**So the honest reading is not "the cap is a 36.5% win". It is: the cap is a 36.5% win whose price is
interrupt latency, and the price is currently paid in cycles rather than in accuracy.** Under this project's
rules that is not a result. It is the definition of the next piece of work.

## The new hypothesis this loop is built on

**H-DSP-DEADLINE: if the device deadline terms bind inside the cap, then the cap becomes delivery-invisible
and the 36.5% is free.**

The claim is specific enough to be wrong. If the DSP adapter and the audio DMA report their *own* next event
cycle instead of a batch rate that sits above any sensible cap, then a long turn stops at the event,
delivers at the same cycle it delivers at with a 256-cycle cap, and the delivery hash holds. The success
test is already standing: **dynamic cap, digest equal to `2b7a1fa5...`, host turns still near 521M.** If
that cannot be reached, the fallback is not silence - it is the owner's decision, stated as a fork:

* make delivery cap-invariant (more runtime work, the current preference), or
* declare the cycle-domain *delivery-cycle* history intentionally cap-dependent and re-baseline that one
  record, with review, the way the 2026-08-31 short-route rebaseline was done.

That second option is a project decision, not an agent decision, and it is the only route by which a
digest may change here. Re-recording a digest to make a run pass stays forbidden.

**Read before building this, because the source says the naive version cannot work.** The DSP adapter is
advanced from `host_sync_dsp_cycles` at the end of a turn, in whole quanta of `DSP_LLE_UPDATE_RATE`
(`main.c:1504-1514`): `g_dsp_adapter_update_elapsed += elapsed_cycles` and then `while (elapsed >=
DSP_LLE_UPDATE_RATE) host_dsp_run_cpu_cycles(...)`. `DSP_LLE_UPDATE_RATE` is **12,600**, so the DSP's
own advance granularity is already far coarser than any cap this project allows (cap <= 65,536, dynamic
1,024). The deadline term reports `12,600 - elapsed` (`main.c:448-452`), which can only bind when elapsed
is near 12,600 - never inside a 256- or 1,024-cycle cap. So the DSP term **cannot** be made to bind by
turning the cap up, and the delivery shift is not a term that binds late; it is the *publication* of a
DIRQ raised inside an update quantum that is itself coarser than the cap. Making delivery cap-invariant
therefore means driving the adapter by its own next event cycle rather than by accumulated turn time, which
is a change to how the DSP is scheduled, not a constant. That is the real content of H-DSP-DEADLINE, and it
should be costed as such before it is attempted.

## Retired by measurement, do not reopen

* **H-INTERRUPT-CHURN - falsified.** 0.009% one-cycle evaluations in the intro. There is nothing to coalesce.
* **H-SAVE-CONTINUE - killed.** The route card differs from the user card by 14 header bytes; there is no
  distinct Outset save to continue into.
* **H-DSP-SHADOW - falsified.** With the LLE DSP adapter off the guest stalls in JAS before the title;
  `saved_pc=0x80307EAC` waiting on a DSP interrupt that never comes. The adapter is load-bearing.
* **H-INTRO-HOLD - falsified.** A real OS-delivered four-second held START did not end the intro.
* **H-RENDER-OFFLOAD as the lead - deprioritized.** 3.0% of the play scene against 21% of the title path.
* **A single press at the title - retired as a test.** Thirteen presses 0.32 s apart, or one press held
  across the confirm poll, is the protocol. A one-press negative in the title screen proves nothing about the
  keyboard.

## The numbers on this host

`scripts/bench.sh` still runs the shipping configuration, cap 256, and reproduces the 2026-09-01 baseline.

| number | baseline | cap dynamic, this session |
| --- | --- | --- |
| route digest | 2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b | **diverged, one record** |
| host turns | 820,034,526 | 521,121,740 |
| median fps (window 13910:14100) | 3.10 fps this host | 2.41 fps (see below) |
| p99 frame time | 512.35 ms | 614.04 ms |
| peak RSS | 262,799,360 B | 262,914,048 B |
| **time-to-playable** | **15.3 min, 919.8 s** | not measured |

The clean number is the turn count: it is host-independent and it moved 36.5% with a one-line configuration
change. The fps numbers from this experiment are **not** an A/B and are not a result. The dynamic run's own
rates are whole route 681.06 s for 14,099 retraces, 20.7 fps, and intro (773 -> 13850) 540.71 s for 13,077
retraces, **24.2 fps**, against the product path's 15.65 fps intro at cap 256 - but the cap-256 control in the
same harness is the run whose median was dragged to 1.94 fps by disk work during its live window, so that
comparison is contaminated and is not claimed. Its play-scene window also came out *slower*, 2.41 against
3.10 fps, which the next experiment must explain or the cap is not a win on the phase that owns playability
either. fps is re-measured, with a live control, or not quoted.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path, or a human can now see, hear
or do something they could not before. A profile is a cost paid to find the work, never progress by itself.
An experiment that produces a number nobody can use - like this cap run - counts as a falsification and is
worth exactly one short section, and it retires a lever that would otherwise have been built.

One hypothesis, one small change, one build, one measurement, and at most one short section appended to
`docs/archive/status/CURRENT.md`. No status document may be longer than the change it describes. Never add a new
tier, slice or patch for a unit that already compiles whole.

If an iteration ends with no change to an observable and no change to a number it **failed**: revert it and
change the mechanism, not the constant. Three failures of the same shape means stop, measure the layer
underneath, and say so out loud. Report to the user every three iterations in one paragraph.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated game code, saves, captures,
  device data, signing material or leaked source. The user-owned GZLE01 stays private and out of git, every
  PPM and PNG stays in /tmp, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card is at `~/Library/Application Support/BlueWake/GZLE01.card`, sha256
  `b0163d86...`; drive the new-game flow against a copy and keep the /tmp backup current.
* One BlueWake process at a time.
* No silent stubs, no no-op substitutes for required services, no weakening or deleting a test because it
  exposes a failure, no captured emulator state as authentic initialization. **A synthetic button press is a
  route driver, not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass. A change that buys fps by moving the digest is a
  behaviour change, not a result, until the delivery question is settled the way the section above says.
* Never write that no product gate advanced as a successful outcome; that sentence is the failure signature
  of this project.
* **No disk-heavy work while a measurement run is live.** A repo-wide scan, `lldb`, `atos` or `otool`
  during a live window drags the fps it is measuring. The turn count and digest survive; the fps does not.

## The work, in order

1. **H-DSP-DEADLINE.** Make the device deadline terms bind inside the cap so a long turn stops at the event
   rather than after it: read `host_cycle_deadline_distance`, the DSP adapter's update rate and the audio
   DMA's work-chunk accounting, find which term is sitting above 256, and bring it down. Success is dynamic
   cap with the baseline digest and turns still near 521M. This is the largest measured lever this project
   has found and it is one file.
2. **Re-measure the cap A/B's fps properly**, with a live cap-256 control in the same harness on the same
   host, and explain the play-scene window, 2.41 against 3.10 fps. A cap that wins turns and loses the play
   scene is not a win on the phase that owns playability.
3. **H-GUEST-DISPATCH.** 58-62% of both phases, flat, spread thin (top symbol 14.0%). The long pole. Measure
   per-instruction cost before touching the recompiler.
4. **H-DSP-THREAD.** 28% of the intro, about 7% of the play scene. Parallelize the adapter off the emulation
   thread, holding the digest and the paced-PCM fingerprint. Its weight changes once step 1 lands.
5. **DONE - the owed report clause: Link under control of a real key press on Outset, with frames.** Real
   OS-delivered presses on the play scene land in the guest SI word (`cpad_trig=0x0010`, `jut_trig=0x1000`
   at the press retrace, `0x0000` at every neighbouring poll) and the run stops normally at pc=0x80307ef4
   with no crash. Frames in `/tmp/bw-outset3/`. What is *not* yet done is a controlled displacement test -
   one press moving Link a measured distance - which needs a player-position trace. That is the residue,
   not the clause.
6. **M1c, then M2, M3 and H1/H2/H3** as recorded in v4: one press by a human hand on a double-clicked window;
   Outset median at least 15 fps then 30 fps with the digest unchanged; Link hair correct at the GX owner
   with a screenshot and no geometry regression; continuous intelligible audio; a physical controller
   including reconnect; a ten-minute unattended soak with bounded RSS; and one-command clean build and launch
   from a fresh clone with the private disc.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS START appears, and the file
menu comes up; the new-game intro costs X minutes and Y reaches Outset; here is the screenshot of Outset with
Link under your control.* Silence is not acceptable; neither is a document about why it cannot be done.
