# BlueWake goal loop - v53 (2026-09-22): the save path is closed, and the budget is the bar

**User-started and not blocked by the user; write the loop and go.** The governing
specification is [PRD.md](PRD.md), the operating procedure is
[GOAL_LOOP.md](../GOAL_LOOP.md), and the previous workstream page is
[GOAL_PROMPT_V52_2026-09-22.md](GOAL_PROMPT_V52_2026-09-22.md). What the PRD demands
that this loop is measured against, restated once so it is not re-derived:

- NFR-001: original 30 FPS presentation at 100 percent emulated speed; release needs
  mean emulated speed of at least 99.5 percent and a 1st-percentile of at least 95
  percent during active gameplay, frame-time p95 at most 36.7 ms and p99 at most
  50 ms, and zero interpreter-executed guest instructions across the acceptance
  corpus.
- Definition of done item 9: original game speed sustained across representative
  workloads on the documented minimum devices.
- Section 18: the agent may instrument, test, patch, commit and maintain the
  ledgers; it may not weaken a gate, reduce scope, or publish.

**The target does not move.** 60 retraces per second, 16.667 ms per retrace, on the
certified route, headless and rendered. One retrace is one guest retrace; 30 FPS
presentation is every other retrace, which is what the PRD calls authentic.

## Where this loop enters

v52's queue is spent: the split was taken, the cycle accounting was priced, the GX
FIFO translation moved to a worker, and the save-continue path - v52's item 4, the
last thing on that list that was engineering rather than performance - is now
measured end to end. The card the certified route produces holds the empty file the
card manager creates at file-select and nothing else; the pause menu's Save screen
writes the gameplay save; the guest's own reset quits; a separate boot with that
card reaches event-free gameplay at retrace 833 against 20,338 for a cold new game.
That is milestone 9's save, quit and reload, and the PRD's time-to-playable reading,
at one predicate on one build. The internals are in [status/CURRENT.md](status/CURRENT.md).

**What that leaves is the number, and only the number.** 409.2 M instructions per
play retrace on the bench window against the ~233 M the 16.667 ms target needs: a
1.7x gap, headless. The rendered tail is the other half of NFR-001 and it is the
half a player feels. Everything below is ordered by what is already priced:

| item | price | state |
| --- | --- | --- |
| the pc-keyed dispatch cache | designed, measured as a static count, never priced on the route | unpriced; needs the hot ten recompiled against the spliced header |
| cycle accounting | 9.8 percent of the emitted body, priced with ablate_chunk.py | design refused by the route once (a duplicated precise tail); the block-entry decision is untried |
| the donor DSP LLE | 10.06 percent of the per-turn device service; 31 host instructions per emulated DSP instruction | only 1.5-2.5 percent is reachable by dispatch work |
| the overlap observation's guard | ~12 instructions at 100 percent of boundaries, nil guest output in play | the crumb with the best frequency-to-risk ratio |
| the three device-predicate frames in the refresh | unknown | needs a registered ref/ patch before it can be touched |
| the rendered tail | median 40.72 ms, p90 48.85 ms after the worker; Aurora's render worker still idle ~94 percent | the p99 campaign has not started |

## The queue, ordered by what is known rather than by what is hoped

**1. The host's per-turn cycle credit, because that is what blocks the largest lever.**
The emitted body's residual cost is not the memory path - that helper is already an
always-inline two-compare MEM1 fast path, and the restrict ablation was a null - it is the
per-guest-instruction traffic to the CPUState: 1.71 gpr accesses plus 1.78 pc touches per
instruction on chunk 0144, which the yield discipline forces because any instruction can
return to the host. The design that removes it is the prepaid/twin pair, and the route
refuses that pair for an unexplained reason: 2.8 times the turns with the guest cycles
identical. So the next measurement is the host's own accounting - what flush_elapsed
credits per turn, what the deadline census says sizes the window, and whether a
coarse-charging body can be made credit-identical - and it is host-side, so it needs no
rebuild. The first reading is in: **half the host turns credit nothing** (1,384,094 of
2,769,817 on the 400-retrace control, with the total credit correct at 7.95 M cycles a
retrace), so the twin paid in turns because the zero-credit ratio moved, not because the
guest changed. The host-side fix to try next is to stop charging a full per-turn pass for
a turn that advances nothing, and to stop gating the retrace boundary on the credit stream
alone. Everything else in this list is smaller than what that unlocks.

**3. The dispatch cache, bundled into the next full rebuild.** The pc-keyed cache is
designed, its tooling is in the tree, and its static count is a third of the dispatch
entry's hot path. It cannot be priced by the hot-ten screen: the hot ten are 26 percent
of the window, so a design worth ~0.75 percent inside them shows as ~0.2 percent, below
the half-point this ledger resolves across independent pairs. It needs the 2h15 rebuild -
and that rebuild should carry every other header-shaped change with it, because the
rebuild is the expensive part.

**4. The save path's acceptance form - done, and the product question it opens.**
scripts/save_continue_acceptance.sh now runs save, the guest's own quit and the reload
unattended against the certified card copy, asserting the route milestones, the save
screen's proc chain, the card write counter, the card unmount, the card file's data
hash (1,218 of 98,304 bytes) and the reload reaching the play scene with no new-game
intro and no name entry; it passes, and it prints the pair the PRD's bar is read
against (control at retrace 833 / 13.9 s against 20,338 / 339.0 s). What is left for
this item is the real-key form, and one product check it raises: the pause menu's page
switch reads the *analog* L/R trigger, so a human reaches the Save page only if the
app's keyboard or pad mapping drives the analog trigger and not just the digital
button. That check has been made and it was a bug: a digital L or R now implies full analog
travel in the wire encoder (pad_wire.c), the certified route is unchanged by it (32,203,791
turns, digest 92dd816c), and the HLE PADRead path still needs the same rule in the pinned
dependency.

**5. The rendered level, and variance as its own problem.** The tail is not an event:
two headless passes of the same route share **zero** of 601 retrace times, differ by 1.50
ms on average, and their 115 and 108 ms stalls are private to each run. So the durable
number is the level (rendered median 34.80 ms, headless mean 36.26, reproducible) and the
p95/p99 gates are *variance* clauses whose work is host hygiene on the frame path - pages
faulted in, no allocation or lazy work in the frame, no scheduler fights - not a hot spot,
because there is none. The level below follows: The renderer has been eliminated as the
tail's owner: headless and rendered have the same mean, p90, p95 and p99 over the steady
window (36.26 / 45.65 / 47.14 / 48.89 against 36.00 / 45.62 / 46.98 / 47.94), the
headless median is *worse* (40.34 against 34.80) and the headless run has the one 115 ms
stall (scripts/frame_tail.py, both logs). The rendered path's problem is its *level* -
a 34.80 ms median against a 16.667 ms retrace target - and the tail is shared with
headless, so it is one problem: the main thread's emitted body and per-boundary work.
What remains specific to the rendered configuration is the instruction differential
(36.9 percent above headless), which is a count and not a wall time. A FIFO-write census
that would say whether the present retraces carry more guest submission is parked, not
done: its first version died at retrace 565 with no stop line and was reverted with the
revert verified, so the next version proves itself on a bounded headless run first.

**6. The overlap guard, last, with its price now known.** The always-on part is a guard
and an object read, about nine instructions at 94.14 percent of boundaries - an 0.8
percent ceiling - and its entire output is fourteen phase changes in 14,700 retraces.
The phase read already runs at 5.4 percent of boundaries, so the money is in the guard,
and the guard cannot be sampled less often without risking the digest record that made
the per-boundary cadence necessary in the first place.

**7. Hygiene, in the order it will bite.** Register the GX deferral and worker diff in
patches/ and config/dependencies.lock.json: the working tree of ref/recompcore is the
authority and the exported series is stale (only 0003 and 0018 apply), so a reverse-apply
today destroys the landed work. Second, note that scripts/gen_edge_intercept_table.py now
reads both edge_intercepts.c and edge_intercepts.h - the header-only case list it could
not see was the four chassis re-entry addresses, and dropping them stops the route before
the title screen. Third, scripts/bench_chunk.sh now bounds every sweep in wall clock; an
ablation that changes memory semantics can drive the sweep into a guest loop that never
returns, and `unchecked` did that for seventeen minutes on 2026-09-22.

**Not to be re-run, because they are refuted with numbers.** The per-term mask over the
interrupt sources; the redundant re-probes in the intercept predicate; the access-path
clause and the pc-store; branch hints on the hot guards; the register-file hypothesis; the
flat-body prepaid twin (route-refused); the loop-scoped prepaid twin (null); a digital R
press as the pause menu's page switch (the lock reads the analog trigger, measured);
d_menu_window.cpp's dMs_c as the collect page's save screen (it is a member at
dMc_c+0x2784, measured); an arbitrary retrace floor for the save route's gate (the control
window it was hiding is 82 retraces wide); `unchecked` as a sweepable ablation (it does
not terminate); the hot-ten screen as a way to price anything worth less than one
percent of the window (it dilutes by four); trimming the emitted body's accounting as a
route to NFR-001 (its roofline is 20.00 instructions per guest cycle, which is the whole
authentic-speed budget, before any host service is added); and a larger DSP service rate as a way to
buy turns (the cadence is guest-visible: -0.6 percent and a diverged route).

## The protocol, as of today

- scripts/bench_instructions.sh is the primary instrument: two ceilings differenced,
  /usr/bin/time -l instructions retired, with guards that refuse any pair whose stop
  pc, turn count, title or play milestones do not match the recorded reference.
  Recorded stops: 13,900 -> 0x80307EF4, 29,937,744; 14,100 -> 0x80307EF4,
  32,203,791 (certified, route digest 92dd816c...); 14,700 -> 0x8027FA30, 40,502,699.
- **Quote instructions, not milliseconds.** Wall clock varies by a third between
  processes; an instruction count on a digest-gated route does not.
- **A count is not a price.** ablate_chunk.py for a code shape, a private build for a
  host change, a census for a frequency. A static disassembly count screens a
  candidate and does not price it.
- **A host turn is a unit of the gate.** Anything that makes the chassis return to
  the host at an address the route visits changes the turn count the certified stops
  are quoted in. Measure it, and gate instrumentation on the route that needs it -
  the menu-path observation added this session moved the certified stop by 85 turns
  until it was made conditional, with the digest unchanged either way.
- **Verify against the count the change could break.** The save path was accepted
  because the card's own file hash changed and the guest's own proc field walked the
  save chain, not because a button was pressed.
- **Freeze what you measure.** Copy the composite, build host candidates in their own
  build directory, and check the private build against the shared one with nm -S.
- An iteration counts when a measured number moved on a named path with the digest
  green and the run verified, or when a new instrument answered a question that
  changed a decision, or when a candidate was refuted with a number. Report to the
  user every three iterations.

## Fences

Inherited and not negotiable: no Nintendo data, original binaries, generated game
code, saves, captures, signing material or leaked source in the repository; one
BlueWake process at a time; no timing measurement during a build; ref/ is a pinned
dependency whose uncommitted state *is* the patch series, so reverse an edit or
restore from patches/ and never git checkout a file there; register every ref/ patch
in config/dependencies.lock.json; do not weaken a clause to make a log pass; do not
re-run the refuted candidates.

Two this loop keeps, because they were both earned today. **A generated artifact
must be able to prove itself**, and the census that counts something is only
evidence if the count it could have broken is reported. And **a route driver is a
measurement, not the product**: the pad-based save is evidence that the guest's own
save path works, and it is not a substitute for the acceptance form item 3 asks for.

