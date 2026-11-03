# Route B Play LkD ARAM Qualification

**Date:** 2026-09-02
**Blocker:** `BW-P4-0092`
**State:** `ADVANCE`
**Input alias:** `GZLE01-disc-image`

## Public Contract

`tests/route_b_play_phase0_test.cpp` executes unchanged play `phase_0` with
the complete original `JAInter::BankWave::checkAllWaveLoadStatus` owner. Four
paths prove:

- wave status 1 returns `cPhs_INIT_e` without state or mount activity;
- an already-matching LkD index advances without remounting;
- event clear selects `/res/Object/LkD00.arc`;
- event set unmounts an old index-0 archive and selects `LkD01.arc`;
- both new requests use default mount direction and `MOUNT_ARAM`.

The full public foundation matrix passes 15/15 in Debug, Release, and strict
ASan/UBSan.

## Private Vertical Contract

`tests/route_b_private_play_lkd_probe.cpp` composes the same unchanged
`phase_0` and `phase_1` with original `mDoDvdThd`, JKR DVD/ARAM archive and
transfer owners, the promoted host worker, Aurora MEM1/ARAM, and the private
disc reader.

The event-clear route selects `/res/Object/LkD00.arc`. Unchanged `phase_1`
waits until the original command completes, publishes a mounted ARAM archive
with 456 files, deletes the command, installs process ID 7 and start stage
`sea_T:44`, and reaches exactly one `Stage` request and one material-create
call. Those final two calls are recorded seams and are not stage-resource or
J3D acceptance.

Debug, Release, and strict ASan/UBSan runs produce the same result. No source
patch is added for this tier, no private bytes are retained, and no DSP policy
is selected.

## Successor

`BW-P4-0093` owns original stage-resource admission. First census authentic
`Stage.arc` resource types and compile the exact `dRes_control_c` closure once;
do not widen the earlier System-only resource fence or J3D graph without that
measurement.
