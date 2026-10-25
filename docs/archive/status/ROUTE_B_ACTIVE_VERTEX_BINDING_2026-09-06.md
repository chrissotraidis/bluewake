# Active J3D vertex binding — 2026-09-06

Verified headless increment from `512470d`. BW-P4-0126 remains active; P4 has
not passed. No GPU, visible gameplay, live input or normal-boot claim.

## Fix and focused evidence

Patch 0208 replaces the reached native J3DShape dynamic array-base writes, which
masked host pointers into legacy 31-bit GameCube addresses unsupported by Aurora.
The original current position/normal/color pointers remain authoritative. Native
bindings use exact decoded spans for resource pointers and original format/count
spans for transformed pointers. Null arrays have zero extent; stride/extent
checks bound metadata. Non-PC behavior and the original NBT normal-array skip
remain unchanged.

All modes pass 180 original input/update/collision/camera/PLAYER draw-list
iterations with real GX initialization and 85,024-byte first/last captures.
Exact command-record checks cover all 21 main-body shapes on the first and last
frames: native 64-bit pointers, extents, strides, byte-order flags and padding.
This replay's body arrays are resource-backed. A separate metadata-only control
uses non-resource position/normal buffers, then restores original packet state;
fixture bytes are never drawn. It does not qualify full CPU-skinning presentation
or previously unobserved NBT content. The initial test assumption that actual
body buffers were CPU-transformed was disproven and recorded, not hidden.

Movement remains displacement 105.843 with 119 changing foot-matrix samples;
release ends at zero speed. The following-camera and actual-model packet
identity controls remain intact.

## Verification

- Focused captures and exact binding controls: Debug, Release, strict ASan/UBSan.
- Public CTest: 82/82 in each mode.
- All 20 accumulated private/composed probes plus separate PLAYER event identity:
  all modes pass.
- Animation oracle corpora: 11,076 parameter and 12,880 emission cases per mode.
- Production create/phase-three/runtime censuses retain expected exit 2 and
  unchanged 4/41/52 missing symbols per mode.
- Eleven asset-preparation tests, both source preparers and whitespace checks.
- Protected source and dependency-lock hashes remain unchanged; audit retains
  only the known 20 intentionally tracked local-research evidence files.

Logs: `/tmp/bluewake-player-presentation-headless-sanitize-run.log`,
`/tmp/bluewake-dynamic-presentation-{debug,release}-run.log`,
`/tmp/bluewake-active-array-public-*-ctest.log`,
`/tmp/bluewake-active-array-matrix-*.log`,
`/tmp/bluewake-active-array-*-{parameter,emission}-oracle.log`, and
`/tmp/bluewake-active-array-census-*.log`.

## Opt-in presentation and next integration

The render probe now accepts `--visible` for an experimental Metal window,
bounded presentation readiness, GX command consumption, and 180 paced scripted
frames. Default execution remains headless. GXInit runs in both modes because
original J3D setters require initialized shadow registers. The visible branch
builds but **has not been run**: Simulator remains open and the user's choice
about closing it is pending. No graphical application has been closed or added.

Next observe renderer consumption after resolving that constraint. This first
presentation is Link-only and scripted, not a controllable Outset session.
Integrate original Room44 BG models and live input, then scene progression.
The existing BG probe uses the same sea_T/Room44 archive as PLAYER; world
integration must adopt the BG actor's original collision owner rather than
register duplicate diagnostic ground geometry. Retained selected-phase/raw
game-info setup, pending room work and `_Exit` remain diagnostic limitations;
normal boot, full Painter, audio output and complete lifetime teardown are open.

Investigation history: `ROUTE_B_PLAYER_RENDER_WIP_2026-09-06.md`.
