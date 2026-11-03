# Route B Stone2 Profile Execution - 2026-09-06

## Boundary

The exact Room44 DZR contains one `Stone2` request: ACTR index 144, archive
type `Ekao`, parameters `0xf37f6c3f`, position
`(-208620.0, 2300.0, 317730.0)`, Y angle `14381`, Z parameter `255`, room 44,
and set ID `0xffff`. After patch 0134 made the complete source compile, its
unresolved link closure still mixed the desired stationary wait lifecycle with
carry, drop, damage, event, audio, item, particle, and camera behavior.

## Result

Patch 0135 partitions only the reached wait-mode lifecycle. A standard process
request now enters original `g_profile_Stone2`, consumes the exact append,
acquires and synchronizes `Ekao`, creates the original J3D model at resource
index 4, creates and registers its move-BG collision from index 7, and finishes
the original actor create method. Two unchanged profile executes decrement the
retail wait counter from 2 to 0, lock the move-BG object, publish the collision
cylinder twice, and update the actor matrix, attention point, eye point, and
culling sphere.

Original draw selects the BG list, performs the source model update/view path,
submits one authentic material packet through the real J3D draw buffers, emits
the reached environment-light call contract, observes the exact type-3 shadow
scale `0x144`, and restores the default buffers. Standard delayed deletion then
enters the original actor and move-BG delete methods, releases the exact
collision pointer, destroys the actor solid heap, and decrements the `Ekao`
resource owner before the fixture releases its final archive reference.

The target-PC ABI puts this polymorphic actor's source-visible `fopAc_ac_c`
subobject eight bytes after the standard process allocation's retail header.
The wait tier therefore mirrors the standard append into that source-visible
base and reads parameters from the unshifted process header. Heap teardown is
returned to the source-visible owner before generic deletion. These are named,
opt-in host boundaries; retail code is unchanged. Carry, non-wait modes,
camera, and particle emission abort if reached. Ground correction, switch,
event lookup, collision service, lighting/shadow, audio cleanup, and resource
mounting remain bounded call-contract owners.

Patch 0135 adds 93 lines across the 1,276 pre-patch lines of
`d_a_stone2.cpp` and `d_bg_s_acch.cpp`, a measured 7.29% adaptation density.

## Exact evidence

- Private archive: `/res/Object/Ekao.arc`, 16,640 bytes, SHA-256
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Each Debug, optimized Release, and strict ASan/UBSan run reports:
  `room=44 actr=144 params=f37f6c3f type=3`, resource
  `acquire-sync-release`, `model=4`, `dzb=7`, `wait=2`, `lock=1`,
  `collision=2`, `draw=1`, `packets=1`, `shadow=1`, and delayed delete with
  one collision release and one heap destruction.
- Debug probe SHA-256:
  `ebed09ea0c7d86643b6d5aa65db8c9ce58b66f73786d7f023f99612dcbe0f75d`.
- Release probe SHA-256:
  `7a54c39c58a0075afc850bf212038120be279b0024d8f3dc78a75d31c83cd410`.
- ASan/UBSan probe SHA-256:
  `4ae291e5ea104581681d16ac90ed611dbf19f6b7ed0289c3fe09b44e75f6375c`.
- The existing exact BG create/execute/draw/delete/recreate probe also passes
  in all three configurations after its bounded move-BG constructor fixture
  was made deterministic.
- All 65 registered public tests pass in Debug, optimized Release, and strict
  ASan/UBSan.
- Patches 0001 through 0135 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- The repository audit remains failed only by the known 20 intentionally
  tracked `local-research/` evidence files; this checkpoint adds none.

## Limitations

This proves one stationary type-3 `Ekao` lifecycle, not functional ground or
collision queries, carry/drop/damage response, item creation, particle or
audio behavior, camera interaction, raster presentation, or another Stone2
type. The child is independently qualified but is not yet retained by the
exact ROOM_SCENE parent. The other 48 cancelled Room44 requests remain closed.

## Next Boundary

Retain only ACTR 144's exact `Stone2` request beside BG, KYTAG01, Obj_Wood,
and GRASS in the existing private Room44 parent. Prove the child consumes the
parent-owned `Ekao` archive, completes its wait execute/draw contract, and
crosses delayed deletion before BG cleanup and parent resource/archive/heap
teardown. Continue cancelling the other 48 requests and do not broaden the
closed gameplay behaviors.
