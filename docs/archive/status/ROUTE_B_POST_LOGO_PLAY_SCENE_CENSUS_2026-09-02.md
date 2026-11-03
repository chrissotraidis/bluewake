# Route B Post-Logo Play-Scene Census

**Recorded:** 2026-09-02  
**Blocker:** `BW-P4-0088`  
**Result:** `BW-P4-0088` and `BW-P4-0089` resolved; `BW-P4-0090` active

## Route Correction

The unchanged logo path calls `dComIfG_changeOpeningScene` with
`fpcNm_OPENING_SCENE_e`. The profile table maps that name to
`g_profile_OPENING_SCENE` in `d_s_play.cpp`, using `dScnPly_ply_c` and its
ten-phase stage initializer. The newly qualified `dScnOpen_c` instead owns
`fpcNm_OPEN_SCENE_e` and `fpcNm_OPEN2_SCENE_e`, the separate story-scroll
route. Story-scroll presentation is valid progress but is not post-logo boot
continuity.

The original f_pc/f_op scene request, overlap, and manager sources already
compile in the Route B foundation. The first missing post-logo owner is the
play-scene profile and initializer, not a new host scene-request service.

## Compile Census

An unchanged target-PC compile of the 1,567-line `d_s_play.cpp` fails before
the source body with 16 strict errors across eight transitive J3D, JStudio,
JGadget, and GD headers. Reusing the existing opening frontier repeats the
same broad-header shape and reaches Clang's 20-error cap through
`d_s_play.h`, actor, audio, and J3D declarations.

The two attempts materially repeat the same experiment shape. Whole-unit
header portability is therefore closed under the goal loop's anti-stall rule;
the result does not justify patching J3D/JStudio header by header.

## Next Boundary

Partition the exact original `heapSizeCheck` and `phase_00` definitions behind
a target-only compile tier. Supply only their directly reached heap, audio,
graphics, reset, title, tribox, and game-state declarations, then normalize
the unresolved symbols. Later play-scene phases remain fail-closed. The first
executable proof must retain original phase ownership and may not replace
readiness with a success stub.

## Phase 00 Result

Patch 0041 retains the exact original `heapSizeCheck` and `phase_00` bodies.
One missing direct `JUTAssert` include was the only compile error after
partitioning. The resulting object has 25 normalized unresolved calls owned
by heap health, audio stream/reset state, graphics reset actions, and
game-state reset.

The focused executable proves five source-owned paths:

- stream-buffer busy returns `cPhs_INIT_e` without side effects;
- healthy opening returns `cPhs_NEXT_e` and initializes game state;
- reset recovery invokes every original audio, graphics, title, tribox, and
  next-stage reset action;
- low-memory opening requests reset with flag `0x80000000`; and
- low-memory `PLAY_SCENE` emits the original warning without game-state init.

Release and strict ASan/UBSan pass. The retained 84-line retail body has zero
semantic changes. The 20 target-only guard/include additions are 1.28% of the
1,567-line source unit.

## Successor Boundary

`BW-P4-0089` groups exact `phase_01` through `phase_3` as one cohesive
asynchronous bootstrap census: first-wave readiness, optional `LkD` ARAM
mount, stage-resource admission, stage-info and particle-scene load, and final
readiness. The large world-construction body in `phase_4` remains fail-closed.
Compile this boundary once, normalize its owner set, and then choose the
smallest authentic executable chain supported by already-qualified owners.

## Bootstrap Census Result

Patch 0042 retains exact `phase_01` through `phase_3` and excludes `phase_4`.
All 92 retained source lines compile on the first bounded attempt. The only
warning is macOS deprecation of unchanged retail `sprintf`. No retained body
line changes; the patch adds 18 partition/include lines, 1.15% of the full
unit. Cumulative play-owner partitioning is 38/1,567 lines (2.42%).

The resulting object has 35 unresolved symbols: 29 product/runtime owners and
six C++/libc toolchain symbols. One frontier correction was required before
recording the set: `checkAllWaveLoadStatus` is inherited inline from
`JAIBasic` and delegates to `JAInter::BankWave`, rather than belonging to
`JAIZelBasic`.

Archive mount/synchronization and BankWave load/status foundations already
exist in Route B. Authentic execution first stops at source incompleteness:
`JAIZelBasic::check1stDynamicWave()` is an empty nonmatching body with no
return, and `load1stDynamicWave()` is a nonmatching logging placeholder.
Checked-in generated C preserves the exact GZLE01 ranges at `0x802AB678`
(0x7c bytes) and `0x802AB374` (0x304 bytes). `BW-P4-0090` starts with a
differential reconstruction of the smaller check owner; no readiness constant
may stand in for it.

## Dynamic-Wave Check Result

Patch 0043 reconstructs `JAIZelBasic::check1stDynamicWave` in its original
source owner. The implementation follows the complete 0x7c-byte GZLE01 range:
select both bank IDs from `m_dy_wave_set_1st`, skip zero IDs, query nonzero IDs
in table order, compute `2 - status`, and pack the results into the high and
low bytes.

An independent 36-case matrix covers every zero/nonzero bank pair and all
status values 0-2. It verifies result packing, query count, and query order in
Debug, Release, and strict ASan/UBSan. This is behaviorally equivalent
reconstruction, not a byte-match claim. `BW-P4-0090` remains active at the
larger 0x304-byte `load1stDynamicWave` owner, whose acceptance must compare
ordered BankWave mutations and final field state across every branch family.

## Dynamic-Wave Load Result

Patch 0044 reconstructs `JAIZelBasic::load1stDynamicWave` from the complete
0x304-byte GZLE01 range. An independent 2,048-case matrix varies all three
branch triggers, presence of both entries in both old tables, both direct
field-selected banks, and both new table entries. It compares exact operation
order: second-bank-first cleanup, then erase, status zero, group minus one;
direct and new-table loads follow in retail order. It also verifies every
fixed field reset and the two diagnostic arguments.

Five separate cases prove all BGM exceptions: three preserved sub-BGM IDs,
the preserved island main-BGM multiplier, and the morning sound-state flag.
Debug, Release, and strict ASan/UBSan pass. This is behaviorally equivalent,
not byte-matched, and does not claim DSP acceptance. `BW-P4-0091` now owns the
complete original `m_Do_audio.cpp` 36-frame timer path into unchanged
`phase_01`.
