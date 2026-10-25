# BlueWake goal loop - v51 (2026-09-21): the per-block constant

**User-started. Nothing is blocked by the user; write the loop and go.** The
two decisions v50 left open are now the loop's own, decided here and recorded:

1. **The target stays 60 retraces per second** (16.667 ms per retrace) on the
   certified route, headless and rendered. The loop may not renegotiate it. What
   it owes the user, once it has measured the ceiling of this architecture, is
   the number - not a quiet substitution of a lower one.
2. **Time-to-playable is read against continuing from a save the game itself
   wrote**, which the PRD's wording allows and which fits inside five minutes.
   Cold-new-game is 337.6 s of authored content at authentic speed and cannot
   fit at any emulator speed; that is arithmetic, not a defect, and it is
   recorded rather than attacked. The save-continue path is engineering and is
   on the queue, because it is also P4 milestone 9.

The program (P0 truth and hygiene through P7 iOS and iPadOS) is unchanged and
lives in [GOAL_PROMPT_V50_2026-09-21.md](GOAL_PROMPT_V50_2026-09-21.md). This
page is the active workstream inside P1, the way v49 was the active workstream
inside the emitter workstream it closed.

## Where this loop enters

Two things changed today and both change what the next iteration should be.

**The reconcile is done.** The ledger's two play-window numbers are both right
and are different windows: opening 53.6 M instructions per retrace, early play
13,900-14,100 **402.1 M**, the certified window 285.9 M (a third of it cutscene),
steady play 14,100-14,700 **492.1 M**. The scene is not stationary - turn
density per retrace runs 1,998, 11,330, 30,498 as Outset fills in - so the
certified 1.81x is the early window's number and the steady window is 2.2x.
Authentic speed needs 24 to 29.5 G instructions per second depending on window;
this host retires about 14 G.

**The per-block path is the cheap, measurable lever, and the fast-reject priced
it.** The edge fast-reject landed at **-4.42 percent** by replacing a miss path of
about twenty host instructions with one hash probe, at every generated-block
dispatch. That is 21.5 M instructions per play retrace removed.

**The rule this yields, and it is the loop's working model - corrected 2026-09-22
by the boundary census, which counted the quantity 0.22 percent was inferred
from:**

> One host instruction removed from the per-block dispatch path is worth about
> **0.08 percent** of the play window, because a play retrace runs 379,738
> boundaries and not the 1.07 M the 0.22 was derived from, and the miss path the
> fast-reject removed was about 56 instructions rather than about twenty. Ten
> instructions are 0.8 percent. Each candidate costs about twenty seconds to
> build and four minutes to measure.

That is a better instrument than any profile in this ledger, because it prices a
change before it is written. The counts and the arithmetic behind the correction
are in `docs/archive/status/CURRENT.md` under 2026-09-22.

## The queue, in the order the model prices it

**1. The overlap-phase observation in the chassis edge service. DONE 2026-09-21, and it was 0.5 percent rather than the predicted 2-5** - the guard under which it runs is false on most boundaries, so the block's live fraction has to be measured before the per-block price is applied to it. See `docs/archive/status/CURRENT.md`. The model stands; the application needs a second measurement.

Inside
`host_chassis_edge_service`, before the intercept predicate, sits this:

    if (g_name_scene_object >= 0x80000000u &&
        (g_file_start_pulse.triggered || !g_file_start_pulse.configured)) {
        const u32 overlap_object = mem_read32(cpu, 0x803F6160u);
        if (overlap_object >= 0x80000000u &&
            mem_read16(cpu, overlap_object + 0x04u) == 1u) {
            const u32 phase = mem_read32(cpu, overlap_object + 0x1Cu);
            ...

`g_name_scene_object` is set when the name scene is created and is never
cleared, so the outer guard stops discriminating early and three guest reads run
at **every** block boundary for the whole play window - about 1.07 M times per
retrace. It cannot simply be deleted: it exists to restore the shipping
per-block observation cadence, and the recorded `overlap_phase` genuinely differs
between ceilings (2 at 13,900, 6 at 14,700), so the digest sees it.

The candidate is to keep the cadence and make a sample cheap: resolve the
constant slot once to a host pointer, cache the resolved overlap object's host
pointer and re-resolve only when the slot value changes, and read the fields
directly. **The trap, recorded so it is not discovered by corrupting the route:**
guest MEM1 is big-endian, so a direct load must swap exactly as `mem_read32`
does, and any address that leaves MEM1 must fall back to the runtime accessor.
Predicted 2-5 percent if the runtime accessors are the ~6-instruction fast path;
the four-minute measurement settles it, and the digest is the gate.

**2. The dispatch entry itself.** `dolrecomp_call` is one indirect call per
block into the 518 MB composite, and whatever lookup sits behind it is the
remaining per-block constant. It lives in the pinned recompcore, so a change
there is a registered patch - but the model says ten instructions is 0.8
percent, and this is the one place where per-block work has not been looked at
yet.

**3. The cycle-accounting design, worth 10-15 percent of the emitted body.**
The largest measured lever in the project and the only one whose size is known
independently of the per-block model. The specific shape to price first is a
decision at **block entry** - `downcount > block_total` means no interrupt can
fall inside the block, so precharge and drop every per-instruction charge test;
otherwise take a precise path - which turns `cycle_block_prepaid` into a
constant within the block and makes the 9.8 percent residual dead code the
compiler can delete. The ledger argued against a duplicated precise tail on
code-size grounds, and that argument has a price: `scripts/ablate_chunk.py`
measures code-shape changes in about two minutes on real in-game state, so price
it there before deciding anything.

**4. The GX FIFO path off the main thread.** ~79 M of the renderer's 101 M per
rendered retrace, all synchronous on the main thread while Aurora's render worker
idles 94 percent of the time. Rendered-only, and the only item on the queue that
touches the p99 tail a player feels. Answer first what pins translation to the
main thread.

**5. The save-continue path**, decided in item 2 above: boot the game, load the
save the game wrote, reach control. It is the TTP-B reading and P4 milestone 9
at once.

## The protocol, as of today

- `scripts/bench_instructions.sh` is the primary instrument, and as of today it
  refuses to report a pair whose stop pc and turn count do not match the
  recorded reference for the ceiling, or whose high ceiling never reached the
  title screen or the play scene. **A normal stop was never evidence that the
  guest ran the route**: the broken fast-reject stopped normally, at
  `0x80301510` after 14,495,606 turns, and read as a 48.2 M per retrace win.
- The ceilings and their recorded stops: 13,800 → `0x80307EF4`, 29,737,920;
  13,900 → `0x80307EF4`, 29,937,744; 14,100 → `0x80307EF4`, 32,203,791 (the
  certified stop, digest `92dd816c...`); 14,700 → `0x8027FA30`, 40,502,699.
- **Quote instructions, not milliseconds.** Wall clock varied by a third between
  two processes running the same 190 retraces (30.47 ms against 40.74 ms) while
  the instruction counts for an identical ceiling differed by 0.29 percent.
- Measure the steady window (14,100-14,700) as well as the bench window, or a
  change is reported at better than its true weight: the two windows differ by
  42 percent in instructions per retrace.
- `scripts/recomp_chunks.sh` screens emitter changes on the hot ten in 8-15
  minutes against a 90-152 minute DOL-only rebuild; `scripts/ablate_chunk.py`
  prices a construct in about two minutes; `scripts/bench_rendered.sh` gives the
  rendered split. Read the run before reading its number.
- An iteration counts only when a measured number moved on a named path with the
  digest green and the run verified, or when a new instrument answered a
  question that changed a decision. Report to the user every three iterations.

## Fences

Inherited from v50 and not negotiable: no Nintendo data, original binaries,
generated game code, saves, captures, signing material or leaked source in the
repository; one BlueWake process at a time; no timing measurement during a build;
`ref/` is a pinned dependency whose uncommitted state *is* the patch series, so
reverse an edit or restore from `patches/` and **never `git checkout` a file
there**; register every `ref/` patch in `config/dependencies.lock.json`; do not
weaken a clause to make a log pass; do not re-run the refuted candidates listed
in v50.

And one this loop adds, because today produced it twice: **a generated artifact
must be able to prove itself.** The edge table's hash parameters and its table
disagreed, and the generator asserted a property it had not established. A
generator that writes an artifact writes the check with it - the emitted key list
and a test that every key is found - or the artifact is a guess with a comment.
