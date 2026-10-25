# Route B Toripost heap ownership failure — 2026-09-06

## Result

Original `daObjTpost_c::_createHeap` now executes against the exact 9,852-byte
Toripost archive on a normally constructed native actor. It returns success,
constructs its 3-joint model and animation-audio object, and preserves the
native actor pointer in J3D user data. **Its allocations are not owned by the
selected JKR solid heap. This is a reproduced blocker, not lifecycle acceptance.**

Debug, optimized Release, and strict ASan/UBSan agree:

```
toripost-heap callback=original user-area=native owned=morf0-model0-audio0 heap=736 cleanup=unqualified
```

`JKRHeap::findFromRoot` rejects the solid heap as owner of all three pointers.
The solid heap was created at maximum size under a separate 2 MiB test parent,
selected before the callback, and adjusted afterward. Its adjusted size is
736 bytes; that is **not** the full allocation footprint or qualification of
the actor's retail `0x7e0` heap request.

## Cause and scope

`JKRHeap.cpp` intentionally excludes ordinary global `operator new/new[]` and
`delete/delete[]` on TARGET_PC. Explicit JKR placement allocation remains
available. Ordinary allocations in Toripost, McaMorf (audio, transform and
quaternion arrays), and J3D model construction therefore use the host allocator.
The three pointer ownership failures are measured; the other ordinary array
sites are a source-level follow-up inventory, not yet an exhaustive runtime
ownership census.

This invalidates an inference that destroying an actor solid heap necessarily
reclaims all its model/morph allocations. Prior tests still establish their
reported calls, state, and packet submission, but `_Exit` and sanitizer runs
with leak detection disabled never established transitive reclamation.
Existing BG and Stone2 cleanup claims must be read as explicit heap/service
teardown, not proof that every ordinary C++ allocation was reclaimed.

## Reproducer

Patch 0141 adds an opt-in heap-only Toripost partition, a TARGET_PC diagnostic
morph accessor, and original constructor support from `d_event.cpp` and
`c_cc_d.cpp`. No retained retail method body is rewritten. The default actor
object remains the full source. Unused interaction virtuals abort in the
test-only fence file; no profile is installed or process creation fabricated.

Build `bluewake_route_b_private_toripost_heap_probe` in the existing
`build/route-b-aurora-gx-{debug,release,sanitize}` configurations. Run it with
the private `GZLE01-disc-image` path as its first argument. Default execution
returns **2** on the reproduced ownership failure. An explicit second argument
`--diagnose-ownership` asserts all three measured escapes and returns 0 only
for that diagnostic expectation. It does not turn the ownership gate green.
The probe deliberately exits without teardown; destroying the heap under
objects that retain pointers into it would not be an honest cleanup proof.

## Verification

Build/test logs are retained privately under
`local-research/evidence/route-b-toripost-heap-ownership-20260906/`.

- All three focused builds and diagnostic expectations pass; default mode
  returns 2 in each configuration.
- Full public builds and 65/65 CTests pass in Debug, Release, and strict
  ASan/UBSan.
- McaMorf, exact Room44 parent, standalone Stone2, and standalone BG private
  regressions pass in all three configurations.
- Patch-series verification passes through 0141; protected recompcore hashes
  remain unchanged; dependency lock unchanged.
- Repository audit still reports the known 20 tracked local-research files;
  no additional private evidence is tracked.
- No BlueWake window or Simulator was launched.

## Next loop

Repair and qualify the **shared source allocation domain** reached by
Toripost/McaMorf/J3D before registering the actor. Inventory ordinary and
aligned allocations, preserve host-library/pre-heap allocations, and use a
bounded native allocation policy rather than a mailbox-specific workaround.
Require pointer membership for the object and transitive allocations, actual
heap reclamation, and repeated construction/destruction across all three
configurations. Audit the same ownership assumption in BG and Stone2.
Only then resume Toripost resource/WAIT/execute/draw/delayed-delete composition.
