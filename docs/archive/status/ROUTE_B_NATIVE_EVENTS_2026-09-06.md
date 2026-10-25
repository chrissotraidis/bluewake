# Native event resource ownership — 2026-09-06

Base `a87ea98`, Apple Silicon macOS 26.6.2, input alias
`GZLE01-disc-image`. BW-P4-0126 remains active; P4/playability is not promoted.

## Implementation and evidence boundary

Patch 0157 adds an intrusive NativeEvent owner to native dRes_info_c.
Synchronous and asynchronous qualified resource loaders decode only DAT-group
event_list.dat, publish stable native bytes, and retire them at resource
destruction. Other DAT resources are unchanged. The original event manager
and event behavior are not replaced. No dependency revision changed.

NativeEvent validates the 64-byte header; seven section counts, alignment,
bounds and non-overlap; fixed-record name termination; referenced staff/cut/
data and flag indices; substance type/range and string termination; and data
next-link acyclicity. The last check protects original synchronous name
lookup from infinite traversal. The parser has a 64 MiB input bound.

The owned copy retains relative section offsets and fixed record sizes, swaps
numeric words/halfwords, and preserves names, sound/advance flags and padding.
Original setData can therefore establish its native pointers unchanged.
Static assertions in the original-source probe verify all four record sizes
and the header size. Source archive storage remains distinct and unmodified
during successful conversion. Mutable records do not alias the original bytes.

An initial parser assumption rejected zero-length substances. LLDB identified
actual sea data record 3066: integer type, start index 1345, length zero.
The decoder now preserves empty records while validating their bounds. The
original getSubstance contract still rejects attempts to access them; the
probe deliberately skips empty records rather than altering that contract.

## Focused runtime

The optional camera event probe now expects exit 0 with or without its private
disc argument. In disc mode it mounts the real sea Stage archive through the
original JKR owner and loads it through original dRes setRes/loadResource.
This diagnostic resource configuration leaves unrelated model resources raw;
it is not a rendered stage or complete scene admission.

Four successful lifetimes each prove:

- One live native event owner is published separately from serialized bytes.
- Original setData sees all 252 events and 992 staff entries.
- Every event name, priority and staff count agrees with independent big-endian
  reads of the source resource.
- Original getSubstance retrieves every nonempty record in the 4,228-entry
  data table. Every returned float/vector/integer bit pattern and string byte
  agrees with an independent serialized read.
- Original manager remove precedes resource destruction; native owner count
  returns to zero and the archive parent heap's exact free size is recovered.

Two additional mounted-copy mutations (invalid event count and cyclic data
next link) return resource failure -1 with BW-EVENT-DECODE, release their
temporary owners, and recover the same parent heap. The user's disc is never
modified. Raw global game-info teardown remains outside this probe's scope.

The public native-event test covers eight mutation-isolated decode lifetimes,
signed halfwords and integer bits, vector floats, preserved byte fields,
intrusive owner-list deletion, a retail-style empty record, and twelve
malformed header/section/name/index/substance/cycle cases.

Not qualified: event start/advance/runProc, JStudio playback, all stage event
files, camera Run, actual PLAYER, live input, normal boot or gameplay. Resource
validation is not proof that every possible event action is implemented.

## Verification

Debug, optimized Release and strict ASan/UBSan pass all 70 public CTests,
the real event probe, eight existing private probes (sea, attention, DZB,
Toripost heap, McaMorf, BG, Stone2 and Room44 lifecycle), and camera constructor,
matrix and mass-composed regressions. Full PLAYER compilation passes all
modes. Run still fails linking with the same eight symbols in each mode.

Five player asset-generator tests and private asset regeneration pass. TWW
preparation through 0157, Aurora verification, shell syntax and whitespace
checks pass. Protected recompcore hashes are unchanged. The dependency lock
SHA256 remains b9e74b327368a9bc6e3c994b6f8794dea92e06bf822512774fb42bf32a240aba.
Audit retains only the known 20 tracked local-research files; no new game data
or private artifact is tracked. No app window or Simulator was launched.

Logs: ignored `local-research/evidence/route-b-native-events-20260906`.
Reproduce by preparing dependencies, reconfiguring the existing three build
trees, building/running the named tests and passing the private disc only to
the private probes. Strict runs set ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
and UBSAN_OPTIONS=halt_on_error=1. Event disc mode now expects 0, superseding
the previous checkpoint's diagnostic exit 2.

## Next action

Return to the remaining original camera Run execution dependencies: actor
globals, assertion confirmation, and the JAIZelBasic interface/seStart gap.
Do not replace the incomplete seStart body with an unlogged no-op. Qualify
the next source-authentic execution boundary toward actual PLAYER/camera
updates. The organizing milestone remains continuous visible Outset control,
then transition/save/reload and authentic normal boot. No external blocker.
