# Reorganization - 2026-09-17

**Why this document exists.** The loop has been working hard and moving slowly,
and the reason is structural rather than a shortage of effort. Two mechanisms
hold the progress rate down, and both are fixable. This is the reassessment, the
ranked plan that follows from it, and the decisions that belong to the user.

## Where the product actually is

BlueWake boots the retail `GZLE01` DOL and all 415 RELs into controllable Outset
gameplay on this Mac, on the product path, from the signed app: real keys, real
video, real audio, the game's own save, and a normal stop. That is the hard part
of the project and it is done. The v48 acceptance run passes every clause it is
asked except one:

    acceptance: TTP-A launch to play-scene: 257.88 s (play-scene at retrace 14050)
    acceptance: FAIL TTP-B launch to interactive is 926.30 s, over the five minute target

TTP-A is at 91% of authentic speed and passes. The failure is entirely "launch to
the first moment a human can act", and it decomposes into two numbers that need
different answers:

| segment | retraces | wall | guest | speed |
| --- | --- | --- | --- | --- |
| launch to play scene | 0 - 14,050 | 258 s | 234 s | 91% |
| play scene to control | 14,050 - 20,044 | 668 s | 100 s | 15% |

The first is nearly authentic already. The second is the whole complaint.

## Cause one: the gate gated the host's schedule

`scripts/bench.sh` admits a change only when the route digest is unchanged, and
the route digest is the project's defence against fps bought by changing guest
behaviour. That instinct is right. The implementation was not: the digest
included `[cycle-delivery] external[...]` records carrying the guest's exact
`pc` at each interrupt delivery, and a `[clock] summary` carrying the route's
exact cycle total. Both are *host turn-boundary* quantities - where the host
happened to end a turn, not what the guest did.

The consequence is a standing deadlock: every mechanism that shortens a host
turn - which is where the play scene's time is - moves those two fields, so it
is inadmissible by construction, no matter how identical the guest behaviour is.
The largest measured win in the project, Dolphin's own DSP idle skip, was
blocked on two cycles out of 114,210,000,002 and a few bytes of pc, with
byte-identical audio. That is not a correctness question; it is a gate that
cannot distinguish the thing we are changing from the thing it is measuring.

D1 already decided this in principle on 2026-09-14 - it canonicalised the
delivery-timing *aggregate* out and wrote that "delivery timing is gated as a
bounded quantity instead of an equality" - but the bound was never implemented.
Somebody forgot to finish D1. Decision **D2** finishes it: the equality keeps
every guest-state record, the record and delivery counts, the milestones, the
normal-stop pc and the card, and the schedule is gated as a bound
(`scripts/route_digest.py`, ±1 cycle per delivery and ±8 cycles on the route
clock), reported on every comparison so it cannot hide.

This is the single most valuable thing to fix in the project, because everything
else is downstream of it. It is also already done and verified - see below.

## Cause two: one host turn is one guest block

The play scene pays a full device-service pass - DSP slice, deadline
computation, interrupt publication, card runtime, cycle-domain bookkeeping - for
every ~20 guest cycles, because `cmake/composite/dispatch_loop.c` returns to the
host after one dispatch when no edge service is registered. The budget it is
given is 256 cycles inside a 1024-cycle horizon, so the pass is being paid 14 to
56 times more often than it was designed for. That is why the same host runs the
retraces before the play scene at 57-59 fps and the play scene at 8.6-9.5 fps.

The obvious fix - register an edge service and let the chassis run the budget
out - has now failed twice, and both failures are informative. A no-op edge
service gives 11-14x on the turn count and stalls the guest in early boot. The
refined version in the tree, which publishes interrupt sources and applies the
same delivery contract between blocks, *also* stalls.

**That stall is now named, and it is not a device.** Resolving the addresses in
the stalled log against the matching decompilation's GZLE01 symbol table: 93% of
the sampled pc are a single instruction inside `SelectThread` (`0x80307EF4`,
`SelectThread+0x14C`), in the OS reschedule path, with the milestone summary
reading all zeros. The guest is not waiting on a device and not in the game's
idle loop; the main thread is never selected. The `OSDisableInterrupts` /
`OSRestoreInterrupts` addresses that first made this look like an interrupt-path
problem are the scheduler's own critical-section bracketing.

That moves the question to "what wakes `SelectThread`", which is answerable. The
decrementer is the host's own candidate: the per-turn epilogue delivers it under
`g_guest_decrementer_pending && (msr & PPC_MSR_EE) && cpu.exception == 0` with no
`bluewake_scheduler_interrupt_safe(pc)` test, while the external path has one. A
chassis that ends a turn at a pc the host did not choose delivers the decrementer
at points the per-block schedule never reached - and `SelectThread` is the
function that consumes it. That is a specific experiment rather than a third
attempt at widening the budget.

The mechanism is still the right one - the diagnosis stands, and the headroom is
real - but it needs to be finished by finding what the boot actually needs from
the turn, not by widening the budget and hoping. That is P2 below, and it is the
only lever that can move the play scene far enough to matter.

## What is now settled, and what it bought

**The DSP idle skip is the shipping default (D2).** Our adapter stepped the DSP
eight cycles at a time through `Interpreter::Step()`, which never reaches the
donor's idle check, so the microcode's wait loop - about 1.35M DSP cycles per
retrace to produce ~800 samples - was executed instruction by instruction. It is
now `BLUEWAKE_DSP_BATCH=128`, and `BLUEWAKE_DSP_BATCH=8` still restores the exact
cadence for a regression check.

**The divergence is bounded, measured and schedule-only.** Across the four
settings that engage the skip (16, 32, 128, 256) against batch 8, on the full
route: the same 1,050 records, the same 1,024 deliveries, 899 of 1,024 delivery
cycles bit-identical, the largest delivery move exactly one cycle, the route
clock -2 cycles in 114,210,000,002, and all four settings identical to each
other across a 16x spread in how much the skip is used. A behavioural difference
would scale with the lever; this does not. Audio PCM is byte-identical
(`3260ead5...`, 30,075,948 bytes, 235.0 s).

**Measured on this host, same composite and card, full 14,100-retrace route:**

| | batch 8 | batch 128 |
| --- | --- | --- |
| wall / guest | 254.45 s / 235.00 s | 186.86 s / 235.00 s |
| real speed | 92.4% | **125.8%** |
| boot phase | 237 s | 135 s (1.76x) |
| play-window median fps | 10.40 | 10.87 |

It buys the boot and 26% of headroom over authentic speed; it does not buy the
play scene, where the DSP has real audio work rather than a wait loop. That is
stated plainly because it is the difference between "the launch is now brisk"
and "the game runs at speed", and only the first is true.

Seven workstreams. W1 gates W2, and W2 is the only one that moves NFR-001. W3 is
worth doing only after W2's remeasurement. W4 is the long pole and it starts with
a measurement, not a change. W5, W6 and W7 are cheap, independent of the rest,
and worth landing early because each gives back loop throughput rather than a
number.

### W0 - Done: the gate and the DSP idle skip

D2 re-baselined the route digest so guest state is gated by equality and the
host's turn schedule by a bound, and Dolphin's idle skip became the shipping
default. Measured 254.45 s to 157.81 s on the full route at unchanged host turns
and peak RSS, 92.4% to 148.9% of real speed, byte-identical audio, 217/217 tests.
It buys the launch and 26% of headroom; it does not buy gameplay speed.

### W1 - Done: the chassis budget window now runs the identical route

Ten experiments, five refuted hypotheses, two instruments and one wrong argument
later, this is closed. The chassis path produces the accepted route digest
**UNCHANGED** on the full 14,100-retrace route at **181.5% of real speed** against the
shipping build's 148.1%, in **148,527,447 host turns** against 521,124,876, with a
byte-identical guest route: 1,050 records, normal stop at `pc=0x80307EF4`, play-window
median fps 11.30 -> 16.60.

**The cause was not a service, a cadence or a budget.** The shipping turn body is not
generic: it carries pc-keyed actions, and the decisive one is at `0x803193AC`, which
arms the JAudio DSP boot handshake. Those fire in the shipping build only because it
takes a host turn at every block boundary, so every watched pc is visited; a chassis
that returns on interrupt conditions alone skips all of them. That is why the guest lost
its own audio initialisation at retrace ~1.8 and everything downstream followed - 0 DMAs
from the DSP against 71,538, 17 ARAM transfers against 4,310, two DSP-mailbox reads
against 123,067. `edge_intercepts.c` was written to expose exactly that address set, is
unit-tested, and had never been called: dead code.

**What fixed it, in three steps, each measured.** Return to the host when
`bluewake_edge_requires_host` matches, with `host_canonical_linked_pc(pc)` as the
canonical argument - a wrong argument the first time, which recovered the whole audio
layer and still missed every milestone that watches a canonical address. Then the one
observation outside that table: `new_game_intro` at `0x80018554`/`0x8001199C`, one-shot,
so free after it fires. Then cadence rather than state: the overlap phase, which is
recorded as 5 against shipping's 6 while the overlap object is bit-identical at 201 of
201 retrace-aligned samples in the two configurations.

The lesson worth keeping is that the two failed attempts were not wrong about the
mechanism - they were incomplete about the contract. A host turn is not a device service
pass; it is the period at which every pc-keyed host decision is made, and the chassis has
to reproduce those decisions at the same points, not merely the ones about interrupts.

### W2 - Done: promoted as far as the host allows, and re-ranked from a fresh profile

The acceptance run that would promote `BLUEWAKE_CHASSIS_BUDGET` to the default could not be
taken: the focus preflight refused in 0.9 s because another application holds the key
window. That is the W6 wall, so the chassis stays behind its switch and the promotion is
one line waiting on an idle host. Everything else was done.

Re-profiled the chassis build - the fastest correct configuration - on an 18,000-retrace
route so the play window lasts minutes: 71.4% of a play-scene frame is inside
`selected_dispatch`, 70.8% is the translated bodies under it, the DSP LLE clock is 7.4%,
the edge service 6.8%, and the rest is single digits. W3's items came back at 1.2%, 0.7%
and 0.2%.

### W3 - Closed as superseded, not done

The three items this workstream named were per-turn costs, so W1 removed most of them along
with 3.5x of the turns: a fresh profile measures them at 1.2%, 0.7% and 0.2%, about 2.1%
together. Touching the alias fast path and the allocator for 2% is not a trade worth
making, and the numbers say so. W3 closes with that as its evidence rather than being
quietly dropped.

### W4 - The recompiler is the whole frame, and its first question is answered

This is not a dispatch-ABI problem. The split the plan asked for came back 6,980 of 7,035:
the translated `func_*` bodies are 70.8% of a play-scene frame and the dispatcher's own
entry and exit are unmeasurable at this sample size. It is the executed translated
instruction stream, and the guest's own hot spots are named - `CalcDivideInfo__15cCcD_DivideArea`
at 18.8% (collision divide-area) and `fpcEx_ToLineQ__FP18base_process_class` at 11.2%
(actor line-queue scan) are 30% of a frame between them, with a long tail across hundreds of
bodies behind them.

Authentic speed needs 1.81x from here (play-window median 16.6 fps against 30), and the host
cannot deliver any of it, because the host is no longer where the time is: that 70.8% has to
become roughly 2.4x cheaper, or the guest has to do less. Next step is one of those two
bodies measured in isolation, to establish *why* its translated form is expensive. The 32 KB
local frame, which forces address materialization for every local access above the 16 KB
immediate range, is the first named candidate and a generator change is the only fix in that
class - measure before changing the generator.

**Retraction: the guard is emitted per instruction but skipped per block, so it is not the
cost.** An earlier version of this section read `cycle_block_prepaid = true` occurring zero times
in the hot chunk as the guard running on every instruction. The flag is assigned from
`dolrecomp_block_can_precharge(ctx, N)`, a runtime helper that returns true whenever the block's
cycles fit the remaining deadline budget, so grepping for a literal measured nothing. That body
has 4,104 instruction labels and 1,119 precharge evaluations over blocks of 1 to 6 cycles
against a 256-cycle budget: the guards are skipped most of the time, and the count is a
code-size fact rather than a dynamic one. The experiment that removed them was invalid twice
over - it double-charged every precharged block, and it removed pc-stamped re-entry points - so
it neither measured the guard nor established that the re-entry points are load-bearing.

**What W4 actually has to explain.** 70.8% of a play-scene frame is translated bodies and the
play scene runs at 28% of authentic speed, which is 7.4 ns of host time per guest cycle against
the console's 2.06 ns cycle time. The candidates are the memory-based register file, where every
guest register access is a load or store through `ctx->gpr[]`; the 1,152 memory helper calls in
the one body read so far; code size, at 20.8 lines of C per guest instruction and a 4,104-case
jump table; and the host's per-turn work. Separating "expensive per instruction" from "the guest
does a lot of work" needs a number nobody has: cycles per guest instruction inside a body.

**The cheap way to that number is a standalone microbenchmark.** Link one chunk object against
the runtime with a synthetic `CPUState`, call `func_802416E0` in a loop, and time it. Minutes,
no composite rebuild, and it speaks directly to the fork above.

A hand prototype in a single chunk is possible and costs minutes, but only in the cheap form:
mirror `downcount` in a function-local, write it back before every memory helper call and
before every return, and reload it after. That removes roughly two of the five instructions the
guard costs, and it is fiddly enough across 1,152 helper calls that the generator-time version
above is the better use of the effort.


**W4 update, after the measurements: no single piece is large, and the next instrument must
measure time rather than size.** Four prices, each from one variant of the same chunk built with
its real flags (one file, `__text` before and after, seconds, no composite rebuild): guard bodies
-4.3%, standalone `ctx->pc` stores -10.8%, collapsing the duplicated guard arms -2.0%, and an
inline FP-enabled fast path at the 576 `ppc_fp_available_inline` sites -1.0%. About **18%**
together, and the helper - the item that looked structurally worst, a trivial two-instruction body
called out of line once per FP instruction - is the smallest of them.

Two things follow. First, the envelope items are still worth taking, because 18% of the translated
code is 18% of 70.8% of a frame, but none of them alone is the 1.81x. Second, the piecewise prices
may understate the envelope's true share, because the compiler shares duplicated envelope code
across neighbouring instructions, so deleting one piece charges it only its marginal size. That
cannot be settled by editing the emitted C: the labels the envelope would delete are the targets of
roughly 4,000 translated `goto`s, so the variant will not compile and rewriting the gotos would
change what is measured.

So the next step is the microbenchmark that has been on this list for three entries: link one chunk
against the runtime with a synthetic `CPUState`, call the body in a loop, and time it. It is the
only instrument that gives cost per guest instruction in *time*, which matters because the guard
pieces are skipped at run time by precharge and so are worth less dynamically than their code size
suggests, while the work lines are worth more. Everything that remains needs the one planned
rebuild that regenerates the chunks, and it should be entered with that timing number in hand
rather than with a code-size proxy.

**Retraction: the pc store is load-bearing, so the pass has no large item left.** An earlier
version of this section named dropping the per-instruction pc materialisation (10.8%) as the pass's
first change, arguing that nothing reads `cpu->pc` mid-block in a way that decides anything. Running
it on one chunk disproved that in four minutes: the route went from 134.39 s to 457.21 s, host turns
from 148.5M to 586.5M, the final pc from `0x80307EF4` to `0x80303A7C`, and the digest diverged - while
the play window's median fps *rose* to 30.82, which is the one number in that run that means nothing,
because it measures a derailed guest. The pc is the host's resume address: control leaves the
generated code at every block boundary, the host re-dispatches at `cpu->pc`, the chassis edge service
reads it to decide interception and interrupt safety, and `bluewake_scheduler_interrupt_requires_host`
tests it. It must be materialised at every point control can leave, which in this emitter is every
instruction. The check asked who *reads* pc inside the helpers - eleven, all `fprintf` arguments -
when the question is who *consumes* it to decide where execution continues.

So the envelope now stands at guard bodies 4.3%, collapsed guard arms 2.0% and the FP helper 1.0%:
about 7%, none of it large, and none of it the 1.81x. The measurements say a recompiled-C emitter of
this shape has a floor of several host instructions of bookkeeping per guest instruction, and that
no available trim reaches the target.

**And there is no large lever left in this architecture.** The pieces identified across this
workstream - per-instruction envelope 17%, inline memory-access checks ~18%, guard bodies 4.3%, FP
helper 1% - do not sum to the 1.81x the play scene needs, and the removable ones are the small ones.
An `lwz` emits 22 lines of C for 4 lines of work; a `bc` emits 27 for 6. The 81 machine instructions
per guest instruction are ~17% envelope and the rest work, dominated by the inlined access path
(`mem_read32` carries the alias-flag load, the tagged test, the bounds check and the RAM pointer load
before the load itself), with ~900 memory operations, ~780 branches and the FP path behind it.

An earlier version of this section proposed the LLVM backend as the candidate with the right shape.
It is not: `ref/DolRecomp/src/backend/llvm/` was explored hard between 2026-08-25 and 2026-09-01 and
DECISIONS.md already records the outcome - the strict legacy-prefix tier "does not expose the
required 1.5x opportunity" at 25.00 user seconds, the DOL-only route reaches a null archive and
enters retail `PPCHalt`, and "speed cannot justify replacing a guest-inexact route". Re-opening a
closed decision needs new evidence, and there is none.

So the honest position for the user: reaching 30 fps means halving host cycles per guest cycle from
28.4 to about 14, and nothing measured here does that. This is a finding about where the cost lives,
not a plan to remove it, and the next decision about W4 is the user's - whether to accept a lower
presentation target, fund an architecture change with a measured basis, or stop at the boot and
launch improvements W1 already delivered.

The target is unchanged: the play scene runs at 16.6 fps against 30, which is 1.81x, and 70.8% of
a play-scene frame is these translated bodies.

**One lever of real size, found last: the `switch (ctx->pc)` costs 54% of the code and the whole
32 KB frame.** Every generated function opens with a `switch` over all 4,104 instruction addresses,
and clang lowers it by materialising a 4,104-entry jump table **on the stack** - 4,104 x 8 bytes is
32,832 against a measured 32,656-byte frame - and copying it at every entry. Deleting that block from
the emitted C takes `chunk_0144_text1_802416E0.c` from 1,345,224 to 615,164 bytes of `__text`
(**-54.3%**) and makes the frame warning disappear.

The body is entered **27,263,164 times** over the route (measured, digest UNCHANGED), so the table
setup is about 1.9% of the route's retired instructions: the switch is a large code-size item and a
small dynamic one, and the same caution applies to every share in the attribution tables above - code
size is not time.

What makes this the pass's best candidate is that it is cheap to do correctly and cheap to verify:
replace the per-function pc switch with a `static const` resumption table, or any scheme that does not
build one per call. The size effect is already measured at 54.3% of one body; the dynamic effect will
be a couple of percent plus an I-cache improvement that cannot be measured without the rebuild. It
goes in the same pass as the envelope items.

**The pass's content, and it is now a finishing problem: the per-call prologue.** Every chunk function
opens with `switch (ctx->pc)` over 4,096 cases covering a contiguous 16 KB range, and clang lowers that
by building a 4,096-entry jump table **on the stack** - 32,832 bytes, matching the measured 32,656-byte
frame - and copying it at every entry. Replacing it with a `static const` table of label addresses and a
computed goto is exactly equivalent for a contiguous range, and it is measured:

| | baseline | one body | eight bodies | **32 bodies** |
| --- | --- | --- | --- | --- |
| play-window median fps | 16.64 | **19.35 (+16.3%)** | **26.39 (+58.6%)** | **29.92 (+79.8%)** |
| p99 frame time | 86.97 ms | 78.63 ms | 64.43 ms | 58.88 ms |
| wall / guest | 174.6% | - | 208.5% | **246.3%** |
| route digest | `92dd816c...` | UNCHANGED | UNCHANGED | **UNCHANGED** |

The digest, the host turn count and the normal stop are identical in every run, so the route is the same
route and the gain is entirely the prologue. **The play scene is at 29.92 fps against the PRD's 30 fps
authentic presentation baseline - 99.7% of it - and 217/217 host tests still pass.**

What remains is to make it reproducible rather than local. The eight bodies are hand edits of gitignored
generated files, invisible to `git status` and lost by regeneration; the emitter change in
`ref/recompcore/DolRecomp` is the form that ships - parse the contiguous case range, emit a static label
table, dispatch with a computed goto - followed by regeneration and the full rebuild that the earlier
entries priced in hours. Applying it to all 757 chunks rather than the eight hot ones should be worth
more than what is measured here, and the emitter binary can be built on its own to check the change
compiles before any regeneration is attempted.

Also for the pass, from the same evidence: collapse the duplicated
`if (cycle_block_prepaid) { ... }` / `if (!cycle_block_prepaid) { ... }` pair into one budget test and one
conditional charge (provably equivalent, 2.0% of code). Not in the pass: per-instruction pc
materialisation, which is load-bearing through a path line-level reasoning does not expose - refining the
removal to spare the early-return helpers gave the identical divergent digest, and the invariant "keep a
store whenever a `return` follows" leaves 1 removable store out of 3,782.

### W5 - Settle the TTP-B bar, because it is a definition and not an engineering result

20,044 retraces is 334 s of authored content at an authentic 60 Hz before
emulation speed is a factor, so a cold new game cannot reach control inside 300 s
even at 100% emulated speed. The PRD forbids a save-state warm start; it does not
forbid continuing from a save the game itself wrote, which is the game's own
persistence rather than captured initialization state. That path - boot, title,
file select, load, control - fits inside the bar with room, and it is P4 milestone
9, which is half done (write proven, quit and reload open). Closing it is cheap
and it settles the bar with the same run.

### W6 - Make validation independent of the user's desk

Every key-driven verification needs the app to hold the key window, and macOS 26
refuses `activateFromApplication:options:` while another application has focus, so
the acceptance pass, the save artifact and the reload test all require an idle
host. That gates half the loop's throughput on the user not using their own Mac,
which is a large part of why iterations feel slow.

Three routes, cheapest first. Make the app take its own window at launch in
Aurora's `window::initialize`/`show_window` - the 09-14 entry nominated this as
the fix, so first confirm whether it ever landed as a patch. Failing that, drive
the route through the pad layer for harness purposes only, keeping the
human-press run as the sole keyboard evidence; the loop already distinguishes a
route driver from evidence, and this is that distinction applied. Failing that,
run acceptance in a second login session.

### W7 - Stop paying the ledger's context cost on every iteration

`docs/archive/status/CURRENT.md` is 758 KB and contains literal duplicated paragraphs -
D1's appears twice inside the DSP entry, and the same repetition is visible in the
Route B banners. There are 25 `GOAL_PROMPT` versions. Every agent that starts work
reads this before it can do anything, so the sprawl is paid on every iteration by
every participant. Move durable state into the small files that already exist
(this plan, `DECISIONS.md`, `PERFORMANCE.md`, `BLOCKERS.md`), make `CURRENT.md` a
rolling window with a hard size, and fold the superseded `GOAL_PROMPT` drafts into
one archive file. Cheapest per-iteration saving available, and it depends on no
measurement.

### Order of work

    W1 -> W2 -> W3 -> W4
    W5, W6, W7 in parallel, all cheap

W0 is done. W1's first experiment is a measurement with no behaviour change, so it
costs one build and one bounded run, and it either confirms a small fix or
redirects the search. That is the shape every step in this plan should have.

## Decisions the user owns

1. **D2**: accept the re-baselined route digest - guest state by equality, host
   schedule by the ±1 / ±8 bound - and keep the DSP idle skip as the shipping
   default. Rejecting it costs 1.68x across the route and 1.76x on the launch.
2. **TTP-B**: is the five-minute bar read against a cold new game, which the
   PRD's own speed requirement makes unreachable at authentic speed, or against
   a human reaching controllable Outset gameplay, which the save-continue path
   satisfies and which is what the clause's wording says?
3. **Priority**: if the play scene at ~9 fps is the felt problem rather than the
   launch time, P2 should displace everything else, including the acceptance
   runs, because every acceptance run at 15% speed costs four minutes more than
   it should and will keep doing so until P2 lands.

## What not to do

Do not re-propose batching turns without the per-block contract (twice failed,
recorded). Do not weaken TTP-A or TTP-B to make a log pass. Do not run the
acceptance test on a loaded host - it needs an idle host for key focus, and its
wall-clock numbers are read from the same clock. Do not treat a document as an
iteration: an iteration moves a number on a named path.
