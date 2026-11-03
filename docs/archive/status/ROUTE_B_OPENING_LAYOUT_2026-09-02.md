# Route B Opening Layout Qualification

## Result

`BW-P4-0084` passed its first executable opening-scene boundary. One
private-disc process mounts authentic `/res/Object/Opening.arc` through
Aurora DVD and original JKR, expands it to 3,145,536 bytes, invokes original
`J2DScreen::set` on `Opening.blo`, and resolves all eleven pane tags used by
the original `dScnOpen_proc_c` constructor. Debug, Release, and strict
ASan/UBSan pass. The established logo preload regression still completes all
33 requests and emits one opening transition; the scoped foundation matrix
remains 10/10 in all three configurations.

Patch 0033 centralizes target-PC big-endian scalar reads in
`JSUInputStream`, swaps the five raw J2D headers, uses native pointer
arithmetic in `JSUMemoryInputStream`, and makes one explicit `strlen`
narrowing. It changes 53 of 1,869 owner lines (2.84%). The opening sources
remain unchanged: a target-local frontier include set avoids importing the
unrelated `dolzel.pch` J3D/JStudio graph. Patch stack 0001-0033 replays.

## Census

Original `d_s_open.cpp` and `d_s_open_sub.cpp` total 574 lines and compile
strictly. Contrary to the initial hypothesis, they have no direct JStudio
dependency. Their unresolved set is bounded to existing process/scene,
heap/resource, J2D, pane-helper, input, and audio owners.

The authentic execution stop is reconstruction completeness. Checked-in
`dScnOpen_message_c::exec()` asserts unconditionally, while the 0x1D8-byte
retail routine calls original `fopMsgM_msgDataProc_c::stringSet` and manages
message fade/repeat state. The 0x450-byte retail `set_message` also calls
`getMesgHeader`, `getMesgEntry`, `getMessage`, `dataInit`, `stringLength`, and
`stringShift`. Those owners exist inside the monolithic 10,205-line
`f_op_msg_mng.cpp`; substituting completion values would fabricate scene
success.

## Reframed Action

Keep `BW-P4-0084` active as `ROUTE_B_OPENING_MESSAGE_RUNTIME`. Perform one
method-level source/link census for only the retail methods reached by
`dScnOpen_message_c::set_message/exec`, including their message archive/font
inputs. Select either an authentic bounded source slice or a materially
different route. Do not import all of `f_op_msg_mng.cpp`, implement a
simplified text parser, weaken pane coverage, or widen into title/J3D/actors.

Evidence is preserved under
`local-research/evidence/route-b-opening-layout-v1-20260902/`. Evidence README
SHA-256: `7811db319a548c060a0152a6ff2d3d7e95fb67e42b24ed0adef7b16d1f07ca89`.
