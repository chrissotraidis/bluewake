# Loop Reorientation — 2026-08-22

**Author:** external reviewer (out-of-loop), at the user's request.
**Audience:** the autonomous implementation loop (`docs/GOAL_LOOP.md`).
**Status:** advisory. Read this at Step 0 alongside `PRD.md` and `CURRENT.md`.

---

## 1. Verdict

You are on the **right route** and your low-level work is real: P0–P3 are
genuinely passed, the composite links and validates, and your P4 boot trace is
an impressive, authentic reconstruction up to the logo's first draw.

But you are currently in an **anti-stall violation** (`GOAL_LOOP.md` §2.10 and
§6). You have spent most of 2026-08-22 tracing a downstream symptom — ARAM
`JKRExpHeap`/`allocBack` corruption at `0x802B5FCC` — that **only appears when
your own rejected timer workaround is active**, while the actual root blocker
goes unfixed. The experiment shape "trace one function-boundary earlier, find a
garbage pointer, remove the probe" has now repeated well past three materially
identical iterations.

Burn-rate for the day, for the human reading this: **221 evidence logs, 82
commits, 34 same-day branches, and at least 14 add-then-remove instrumentation
reversions recorded in `CURRENT.md`** — with net-zero progress on the runtime
subsystem that is actually blocking the gate.

## 2. The single root cause you have already proven (but not named)

Every symptom you are chasing traces to one missing subsystem: **the runtime
has no authentic time model.** Your own evidence establishes this end to end:

1. `runtime/host/src/main.c:4533` — the composite "does not emit downcount
   charges," so the host advances **`timebase += 1` per dispatched block**
   (`:4543`). Guest time is fabricated at one tick per block, decoupled from
   any real cycle count.
2. The decrementer is a **diagnostic gated behind `BLUEWAKE_ENABLE_GUEST_DECREMENTER`**
   (`:662`). On the default authentic route the SPR-22 decrementer never
   decrements and never fires (`:4546`), so the display alarm queued by
   `OSSetAlarm` (deadline `0x22F4DC0B`) is never delivered → the main thread
   sleeps in `JFWDisplay::threadSleep` forever → boot stalls at the logo. This
   is exactly the P4 milestone-2→3 wall in `CURRENT.md`.
3. When you *do* enable the decrementer, it is decremented by `elapsed_timebase`
   = +1/block (`:4552`) — a rate that is **not coherently related** to the
   timebase, to `__OSGetSystemTime`, or to the guest's real cycle-based alarm
   deadline. So execution advances on an inauthentic clock, scheduling diverges,
   and memory diverges with it.
4. The ARAM heap corruption is wired **into that broken decrementer path**
   (`:4566`–`:4576`, the `heap_host_delta` snapshot past 7,000,000 blocks). It
   is a consequence of running the guest on an incoherent clock — not an
   independent allocator bug.

You have even proven it is not an allocator bug: `tests/test_aram_allocator.py`
shows retail `allocBack` is correct, and `CURRENT.md` records that **"No single
coherent 32-bit `allocBack` operand tuple explains both corrupted writes."**
That is the tell. Algebraically-incoherent corruption downstream of a
fabricated clock is a **poisoned-execution symptom**, not a defect at the point
where it surfaces. Tracing it one boundary earlier will never terminate.

## 3. What to stop doing

- **Stop the ARAM / `allocBack` / `mTailUsedList` / message-object hunt.** Mark
  `BW-P4-000x` (the malformed-ARAM-request blocker) as **`SUPERSEDED_BY_DECISION`**:
  it is not reproducible on the authentic route and is contingent on a rejected
  hook. Do not open another "trace `prepareCommand`/`orderAsync` one store
  earlier" iteration.
- **Stop building removable `opt-in` host hooks** for alarm/decrementer
  delivery and then deleting them. That cycle has run ~14 times. Each one
  produces an inauthentic trace and no promotable change.
- **Do not** force a wake, inject a tick, synthesize an alarm callback, or
  reorder the alarm queue. (You already know this; it is restated because the
  pressure to do it will come back.)

## 4. What to do instead — one blocker, promoted for real

Reclassify the active work as a single **`CPU_RUNTIME` blocker: "no authentic
guest time base + decrementer delivery."** This is the earliest unmet root
blocker under `GOAL_LOOP.md` Step-1 priority (a hang/hardware-service defect
that gates authentic progression). Fix it **in the runtime as a promoted
subsystem**, not as an env-gated diagnostic.

The falsifiable hypothesis, in your own template:

> Because the composite emits no cycle charges, the host's `timebase` (+1/block)
> and the SPR-22 decrementer are incoherent, so `OSSetAlarm` deadlines are never
> reached and no `DecrementerExceptionCallback` runs. If the runtime drives
> `timebase`, the decrementer, and `__OSGetSystemTime` from **one** monotonic
> block→cycle clock, and delivers the decrementer exception when the guest's
> decrementer crosses zero against that same clock, then the display alarm fires,
> `JFWThreadAlarmHandler` runs, and main resumes for a second draw — while the
> authentic no-disc control still stops normally (control observable unchanged).

Two viable implementations, in preference order:

1. **Coherent synthetic clock (smallest change; do this first).** Make one
   monotonic guest cycle counter the single source of truth. Derive `timebase`
   (TBL/TBU), the SPR-22 decrementer, and `__OSGetSystemTime` from it with the
   real GameCube ratios (timebase = bus clock / 4; decrementer ticks at the
   timebase rate). Deliver the decrementer exception when the decrementer
   register crosses zero *against that clock*, running the real translated
   `DecrementerExceptionCallback` (you already reverse-engineered its
   semantics: remove head, clear handler, rearm periodic, invoke on the
   exception context, reschedule, restore). This is **on by default**, not an
   opt-in. It does not require regenerating the composite.
2. **Regenerate the composite with per-block cycle charges** (larger, cleaner,
   later). If the synthetic clock cannot stay coherent through scheduling, the
   real fix is a CPU ABI that emits `downcount` charges so `timebase` advances
   by true instruction cost. Note `runtime/host/src/main.c:4533` already
   anticipates this ("consuming charges when newer generated code supplies
   them"). Treat this as a `PIVOT_SUBSYSTEM` on the generator, gated by a
   determinism/regeneration test — not an ad-hoc edit to generated C.

Verify in tiers (`GOAL_LOOP.md` Step 8): a synthetic decrementer/alarm unit
test first (public, no disc), then the composite smoke, then the authentic boot
route. Success = **a promoted, default-on time service that produces the second
logo draw and the title/opening request without any removable hook.**

## 5. Only then, re-examine ARAM

Once the clock is authentic and default-on, replay the boot and **re-observe
whether the `0x802B5FCC` ARAM request is still malformed.** Three outcomes:

- It disappears → it was a timing/scheduling artifact of the fake clock.
  Confirmed by construction; close it.
- It persists but the corrupted values are now *coherent* → it is a real
  translator/runtime ABI or memory-ordering bug. Now — and only now — a narrow
  reproducer at the translator boundary is the right move (not the heap
  allocator).
- It persists and is still algebraically incoherent → suspect a translator
  memory-write-ordering defect upstream of the heap, and reduce there.

In all three cases you learn the answer from **one authentic run**, instead of
another N boundary-tracing iterations on a poisoned one.

## 6. Process note (for the loop and the human)

The §6 anti-stall protocol exists precisely for this. The signal that it should
have triggered days of iterations ago: the *same* "next smallest action —
trace one boundary earlier; do not synthesize" sentence recurs down the whole
back half of `CURRENT.md`. When the next action only ever moves the probe one
function upstream and never changes the *layer* of the hypothesis, the blocker
is misframed. Zoom out to the subsystem (here: time), not in to the next
instruction.

Consider also pruning the 34 same-day `codex/bluewake-p4-*` branches once this
lands; they encode the thrash, not the state, and `CURRENT.md` already holds
the authoritative history.
