# Original PLAYER phase two — 2026-09-06

Base `69a5ff1`; BW-P4-0126 remains active. Phase two passes; full creation and
P4 do not. Apple Silicon macOS 26.6.2 / SDK 26.5, private GZLE01 input and
unchanged pinned TWW/Aurora dependencies.

## Measured causes and changes

The preceding heavy-boots allocation failure was not evidence that the
original 0xB0000 actor heap was too small. LLDB showed raw archive bytes being
interpreted as J3DModelData, including 4,096 joints and draw matrices. The
resource belongs to the BDLC archive type, omitted from native admission.
Patch 0168 admits BDLC through the original BDL loader with 0x2020 flags and
toon setup, matching the existing retail BDLC branch. Actual HBOOTS has four
joints, three draw matrices, three shapes and three materials. No heap growth.

Execution then reached initTextureAnime and failed on a malformed native
name-table pointer in original setAnmTexPattern. Patch 0169 uses existing
big-endian wrappers for BTP block/table scalars, offsets, texture indices and
material IDs. Original loading, sampling and material binding remain intact.
This qualifies the reached Lkanm scratch-loader path, not general BTP dRes
admission or malformed-file validation.

## Independent real-data checks

The PLAYER resource probe enumerates four real Link BDLC models. Native
joint/draw-matrix/shape/material counts match all 16 bounded raw block counts.

For the actual TMABAA face BTP, the test reads/decompresses into the original
0x1000 buffer extent, loads a separate copy through J3DAnmLoaderDataBase,
and compares five tracks against 205 independently decoded frame samples,
including fractional frames and lower/upper clamps. Names match serialized
strings; original material binding matches a linear-name oracle on CL.
The temporary animation object is deleted before its backing copy expires.

Failed test attempts are retained in the development history: direct private
model-field assertions did not compile, so the final assertions use public
actor matrix and virtual joint-matrix access. Direct archive fetch returned
compressed bytes and cached them, failing the BTP signature assertion; the
test now uses the same read/decompression API as original initTextureAnime.
No production algorithm was changed to accommodate those harness errors.

## Actual initialization result and boundary

After real Link/Lkanm loading and the prior combined camera/service checks,
original phase_2 invokes the actual constructor and playerInit/createHeap.
It returns cPhs_NEXT_e (2), with nonnull actor heap, published cull matrix,
joint-zero matrix and player identity. This passes all three configurations.
Diagnostic audio methods still abort on use; none is reached here. The probe
supplies cleared process storage and diagnostic camera/stage/publication
inputs. It does not run original phase one, normal process admission or
global/player teardown, and exits via _Exit. Leak detection remains disabled.

Source inspection identifies the next necessary boundary: original phase_3
calls makeBgWait, waits for real ground, chooses the starting behavior and
calculates initial animation/models. Skipping directly to execute after
phase two would omit that work. Next: compose this phase against real Room44
collision, then continuous actual PLAYER and camera updates. No manually
injected procedure, successful audio substitute or gameplay claim.

## Verification

- Debug, optimized Release and strict ASan/UBSan: full phase-two probe and
  --resources-only both exit 0, including all prior combined service checks.
- All three modes: public CTest 72/72; private player-model, camera Run,
  camera events, sea, attention, DZB, Toripost heap, McaMorf, BG profile,
  Stone2, Room44 lifecycle, camera constructor/matrix/mass regressions pass.
- Production link censuses deliberately still fail in every mode: four
  phase-two audio symbols; phase-two/execute retains 118 symbols. These are
  link requirements, not counts of runtime calls or remaining tasks.
- TWW preparer through 0169 and Aurora preparer through 0001 pass; the latter
  is invoked with bash. Shell syntax, diff whitespace and ten player-asset
  generator tests pass. Protected recompcore and dependency-lock hashes
  remain unchanged. Audit fails only for the known 20 tracked research files.

Reproduce with `cmake --build build/route-b-aurora-gx-MODE --target
bluewake_route_b_private_player_init_probe -j 8`, then that executable with
the validated private disc argument, optionally followed by --resources-only.
Strict environment: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1`. Public tests use `ctest --test-dir
build/route-b-MODE --output-on-failure`. MODE is debug, release or sanitize.

Ignored logs and executable hashes:
`local-research/evidence/route-b-player-phase-two-20260906`.
No GUI/Simulator launched. No external blocker; no phase-three, update, draw,
input, audio, save/reload, normal boot or P4 promotion.
