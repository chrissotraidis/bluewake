# Original PLAYER mirror initialization — 2026-09-06

Base `f3df846`; BW-P4-0126 remains active. This checkpoint removes the
mirror init/vtable gap from the real PLAYER creation closure; it does not
execute PLAYER, mirror draw/update, GPU rendering, or promote P4.

## Implementation and executed evidence

Patch 0162 partitions original d_drawlist mirror/shadow-polygon owners without
substituting their algorithms. The original init method now executes eight
construction/destruction lifetimes in the combined model/particle/camera
probe, alternating explicit and null/default texture arguments. Both resolve
the actual Always SHMREF texture. Aurora texture data address, dimensions and
format match the original resource header. Always admission precedes these
checks; prior System/model tests remain intact.

The private asset generator adds six exact mirror display lists from the
locked DOL plus eight Vec vertices decoded from big-endian float32 data into
exact native C++ hexadecimal literals. All thirteen generated includes remain
ignored. Ten synthetic generator tests pass, covering descriptor/range/identity
validation, deterministic output, exact float32 round trips including signed
zero and exponent extremes, and nonfinite/missing vertex rejection.

Original GXSetArray declares three arguments, whereas pinned Aurora implements
five under the same C symbol. Mirror draw's native branch now uses a separately
compiled, typed Aurora bridge that supplies the full 96-byte array extent,
12-byte stride and little-endian flag. Retail code is unchanged. A recorded
command test verifies the full native pointer and exact metadata bytes.
This test calls the bridge directly; it does not qualify mirror draw. Other
original three-argument call sites remain outside this repair's acceptance.

## Frontier and verification

The creation-only census now fails with 10 unresolved symbols in each mode:
fopKyM_create, fopAcM_getWaterY, dSv_info_c::isSwitch/onSwitch,
dSv_event_c::isEventBit, JPAFieldManager::init, and four JAIZelBasic
voice/BGM methods. Creation plus execute retains 139. These are static
retention counts, not observed startup traces or remaining task counts.

Debug, Release and strict ASan/UBSan pass the combined mirror/model/particle/
camera probe, public 72/72 CTests, and existing private camera/event/sea/
attention/DZB/Toripost/McaMorf/BG/Stone2/Room44 and camera constructor/matrix/
mass regressions. Both source preparers pass (TWW through 0162, Aurora 0001).
Asset regeneration, shell syntax and whitespace checks pass. Protected runtime
and dependency-lock hashes remain unchanged. Audit has only its known
20 tracked local-research files. No GUI or Simulator was launched.

Evidence: ignored `local-research/evidence/route-b-player-mirror-20260906`.
Build `bluewake_route_b_private_player_model_probe` in each Aurora-GX/DVD
tree and pass the validated disc path. Strict execution uses
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
The existing diagnostic water boundary still aborts on use; no fake water,
audio, Link, or successful particle-field behavior is introduced.

Next: compose original save-switch/event-bit and JPA field owners, then
remaining creation requirements toward actual Link execution. No external
blocker.
