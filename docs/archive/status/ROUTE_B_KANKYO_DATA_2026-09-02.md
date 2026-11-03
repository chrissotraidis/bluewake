# Route B KANKYO Weather Data - 2026-09-02

## Result

The complete unchanged 448-line `d_kankyo_data.cpp` now compiles and links
with the previously qualified `g_env_light` owner. Its object has exactly one
import, `g_env_light`. A public test exercises all seven pointer accessors and
both valid fog-table selections.

The test proves that normal, boss, and menu schedules are non-null and
distinct, checks their first and final active ranges, verifies representative
palette, palette-selection, environment, and sky values, and requires exact
ten-entry fog-table publication for indices zero and one. Linking this object
into the KANKYO creation probe removes all seven weather-data imports and the
environment-owner import.

This qualifies immutable default environment data and valid fog-table
publication only. It does not qualify KANKYO process creation, stage-provided
environment overrides, invalid fog indices, or lifecycle behavior.

## Evidence

- Unchanged data object SHA-256:
  `c72b62e7fbcd330bdf57d2d20559463c03df38f06c235dffd0acff546024d10e`.
- The object imports only `g_env_light`.
- Patch replay still passes through patch 0094; no target-source patch was
  needed for `d_kankyo_data.cpp`.
- No BlueWake process or Simulator was launched.

## Next Boundary

Retain unchanged `dKy_wave_chan_init` and `dKyw_wind_set` as narrow source
tiers and test their exact dirty-to-retail state. Keep game/save state, JAudio,
HIO, lifecycle execution, other profiles, and aggregate stage creation closed.
