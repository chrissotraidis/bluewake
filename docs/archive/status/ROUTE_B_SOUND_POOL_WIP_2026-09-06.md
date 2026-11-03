# Sound/SE ownership reconstruction — WIP, 2026-09-06

Base/last verified checkpoint `39962e1`. BW-P4-0126 remains active, P4 remains
open, no external blocker. Do not promote this to completed SE registration
or working audio. Changes are uncommitted while the owning subsystem continues.

## Implemented

Patch 0182 reconstructs the following missing bodies from the SHA-1-verified
GZLE01 DOL (`8d28bab68bb5078c38e43f29206f0bd01f7e7a67`):

- LinkSound::getSound, 0x8029B4AC/0x54.
- LinkSound::releaseSound, 0x8029B500/0x70.
- SeMgr::getSeParametermeterPointer, 0x80294938/0x5C.
- SeMgr::releaseSeParameterPointer, 0x80294994/0x7C.
- JAISound::initParameter, 0x8029AEF8/0xD4.

Both pools remove the free head and insert at the used head; release unlinks
the used member and prepends it to free. Retail does not maintain a null
back-link on free heads. Sound release clears field_0x38 but does not clear the
external caller handle or change sound state. The higher owner must do those
things. SE-parameter release accepts null; sound release requires a valid
used-list member. Do not add silent duplicate/foreign-release acceptance.

initParameter copies Actor pointer/context data, publishes the optional
caller handle, sets fade/owner/info fields and resets the specified counters.
Native JAISound fields 0x28 and 0x2c are now Vec*/void*, preserving the second
vector and identity pointer rather than truncating them. The retail declarations
remain unchanged on non-native builds. If actor's first pointer is null, its
second/third pointer fields are cleared but context survives; a null actor also
clears context. The sound-info pointer remains borrowed, not decoded or owned.

The new pool tier composes original JAISound/SeParameter constructors and
LinkSound::init with these reconstructed methods. Other methods stay excluded;
test-only virtual fences abort. Neither this pool tier nor the existing voice
observers are admitted to PLAYER.

## Focused evidence and current verification

The strict ASan/UBSan sound-pool test passes:

- Original JAISound field initialization and SeParameter member-array defaults.
  The latter's empty constructor body still executes member constructors.
- Original LinkSound::init on a real JKRSolidHeap with capacities 2, 3, 8, 32;
  two camera-position records per sound, aligned allocation and exact parent
  heap free-size recovery after pool-heap destruction.
- Exhaustion, arbitrary used-list release and reuse against an independent
  vector-list model, including 4,096 mixed operations per capacity. Parameter
  pool fixtures cover capacities 1, 2, 3, 8 and null release.
- Full native pointer identity, nullable actor/handle initialization, packed
  context preservation, caller-handle separation and unchanged release state.

This uses direct instruction inspection plus independent semantic controls,
not an instruction-execution oracle or a byte-match claim. Original pool init
is not qualified for zero/one sound capacity. Double release, foreign members,
threaded access and full JAI heap lifetime are not accepted.

The first focused link needed the existing static-REL registry dependency
retained by foundation objects; the test now links its real owner. No stub was
added. Patch preparer through 0182 and diff checks pass. Protected hashes and
dependency-lock hash remain unchanged; audit output is identical to the known
20-file policy failure. Full three-mode public/private rebuilds completed
after patch 0182's shared-header change. Public CTest is 80/80 in each mode;
extended callback oracles remain 11,076 parameter and 12,880 emission cases
per mode. Private logs reach the final camera-mass probe in each mode with
prior commands success-gated. On resume the old handles were missing and
process inventory found no remaining build/probe process; completed logs were
inspected rather than restarting an assumed live run.

Logs use `/tmp/bluewake-sound-pool-*.log`, copied to ignored evidence alias
`route-b-sound-pool-wip-20260906`. No build remains live from that matrix.

Follow-on patch 0183 changes sePlaySound's native slot type to JAISound*,
including both table and 16-slot allocation expressions in the original
initializer. Non-native builds retain u32 storage. This follows the actual
pointer comparisons/clears in releaseSeRegist; it is not a numeric ID mapping.
The focused strict pool target passes after 0183. The full matrix above
predates 0183, and original SeMgr::init is still excluded in the pool tier,
so allocation execution and complete registration remain unqualified.

## Registration frontier inspected, not implemented

Retail storeSeBuffer (0x80294A10/0x4AC) selects a category pool, handles existing
handles/duplicate actor sounds, applies per-actor priority limits and possible
distance-based eviction, then acquires sound and SE-parameter records. On
parameter exhaustion it returns the acquired sound. On success it initializes
move parameters, installs parameter ownership, sets Stored/track-0xff state,
and calls initParameter. It requires checkSoundHandle, sound priority/category,
releaseSeRegist and real stop semantics; do not replace them with successful
test responses in PLAYER.

releaseSeRegist (0x80294814/0x124) handles sequence-track/mute work, clears the
category's active sound entry, clears the external handle, changes state,
returns the SE parameter, and finally releases the sound record. Inspection
shows sePlaySound entries hold JAISound pointers; patch 0183 replaces the
u32** declaration and u32[16] allocation with native pointer storage.
releaseSeBuffer (0x80294EBC/0x58) ignores inactive sounds,
immediately unregisters Stored/zero-fade sounds, and otherwise requests volume
fade through the still-missing interpolated setter.

Next continue this whole registration lifetime with typed tables, verified
sound-info decoding, required dispatch/parameter owners and pool exhaustion/
rollback tests. Then replace diagnostic voice responses in frame playback and
continue remaining runtime closure to real-ground makeBgWait. Do not stop at
an isolated pool-method or symbol-count milestone.
