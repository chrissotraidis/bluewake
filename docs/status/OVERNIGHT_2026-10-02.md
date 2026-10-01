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

### Audio recovery resumes an already full queue

Patch 0127 resumes paused output with an existing prebuffer before the push
path can discard overflow/fast-forward samples and return. It retains queued
PCM rather than clearing it to make the regression pass. The previous dummy
test explicitly cleared the queue before resuming and missed this state.

The extended actual SDL dummy-device test fails against the pre-fix audio
object at its full-queue recovery assertion; it passes against the candidate
runtime on the M3 Max. Existing unavailable-device backoff, discard/drop
counts, exact sink capture and empty-queue prebuffer checks still pass. It is
registered in native Windows CI. Real speaker/interruption acceptance remains
unverified. The runtime commit and exported patch/pins follow the required
maintainer workflow.

At 00:23 JST the iPhone is discoverable and its app is container-accessible,
but the Documents listing timed out after 30 seconds. No install/launch or
data mutation has been attempted there. SpaghettiPad owns the physical iPad
through 00:30 JST; BlueWake has not touched it.

## Validation after the six fixes

M3 Max Mac: all **241/241 registered CTests passed** (3.12 seconds).
Mac host and iOS app targets both compiled against runtime
`34dee04950e950ab5c84b38428a8276d2a99e713`.
Candidate executable SHA-256: Mac
`af07435568a4ed2d397168f3e9f3d2d44440c84d2c9e08f063eff58f04daa6ff`;
iOS `907ffed663f8a294c5825bff511cd77c31041e53ee568bee9e19435f139e4f62`.
These builds do not by themselves establish gameplay. Public-source safety audit
passed after all six fixes. Native Windows CI and real device tests are pending.

At 00:33 JST, both iPhone Documents listing and the changed experiment of a
direct Preferences copy had timed out. No iPhone install or launch was attempted.
SpaghettiPad released the iPad at 00:30; BlueWake reserved 00:35–01:30 JST.
The iPad reports no passcode requirement; a Documents backup is in progress.

## Trace capture ownership

Runtime `10ae01a8`, patch 0128, refuses FIFO worker startup while capture is
armed, including before the requested first frame. The previous condition only
refused startup during the recording window. A synthetic regression compiles the
actual production start function with a bounded worker: it fails at the armed,
pre-window assertion against runtime `34dee049`; it passes after the change with
ASan/UBSan and CTest. It also starts and joins the normal worker 100 times and
checks a repeated start does not replace a joinable thread. Native Windows CI is
registered; GPU trace replay/pixel equivalence remains unverified.

## Physical M2 iPad initial checks

The folder-copy timeout was narrowed: explicit card, SRAM and preferences copies
succeeded. Card and preferences second reads matched SHA-256 before installation.
The app was upgraded in place using the existing authorized provisioning profile;
the preserved card's post-install hash still matched. Signed executable SHA-256
`8c102a979228f0075ceccea8038f7ec0c78d11f969c4777a421c64cbf4d26ce0`, source
`68b7db7`, runtime `34dee049`, private bundled module SHA-256
`55e9b802bbcd38afc55cceab30ca724e3db606b95faeeba978ebbc230fe7a709`.
The module reports ABI 3, CPU ABI 6, 748 chunks and 415 RELs; it lacks the new
option API. An iOS option-module compile is running separately and has no gameplay
acceptance yet.

Two bounded 2,400-retrace title/bird-attract runs used HLE, 1x scale, normal
pacing/transitions, mods and HD textures off, Smooth Motion off, independent test
card/SRAM paths and session-only NSArgumentDomain preference overrides. Native
QuickTime capture confirms the title scene, bird and Link on the tower. This is
title/attract scene evidence, not interactive Outset gameplay. All thermal logs
were nominal (0). Both exited with code 0 and reported zero dropped audio frames.
Player preferences read back byte-identical after the tests. The local 79-second
QuickTime recording has video and audio tracks; it stays private and does not
prove audible speaker quality.

| Stream / window | First run p50/p95/p99 ms | Repeat p50/p95/p99 ms | Rate | Stalls >50/>100 ms |
| --- | --- | --- | --- | --- |
| VI, retraces 600–2400 | 16.69 / 21.14 / 21.74 | 16.80 / 20.34 / 20.84 | Both 59.94/s | Both 0 / 0 |
| Display, presents 350–1300 | 33.37 / 34.26 / 35.02 | 33.36 / 34.26 / 35.08 | Both 29.95/s | First 1 / 0; repeat 0 / 0 |

Audio starvation/stretch totals were 162/1498 and 183/2048. The first run's full
startup display stream included 166 and 137 ms stalls; the post-startup windows
above deliberately do not hide those events or call the whole run stall-free.
VI and display clocks have different origins and are analyzed independently.
The 30 Hz game updates remain distinct from 60/s VI delivery and display rate.
There is no before/after optimization claim from these two same-build runs.

An attempted restored-Outset launch lost the devicectl console connection and ran
without the requested session environment, instead showing the narrated intro
with stored mods/HD preferences. It is rejected as an Outset/performance result.
Its persistent log is preserved locally; the test process was stopped and a
smaller launch environment is the next discriminating experiment. The migration
created the Application Support card while retaining the Documents card; their
hashes are being checked. No player save deletion or preference reset was used.

## Renderer shared-field races

Runtime patch 0129 uses relaxed atomic accesses for the frontend failure flag
and submitted/rejected counters. The normal recording mutex continues to protect
frontend structures; the flag and counters are independent status/diagnostics.
The regression links the actual renderer globals and makes concurrent reads and
writes. ThreadSanitizer identifies data races in all three globals against runtime
`10ae01a8`. The same instrumented probe passes against the candidate with no TSAN
reports, and its normal CTest passes. The iOS target compiles. This is bounded
shared-field evidence, not a whole-GPU TSAN pass or an FPS improvement. Native
Windows registration and broader renderer/gameplay validation remain pending.
The normal Mac CTest target now enables ThreadSanitizer so reverting a field to
plain shared storage fails the regression. All 243 registered tests passed after
rebuilding every Mac target (18.84 seconds under background compilation).
Mac/iOS executable hashes before signing are respectively
`44658eb358076bbb85cac23901e6cc14debb928b0881823ddf2762dcd0b0c46d` and
`77b902d669eb2a5af5a84d8888489de167f6daee538350e68169594a2bf80e42`.

The initial devicectl process filter using the displayed “Executable Path”
column returned an empty result despite a live process. Subsequent checks use
`--search BlueWake` and inspect the actual executable/PID. A console launch
connection error does not mean the app stopped. Default-settings runs are
explicitly stopped, and the player card was read back intact: the migrated
Application Support card hash matches the preserved Documents card exactly.

A smaller restored-Outset environment worked. Native observation confirms Link
on the bridge to the house. The longer run restored at retrace 1001 and stopped
normally at 4600. In retraces 1300–4500: VI p50/p95/p99 16.69/19.80/20.36 ms,
59.94/s, no stalls above 50 or 100 ms. In presents 150–1750: display
33.36/34.58/35.10 ms, 29.94/s, no stalls above 50 or 100 ms. Thermal state
remained nominal; audio drops were 0, starvation/stretch 64/3084. This stationary
bridge view is not the required busy-area or sustained-session gate. A repeat
console launch lost its session environment, so it is rejected and a no-console
launch with prefixed environment variables is the next changed experiment.

## Remaining work

- Matched scene/device frame-time and audio-drop measurements, then supported fixes.
- Build-time/resource measurements and safe improvements.
- Remaining renderer race/trace review and hardware acceptance from SUGGESTIONS.md.

No release readiness or whole-game stability claim is made.
