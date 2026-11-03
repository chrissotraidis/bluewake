# Route B Stone2 Source Portability - 2026-09-06

## Boundary

After exact GRASS composition, the smallest remaining Room44 cohort is the
single `Stone2` request produced by the `Ekao` ACTR record. Its 812-line
translation unit stopped before link or execution at five strict target-PC
compile errors: two float zeroes initializing integer fields, two unsigned
`0xffff` constants initializing signed sentinels, and direct access to the
retail-only `dCamera_c` body omitted from the portable camera process.

## Result

Patch 0134 closes only those representation boundaries. The two zero literals
now match their integer fields, the two negative cull sentinels are expressed
as `-1`, and target PC routes the damage-only camera lockoff through a named
host fence. The original retail camera call is unchanged outside target PC.

The complete original `d_a_stone2.cpp` now compiles under the standard strict
Route B flags in Debug, optimized Release, and ASan/UBSan. Its object target is
part of the default Route B build so future full matrices cannot silently
regress this source frontier. No profile method is executed and no resource,
model, collision, or move-BG claim follows from this checkpoint.

## Evidence

- Debug Stone2 object SHA-256:
  `bd6c7f77ece59681bef7d405cf6cff2cd97f99e36bea07940bbd998669c079a9`.
- Release Stone2 object SHA-256:
  `498d073fdce2e3e00545ac2e581fee9b219e501fd657debe76acf036b621f7e1`.
- ASan/UBSan Stone2 object SHA-256:
  `2acad126de53a14acc369ae3b8e92ad089f5fbdac514ed521fbe625a210cbd13`.
- All 65 registered public tests pass in Debug, optimized Release, and strict
  ASan/UBSan with the Stone2 object in each default build.
- Patches 0001 through 0134 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- The repository audit remains failed only by the known 20 intentionally
  tracked `local-research/` evidence files; this checkpoint adds none.

## Limitations

Stone2 is not linked into a runnable profile probe, registered with the process
runtime, or composed under ROOM_SCENE. Its current object has a broad unresolved
owner surface spanning resource phases, J3D model creation/draw, move-BG,
background and collision services, particles, event/audio state, camera
lockoff, and teardown. Carry, drop, damage, item, particle, and camera behavior
remain explicitly closed.

## Next Boundary

Measure and partition the Stone2 link closure around its exact Room44 wait-mode
path. Keep resource phase ownership, J3D model creation, move-BG construction,
one execute/draw cycle, and deletion as the intended live slice; classify the
damage/carry/event/audio/particle/camera branches as explicit dead fences.
Then register and execute one standard synthetic `Ekao` request before exact
parent composition or admission of any other remaining profile.
