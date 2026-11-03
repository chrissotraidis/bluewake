# Native original JStudio parser — 2026-09-06

Base `8161af3`; BW-P4-0126 remains active. No P4 promotion.

Patch 0177 repairs the three observed pointer-to-int compilation failures in
the full original stb-data-parse.cpp with byte-pointer arithmetic. Native
sequence headers and JGadget's original variable-integer decoder now use
alignment-safe big-endian byte reads. Retail integer-read branches remain
unchanged. The original sequence/paragraph control flow and byte-oriented
paragraph-data routine are retained. Full JGadget binary and JStudio data/parser
objects now compose with PLAYER; no successful behavior fence was introduced.

## Independent parser evidence

New public test `bluewake_route_b_jstudio_data_test` constructs bytes using
division/remainder, independently of production readers. At each of eight
buffer alignments it checks:

- 24 variable-integer pairs spanning 16/32-bit formats, zero, boundary and
  high-bit values, including optional/null bit-width output;
- five sequence types covering stop, immediate and content-bearing branches;
- twelve padded-paragraph cases including zero length and both header formats;
- all 256 status bytes at four explicit count values, verifying sizes, counts,
  content and next pointers.

A separate null paragraph-data control preserves the original untouched status.

All buffers are valid, caller-owned and sufficiently sized. This API has no
length parameter: truncated/malicious input rejection is not qualified. Nor
does this qualify all STB header/block fields, payload-value iterators, null
status consumers, real demo playback or PLAYER demo progression. Other
JGadget raw/big-endian template readers still require ownership review when
admitted. Do not generalize these tests to whole-format conversion.

Initial builds exposed missing iterator.h and then conflicting retail/native
iterator declarations in the test. The targets now use the existing native
bg_profile iterator shim, with the existing MSL fallback include path.

## Verification

All 75 public tests pass Debug, Release and strict ASan/UBSan. Actual Link
phase-two/identity and PLAYER model probes pass all modes. The accumulated
private camera, sea, attention, DZB, heap/morph, BG, Stone2 and room lifecycle
probes pass after rebuild/replay for header-impact verification. All-mode
production censuses retain four/55/68 symbols for phase two, phase-two/three
and phase-two/execute (intentional linker exit 2). No GUI/Simulator,
visible-frame, input, audio, performance or normal-boot claim is made.

TWW preparation through 0177, Aurora preparation, diff checks and ten player
asset Python tests pass. Both protected recompcore hashes and dependency-lock
hash are unchanged. Audit retains only the known 20 tracked research files.
Evidence is private under ignored evidence/route-b-jstudio-parser-20260906.

## Anti-stall review: next tier must address the integrated PLAYER closure

Stage services, day processing and this parser are three successive supporting
increments without phase-three execution. Their service oracles and phase-two
replays do not establish a new vertical gameplay milestone. Invoke the goal
loop's anti-stall review now; do not continue one-symbol/one-green-test
checkpoints as the main plan.

The current phase-two/three census has 55 unresolved symbols: 30 audio and 25
other actor/runtime symbols. This is static linkage, not a dynamic call count.
The broad closure comes from original makeBgWait selecting initial procedures
that retain other PLAYER behavior. It must not be evaded by skipping phase
three or injecting a chosen procedure.

Next action: review the remaining owning subsystems as a group, separating
existing complete source, native compile/ABI defects, and missing behavioral
source. Choose a coherent admission/reconstruction plan with an explicit
actual phase-three execution endpoint on real Room44 ground. In particular,
the pinned Link voice methods are empty nonmatching bodies; source inclusion
alone cannot qualify audio. Use the verified private DOL or qualified original
source as an independent behavioral oracle for reconstruction. Keep the
existing fail-on-use diagnostic boundaries explicit, not silent successful
audio. P4 and full PLAYER admission/delete/update/draw/input remain open; no
external blocker has been found.
