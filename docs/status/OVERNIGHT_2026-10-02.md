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

### Windows save-state flush has write access

The completed gzip file was reopened read-only before `_commit`; Microsoft's
CRT calls FlushFileBuffers, which requires write access. Reopening `r+b` fixes
the mismatch. The previously failing writable-flush-contract shim now passes
under ASan/UBSan on the M3 Max. This is a synthetic Windows contract, not a
native Windows execution. The complete state-container regression passes on
Mac and is now registered in Windows CMake/CI, including compression truncation
and preservation after a rejected second write. Native CI result is pending.

### Producer failures preserve the previous state

The host's own serialization status now reaches the writer's finish operation;
an incomplete attempt closes/removes only its staged output. Previously the
writer knew about gzip/chunk failures but not missing alias storage, allocation
or subsystem serialization failures in the caller. It could publish a partial
state, then the host reported failure.

A regression compiles the actual host save function with synthetic CPU/RAM and
subsystem inputs. Missing alias storage and failed field serialization both
preserve the previous decompressed state byte-for-byte. The alias case fails
against the old host function; both cases pass with ASan/UBSan and in CTest on
the M3 Max. The state-container regression also tests an explicit incomplete
attempt. Both targets are registered on Windows; native CI is pending. No game
module is loaded, and gameplay state restore remains unverified.

The pre-pass Mac/iOS app executables were preserved locally for later control
runs: Mac `f4f377b4478975b90902d98e3c3e27ffc5531f68096e5492fd2f5d51d5a51993`,
iOS `d1461c63bba43c03717a6a93beb2aac119c28a60307e0c9eff0a590c4bc138ae`.

### Card memory follows a published replacement

Runtime `ba0a5d52ff78f8f975c767038d49497d9651369c`, patch 0126, distinguishes
failure before publication, publication with a sync error, and successful sync.
Only the first rolls memory back. All sync errors still return I/O failure;
this does not claim power-loss durability. Write, metadata, create, delete and
format now retain the same contents as the published disk container.

The expanded real card-I/O regression injects directory-fsync failure. It
fails against runtime `6699be9` because memory differs from disk, and passes
with ASan/UBSan on the M3 Max. A following metadata-only save preserves the
published payload. All three card/dispatch CTests pass. These are synthetic
cards with no module. The runtime commit was pushed to the maintainer's
bluewake-next branch, exported, and all three required pins updated.
Forced-reboot durability and real in-game error handling remain unverified.

### Failed Apple recovery keeps the canonical card present

Backup recovery now copies and flushes the damaged original to its unique
preserved path, retaining the canonical file until atomic replacement succeeds.
Only the player's explicit new-card choice moves the canonical file aside.
Previously failed replacement left the canonical path missing, so the next
launch skipped recovery and created an empty card.

The regression compiles the actual Objective-C replacement method and startup
guard with Foundation on the M3 Max, mocking only the failure alert. It fails
against the old method. With ASan/UBSan and CTest it verifies byte-identical
preservation, the next-launch recovery condition after injected rename failure,
successful backup restore, and an explicit new-card choice. No game module is
used. UIKit presentation, app interruption/termination and physical-device
recovery remain unverified; this is a filesystem/control-flow regression.

## Remaining work

- Paused full audio queue blocking output recovery.
- Matched scene/device frame-time and audio-drop measurements, then supported fixes.
- Build-time/resource measurements and safe improvements.
- Remaining renderer race/trace review and hardware acceptance from SUGGESTIONS.md.

No release readiness or whole-game stability claim is made.
