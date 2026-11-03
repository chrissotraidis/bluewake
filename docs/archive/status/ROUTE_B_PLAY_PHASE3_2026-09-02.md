# Route B Play Phase 3 Barrier

**Status:** qualified on 2026-09-02.

## Source Contract

Unchanged `phase_3` in `ref/tww/src/d/d_s_play.cpp` has two ordered wait
conditions. A non-null particle command must report synchronized before
`JAIZelBasic::check1stDynamicWave` is consulted, and the phase advances only
when the resulting wave status is zero.

The compiled source body has zero semantic edits. Its existing Route B
partition only excludes unrelated phases and supplies direct includes.

## Public Matrix

`tests/route_b_play_phase3_test.cpp` executes five cases:

- null command and ready waves advances;
- null command and busy waves waits;
- busy command waits without querying either wave bank;
- ready command and busy waves waits;
- ready command and ready waves advances.

The narrow audio fixture has valid C++ lifetime and an asserted 702-byte
native prefix before `mFirstDynamicSceneWaveIndex`, matching the compiled
target-PC layout. The command fixture has real class lifetime and uses the
promoted atomic `publishDone` / `sync` contract. It does not simulate a DVD
transfer; that is covered by the private route.

## Authentic Route

`tests/route_b_private_particle_request_probe.cpp` starts the original DVD
worker, loads all 168,256 bytes of authentic `Pscene001.jpc`, verifies them
against an independent disc read, selects two first-wave banks at status 2,
and calls unchanged `phase_3` on the completed command. Debug, optimized
Release, and strict ASan/UBSan all advance. All 19 public tests also pass in
the same configurations.

Private logs and hashes are preserved under
`local-research/evidence/route-b-play-phase3-v1-20260902/`.

## Successor

`dPa_control_c::createScene` now passes over those authentic bytes, preserving
seven common emitters and adding zero scene emitters from the 3-resource / 5-
texture Outset archive. The next action is one owner census of the remaining
play `phase_4` body. Particle callback execution and world construction remain
closed.
