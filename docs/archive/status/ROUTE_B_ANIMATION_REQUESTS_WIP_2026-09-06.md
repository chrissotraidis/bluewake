# Animation request reconstruction — WIP, 2026-09-06

Base: `22dda48`. BW-P4-0126 remains active; P4 remains open. This is
uncommitted subsystem work, not a gameplay milestone or a fully verified
checkpoint. The integrated endpoint remains original Link makeBgWait on real
Room44 ground, followed by continuous input/collision/camera/draw.

## Implemented and measured

Patch 0179 reconstructs JAIZelAnime::setSpeedModifySound from the verified
GZLE01 DOL range 0x802ACD34, size 0x26C. The new parameter-only test composition
uses original animation construction/state and test-only pitch/volume request
observers. PLAYER still excludes this callback and retains its abort fence.
No empty JAISound parameter implementation is admitted as working audio.

The independent private oracle decodes the 155 retail instructions, reads
constants from the SHA-1-verified DOL, and models this bounded instruction
subset. Only the two pitch/volume call targets may be intercepted. Unknown
instructions and uninitialized memory reads fail. It is not a game runtime,
does not model FPSCR exceptions, and compares NaN classification rather than
payload identity. The initial paired-single fixture incorrectly assumed a
single-lane save; the instruction encodes both lanes and the model was fixed.

The first corpus contains 2,880 category/crawling/age/speed/factor cases.
An additional deterministic 8,192 cases independently vary signed factors,
starting pitch and speed bit patterns, volume bytes and unsigned age extremes.
Strict ASan/UBSan passes all 11,072 oracle rows plus four hand-computed controls.
Earlier three-mode logs show public CTest 77/77 and the original 2,884-case
parameter run passing Debug, Release and sanitizers. The expanded 11,076-case
run subsequently passes Debug and Release too. Strict private Link
phase-two/event-identity passes; the entire private regression matrix has not
been rerun for this WIP. Patch preparer check through 0179 and git diff --check
pass (preparer retains the pre-existing patch-0171 blank-line warning).

Reproduction:

```sh
# Python environment requires Capstone 5.0.6; output remains ignored/private.
python tests/route_b_animation_parameter_oracle.py generated/full/main.dol > /tmp/bluewake-animation-parameter-oracle.tsv
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/route-b-sanitize/bluewake_route_b_animation_parameter_test /tmp/bluewake-animation-parameter-oracle.tsv
```

## Next emission owner, not another isolated symbol checkpoint

JAIZelAnime::startAnimSound (0x802AC888, size 0x4AC) is empty upstream.
The inspected retail body includes suppression flags, stream-dependent ID
remapping, camera-space distance culling, material/reverb transport, existing
slot stop, replacement start and port-9 reverb assignment. These ordered effects
must be reconstructed together; request recorders cannot prove slot/backend
ownership. Its distance calculation includes frsqrte and double refinement;
using host sqrt alone is not evidence of retail bit equality.

Native header inspection confirms the following mappings; numeric annotations
are retail offsets, never native byte offsets:

- JAIBasic::mAudioCamera -> JAInter::Camera::field_0x8 is the camera matrix.
- JAInter::Actor::field_0x4 is the position pointer; field_0xc is already a
  native u32 context after patch 0158, with packed reverb in its high byte.
- JAIZelBasic flags retain typed members field_0x0207, field_0x0201,
  field_0x0206, field_0x0045 and field_0x0046.
- The scene suppression reads field_0x0224, mIslandRoomNo, field_0x0239
  and field_0x0028. Their numeric meanings must not be guessed from names.

Lower owners remain incomplete: JAIZelBasic::seStart and JAISound's
setSeInterVolume/setSeInterPitch contain missing behavior. Reconstruction of
the callback must not silently promote those bodies. Continue the coherent
animation emission/request-slot subsystem with retail call-order and ownership
oracles, then integrate remaining runtime dependencies and execute phase three.
There is no external blocker and no new playability claim.
