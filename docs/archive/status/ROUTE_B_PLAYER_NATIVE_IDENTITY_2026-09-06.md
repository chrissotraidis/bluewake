# Native PLAYER identity and process-pointer reframe — 2026-09-06

Base `8ff26c9`; BW-P4-0126 remains active. No identity or P4 acceptance.
Apple Silicon macOS 26.6.2 / SDK 26.5, unchanged pinned dependencies/GZLE01.

## Event-control composition and measured failure

The complete original d_event.cpp compiles without source edits. PLAYER now
uses this owner rather than the camera's constructor-only event fragment;
the other camera support sources keep their existing configuration.

After actual phase two, the private probe exercises original order with eight
entries, stable equal-priority insertion and an independent expected traversal,
rejects a ninth entry, verifies transported fields, rejects compulsory while
busy, resets/reuses the queue, checks valid stick-array access, and reads null
and canonical actor-base IDs. These checks pass in every configuration.
They do not execute event progression, admitted compulsory events or talk UI.

Passing the actual derived Link pointer to getPId revealed a different defect:

| Observation | Debug / Release / strict ASan+UBSan |
|---|---|
| Derived object to fopAc_ac_c base offset | 8 bytes |
| Actual stored process ID after diagnostic phase two | 0 |
| getPId called with derived pointer erased to void* | 1 |
| getPId called with explicitly adjusted actor base | 0 |
| Three independent synthetic ID sentinel mismatches | 3/3 |
| Three independent synthetic parameter sentinel mismatches | 3/3 |
| Canonical actor-base sentinel controls | All pass |
| --event-identity result | FAIL, exit 2 |

The sentinel values are 7, 0x12345678 and 0x01020304. They are diagnostic
metadata only; original ID and parameters are restored before reporting.
No gameplay executes with these sentinels. The original raw-pointer assertion
first aborted; the final opt-in mode reports the same unresolved invariant
with stable signature BW-PLAYER-NATIVE-PROCESS-IDENTITY and exit 2. Regular
phase-two mode remains separate and passing, not an identity qualification.

## Cause and product relevance

The host C++ ABI places a leading vptr before the nonpolymorphic actor base
of daPy_lk_c. Typed derived-to-base conversion adjusts correctly. Converting
the derived pointer to void* first loses that adjustment; casting the erased
pointer to base_process_class reads a different part of the object.
getPId delegates through the void-pointer actor/process helpers, and
fopAcM_GetParam has the same shape. makeBgWait itself calls GetParam(this),
so this is on the immediate initialization path, not hypothetical later UI.

The earlier Stone2 notes already record the related normal-process problem:
the manager's allocation-start header and native source-visible actor base
are not the same object location. That old proof uses unique-profile lookup
and a bounded bridge, not a generic solution. The new Link result makes that
debt the immediate owning frontier. Queue/link success cannot supersede it.

## Required next contract and falsifiable acceptance

Do not fix only the displayed ID or hard-code an eight-byte adjustment.
Establish one reusable native contract that distinguishes allocation address,
complete C++ object address, canonical actor base and process header. Derive
conversions from types/profile metadata, never game addresses or unique-name
search. Preserve pointer type before erasure at actor/event API boundaries;
already-erased arbitrary pointers cannot be safely guessed back into shape.

Before resuming phase three, prove:

1. Original process allocation and placement construction preserve the
   authoritative header, ID, parameters, layer and tags without vptr overlap.
2. Original create/execute/is-delete/delete profile callbacks receive the
   pointer form required by their actual C++ signatures.
3. Typed actor/event calls preserve ID and parameters for real Link and
   synthetic nonpolymorphic/polymorphic actors, including two simultaneous
   instances of the same profile with distinct non-1 IDs.
4. Search and event partner resolution distinguish those instances by ID;
   removal invalidates lookup, and storage reuse never aliases a stale ID.
5. Heap ownership and deletion recover the original allocation exactly,
   independent of any base-subobject adjustment.
6. Existing Stone2/Room44 lifetimes and actual Link initialization regressions
   pass; the explicit identity diagnostic becomes genuinely passing.

Start with a reduced game-data-free original-process lifecycle fixture and
the retained actual-Link reproducer. A typed API fix alone may be a step, but
does not close the allocation/callback/lifetime portions of this contract.
Then return to makeBgWait with real Room44 ground and initial model state.

## Verification

All three modes: public CTest 73/73 and the accumulated private player-model,
camera Run/events, sea, attention, DZB, Toripost heap, McaMorf, BG profile,
Stone2, Room44 lifecycle and camera constructor/matrix/mass regressions pass.
Final regular PLAYER init also passes all modes with the new queue checks.
Explicit identity mode fails as described; never count that as a green gate.

Production link censuses deliberately still fail in every mode: phase two
four audio symbols; phase-two/three 76 (previously 82); phase-two/execute 89
(previously 95). No required behavior is replaced by successful stubs.
No new dependency patch; TWW through 0171 and Aurora 0001 preparers pass
(Aurora invoked with bash). Whitespace and protected/lock hashes pass.
Audit retains only its known 20 tracked research files.

Reproducer: build bluewake_route_b_private_player_init_probe in each
build/route-b-aurora-gx-MODE tree and run with the validated private disc plus
--event-identity. Omit the flag for the passing phase-two/queue regression.
Strict environment: ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1. Public tests: ctest --test-dir
build/route-b-MODE --output-on-failure. MODE: debug, release, sanitize.

Ignored evidence: local-research/evidence/route-b-player-native-identity-20260906.
No GUI/Simulator launched. No external blocker. Phase three, normal admission,
updates, rendering, input, audio, save/reload and P4 remain open.
