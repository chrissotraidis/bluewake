# Route B Dynamic-Wave Timer Qualification

**Date:** 2026-09-02
**Blocker:** `BW-P4-0091`
**State:** `ADVANCE`

## Scope

Patch 0045 partitions four complete retail definitions from
`m_Do_audio.cpp` without editing their bodies:

- `mDoAud_zelAudio_c::mLoadTimer`
- `mDoAud_zelAudio_c::calcLoadTimer`
- `mDoAud_setSceneName`
- `mDoAud_load1stDynamicWave`

The executable links these definitions to unchanged play `phase_01` and the
qualified behaviorally equivalent `JAIZelBasic::load1stDynamicWave` owner.
`JAIZelBasic::setSceneName` is observed at its direct seam; no readiness value
or timer value is substituted.

## Contract

The focused test proves:

1. Scene selection calls `JAIZelBasic::setSceneName` with the exact name,
   room, and layer, then arms `mLoadTimer` to 36.
2. Unchanged `phase_01` returns `cPhs_INIT_e` while the timer exceeds one.
3. Original `calcLoadTimer` advances the timer from 36 to one.
4. At one, unchanged `phase_01` invokes the reconstructed load once, clears
   the timer, and returns `cPhs_NEXT_e`.
5. A subsequent phase call remains `NEXT` without a second load.
6. A second scene-name request while the timer is live is ignored.

## Verification

- Debug: 14/14 foundation tests pass.
- Release: 14/14 foundation tests pass.
- ASan/UBSan: 14/14 pass with `halt_on_error=1` and leak detection disabled
  because Apple ASan does not support it.
- Patch replay: 0001 through 0045 pass `scripts/prepare_route_b.sh --check`.
- Adaptation: 13 guard/include lines over 273 source lines, 4.76%; retained
  body adaptation is zero.

## Successor

`BW-P4-0092` owns exact play `phase_0`: original all-wave readiness, event-bit
selection of `LkD00.arc` or `LkD01.arc`, old-archive unmount, and the original
asynchronous ARAM mount request. Stage-resource phase 1 and play `phase_4`
remain closed.
