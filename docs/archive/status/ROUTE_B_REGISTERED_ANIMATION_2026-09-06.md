# Real BAS playback through registered sound ownership — 2026-09-06

Base `39962e1`; patches 0182–0187 form one verified ownership checkpoint.
BW-P4-0126 remains active. No external blocker, PLAYER phase-three, audible
output, continuous gameplay or P4 acceptance is claimed.

## Integrated result

The new private probe reads JaiInit.aaf from the verified user disc, obtains
its range-checked sound-table command through the existing AudioInitData
parser, copies the serialized table into a JAI heap, and executes original
SoundTable/SeMgr initialization with reconstructed start/stop/allocation code.
All **2,202 SE entries across eight categories** register and stop. All **160
parameter records** return to their pool. It then reads Lkanm.arc through
original JKRDecomp/JKRMemArchive/find/readIdxResource and copies each BAS into
an aligned, caller-owned 0x200-byte buffer.

All **91 BAS resources / 186 records** replay original animation scheduling
with the previously reconstructed callbacks, in both directions for three
boundary-frame cycles: **4,686 frames / 6,242 live-slot observations** per
mode. Requests now use actual table lookup, start dispatch, category pools,
priority limits, parameter records, setters and stop/handle retirement.
There are no diagnostic voice assignments in this new replay. Every live
sound points back to its owning original animation slot and real descriptor;
the counts of live sounds, parameter records and slots agree. All retire
before resource buffers and the isolated JAI heap; root heap space recovers
exactly.

This is a **Stored-state registration composition**, not JAI frame playback.
It does not artificially promote records to Playing, increment their game
frame counters, run sequence/DSP output or prove audible sound. The old
diagnostic-voice playback target remains unchanged as a separate regression.
Its counts are not interchangeable with the new live-slot observations.

## Source and portability work

0182–0185 are detailed in the preserved sound-pool and SE-registration WIP
reports. They reconstruct sound/parameter pool lifetime, initParameter,
registration arbitration and eviction/rollback, ordered cleanup, stop
dispatch, mute release, movement and randomized volume. Those earlier WIP
reports describe intermediate states, not the current verification ceiling.

0186 adds:

- Original SoundTable initialization with native byte-decoded u16 counts and
  indexes, bounded 18-category record extents and native pointer-table
  alignment. SoundInfo stays a borrowed serialized 16-byte entry; flags and
  priority reads use their byte contracts, not a relocated/native table.
- Actual SeMgr initialization and typed active-sound storage. Retail
  instructions at 0x802928BC–0x802928D4 prove that track totals use every
  second category byte. The original nonmatching source incorrectly summed
  consecutive bytes; that bug is repaired and independently tested using
  different track and per-actor limits in two scenes.
- Basic startSoundDirectID (0x8029050C/0x70), startSoundBasic
  (0x8029057C/0x18C), and makeSound (0x80291114/0x94). Missing descriptors leave
  a caller handle unchanged; cancellation clears an optional handle;
  sequence/stream disable and reserved-SE-sequence conditions are preserved.
  The factory uses compiler-owned JAISound array layout on the selected heap.
- Native FixedSeqSlot pointer ownership, including declaration and both
  allocation expressions in the still-unqualified full sequence initializer.
  No int-address truncation or fixed retail object stride is used.
- AudioInitData exposes its already validated sound-table command. Synthetic
  tests cover command fields, absence and out-of-range rejection.

0187 reconstructs setSeInterPan (0x80299E88/0xA0), setSeInterPitch
(0x80299F28/0xB8) and setSePortData (0x8029A120/0xA4), and composes original
setVolume/setPitch/setPan/setPortData wrappers. Pitch randomization follows
the inspected __cvt_fp2unsigned contract; invalid zero-divisor random ranges
fail explicitly on native builds. Stored port writes set the actual port
value and mask; non-Stored track-port service remains fenced in this tier.

All reconstruction uses direct inspection of the pinned GZLE01 DOL, SHA-1
`8d28bab68bb5078c38e43f29206f0bd01f7e7a67`, with Capstone 5.0.6 and explicit
opcode decoding where Capstone misidentifies paired-single saves/restores or
does not decode an ordered compare. This is semantic reconstruction, not
byte-matched code or an instruction-execution oracle for the entire owner.
Native audio objects retain disabled float contraction.

## Tests and fixture corrections

Public controls cover original constructors/heap selection, four initializer
lifetimes with default/custom scenes, all category-ID extraction values,
table lookup and missing entries, duplicate/priority/cancellation branches,
distance eviction, failed parameter allocation rollback, arbitrary list
release/reuse, handle transfer, exact reset fields, active-table cleanup,
mute/interrupt and fade transitions, and synthetic BAS-to-registration replay.
8,192 random-volume controls and 1,024 pitch/pan controls use independent
integer random selection and exact float-state comparisons. Movement tests
include immediate changes during an active ramp and signed-boundary counters.

The first integrated private replay passed state/heap checks but emitted two
solid-heap free warnings. Source inspection found that archive finders use
the system heap. Constructing the isolated JAI solid heap had made it both
current and system heap. Restoring only current heap did not fix the warnings;
restoring the root as system heap did. JAI allocations still explicitly use
msCurrentHeap. No production allocator or warning suppression was changed.
Final replays contain no such warnings and still recover exact heap space.

The typed JAIZelBasic fixture still provides a default derived constructor,
delegates its virtual factory to the reconstructed base factory, and supplies
an identity matrix/no-stream-playing contract. Special seStart and unqualified
non-SE parameter operations fail closed. Sequence table/track construction,
port IO and sequence/stream dispatch observers are diagnostic, not production
services. Original full JAIZelBasic/JAIZelSound allocation and boot initialization
remain open. Private tests use default category policy, not proof of the
game's full configuration or normal AAF initialization procedure.

Invalid category/scene/track indexes, zero/one sound pool capacity, foreign or
duplicate release, null-handle parameter exhaustion, zero random divisors,
whole-initializer OOM recovery, concurrency, FPSCR/NaN payload identity and
complete backend lifetime are not accepted by these tests.

## Verification

- Public CTest **81/81**, Debug, Release and strict ASan/UBSan.
- Prior callback instruction/control oracles: **11,076 parameter / 12,880
  emission** comparisons in every mode.
- New private sound-table/registered-BAS probe and accumulated private rebuilds
  pass in every mode: previous BAS replay, Link init/model/identity, camera
  run/event/constructor/matrix/mass, sea, attention, DZB, Toripost heap,
  MCA morph, BG profile, Stone2 and room lifecycle.
- PLAYER production link censuses remain **4 / 41 / 52** in all modes,
  expected linker exit 2. Actual Link remains phase-two only; no diagnostic
  observer enters PLAYER.
- Player-asset tests 10/10; TWW preparer through 0187 and Aurora preparer pass.
  Both protected recompcore and dependency-lock SHA-256 hashes are unchanged.
- Audit output is identical to the known 20 tracked local-research files;
  no new audit failure. Diff checks pass; no private input/derived artifact
  is staged. No GUI, Simulator, mobile or protected source was changed.

Pinned TWW `03d27aa14389e648f51df2bc055ee3d53a3b67f2`, Aurora
`8b690b60e699c92e3327886ebd84cf7f05c5d36c`; Apple Silicon macOS 26.6.2,
SDK 26.5. Reproduce with `bluewake_route_b_se_registration_test` and
`bluewake_route_b_private_se_registration_probe <private-gzle01-iso>`.
Strict environment: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1`. Ignored evidence alias:
`route-b-registered-animation-20260906`.

## Next product-facing action

Bring the real game-facing audio interface into PLAYER's original phase-three
composition, distinguishing complete original source from missing behavior,
and measure the first reached unqualified service toward actual makeBgWait
with real Room44 ground. Do not assume full DSP reconstruction is necessary
before that first diagnostic execution; fail visibly at any genuinely reached
missing service instead of simulating success. Real JAI frame advancement,
sequence/backend behavior and full actor lifecycle remain required thereafter.
The organizing milestone is still continuous Link/ground/input/camera/draw,
followed by transition and save/reload, not another isolated method count.
