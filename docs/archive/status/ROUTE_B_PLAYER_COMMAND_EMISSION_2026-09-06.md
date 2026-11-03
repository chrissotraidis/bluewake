# Original PLAYER command emission — 2026-09-06

Verified increment from `051bc42`. BW-P4-0126 remains active; P4 has not passed.
This is command-emission evidence, not GPU consumption or visible gameplay.

## Result and changes

The render probe retains 180 original input/update/collision/camera/PLAYER draw
iterations, then invokes original J3D opaque/translucent list consumers into a
bounded 2 MiB GX command capture. Every iteration emits nonempty bounded output;
first/last captures contain 83,968 bytes in Debug, Release and strict ASan/UBSan.
Link-model packet identity, movement, release-to-stop and following-camera
controls remain intact: displacement 105.843, 119 changing foot-matrix samples,
and final released speed zero.

- Original JRenderer.cpp now supplies its texture-cache command writer unchanged.
- Aurora patch 0002 implements original cache-region register packing and GX
  miscellaneous shadow state. Capture policy is selected before begin, matching
  J3D drawInit's policy through end.
- TWW patch 0207 replaces the mismatched three/five-argument matrix-array call
  with the existing native-array bridge. Draw/normal array spans use the actual
  draw-table count; concat-view joint/envelope arrays use their original allocation
  counts. Non-PC paths remain unchanged.
- All four graphics-enabled resource runtime variants export the native-array
  dependency. This fixes camera and asynchronous room-lifecycle link regressions
  exposed by broad verification, without dummy implementations.
- The GX state test checks 24 exact cache-region states, untouched bytes, XF
  count truncation/dirty state, and real begin/end context restoration controls.

## Verification

- Focused 180-frame command replay and GX state test pass all three modes after
  final relinking: `/tmp/bluewake-render-final-{mode}-{player,state}.log`.
- Public CTest: 82/82 per mode.
- All 20 accumulated private/composed probes and separate PLAYER event identity:
  all modes pass; `/tmp/bluewake-render-matrix-*.log`.
- Animation oracles: 11,076 parameter and 12,880 emission cases per mode pass.
- Production create/phase-three/runtime censuses retain expected exit 2 and
  4/41/52 unresolved symbols per mode. Production remains incomplete.
- Eleven asset-preparation tests and both source preparers pass.
- Protected CPU exception/HLE hashes and dependency-lock hash are unchanged.
- Staged/unstaged whitespace checks pass. Repository audit retains exactly the
  known 20 intentionally tracked local-research evidence files, with no new failure.

## Remaining renderer frontier

The capture is not yet renderable evidence. A bounded debugger replay proves
the actual Link consumer calls J3DShape::loadVtxArray -> J3DLoadArrayBasePtr with
a native position pointer above 4 GiB, immediately before a legacy 31-bit mask.
Aurora's command processor explicitly rejects legacy CP array-base writes.
Static VCD/VAT native-array conversion does not replace dynamic buffer rebinding.

Next adapt this reached dynamic position/normal/color binding owner with real
active buffer spans and strides. Original skinning preserves F32/S16 formats;
do not assume float vertices or substitute static arrays for deformed buffers.
Then consume the commands through the renderer and integrate visible Link,
Room44 geometry and live input, followed by scene progression. Simulator was
left untouched; permission to close it for the visible test is still pending.

The retained diagnostic scene/heap setup, pending room request, selected frame
schedule and `_Exit` remain limitations. Full Painter/shadows/world presentation,
normal boot, GPU correctness, live interaction, audio output and normal teardown
are not qualified. Investigation: `ROUTE_B_PLAYER_RENDER_WIP_2026-09-06.md`;
runtime trace: `/tmp/bluewake-dynamic-array-frontier-lldb.log`.
