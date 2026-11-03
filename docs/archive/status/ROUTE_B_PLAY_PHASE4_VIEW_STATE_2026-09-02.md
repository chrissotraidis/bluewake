# Route B Play Phase 4 View State - 2026-09-02

## Result

Patch 0083 extends the unchanged retail `phase_4` body beyond its six fully
owned world-root calls through the next ten source-ordered operations:

1. clear primary player info;
2. clear three player pointer slots;
3. select one render window;
4. set its viewport/scissor from host framebuffer dimensions;
5. clear primary camera info; and
6. clear draw-list window, viewport, and view pointers.

The tier returns immediately before `fopMsgM_createExpHeap`; no message heap,
stage, HIO, attention, vibration, actor initializer, or later phase branch is
entered.

## Ownership

The phase source compiles against the bounded play frontier. A split bridge
keeps frontier function symbols separate from full production headers, then
delegates them to one real `dComIfG_play_c` and one real `dDlst_list_c` built
from patches 0080-0082. It also owns the actual
`mDoMch_render_c::mRenderModeObj` pointer with a 608x448 render mode.

Before phase entry, all reached player, camera, window, viewport, scissor, and
draw-view fields are deliberately dirty. The bridge records all ten calls and
asserts their exact arguments. After entry it proves null player/camera/view
state, one render window, camera IDs `0/0/-1`, depth `0/1`, and matching
608x448 viewport/scissor dimensions. The earlier real particle, background,
collision, demo, sea, and snapshot owners execute first and retain their
existing assertions.

This is not a fake `g_dComIfG_gameInfo`: no aggregate singleton is defined.
The adapter exists only at the already bounded frontier ABI and delegates to
production-layout owners whose construction and mutation passed independently.

## Verification

- Debug: 29/29 tests passed.
- Optimized Release: 29/29 tests passed.
- Strict ASan+UBSan: 29/29 tests passed.
- Patch 0083 changes 10 lines in the 1,624-line source unit (0.62%).
- No BlueWake process or additional simulator was launched.

## Next Boundary

`BW-P4-0103` is resolved. `BW-P4-0104` begins at original
`fopMsgM_createExpHeap(0x73EA1)`, a one-line delegation to
`JKRCreateExpHeap(size, mDoExt_getGameHeap(), FALSE)`, followed by non-null
assertion and publication to the play owner. Stage creation and every later
phase operation remain closed.
