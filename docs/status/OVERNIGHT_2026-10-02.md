# Overnight stability and efficiency pass, October 1–2, 2026

Requested window: 23:51–07:00 JST, ending October 2 (22:00 UTC October 1).
Work began at 23:53 JST from `dc7ef3c`, on PR #12's existing branch,
with RecompCore `6699be9`. Releases remain paused. Smooth Motion remains
experimental and off by default. Private inputs and builds stay local.

This pass fixes the six review findings before measuring frame drops and
build efficiency. Each code change needs a reproduced failure, a focused
regression and validation. Hardware and simulator evidence are recorded
separately; compilation and synthetic tests do not prove gameplay.

## Coordination and devices

At startup, other active local Codex tasks were asked to identify their
device/simulator windows and avoid concurrent writes to this BlueWake checkout.
Read-only discovery sees the physical iPad Pro (6th generation) and iPhone 14;
several other-project simulators are already booted. No existing simulator
has been stopped or reset. Save/preference backups are required before tests.

## Fixes and evidence

### Migration cannot overwrite a competing target

The existing stat-then-rename sequence overwrote a destination that another
startup created between those calls, before the card lock. The new regression
creates that competing target immediately at publication. It fails against
`dc7ef3c` under ASan/UBSan, and passes with POSIX no-replacement hard-link
publication. Windows uses MoveFileEx without REPLACE_EXISTING; an existing
destination wins. The source is preserved in both cases.

M3 Max Mac, synthetic files only, no game module: the same sanitizer regression
passes, and the registered migration, atomic-file and launch-marker CTests all
pass. The test is also registered in native Windows CI; its result is pending.
Physical iOS migration and Windows runtime behavior remain unverified.

## Remaining work

- Windows state writer's read-only flush handle.
- Producer failures publishing incomplete save states.
- Card memory/disk divergence after a post-rename sync error.
- Apple recovery losing its prompt after interrupted/failed replacement.
- Paused full audio queue blocking output recovery.
- Matched scene/device frame-time and audio-drop measurements, then supported fixes.
- Build-time/resource measurements and safe improvements.
- Remaining renderer race/trace review and hardware acceptance from SUGGESTIONS.md.

No release readiness or whole-game stability claim is made.
