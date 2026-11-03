# Actual attention resource frontier — 2026-09-06

- Base: `3df6aee`, camera matrix checkpoint committed and pushed.
- Gate: P4; BW-P4-0126 remains active, `RETRY_NEW_HYPOTHESIS`.
- Input: private `GZLE01-disc-image`; source patches unchanged through 0149.
- No production behavior changed in this diagnostic checkpoint.

## Measured result

The new optional `bluewake_route_b_private_attention_probe` reads actual
Always and System archives through AuroraDisc, mounts copied/decompressed
bytes with original JKRMemArchive, supplies System's actual toon textures,
and admits Always through original dRes. It resolves the attention arrow's
4-joint, 1-material model and 40-frame BCK animation.

All five BPK resources required by the original attention constructor remain
serialized `J3D1bpk1` data after dRes admission. Indices are 69, 71, 72, 70,
and 68 in constructor order. Debug, optimized Release, and strict ASan/UBSan
agree; the diagnostic deliberately exits 2 instead of casting these bytes to
objects and entering undefined behavior. This is a reproduced missing resource
conversion, not attention construction or a newly passing gameplay gate.

The first harness build needed an explicit checked size_t-to-u32 conversion.
The first run omitted System's toon textures: Debug crashed and UBSan located
the null ResTIMG reference at d_resorce.cpp:189. Loading the actual System
textures, as the existing Always model probe already does, resolved that
fixture defect without changing game source. Do not reopen it as a loader bug.

## Source-owner finding and next experiment

The native stage animation branch currently admits BTK and BCK/BCKS, not BPK.
The original J3D loader has color-key logic, but its color-key serialized
header fields/offsets and s16 sample reads have not received the native endian
adaptation already present for transform keys. Simply admitting BPK or treating
a non-null resource lookup as a native object would be invalid.

Next: adapt the original color-key loader/sampler at that serialization
boundary, prove material-name binding and color samples against independent
raw-key expectations for these five real resources, then execute original
attention construction and game-heap ownership checks. The two ordinary new
sites remain suspected allocation escapes, not reproduced defects yet.
Dynamic-collision services, camera Run (25 unresolved symbols at base), actual
PLAYER integration, normal boot, and continuous play remain open.

## Verification and reproduction

All three optional builds succeed and all three runs produce the expected
five-raw-resource failure. Public CTest replay passes 68/68 in all three modes.
The preceding camera checkpoint's full private regression evidence remains
the base; this follow-up does not change its source or runtime configuration.
No app window or Simulator is launched. Probe shutdown uses _Exit and does
not qualify archive unload, attention lifetime, or whole-game teardown.

Build the optional target in `build/route-b-aurora-gx-{debug,release,sanitize}`
and run it with the validated user disc as its sole argument. Strict options:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Expected exit: 2, with `unconverted color resources=5`.
Private logs: `local-research/evidence/route-b-attention-resource-20260906/`.
