# Original PLAYER skin matrix-index setup — 2026-09-06

Base `e9a8ff4`; BW-P4-0126 remains active. Actual init advances, not completes.

## Reproduced representation and ownership defects

Original J3DSkinDeform::initMtxIndexArray reads GX display-list vertex counts
and position/normal indices through u16 pointers. The data are packed and
big-endian; strict UBSan stopped at the first unaligned count read.
Patch 0167 reads these fields from bytes on native hosts, preserving retail
expressions. A missing normal attribute is not dereferenced on the native
branch. Nearby fast-skin count read/write transport receives the same byte
treatment; fast-skin compaction itself is not runtime-qualified here.

After index decoding was fixed, an ownership assertion reproduced plain
new[] allocating both matrix-index arrays outside the intended JKR heap.
They now use the established JKR_NEW path. Algorithmic mapping remains
original; diagnostic-only const array accessors support verification.

## Executed real-data contract

The existing resource-only PLAYER probe uses real Link SHMS model data.
A separate bounded byte parser derives vertex count/stride, packed positions,
normals and matrix slots; all referenced indices and matrix IDs must fit the
model's declared bounds. It reconstructs expected mappings for 452 packed
vertices, 155 positions and 217 normals.

Original initMtxIndexArray runs in a dedicated solid heap and matches every
mapping, including zero-filled unused positions. Both arrays belong to that
heap. A repeated call succeeds without recreating the arrays, and destroying
the heap recovers exact parent free space. Original destructor has no
individual buffer ownership; lifetime follows the actor solid heap.
These checks do not qualify actual vertex deformation, GPU output or the
fast-skin compaction branch.

## New actual-init frontier

Normal invocation now passes the previous skin stop. Strict execution later
reports allocation failures of 0x30008, 0x30000 and 0x24000 bytes, then fails
the nonnull model assertion. LLDB identifies createHeap -> initModel with
resource index 45, the first HBOOTS model, inside the original 0xB0000
estimated actor heap.

The cause of those allocation sizes is not yet established. Next: compare
native model counts/display-list requirements with serialized data and
allocation ownership before changing heap policy. Do not mask an endian or
count defect by enlarging the heap.

## Verification

Resource mode passes Debug, Release and strict ASan/UBSan with BRK and all
prior combined model/mirror/particle/field/save/water/camera checks. Public
72/72 and existing private camera/event/sea/attention/DZB/Toripost/McaMorf/
BG/Stone2/Room44 and constructor/matrix/mass regressions pass all modes.
Full initialization remains a recorded strict-mode failure, not a passing
regression. Both preparers pass (TWW through 0167; Aurora 0001 invoked with
bash). Shell syntax/whitespace and protected/lock hashes pass unchanged.
Audit retains only its known 20 tracked local-research files.

No external blocker, GUI or Simulator launch. Ignored evidence:
`local-research/evidence/route-b-player-skin-indices-20260906`.
Use `bluewake_route_b_private_player_init_probe` with the validated disc and
--resources-only for passing resource tests. Strict environment:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
