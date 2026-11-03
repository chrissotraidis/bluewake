# Host Interception Inventory

**Recorded:** 2026-09-01  
**Owner:** `BW-P4-0066` generated-edge-to-host interception contract

## Why This Exists

The rejected composite dispatch loop proved that budget and exception state do
not fully describe a safe generated edge. It reduced host turns by 64.5%, but
skipped host-owned address interception and failed before title readiness with
zero DSP interrupt delivery. This inventory defines the contract that must be
made explicit before chaining can be retried.

## Current Boundary

The ordinary loop in `runtime/host/src/main.c` begins near line 3811 and does
not call `mod->dispatch` until near line 8889. A static scan finds 555 PC-related
references in that interval. Most are diagnostics, route observations, or
bounded reports. The following groups can mutate execution or required device
state and therefore cannot be skipped by a chained edge.

## Semantic Intercepts

### CARD runtime

`runtime/host/src/card_runtime.c` owns 21 fixed public/synchronous/async CARD
entry addresses. Dispatch changes registers and PC; callback service can also
change CPU state. The complete address switch belongs behind a typed predicate,
not a duplicate list in the composite.

### DVD compatibility

- `0x8030F2B0`: path-to-entry bridge
- `0x8030F618`, `0x8030F5A4`: open/fast-open bridges
- `0x80017FD8`: archive path-to-entry bridge
- `0x8030FADC`, `0x8030FBCC`: async/synchronous read bridges, including guest
  memory writes and nested callback delivery

These entries mutate GPRs, PC, DVD command state, guest memory, or callbacks.

### REL lifecycle

- `0x80240744`: archived REL materialization and activation; may consume the
  turn without translated dispatch
- raw module-1 entry `0x81E000D4` and the dynamic raw-base `+0xD4` alias:
  redirect to the linked composite entry

The dynamic alias means a fixed literal table alone is insufficient.

### DSP and interrupt lifecycle

- `0x803193AC`: arms the JAudio DSP task boot handshake. Skipping this address
  is the direct observed explanation for zero DSP work in the rejected loop.
- External interrupt delivery is tested before dispatch using current MSR,
  scheduler safety, and pending PI state. Exact cycle budgeting limits when
  this is needed, but chaining must still yield when delivery is eligible.

### Callback service

CARD callback return/poll service runs before dispatch and is stateful. Its
need to yield is not represented by a fixed guest address alone. Nested DVD
callback delivery already has its own exact dispatch-preparation callback.

## Observability Intercepts

Boot milestones, route summaries, scene state, diagnostic traces, and visual
capture predicates currently observe many fixed PCs in `main.c`. They should
not define product semantics, but the promoted exact route oracle depends on
their observations. A successor must either yield at the oracle's observation
addresses or move those observations to stable subsystem events before its
digest can be compared honestly.

## ABI Direction

The next design should expose one host-owned edge decision to the composite:

1. Fast reject ordinary generated addresses.
2. Yield for a semantic fixed address, dynamic REL alias, pending callback, or
   deliverable interrupt.
3. Preserve exact budget/deadline and exception boundaries.
4. Keep the address inventory in one host-owned typed module.
5. Treat diagnostic/oracle observation as a separate declared class.

The first implementation tier is a focused predicate test over every semantic
address and negative ordinary addresses. Only after that passes may the full
edge loop be restored for one 700-retrace qualification. Acceptance remains
the exact 1,050-record route, all 1,046 invariants, and at least 40% fewer host
turns. No numeric loop-limit variants are authorized.

## Tier 1 Result

The address-only tier is implemented in `edge_intercepts.c`, with CARD address
ownership retained in `card_runtime.c`. Its focused test covers all 30 current
semantic fixed addresses, the dynamic module-1 alias, and ordinary negatives.
All 216 tests pass. A behavior-neutral 700-retrace qualification matches all
1,050 route records, 1,046 invariants, and the accepted 62,038,491 turns.

Static follow-up narrows the next tier. CARD callbacks are queued only while a
CARD address intercept already owns the turn, and the callback-return sentinel
is outside generated code, so both paths structurally yield to host service.
Interrupt eligibility should become a one-cycle exact budget through the
promoted observation/deadline contract. Focused tests must prove those claims;
new state callbacks are not justified unless either proof fails. The generated
loop remains removed until those boundaries and the callback ABI are covered.

## Tier 2 Result

The composite callback ABI and dispatch primitive now exist but remain
default-inert. Focused tests prove legacy one-dispatch behavior and every
fail-closed edge boundary, including no cycle progress on the first dispatch.
All 217 tests and the exact behavior-neutral 700 route pass.

Before the host installs the callback, the route-oracle observation addresses
must become an explicit second class beside semantic intercepts. This is needed
for honest digest comparison without pretending diagnostic observations are
product semantics.

## Tier 3 and 4 Result

A pure selective predicate failed at the first external delivery. Promoting the
callback to a mutable edge service made early delivery independent of edge
topology. Static adjudication of the next divergence also exposed PE-finish as
semantic behavior incorrectly gated by diagnostic observations; it now belongs
to the GX draw-done `PC/LR` boundary.

The resulting 700 route recovers every milestone and subsystem count and keeps
the first 1,024 delivery records exact, but its later aggregate delivery hash
differs. It also gains only 10.1% user CPU despite 67.6% fewer outer turns.
Therefore this inventory is complete enough to reject per-edge host service as
the performance mechanism. The ABI remains inert scaffolding; the next design
must publish due events at generated check sites without invoking host service
for every ordinary edge.

## Tier 5 Result

Interrupt delivery is also an edge-state transition, not only a deadline. The
scheduler now owns one focused eligibility predicate: external source pending,
MSR external interrupts enabled, and the new PC outside the three unsafe
context windows. Using that predicate in the pure callback makes the early
route exact, but invoking the callback on every edge still consumes most of the
gain.

The inventory must therefore become data, not repeated host control flow. The
next ABI should expose a compact address index and pending signal to the
composite chassis. Local rejection must have no false negatives; false
positives may call the authoritative host predicate.
