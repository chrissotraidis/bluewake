# Route B Play-State Owner - 2026-09-02

## Scope

`BW-P4-0103` follows the fully owned six-call world prefix in retail play
phase 4. Before the phase mutates player, camera, render-window, and draw-view
state, Route B needs a real lifetime for the production `dComIfG_play_c`
layout. This tier qualifies that lifetime only; it does not claim the setter
band or global draw-list ownership.

## Census

A full-header static construction census compiled on Apple Silicon and
retained 27 undefined symbols. Ten represented production owners. Existing
background/collision tiers already supplied `cBgS_ChkElm::Init`,
`dCcMassS_Mng`, and `cCcS`; the new compact source band was:

- `dADM` and `dADM_CharTbl` construction;
- `dEvt_control_c`, `dEvt_order_c`, and `dEvDtBase_c::init`;
- `dVibration_c::Init/setDefault` and construction;
- `dDetectPlace_c` and `dDetect_c` construction; and
- original `dComIfG_play_c::ct/init`.

The preserved census is in
`local-research/evidence/route-b-play-owner-census-v1-20260902/`. Its object
SHA-256 is
`6888609307323fee7d2790b70a433e66292e8851dba6e6553020c64743e81169` and its
symbol-list SHA-256 is
`5d7a9a2e258a0b0a5df37bcc9e1e3d4a5bcb5d282bfa07868e89bf14dd311fa1`.

## Implementation

Patch 0080 adds target-PC-only partitions to six original translation units.
The PowerPC/default route is unchanged. The public tier links those retained
bodies with the already qualified real background and collision owners.
Unentered attention, vibration, stage-vtable, reporting, and destruction
paths are aborting test fences.

The test placement-constructs the real `dComIfG_play_c` in aligned static
zero-initialized storage. This is intentional: the retail game-info owner has
BSS storage, and portions of the constructor graph rely on that initial zero
state. The test does not invoke host shutdown destruction, which is outside
the console process lifetime being qualified.

The unchanged constructor establishes:

- zero render windows;
- null particle and demo owners;
- Link demo animation index `-1`;
- ship, ship-room, raft, and prior raft-room IDs `0xff`;
- null primary player and all three player pointer slots; and
- primary player camera ID `-1`.

Patch 0080 adds 48 partition/include lines across 4,286 reached source lines,
an adaptation density of 1.12%.

## Verification

The complete public matrix passes:

- Debug: 27/27;
- optimized Release: 27/27; and
- strict ASan+UBSan: 27/27.

No BlueWake process or additional simulator was launched for this tier.

## Boundary

Patch 0081 subsequently retains original
`dDlst_window_c::setViewPort/setScissor` and exercises the real play inline
player/window/camera setters. Non-default mutation is directly verified for
the player and all three player-pointer slots, camera pointer and IDs, cleared
attention state, render-window count, viewport, scissor, near/far depth, and
window camera ID. The complete 27-test matrix passes in all three build modes.

Patch 0082 then retains original `dDlst_list_c` construction. Its census
requires the already-qualified real J2DPicture and J3DPacket owners, the real
J3DDrawBuffer destructor, and one aborting unentered line-packet draw virtual.
A production object constructed in zeroed storage proves initial null, dirty,
and cleared window/viewport/view pointers in all three build modes. The patch
adds 17 partition/observer lines across 2,873 touched source/header lines
(0.59%). Census object SHA-256 is
`4b5c65423de917a1c9b0cc5db9d85dce4a139ddd73c1a68f7327313d9c226058`;
undefined-symbol list SHA-256 is
`f9d60bdf0288387d51335ba8dde9f5bfc7403a152db81e4e208270cf0892d6e4`.

Patch 0083 subsequently composes all ten exact phase-4 state calls through
those qualified owners and stops before message heap creation. Exact call
order, arguments, 608x448 host render dimensions, and dirty-to-retail state
pass in all three build modes. The complete composition record is
`ROUTE_B_PLAY_PHASE4_VIEW_STATE_2026-09-02.md`.

Message heap, stage, HIO, draw-buffer initialization, rendering, callback
execution, and fabricated game-info state remain closed.
