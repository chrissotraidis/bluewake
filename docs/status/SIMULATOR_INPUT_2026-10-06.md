# Targeted simulator input pass — October 6, 2026

## Result

Found and repaired a stale-touch defect in the Apple mobile overlay. Opening touch settings cleared
the pad sent to the game but retained the overlay's held buttons and sticks. After closing settings,
another touch could publish the old input again. This is separate from physical-controller deadzone
#138, remapping #66 and camera inversion #73; none of those reports is closed by this repair.

The overlay now uses its existing full `clearTouchInput` method before presenting settings, alerts,
the file picker and share sheet. The ellipsis menu calls the same method through a weakly captured
callback. This clears local buttons, stick positions and button appearance as well as the game-facing
pad. The runtime, desktop input, saved mappings and experimental flags are unchanged.

## Matched reproduction and verification

- Base: BlueWake `273cb9d`; RecompCore `d5b92fbb7d1d2ccd05d72d448e512dc06120efc9`.
- Release arm64 host on the iOS 26.5 iPad Pro 12.9 simulator, on Chris's M3 Max Mac.
- Same copied Outset checkpoint and private scratch card/settings; live pad enabled, scripted guest
  button injection removed, both cache-flush and deferred-DVD candidates off.
- Existing UIKit handler hooks: `BLUEWAKE_TOUCH_TAPS="2@A@20;15@B@0.2"`,
  `BLUEWAKE_SHELL_DEMO=settings`, `BLUEWAKE_SHELL_DEMO_AT=5`, `BLUEWAKE_PAD_TRACE=1`.
  Settings opens at 5 seconds and closes at 13; the simulated A hold ends at 22.
- Before: pressing B after closing settings sends `0x0300` (A+B), then releasing B sends `0x0100`
  (stale A). The old A remains until its scheduled release.
- After: the same B press sends `0x0200` (B only), then `0x0000` on release. The late A release is harmless.
- One follow-up holds the movement and camera sticks at half deflection across the same panel cycle:
  `2@move:0.5,0@20;2@c:0,0.5@20;15@B@0.2`. Both axes remain neutral after closing settings and during B.
  `BLUEWAKE_KEY_TAPS="17@j@0.2"` still sends A and then neutral through SDL's keyboard path.
- The settings pause reason is `0x4` while open and `0x0` on close; the game resumes.

These are controlled UIKit-handler tests with guest pad traces, not physical finger/controller tests.
The desktop UI tool could not attach to Simulator, so opening each menu and system sheet manually
was not checked. The analogous call sites were reviewed and compiled; only the touch-settings panel
was exercised in the matched game runs. Simulator timing is not an iPad performance measurement.

## Settings persistence probe

A temporary arm64 simulator executable compiled the actual `controller_settings.mm` against
Foundation and SDL headers, using its own `BlueWakeInputProbe` preference domain. Three separate
processes verified:

1. Assigning A to B's native button swaps B back to A's old button without duplicating mappings.
2. The swap and both camera-inversion preferences survive process exit and reload.
3. Reset Buttons restores all six defaults, persists across another reload, and preserves inversion.

All assertions passed. This verifies the settings backend, not menu selection, controller reconnect,
physical input or camera direction on land/swimming/boat. iOS camera inversion applies to physical
controller axes; touch C-stick behavior cannot establish that hardware result.

## Checks and remaining work

- Simulator app rebuilt and installed in place; matched before/after and stick/keyboard runs passed.
- Repository audit, attribution and whitespace checks passed. Required Windows host CI is tracked on
  the accompanying PR; the Apple overlay is not compiled by that job.
- Existing simulator files were backed up and compared after testing. No private input or raw evidence
  is committed. The physical iPad and Windows machine were not used.
- No new reporter evidence changed the prioritized rendering/audio/performance investigations during
  this pass. Follow [TECH_DEBT.md](../TECH_DEBT.md) for those remaining gates.
- Next Apple acceptance: on-device touch hold → menu/panel → resume, plus physical remapping and camera
  checks. Windows requires its separate controller tests; this Apple-only change needs no Windows fix.
