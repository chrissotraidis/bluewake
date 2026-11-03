# PLAYER draw-list consumption — WIP 2026-09-06

> Active-array headless work below is now fully regression-qualified; current
> result is `ROUTE_B_ACTIVE_VERTEX_BINDING_2026-09-06.md`. Visible execution is
> still unrun. Earlier pending/test-failure statements below are history.

## Resumed after stable 512470d: dynamic vertex binding

### Latest controls and presentation preparation

Strict replay now verifies complete Aurora binding records (full 64-bit pointer,
byte extent, stride, native byte-order flag, command family and padding) for all
21 actual main-body shapes on the first and last frames. Initial test assumption
`dynamicBindings > 0` failed: these main-body packets use resource arrays, not
CPU-transformed arrays. That does not invalidate the reached address-truncation
defect. The corrected coverage reports zero non-resource main-body bindings and
separately tests synthetic non-resource position/normal buffers through the
original loadVtxArray method, restoring original packet state before continuing.
Fixture bytes are captured only, never drawn. NBT behavior outside the observed
shape set and full CPU-skinning presentation are not newly qualified.

An opt-in `--visible` render-probe path now links Aurora Metal initialization,
bounded event/presentable polling, GXCallDisplayList consumption and 180 paced
scripted frames, followed by Aurora shutdown. It is not full Painter/world,
normal boot, live input, complete teardown or performance acceptance. It has
not been run. The default remains headless; both paths now initialize actual GX
shadow-register defaults with GXInit before original J3D operations. Headless
strict replay with these defaults passes all 180 frames, 85,024 captured bytes
first/last, and exact array controls.

Evidence: `/tmp/bluewake-dynamic-array-controls-{build,run}.log`,
`/tmp/bluewake-player-presentation-sanitize-build.log`, and
`/tmp/bluewake-player-presentation-headless-sanitize-run.log`. These handles are
terminal. Debug/Release builds and focused runs also pass under
`/tmp/bluewake-dynamic-presentation-{debug,release}-{build,run}.log`.
Both report the same 21 main-body shape controls, separate transformed-pointer
fixture, 85,024-byte first/last captures and unchanged movement/camera results.
Accumulated public and private rebuilds are running under
`/tmp/bluewake-active-array-{public,private}-{mode}-build.log`; public CTest logs
use the corresponding `-ctest.log` suffix. They must finish before a new checkpoint.
Simulator PID 43578 remains live; a renewed user question asks whether to close
it for the bounded visible test. No closure or additional GUI launch is authorized
yet. Continue all-mode headless checks while awaiting that choice.

The prior matrix/GX-state checkpoint is committed and pushed as `512470d`.
New uncommitted patch 0208 replaces TARGET_PC J3DShape::loadVtxArray legacy
CP array-base writes with the real native-array bridge. It retains current
position/normal/color pointers, uses exact decoded spans for resource pointers,
and original format stride times allocation vertex count for transformed
pointers. Null arrays publish zero extent. Stride and 32-bit extent overflow
assertions bound the metadata; non-PC behavior remains unchanged.

The source preparer check and strict render build pass; logs are
`/tmp/bluewake-dynamic-array-prepare.log` and
`/tmp/bluewake-dynamic-array-sanitize-build.log`. Strict 180-frame replay also
passes, ending with 85,024 captured bytes and unchanged movement/camera controls:
`/tmp/bluewake-dynamic-array-sanitize-run.log`. Build and run handles are terminal.
Exact metadata, all-mode and renderer qualification remain pending.
Next add command-record controls at actual Link shape
bindings: full native pointer, owner-derived byte extent, format stride and
native byte order, including resource versus transformed arrays and NBT skip.
Then replay focused modes, consume renderer commands, and run regressions before
another checkpoint. The alternate export index is private under
`/tmp/bluewake-dynamic-array-patch.rwv5Nc/index` (pre-0208 source snapshot).

> Investigation history: the matrix/GX-state increment below is now fully
> regression-qualified. Current result and next action are recorded in
> `ROUTE_B_PLAYER_COMMAND_EMISSION_2026-09-06.md`. Earlier pending/failure
> statements below are historical, not current status.

Stable checkpoint remains `051bc42`; BW-P4-0126 remains open and P4 has not
passed. These changes are uncommitted and not regression-qualified.

## Latest result: matrix ABI repaired, command capture passes

Patch 0207 binds native J3D position/normal matrix arrays through the existing
five-argument Aurora bridge. The original draw-table entry count supplies draw
and normal spans. Alternate concat-view paths select the original joint count
or weighted-envelope count, matching J3DModel's actual allocations. Non-PC
setters and calls remain unchanged. No unlimited/synthetic host span is used.

Debug, Release and strict ASan/UBSan now pass all 180 original PLAYER command
emission frames. First/last captures are 83,968 bytes in every mode, with the
same movement, camera and actual-Link packet controls. Capture context policy
is selected before GXBeginDisplayList so J3D drawInit cannot change whether the
begin/end pair restores state halfway through the capture.

The new Aurora GX state test passes all three modes: 24 exact cache-region
states (all valid even/odd cache-size combinations and both mip flags), untouched
byte preservation, XF count truncation/dirty-state controls, and real display-list
begin/end shadow-state restoration enabled/disabled. This does not validate a
hardware texture-cache performance model or rendered texture correctness.

Next source-owned rendering frontier: J3DShape::loadVtxArray still calls
J3DLoadArrayBasePtr, truncating dynamic position/normal/color pointers into legacy
CP array-base writes. Aurora's cp_arraybase_unsupported explicitly rejects that
command family. Static VCD/VAT arrays already use native bounded metadata, but
they cannot replace dynamic/deformed vertex bindings. Adapt this reached owner
with correct active buffer spans/strides and native byte order, then consume
commands through the renderer. A successful byte capture is not renderability.

A bounded LLDB replay confirms this helper is reached on the actual Link draw:
J3DMatPacket::draw -> J3DShapePacket::drawFast -> J3DShape::drawFast ->
setArrayAndBindPipeline -> loadVtxArray -> J3DLoadArrayBasePtr. The first binding
is GX_VA_POS with a native pointer above 4 GiB, immediately before the existing
31-bit mask. Evidence: `/tmp/bluewake-dynamic-array-frontier-lldb.log`; the
debugger session exited normally after recording the breakpoint and stack.
This identifies a required active runtime adaptation, not an inferred optional
renderer branch. Original J3DCluster skinning preserves F32 versus S16 formats;
the next bridge must use active buffer format/count, not assume float vertices.

Current all-mode public/private regression work is in progress; no stable commit
or gate promotion yet. New logs: `/tmp/bluewake-player-render-context-sanitize-run.log`,
`/tmp/bluewake-player-render-{debug,release}-run.log`,
`/tmp/bluewake-gx-state-{sanitize,debug,release}-run.log`,
`/tmp/bluewake-render-public-*-{build,ctest}.log`, and
`/tmp/bluewake-render-private-*-build.log`.

Public regression has completed: 82/82 in all three modes. The 11 asset tests,
both preparers, 11,076 parameter and 12,880 emission oracle cases per mode,
and whitespace checks pass. Protected source and dependency-lock hashes remain
unchanged. Repository audit reports only the known 20 intentionally tracked
local-research evidence files.

The accumulated private build exposed one integration regression in every mode:
the camera probe retains original J3DShape drawing but did not link the native
array bridge (undefined bluewake_gx_set_native_vertex_array). The graphics-enabled
shared resource runtime now exports that dependency to all consumers, rather
than relying on PLAYER-only linkage. All-mode private rebuilds are running after
this correction; private executions and production link censuses remain pending.

## Earlier measured frontier (superseded by 0207)

The new `bluewake_route_b_private_player_render_probe` retains the original
180 input/collision/camera/PLAYER submission loop and adds original J3D draw-list
consumption into a bounded 2 MiB GX display-list capture. It is not full Painter,
GPU consumption, room rendering, normal boot, or visible gameplay acceptance.

The first build exposed three missing symbols: GXInitTexCacheRegion, GXSetMisc,
and JRNLoadTexCached. The original JRenderer.cpp now composes unchanged. Aurora
patch 0002 implements texture-cache region register packing and miscellaneous
GX shadow state; the preparer now verifies both modified Aurora source files
against the ordered patches. Its behavioral unit tests and broad regressions
remain required before promotion. In particular, context save/restore behavior,
untouched region bytes, cache-size combinations, and XF flush state need controls.

The sanitizer build links. The strict runtime exits 134 on the first consumed
Link draw, with this measured call chain:

```
original opaque P0 list -> J3DMatPacket::draw -> J3DShapePacket::drawFast
 -> J3DShape::setArrayAndBindPipeline -> J3DSys::setModelDrawMtx
 -> GXSetArray: UBSan invalid bool value 164
```

The original inline setter calls the three-argument GameCube GXSetArray API.
Aurora exports the same C symbol with five arguments (attribute, pointer, byte
size, stride, native-endian boolean). Existing BlueWake native vertex-array
bridging covers a separate path, not these original position/normal matrix
setters. This is an ABI failure, not evidence of bad player matrix values.

Next action: adapt original J3D matrix-array binding at its source-owned seam,
with byte spans derived from the real allocated matrix owner and native byte
order. Do not pass an invented unlimited array size or merely coerce the invalid
boolean. Include the shape-matrix alternate table paths. Then rerun original
PLAYER consumption, validate the new GX state services, and progress to visible
Link plus Room44 terrain/live input. Resolve display-list context policy before
capture begins so begin/end cannot disagree about state restoration.

Evidence: `/tmp/bluewake-player-render-jrenderer-build.log`,
`/tmp/bluewake-player-render-gx-state-build.log`,
`/tmp/bluewake-player-render-gx-state-run.log`, and
`/tmp/bluewake-player-render-array-abi-run.log`. The latter contains the symbolized
sanitizer stack. Both build/run handles are terminal. The source preparer check
and tracked whitespace check pass; no broad regression or commit is claimed.

Simulator PID 43578 was observed live during this iteration and left untouched.
No second GUI instance was opened. Permission to close it for a visible test
has not been received; headless development can continue without that decision.
