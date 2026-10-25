# Route B KYTAG01 Profile Execution - 2026-09-02

## Boundary

Execute one exact Room44 request through the original standard process and
actor lifecycle before admitting the other 178 visible-room requests.
KYTAG01 was selected by the profile census because it has no archive, model,
collision, draw-packet, or child-process fanout.

## Result

The original `g_profile_KYTAG01`, original `fopAc_Create`, and original
KYTAG01 create, execute, draw, is-delete, and delete methods now run through
the native process framework. The test proves:

- static-REL profile lookup and standard request allocation;
- room 44, parameters, position, angles, scale, and process ownership;
- preservation of the room layer and profile list after actor construction;
- one environment wave influence with 1,000 / 2,000 radii;
- successful execute, draw, and is-delete methods; and
- wave-slot and wave-count cleanup through the original delete method.

The first linked run crashed in `fpcLyTg_ToQueue` because `fopAcM_ct` used
`new (ptr) ClassName()`. Modern C++ value-initialized the implicit outer actor
class and zeroed the process header that `fpcBs_Create` and `fopAc_Create` had
already populated. Patch 0122 uses placement default-initialization on
`TARGET_PC`, which still runs actor constructors but preserves framework-owned
state. This is a shared native actor-lifecycle correction, not a KYTAG01
special case.

The actor core's 18-symbol link closure is represented by existing qualified
owners plus a focused fence module. Reached TEV initialization reproduces the
retail field state. Unreached menu, demo, map, culling, audio-deletion, and
actor-heap services remain fail-closed or inert only where the KYTAG01 route
cannot call them; they are not claimed as complete runtime services.

## Verification

- Patch stack `0001-0122`: replay check passes.
- Debug: 61/61 CTest tests pass.
- Optimized Release: 61/61 CTest tests pass.
- ASan/UBSan: 61/61 CTest tests pass with `detect_leaks=0` because macOS does
  not support LeakSanitizer in this configuration.
- Patch SHA-256:
  `801c12aa563d431e148e43b016e378d2822bf7e9824df422fd5dadce321e1444`.

No BlueWake process or Simulator was launched.

## Next Boundary

Promote the original GRASS batch-contributor profile next. It represents 119
of Room44's 179 requests, compiles unchanged, and has a bounded 22-symbol
owner frontier. Prove the exact title-request parameter distribution, shared
grass/tree/flower packet publication, generated instance counts and room
ownership, and the intentional `cPhs_ERROR_e` proxy disposal. Keep Obj_Wood,
BG, the remaining profiles, phase 4, BlueWake, and Simulator closed.
