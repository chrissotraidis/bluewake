# SE registration and stop ownership — WIP, 2026-09-06

Last committed checkpoint `39962e1`. This extends uncommitted 0182–0183 with
0184–0185. BW-P4-0126 remains active; no external blocker or P4 promotion.
The previous status-answer turn made no implementation progress. This turn
reconstructs the missing registration lifecycle and removes several of its
diagnostic responses, rather than creating another pool-method checkpoint.

## Implementation and provenance

Direct instruction inspection uses the pinned GZLE01 DOL, SHA-1
`8d28bab68bb5078c38e43f29206f0bd01f7e7a67`, and Capstone 5.0.6. Paired-single
save/restore opcodes and unsupported ordered floating comparisons are decoded
from their opcode fields, not Capstone's unrelated PPC64 aliases.

Patch 0184 reconstructs:

- SeMgr::changeIDToCategory, 0x8029480C/0x8.
- SeMgr::releaseSeRegist, 0x80294814/0x124.
- SeMgr::storeSeBuffer, 0x80294A10/0x4AC.
- SeMgr::releaseSeBuffer, 0x80294EBC/0x58.
- JAISound::checkSoundHandle, 0x8029AE88/0x70.

The registration path includes existing-handle arbitration, same-actor/same-ID
reuse, incoming serialized flag 0x80000, one-shot replacement, per-actor
priority limits, the equal-priority fading exception, distance-based eviction
when the sound pool is exhausted, and rollback when the parameter pool fails.
Only the first element of the retail candidate array is subsequently read;
the native implementation retains that minimum-priority candidate directly,
with last-visited ties. It does not reproduce unused stack writes.

SoundInfo::readFlags reads four serialized big-endian bytes without alignment
or host-order assumptions. Original getSwBit now uses that accessor. Priority
remains byte four. This is not complete SoundTable initialization or decoding
of its other fields; descriptors remain borrowed serialized data.

Release preserves the actual ordering: sequence-port/interrupt work for a
non-Stored sound, mute release, active-table clear, caller-handle clear,
Inactive/track-0xFF state, parameter release, and sound release. Sound field
0x3c remains stale while free, as in retail; a new registration overwrites it.

Patch 0185 reconstructs:

- JAIBasic::stopSoundHandle, 0x80290708/0xD8: null acceptance and SE/sequence/
  stream routing, with an assertion for the invalid category.
- SeMgr::clearSeqMuteFromSeStop, 0x802942B0/0xD0: eligible sequence selection,
  literal per-sequence mute-bit clearing, and volume restoration only when
  the mask becomes zero. Native shifts preserve PPC slw count behavior.
- JAISound::setSeInterRandomPara, 0x80299CD4/0x114: original RNG progression,
  float-to-u32 selection, asymmetric inclusive range and ordered clamping.
- JAISound::setSeInterVolume, 0x80299DE8/0xA0: type/parameter guards, optional
  randomization and movement entry.
- MoveParaSet::set/move, 0x8029AFCC/0xB0 and 0x8029B07C/0x48: exact control
  results, unchanged-target suppression, single-step/divided movement and
  modular counter decrement. A zero-frame set deliberately preserves an
  existing counter/step; this is not silently normalized.

Original JAIBasic construction, JAISound stop, sequence-volume setter and
track-interrupt setter execute in the new diagnostic registration composition.
Float contraction is disabled for its source objects. No original complete
body was replaced with a successful stub. Neither tier is admitted to PLAYER.

## Executable evidence

The new public registration test uses real original sound/SE constructors,
original solid-heap pool allocation and reconstructed pool operations. Tests
exercise two category pools, handle transfers, duplicate and priority branches,
selected/all-camera distances, tie/negative/NaN distances, allocation failure
and reuse, exact parameter resets, fade transitions, dispatch category choice,
mute/interrupt state, and ordered ownership retirement with heap recovery.

8,192 random-volume cases compare against an independent integer expression
for the retail random-float conversion, including RNG state and exact output
bits. Movement controls include immediate changes during a live fade and
unsigned counters crossing the signed boundary. These are semantic controls
backed by direct instruction inspection, not an instruction-execution oracle
or a byte-match claim. Previous callback instruction oracles remain separate.

The first test link exposed the diagnostic SeqParameter fixture's MuteBit
constructor dependency. Its fixture constructor was made explicit; production
source was not changed to bypass it. The next pass replaced test stop,
fade-volume, mute-release and interrupt responses with actual source owners.

Remaining test-only contracts are sequence-track allocation/construction,
sequence table lookup, track-port IO and non-SE release observers. They do
not supply sound samples, run DSP, implement sequence playback or certify
production JAI allocation/teardown. The real BAS playback target still uses
its previous diagnostic voices and has not yet been connected to this tier.

Preconditions/exclusions: valid category/scene/track indexes, nonzero actor
limits, original pool capacities at least two, valid live pool membership,
and positive random ranges with nonzero doubled divisor. Retail parameter
exhaustion dereferences the output handle unconditionally; null-handle failure
is not claimed safe. Duplicate/foreign release, invalid configuration,
concurrent access, full SoundTable admission, FPSCR/NaN payload identity and
complete backend operation remain unqualified.

## Verification state

- Public CTest **81/81** in Debug, Release and strict ASan/UBSan.
- Prior extended animation parameter/emission oracle replay passes all modes:
  11,076 parameter and 12,880 emission cases per configuration.
- TWW preparer through 0185, Aurora preparer and diff checks pass.
- Protected recompcore and dependency-lock hashes are unchanged.
- Player-asset tests pass 10/10. Audit output is byte-identical to 0182's
  known 20-file tracked-evidence policy failure; no new failure.
- Accumulated private rebuild/replays pass in every mode: real BAS playback,
  Link init/model/identity, camera run/event/constructor/matrix/mass, sea,
  attention, DZB, Toripost heap, MCA morph, BG profile, Stone2 and room
  lifecycle. BAS totals remain 91 resources/186 records/4,686 frame calls/
  1,205 starts/1,205 stops/16,023 parameters/zero extra requests.
- Production censuses remain **4 / 41 / 52** in all modes, expected linker
  exit 2. No actual phase-three execution; no checkpoint commit yet.
- TWW pin `03d27aa14389e648f51df2bc055ee3d53a3b67f2`; Aurora pin
  `8b690b60e699c92e3327886ebd84cf7f05c5d36c`; Apple Silicon macOS 26.6.2,
  SDK 26.5. No GUI/Simulator launched, and all verification processes ended.

Logs use `/tmp/bluewake-se-registration-*.log`. Strict environment remains
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Ignored evidence alias: `route-b-se-registration-wip-20260906`.

## Next integrated boundary

Continue actual sound-table/category allocation and
basic start dispatch plus remaining animation parameter owners. Replace the
real-BAS scheduler's diagnostic voice assignment with this real registration/
stop lifetime before proposing a stable checkpoint. Keep sequence/DSP services
explicitly unqualified where still missing. Then continue to actual Link
phase three/makeBgWait on real Room44 ground and continuous input/camera/draw.
No isolated symbol-count or passing-test increment substitutes for that path.

The next frontier was inspected, not implemented: original SoundTable::init
reads 18 serialized count/index pairs with raw u16 accesses and allocates its
native pointer table with alignment four. Preserve borrowed serialized entry
ownership while fixing/qualifying those boundaries, resource extents and heap
lifetimes. Basic startSoundDirectID (0x8029050C/0x70) leaves an existing handle
unchanged on missing info; startSoundBasic (0x8029057C/0x18C) routes by type,
applies sequence/stream disable flags, skips the SE sequence's ID, chooses a
default sequence handle from info byte five, and applies SE category cancel
flags (clearing an optional handle). That default sequence table currently
has an int* declaration despite holding sound pointers. Qualify the whole
pointer owner, not an address cast. makeSound (0x80291114/0x94) allocates an
array of real JAISound objects on field_0x8's heap when provided, otherwise
the current JAI heap; native sizeof/array construction must own its layout.
