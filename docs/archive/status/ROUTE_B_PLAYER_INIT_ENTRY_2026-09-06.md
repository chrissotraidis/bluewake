# Actual PLAYER initialization entry — 2026-09-06

Base `c04ebbd`; BW-P4-0126 remains active. Actual original phase two now
executes in a new optional diagnostic; it does not finish.

## Reproduced and repaired texture defect

Real Link archive admission initially fails strict UBSan in
J3DGDSetTexLookupMode: -55.68 is outside unsigned-char conversion range.
The original expression casts lodBias*32 directly to u8. Retail translated
instructions at 0x802D741C–0x802D742C perform fmuls, signed fctiwz,
store/load and low-eight-bit rotation/masking. Patch 0165 introduces a signed
s32 intermediate only on the native branch, then narrows to u8. It does not
change retail code or clamp legitimate signed bias.

Eight direct J3D GD command captures verify negative/zero/positive texture
bias encodings, including the failing -1.74 case yielding byte 201. These
checks use GD command storage, not Aurora's separate GXInitTexObjLOD path.
They cover valid texture-domain inputs; arbitrary nonfinite/out-of-range LOD
admission is not qualified. The previous GD current pointer is restored.

## Actual initialization reachability

The new private_player_init_probe derives the existing combined model/camera
fixture, adds actual PLAYER objects and a diagnostic-only audio boundary file.
It loads /res/Object/Link.arc through original dRes, mounts Lkanm.arc as the
animation archive, supplies cleared aligned process storage, initializes the
original cMl heap, publishes the player pointers and calls original phase_2.
Phase two performs original placement construction, playerInit and createHeap.
No alternate Link implementation or successful audio stand-in is used.

Camera/stage state and player pointer publication are diagnostic inputs.
Normal process admission, phase one, original boot, actual camera updates
following Link, PLAYER execute/draw and teardown are not tested here.
All four previously unresolved audio methods have explicit abort-on-use
definitions in this diagnostic alone. The production censuses retain their
real missing-symbol boundary. Two BGM bodies exist upstream; two Link voice
bodies remain absent. None of these methods is reached before this failure.

## Current runtime stop

All three modes abort (exit 134) at JUTNameTab's mNameTable assertion.
The sanitizer debugger stack identifies:

phase_2 -> playerInit -> fopAcM_entrySolidHeap -> daPy_createHeap ->
createHeap -> entryBrk(resource 86, TSWGRIPMSAB) ->
J3DAnmTevRegKey::searchUpdateMaterialID -> JUTNameTab::getName.

The resource ownership/decoding cause of the missing name table is not yet
proven. Fix that upstream binding next, then replay the same actual init
probe. Do not work around it with missing-material success or blank names.

## Verification

New init-probe builds succeed in Debug, Release and strict ASan/UBSan; all
three reproduce the stated failure. Existing combined model/mirror/water/
save/field/particle/camera probes pass, including eight new LOD encodings.
Public 72/72 and existing private camera/event/sea/attention/DZB/Toripost/
McaMorf/BG/Stone2/Room44 and constructor/matrix/mass regressions pass all modes.
Both preparers pass (TWW through 0165, Aurora 0001); shell syntax, whitespace
and protected/lock hashes pass unchanged. Audit remains the known 20 tracked
local-research files. No GUI or Simulator launched.

Ignored evidence: `local-research/evidence/route-b-player-init-entry-20260906`.
Run the optional target with the validated disc path; strict environment is
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Debugger failure stack was captured using LLDB's on-crash backtrace command.
This checkpoint records a reproducible runtime frontier, not a passing PLAYER
initialization or P4 gate.
