# Route B KYEFF2 Process Creation - 2026-09-02

## Result

Patch 0091 retains the original `dScnKy_env_light_c` global owner,
constructor, and reached color helpers from `d_kankyo.cpp`, plus unchanged
`dKyw_wether_init2` from `d_kankyo_wether.cpp`. The complete unchanged
`d_kyeff2.cpp` profile and `f_op_kankyo.cpp` generic owner run through the
real process runtime and standard-create request.

The test models the authentic topology: the ten-list root owns a live
16-list node layer, matching `fpcNd_Create`, and the request runs on that
current layer. The completed KYEFF2 process is discoverable by ID in retail
list `0x0C`. Its profile pointer, profile/name IDs, environment submethod,
draw priority, draw tag, creation result, and initialized state all agree
with the original profile. Dirty `mVrkumoStatus` and `mVrkumoCount` values
reset to zero through the original create callback.

This qualifies KYEFF2 process creation only. The profile's delete, execute,
and draw callbacks remain linked but are fail-closed fences; no weather move,
packet draw, or deletion behavior was exercised.

## Census

- Original KYEFF2 profile object SHA-256:
  `f06da2445ecde8a981efd31221004530f375820308cb445da6166fa15177ca54`.
- Original generic environment-process object SHA-256:
  `a6513ddf9c85773133fad55ec3d9415c64065fb7d90f97e9e01defc7cdfe041f`.
- Partitioned environment-light owner SHA-256:
  `d509bff8731eecca5278fe74a1e21d226d8e78a3e549a25cc7ae3c43cb3a600c`.
- Partitioned weather initializer SHA-256:
  `5ad674a236f47a9bafb6649d3804652e3c26e2415d776122ebf86da2841cc288`.
- The environment-light owner has no product imports. The initializer imports
  only `g_env_light`. The unchanged profile retains the generic leaf and
  environment method tables plus its four weather callback symbols.

## Adaptation

Patch 0091 adds 15 partition/include lines to the original 3,721-line
`d_kankyo.cpp` and 11 to the original 1,433-line
`d_kankyo_wether.cpp`: 26/5,154 lines, or 0.50%. No entered retail function
body changes.

## Verification

- Debug: 34/34 tests passed.
- Optimized Release: 34/34 tests passed.
- Strict ASan+UBSan: 34/34 tests passed with unsupported macOS leak detection
  disabled and halting address/undefined-behavior checks enabled.
- Patch replay passes through patch 0091.
- No BlueWake process or Simulator was launched.

## Next Boundary

Measure and promote the shared target-PC `camera_process_class` base/view
contract needed to compile original KANKYO and KYEFF. Keep ENVSE's embedded
`dCamera_c` access, weather lifecycle execution, and aggregate stage creation
separate.
