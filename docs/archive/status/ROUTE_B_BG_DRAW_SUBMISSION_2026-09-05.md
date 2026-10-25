# Route B BG Draw Submission - 2026-09-05

## Boundary

Enter the unchanged original `daBg_c::draw` from the exact passing Room44 BG
profile probe. Qualify its three-model calculation and J3D material-packet
submission boundary, BTK frame entry, clipping and environment-light call
contract, BG-list selection, and restoration of the default lists and camera
far plane.

This tier stops before a presented Metal frame, functional clip or collision
queries, actual BTK key sampling, profile deletion, archive unload, the other
50 persistent Room44 requests, or play phase 4.

## Result

The original BG draw method now runs against the same exact
`/res/Stage/sea_T/Room44.arc` actor created by the preceding checkpoint. The
probe supplies genuine opaque/translucent J3D draw buffers for both the BG and
default lists, then calls the profile draw method through
`fpcLf_DrawMethod`.

Patch 0124 adds a narrow target-PC source partition for the original
`mDoExt_modelDiff` and `mDoExt_modelEntryDL` bodies. Each of Room44's three
models reaches its exact root-node calculation callback once in and once out,
and the resulting opaque/translucent packet chains contain a material packet
owned by each exact model. The total walked packet count equals the original
`J3DDrawBuffer::entryNum` publication.

The actor transfers the wave-synchronized wrapper frame 37 into the Room44
BTK animation at draw time. Ordered observers prove the original draw method
sets the system far plane to 100,000, calculates the frustum, clips models 0,
1, and 3, requests `TEV_TYPE_BG0`, `TEV_TYPE_BG1`, and `TEV_TYPE_BG3`, applies
model lighting, restores the far plane to 4,000, returns J3D to the default
opaque/translucent lists, and finally refreshes Room44's TEV structure. A
second call with all four model slots cleared produces no packets while still
restoring the far plane and lists.

Strict UBSan initially rejected the animated model's second texture-coordinate
record as an invalid `GXTexGenType`. Persistent archive and loaded-model
inspection showed that the file bytes were correct: MAT3 stores
`J3DTexCoordInfo` at a four-byte stride. On the host, `ALIGN_DECL` is empty and
the structure was only three bytes, so every record after index zero was read
at the wrong boundary. Patch 0125 adds the explicit serialized padding byte
and a target-PC size assertion. This is a host ABI/layout repair; it does not
alter the retail texture-coordinate values.

## Qualified Contract

- The unchanged original BG profile draw method is entered from the exact
  Room44 actor through the standard draw-method wrapper.
- The wave-synchronized BTK wrapper frame is entered at draw time.
- All three exact models execute one root calculation and publish authentic
  J3D material packets through the original `mDoExt_modelEntryDL` path.
- Model 0, 1, and 3 clipping and environment-light calls occur in source order
  with their exact model, TEV owner, and light type.
- BG opaque/translucent lists are active for all per-model work; the default
  lists are restored before the final room-light call.
- The system far plane changes from 4,000 to 100,000 for BG work and returns
  to 4,000 afterward.
- An all-model-null control publishes no J3D packets and still restores shared
  draw state.
- Serialized `J3DTexCoordInfo` has its required four-byte host layout.

## Remaining Fences

- The probe proves authentic J3D packet submission, not rasterized or
  presented pixels. No native window or Simulator was launched.
- `J3DUClipper::calcViewFrustum`, `clipByBox`, and the two environment-light
  methods are ordered observers in this tier. Their call contract is proven;
  functional clipping and lighting calculations are not.
- The BTK entry observer transfers the exact frame, but serialized key-table
  sampling and texture-matrix animation remain unqualified.
- Collision remains the bounded `Set`/`Regist` ownership proof from the prior
  checkpoint. DZB queries and collision response remain open.
- Profile deletion, collision release, room-status cleanup, archive resource
  release, and unload remain unentered.
- The private probe still uses zero-initialized storage for the real game-info
  singleton; its full aggregate constructor remains outside this tier.
- The duplicate `_OSPanic`/JUT assertion linker warnings are unchanged.

## Verification

- Debug: the exact private BG probe passes; all 63 public CTest tests pass.
- Optimized Release: the exact private BG probe passes; all 63 public tests
  pass.
- Strict ASan/UBSan: the exact private BG probe passes; all 63 public tests
  pass with `detect_leaks=0` because LeakSanitizer is unsupported in this
  macOS setup.
- Exact success marker:
  `bg-profile room=44 models=3 btk=1 brk=0 collision=1 execute=1 draw=1 calc=3 clip=3 light=3 submit=3 restore=1 pass`.
- `scripts/prepare_route_b.sh --check` replays patches 0001-0125 cleanly.

## Natural Stopping Point

Room44 BG now crosses its exact creation, steady execution, and visible-model
submission boundary without claiming presentation. The next blocker is
`ROUTE_B_BG_DELETE_UNLOAD`: drive this exact actor through original profile
deletion and qualify collision release, room-owned publication cleanup,
resource deletion/unload, and repeatable teardown. Functional collision,
other persistent profiles, phase 4, another BlueWake process, and another
Simulator remain closed.
