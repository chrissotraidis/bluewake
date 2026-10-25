# SunPad component transfer (PRD FR-015, section 10)

BlueWake's iPad shell adapts SunPad's mobile interaction layer. SunPad is a donor for behavior; this
file records exactly what came across so a clean BlueWake checkout never depends on a local SunPad
path.

| field | value |
| --- | --- |
| origin | https://github.com/chrissotraidis/sunpad |
| commit | e43f0ea6b797e5110787171957c9dc3c6213269c (branch main, clean working tree) |
| license | GPL-3.0 for the adapted file; BlueWake's shell file carries the attribution |
| adapted into | apple/ios/src/BWGameOverlay.mm (from apple/ios/SunPadGameOverlay.mm) |
| input seam | apple/ios/src/touch_controls.cpp (BlueWake's own; replaces SunPadInputMixer) |

## Kept

- The stick view (circular base, thumb, +y up, reset on release) and its colors: dark move stick,
  yellow C-stick.
- Button set and colors: green A, red B, light X/Y, purple Z, dark Start, L, R and a four-way D-pad.
- The tablet and phone default positions (normalized centers), the sizing model (fixed sizes on
  iPads at least 1000 points wide, a scaled 800x380 reference elsewhere) and safe-area clamping.
  Three tablet defaults (C-stick, L, Start and R rows) moved slightly because SunPad's values
  overlap on an 11-inch iPad.
- Sparse per-control persistence of normalized centers and size scales: an absent entry keeps its
  form-factor default. Keys are BlueWake-owned and split by form factor and schema
  (BlueWake.tablet.v1.*, BlueWake.phone.v1.*).
- The grouped D-pad editor: the four directions move and resize as one object while keeping
  independent hit regions in play.
- The layout editor (drag to move, tap to select, per-control size slider, Done), the settings
  panel (opacity, overall size, hide with a controller, move controls, reset this device's layout)
  and the safe-area-anchored three-dot menu with its menu state rebuilt after each change.
- Controller/touch coexistence: only hardware controllers hide the controls (the simulator's
  virtual ones do not), and touch state is cleared on connect, pause and resign-active.

## Changed

- Pad state goes into Aurora's virtual pad through touch_controls.cpp, with a 120 ms minimum
  hold so a short tap survives until the game's pad read. L and R are ordinary buttons: the
  digital click plus full analog pressure (PRD FR-016).
- Opening the menu, the settings panel, the layout editor, an alert or the share sheet adds a
  reason to a pause set; the guest is held at a frame boundary (recompcore 0067) until the set is
  empty. Backgrounding is one more reason.
- The menu holds BlueWake's items: touch controls, aspect ratio (applies next launch, with a
  notice), game data and saves (where the files are, remove the disc image while keeping saves)
  and Report a Problem (a privacy-safe text report through the share sheet).

## Left out

- Sunshine's FLUDD analog R trigger, its water animation and the wide R control.
- Render-resolution, 60 FPS and experimental performance-mode settings.
- The "modern C-stick" option and every Sunshine file manifest or idle address.
- SunPad's importer: BlueWake has its own first-run import (first_run.m, disc_import.c).

## Regression coverage

- scripts/ios/sim_run.sh --env BLUEWAKE_SHELL_DEMO=settings|layout opens the panel or the editor
  in an unattended run; the log shows the pause reason, the guest held at a present and released
  when it closes.
- Screenshots of the default layout, the settings panel and the editor on the iPad Pro 11-inch
  simulator are in local-research/ipad (private, ignored).
