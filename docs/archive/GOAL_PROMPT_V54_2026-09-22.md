# BlueWake goal loop - v54 (2026-09-22): the rendered crash is the worker's frame transition

**User-started and nothing is blocked by the user; write the loop and go.** The
governing specification is [PRD.md](PRD.md), the operating procedure is
[GOAL_LOOP.md](../GOAL_LOOP.md), and the previous workstream page is
[GOAL_PROMPT_V53_2026-09-22.md](GOAL_PROMPT_V53_2026-09-22.md). What the PRD demands
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
certified route, headless and rendered.

## Where this loop enters

v53 landed the zero-charge tolerance: the dispatch loop now tolerates a bounded run of
blocks that charge no guest cycles, which is route-neutral headless, halves the play
window's turns, and buys **+2.67 percent of the play window** (398.3 M against 409.2 M
per retrace) with the route digest unchanged at 92dd816c... over 1,050 records.

Its rendered half then died, and the death is now explained rather than suspected.
The newest crash report says EXC_BAD_ACCESS, KERN_INVALID_ADDRESS at 0x28 on a thread
whose stack is entirely the FIFO translation worker: g_fifo_worker_main ->
RetailGxFrontend::flush -> ConsumingAuroraRenderSink::submit_packet ->
GxCoreSink::on_consumed_draw -> gx_aurora::core_plan_observer ->
aurora::gfx::gxcore::submit_draw_plan -> aurora::gx::set_logical_viewport ->
aurora::gfx::get_render_target_size.

The disassembly of that last frame names the mechanism exactly. get_render_target_size
loads Aurora's recording-frame pointer, then reads [ptr + 0x28] to compare the pass
count; 0x28 is the fault address, so **the pointer is null**. That pointer is
g_recordingFrame, which current_frame_packet() guards with CHECK(g_recordingFrame !=
nullptr) - a check the release build compiles out. Aurora clears that pointer in
end_frame and sets it again in begin_frame, and aurora_backend_present() runs both in
one function: present submits the finished frame (aurora_end_frame()), then opens the
next one (aurora_begin_frame()).

**So the FIFO worker records draws into Aurora's frame packet during the window in which
the packet does not exist.** It is not a stale pointer and it is not the tolerance: it is
the worker, landed earlier in this ledger when the translation moved off the main thread,
and it is the only thing that can be recording while the main thread is between frames.
Headless is immune because a headless run never opens a recording frame - g_initialized is
false, the aurora sink is never entered, and no headless run has ever crashed there. That
is why the rendered path broke and the headless measurement did not: not two facts to
reconcile, one.

The reading that the tolerance caused the crash is therefore withdrawn. The candidate's
rendered half is unverified, not refuted, and it is the first thing this loop settles.

## The queue, ordered by what is already priced

**1. Make the worker's recording legal at the frame boundary, and this is the loop's first
act.** Two facts have to hold at once: the worker must never touch Aurora's recording state
outside an open frame, and the main thread must never close a frame underneath a worker
batch that is mid-record. The smallest change that gives both is a single mutex held by the
worker across one batch's translation and by the main thread across the present transition
(flush_frame, end_frame, the present, begin_frame). It cannot deadlock, because the main
thread's only wait on the worker - g_fifo_drain at the guest-visible barriers - is never
called from inside that window. It is a host-side change in ref/recompcore, so it needs the
host build and not the two-hour composite rebuild, and it must be registered in
config/dependencies.lock.json and committed **inside** ref/recompcore the way the worker
itself was.

**2. Re-establish the rendered path, then re-ask the tolerance's rendered question.** With
the worker fixed: a rendered run to 14,700 that stops at the certified pc with no crash, and
frame timing over the steady window. Only then does the headless +2.67 percent become
admissible, or the candidate becomes a rendered regression and is reverted for real (with a
git show --stat that shows the source, this time).

**3. The host's per-turn cycle credit, the largest unpriced lever.** Half the host turns
credit nothing - 1,384,094 of 2,769,817 on the 400-retrace control - and the emitted body's
residual cost is the per-guest-instruction traffic to CPUState (1.71 gpr accesses plus 1.78
pc touches per instruction on chunk 0144) that the yield discipline forces. The design that
removes it, the prepaid/twin pair, is route-refused at 2.8x the turns with identical guest
cycles, which the zero-credit ratio explains. The host-side experiment is to stop charging a
full per-turn pass for a turn that advances nothing, and to stop gating the retrace boundary
on the credit stream alone. Host-side, no rebuild.

**4. The dispatch cache, bundled into the next full rebuild.** Designed, tooling in the tree,
a third of the dispatch entry's hot path, and it cannot be priced below the half-point the
ledger resolves without the rebuild - so it rides along with whatever else is header-shaped.

**5. The zero-charge exit's emitter-side alternative.** A block whose block_cycles is zero
charged its minimum would remove the exit at the source instead of tolerating it. It touches
emitted code, so it moves the digest and costs a rebuild; it belongs in the same bundle as 4.

**6. The rendered level and its tail.** Median 34.80 ms against a 16.667 ms retrace target,
p95 46.98 and p99 47.94 over the steady window, and the tail is shared with headless (two
headless passes share zero of 601 retrace times), so the p99 campaign is host hygiene on the
frame path rather than a hot spot. The worker's new serialization point is on this path and
its cost is a measurement this loop owes.

**7. The overlap guard and the hygiene debt.** Nine instructions at 94.14 percent of
boundaries for fourteen phase changes in 14,700 retraces; and the ref/ patch series still
needs every landed diff registered so a reverse-apply cannot destroy the tree.

## The protocol, as of today

- scripts/bench_instructions.sh is the primary instrument: two ceilings differenced,
  /usr/bin/time -l instructions retired, refusing any pair whose stop pc, turn count, title
  or play milestones do not match the recorded reference. Recorded stops: 13,900 ->
  0x80307EF4, 29,937,744; 14,100 -> 0x80307EF4, 32,203,791 (certified, route digest
  92dd816c...); 14,700 -> 0x8027FA30, 40,502,699.
- **Quote instructions, not milliseconds.** A count on a digest-gated route does not vary by a
  third between processes.
- **A count is not a price.** A census for a frequency, ablate_chunk.py for a code shape, a
  private build for a host change.
- **A crash is an attribution, not a verdict.** Symbolize it and read the fault address before
  naming a cause: this crash's 0x28 is what turned "the tolerance is suspect" into "the worker
  records outside a frame".
- **Freeze what you measure.** BLUEWAKE_BENCH_COMPOSITE for the artifact under test; one
  BlueWake process at a time; no timing measurement during a build.
- An iteration counts when a measured number moved on a named path with the digest green and
  the run verified, or a new instrument changed a decision, or a candidate was refuted with a
  number. Report to the user every three iterations.

## Fences

Inherited and not negotiable: no Nintendo data, original binaries, generated game code, saves,
captures, signing material or leaked source in the repository; one BlueWake process at a time;
no timing measurement during a build; ref/ is a pinned dependency whose uncommitted state *is*
the patch series, so reverse an edit or restore from patches/ and never git checkout a file
there; register every ref/ patch in config/dependencies.lock.json; do not weaken a clause to
make a log pass; do not re-run the refuted candidates.

Kept from v53: **a generated artifact must be able to prove itself**, and **a route driver is
a measurement, not the product**.

## What this loop's measurements changed in the queue (recorded the same day)

Item 1 is closed: the rendered crash is fixed and the rendered path is verified end to end at
14,700 - certified stop, exit 0, one present per retrace, no batch parsed without a frame.
Item 3 as written is **withdrawn**: a host sample capture of the headless play window puts the
per-turn host machinery outside the chassis loop and the device service under one percent of
the main thread, so the per-turn cycle credit is not the lever the queue ranked it as. What
the capture does put on the map: the emitted guest bodies are **70.1 percent** of the main
thread, the device service (the donor DSP LLE inside it) **10.6 percent**, and the chassis edge
service **4.5 percent**. Item 5 still stands and is the level the gate turns on: the rendered
play window measures 38.34 ms mean and 33.14 ms median a retrace against the 16.667 ms target.
Item 4 (the dispatch cache) and the emitter-side precharge belong in the next rebuild, which is
paused at 274 of 756 objects with its work preserved. Everything measured is in
[status/CURRENT.md](status/CURRENT.md).
