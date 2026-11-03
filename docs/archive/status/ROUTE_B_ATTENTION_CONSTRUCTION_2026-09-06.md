# Original attention construction with real resources — 2026-09-06

- Base: `631beaa`; BW-P4-0126 / P4 remains active, state `ADVANCE` within
  player/camera prerequisites. No normal-boot or gameplay promotion.
- Private input: `GZLE01-disc-image`; source pin unchanged, patches 0150–0151.
- Debug, optimized Release, strict ASan/UBSan on Apple Silicon macOS.

## Executed result

Original Always resource admission now publishes all five attention-arrow
BPKs as J3D color-key objects rather than serialized bytes. Actual System
toon textures, the 4-joint/1-material arrow model, and its 40-frame BCK remain
in the test. Original material-name search binds every color animation to
model material 0. The five color durations are 13, 10, 35, 40, and 10 frames
in constructor lookup order.

An independent raw-byte oracle checks 1,908 real RGBA channel samples per
mode, at quarter-frame intervals including before/after the animation range.
It uses linear knot search, double-precision Hermite basis functions, and
clamping; original code uses bisection and JMA interpolation. The allowed
difference is at most one byte for floating-point rounding. Synthetic keys
also cover both shared/separate tangent layouts, deliberately distinct
incoming/outgoing tangents, empty/constant channels, saturation, and a
nonzero serialized material ID (0x1234). No game bytes are tracked.

The original dAttention_c constructor and destructor then execute eight
times per mode. Each creates both real arrow morph/model instances and their
color-animation arrays in the original adjusted solid heap. Checks cover
the two outer allocations, model, transform/quaternion arrays, animation
audio object, joint matrix storage, and material/shape packets. Model and BCK
resource pointers remain borrowed from the resource owner. Empty attention
lists and current-heap restoration match original initialization. Every
destruction returns the parent heap to exactly its prior free-space count.

The constructor receives a null player pointer, which it only stores; this
is explicitly not actual PLAYER construction or a targeting update test.
Audio playback fences remain closed. _Exit avoids unqualified global/archive
teardown after the independently verified attention lifetimes.

## Source changes and failures preserved

Patch 0150 adapts the original color-key serialization boundary:

- fixed-size big-endian header fields and offsets;
- typed big-endian s16 sample pointers used by the existing interpolation
  template, retaining original key selection and saturation;
- big-endian material-ID views, including original search's writeback; and
- admission of the existing BPK load branch beside native BTK/BCK handling.

The shared color material-ID type also updates the full-color loader pointer
assignment for compile consistency. Full-color animation format support is
not promoted. This is trusted-resource portability, not a new bounded parser
for arbitrary malformed BPK files; broader J3D input hardening remains open.
An initial build exposed the serialization macro's intentional early undef;
named typedefs keep the runtime views typed without leaking that macro.

Once BPKs loaded, strict execution reproduced both ordinary attention
allocations outside the selected game heap: morph-owned=0, colors-owned=0.
Patch 0151 changes exactly those two new expressions to the already-qualified
JKR_NEW policy. The same assertions then pass across all eight lifetimes in
all modes. Original allocation size, constructor/destructor bodies, resource
selection, and gameplay behavior otherwise remain intact.

## Verification

- Public default builds and CTest: 68/68 in all three modes.
- Actual attention color/constructor probe: PASS all three modes.
- Private DZB, Toripost heap, McaMorf, original BG collision, historical
  standalone Stone2, and Room44 lifecycle regressions: PASS all modes.
- Camera constructor/matrix probes: PASS all modes.
- Camera Run census: still expected FAIL, same 25 unresolved symbols.
- Player asset generator: five tests PASS.
- Source preparation through 0151 and whitespace checks PASS.
- Protected recompcore hashes and dependency lock unchanged.
- Audit retains only the known 20 tracked local-research files.

Reproduce by building `bluewake_route_b_private_attention_probe` in an
Aurora-GX build tree and passing the validated disc as its only argument.
Expected exit is now 0. Strict environment remains
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Logs: `local-research/evidence/route-b-attention-construction-20260906/`.
No app window or Simulator was launched.

## Next action

Qualify original dynamic-collision camera capsule/mass services and compose
them into the camera update closure. Then close the remaining measured Run
dependencies and execute camera/attention updates with actual PLAYER state.
Construction alone does not establish targeting, input, collision response,
visible gameplay, normal boot, transition/save/reload, or full-game acceptance.
