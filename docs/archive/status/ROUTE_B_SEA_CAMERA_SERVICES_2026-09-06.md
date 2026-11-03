# Original sea and camera-support services — 2026-09-06

Base checkpoint `e182c44`, Apple Silicon macOS 26.6.2, input alias
`GZLE01-disc-image`. BW-P4-0126 remains active; P4/playability is not promoted.
State: `RETRY_NEW_HYPOTHESIS` toward actual PLAYER/camera execution.

## Reproduced failures and repairs

Full original sea/weather and environment/graphics units are now composed,
without root-only source tiers. These four files total 8,425 source lines;
their inclusion is not a gameplay or completeness metric. Linking the sea
retains its real virtual draw implementation and therefore environment,
graphics, resource and J3D dependencies, even in a query-only test.

- Full graphics compilation failed on the capture thread's u32 result-token
  cast. TWW patch 0152 converts through uintptr_t; capture execution is not
  qualified by compilation.
- TWW patch 0153 exposes the original static sea packet only under the native
  resource-diagnostics macro. The probe invokes original create/execute/query
  methods; the accessor contains no wave implementation or state replacement.
- Actual Always sea texture initialization failed strict UBSan in Aurora's
  GXInitTexObjLOD: the original -0.9 bias became a negative float converted
  directly to u8. Aurora patch 0001 converts through signed s8 in both LOD
  setters, preserving the signed eight-bit fixed-point encoding. Twenty-four
  synthetic encoding checks cover negative/positive values, truncation and
  clamps; the bias-only setter preserves unrelated mode bits.
- Once texture initialization passed, original sea create reported its height
  grid outside the selected solid heap. TWW patch 0154 changes that reproduced
  allocation escape to JKR_NEW. Four create/retire cycles now recover the
  parent's exact free size.
- Release then exposed original inline GXPosition1x16 hardware FIFO access.
  TWW patch 0155 binds the native declaration to Aurora's implementation.
  Three indices produce independently checked big-endian display-list bytes
  in all modes. This does not claim a rendered sea.
- Full graphics global initialization requires the original __OSBusClock
  symbol. The native OS owner supplies 162 MHz, matching pinned Aurora's
  40.5 MHz OSGetTime conversion. Public tests check second/millisecond tick
  conversions. Capture/alarm timing is still outside this evidence.

Probe-composition failures were also corrected: the environment unit needed
the original src include root for its day-processing include; original math
was linked; borrowed fixture light/geometry constants were removed when their
real owners were present. Unused room-map and story-switch branches remain
explicit abort fences, not successful substitute behavior.

## Runtime evidence

`bluewake_route_b_private_sea_probe` uses actual Always/System archives through
the original resource machinery and Aurora's validated disc reader. The stage
is a typed diagnostic sea stage with no MULT data and room zero, not loaded
normal gameplay. Its pose has a nonzero base height.

- Four original sea create lifetimes initialize actual textures and allocate
  the 65-by-65 height grid in the selected solid heap.
- 880 original wave updates advance all four counters through their different
  wrap periods. Center height varies, ruling out the uninitialized constant
  fallback as the tested implementation.
- 88,000 samples compare original daSea_calcWave against independent double
  barycentric interpolation of the generated grid, covering both triangle
  halves, interior and edge-fade cells. Tolerance is 0.02 world units.
  This checks query interpolation, not an independent reconstruction of the
  entire wave-generation equation.
- Strict area boundaries return the original base-height fallback. Init/retire
  disables queries before heap reclamation; subsequent queries do not use the
  retired grid. Parent free space returns exactly after each cycle.
- Original wind updates publish override strength, upper clamp, event stop
  and smoothing through the original getter required by camera Run.
- Original graphics heaps allocate sea-sized aligned buffers, rotate between
  two owners and reuse the first buffer after freeAll. Diagnostic teardown
  recovers the parent heap. Original blur enable/identity matrix/disable pass.

Not qualified: normal sea actor admission/deletion, MULT-dependent calm areas,
Daiocta story switches, live player poses, sea drawing/lighting/fog, graphics
capture, complete environment updates, camera Run, PLAYER, input or boot.
Resource/archive teardown remains the existing raw-game-info probe boundary;
the diagnostic exits without pretending to destroy an unconstructed global.

## Regression and provenance

Debug, optimized Release and strict ASan/UBSan all pass:

- All 69 default CTests.
- New private sea probe; existing private attention, DZB, Toripost heap,
  McaMorf, BG, Stone2 and Room44 lifecycle probes.
- Original camera constructor, matrix and mass-composed probes.
- Full PLAYER source compilation and locked private display-list preparation.
- Five player asset-generator tests; TWW preparation through 0155;
  Aurora patch verification; whitespace and shell syntax checks.

Run remains an explicit link failure with the same 16 unresolved symbols in
all modes, versus 22 at the previous stable checkpoint: five actor globals,
five event/demo methods, two audio symbols, two resource/J3D symbols, vibration
and assertion confirmation. Adding full sea alone temporarily increased the
count to 25; full environment/graphics reduced it to 18, then original fog
table and native bus-clock owners reduced it to 16. Counts are dependency
measurements, not estimates of remaining gameplay work.

No dependency revision changed. The lock now records Aurora patch 0001 and
corrects its stale local-checkout metadata to the already-used pinned SHA.
New lock SHA256:
`b9e74b327368a9bc6e3c994b6f8794dea92e06bf822512774fb42bf32a240aba`.
Both protected recompcore hashes are unchanged. Audit retains only the known
20 tracked local-research files; no game data or new private artifact is added.
Logs are ignored under `local-research/evidence/route-b-sea-camera-services-20260906`.
No application window or Simulator was launched; no new FPS claim.

Reproduce preparation with scripts/prepare_route_b.sh and
`bash scripts/prepare_route_b_aurora.sh` (both accept --check). Build the named
probe in each existing Aurora-GX tree and pass the private disc path as its
sole argument. Strict runs set ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1. The Aurora preparer verifies the pinned commit
and exact patched file against a temporary Git index; unknown file edits fail.

## Next action

Compose the already-qualified actual resource/J3D owners into the camera Run
target, then resolve its original event/demo, audio, vibration and actor-global
owners. Execute original updates with actual PLAYER as soon as the required
closure permits; do not organize the next loop around another unrelated prop.
The organizing milestone remains visible, continuously controllable Outset,
then transition/save/reload and authentic boot acceptance. Full PRD unchanged.
