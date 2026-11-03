# Route B Game Manager Boundary

**Recorded:** 2026-09-01
**Blocker:** `BW-P4-0071`
**Scope:** macOS source-native Route B

## Finding

`f_pc_manager.cpp` is 341 lines, but the apparent full-game dependency is
mostly one platform presentation path:

- lines 65-237 define embedded DVD-error messages and render them through
  J2D, J3D, GX, the display manager, and JAudio stream control;
- lines 239-264 poll drive/reset state and select that presentation path;
- lines 266-295 contain the process-frame scheduler;
- the remaining functions are thin process-framework adapters and root-layer
  initialization.

The process scheduler itself has no serialized J3D resource ownership. Its
required order is matrix initialization, DVD-error gate, paint, deletion,
priority, creation, pre-execute callback, execution, drawing, and post-draw
callback. DVD blockage ends the frame before process mutation. Priority and
creation failures are fatal contracts in the original and are explicit Route B
results rather than release-build-only assertions.

## Implemented Boundary

`route_b/src/process_frame.cpp` now owns that host-neutral sequence. The DVD
condition/presentation decision is one explicit service callback, so the
native process substrate does not import embedded BMG data, J2D/J3D, GX,
JAudio, or GameCube DVD APIs. A focused test proves normal ordering and the
three early exits.

The thin `fpcM_*` manager entry points and root-layer initialization now also
compile against this contract. This does not advance title boot, but it removes
the false requirement that the manager's platform error UI be ported before
its scheduler can compile.

## Next Smallest Action

This tier passed. The process runtime closes after excluding the superseded
all-actor profile table and adding only adjacent SComponent/host services. It
links and executes root-layer initialization plus one empty frame. Ownership
continues in `ROUTE_B_FOP_BOUNDARY_2026-09-01.md`.
