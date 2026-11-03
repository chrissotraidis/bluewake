# Original Link BRK admission and sampling — 2026-09-06

Base `cdc1724`; BW-P4-0126 remains active. Actual PLAYER initialization
advances but does not complete.

## Root cause and repair

Native dRes selected BTK and BPK for J3D animation loading but left BRK
resources as archive bytes. PLAYER entryBrk cast that serialized pointer to a
J3DAnmTevRegKey, producing the previous null-name-table assertion.

Patch 0166 adds BRK to the original native resource-loading selection.
It applies the existing animation big-endian scalar wrappers to the BRK
header, integer offsets, material IDs and signed channel samples. Original
setAnmTevReg converts offsets through explicit 32-bit integers; original
color/konst interpolation reads the endian-aware sample type. Name-table and
key-table support reuse already-qualified native representations. Original
binding/interpolation algorithms and retail layouts remain unchanged.

These animation objects reference their archive-backed tables. This work
does not claim arbitrary malformed-file validation, independent archive
teardown safety or full save/gameplay acceptance.

## Executed independent checks

The init probe enumerates all 11 BRK resources in the real Link archive and
requires each published animation object to differ from its serialized
resource address. Header frame limits/counts and all 24 material names are
checked against bounded raw-byte reads.

All color and konst channels are sampled at quarter frames, including before
and after their duration, for 10,928 comparisons. The independent oracle
uses raw big-endian signed reads, linear knot search and double-precision
Hermite basis evaluation, not the original native sampler's search/math.
At most one integer of float/double rounding difference is accepted.
Original frame state is restored afterward.

Original searchUpdateMaterialID binds resource 86 (TSWGRIPMSAB) to the real
SWGRIPMS model. Every resulting ID must match an independent linear scan of
model material names, with no missing match accepted.

Passing resource checks use the explicit third argument --resources-only;
that branch ends before PLAYER initialization. Normal invocation still runs
actual phase two. It now passes the old BRK failure and strict UBSan stops at
J3DCluster.cpp:311: a packed display-list vertex count is read through an
unaligned u16 pointer. Nearby position/normal index reads use the same
native-endian/alignment assumption. This is the next owner to repair.

## Verification

Resource mode passes Debug, Release and strict ASan/UBSan with the prior
combined model/mirror/particle/field/save/water/camera checks. Public 72/72 and
existing private camera/event/sea/attention/DZB/Toripost/McaMorf/BG/Stone2/
Room44 and constructor/matrix/mass regressions pass all modes.
Full initialization is checked under strict sanitizers and remains an
intentional recorded failure (134), not a passing regression.

TWW preparer through 0166 and Aurora 0001 checks pass. The Aurora script is
not executable in this checkout; its check was explicitly rerun successfully
with bash after direct invocation reported permission denied. Shell syntax,
whitespace and protected/lock hashes pass unchanged. Audit retains its known
20 tracked local-research files. No GUI or Simulator launched.

Evidence is ignored under
`local-research/evidence/route-b-player-brk-20260906`.
Build `bluewake_route_b_private_player_init_probe`, pass the validated disc,
and add --resources-only for the passing resource check. Strict environment:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
No external blocker; next is original skin-deform packed-index decoding,
then actual initialization, updates and visible control.
