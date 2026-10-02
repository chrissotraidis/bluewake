# PadMint handoff

PadMint is the shared local builder for the Pad projects. BlueWake supplies a
game-specific adapter; PadMint should own the interactive build experience.
The first experimental workflow targets an Apple Silicon Mac producing a personal iOS
IPA from a supported disc. Additional games and platforms can use the same
stage interface after their own validation.

**Full-speed acceptance requirement:** a fresh user's locally generated optimization profile
must reproduce the accepted developer build's performance in matched iPad
tests. Generating an IPA alone does not satisfy this requirement. Do not present
the slower unprofiled build as the recommended release path.

## Current reconciliation evidence (October 3)

The tested PadMint CLI is 0.2.9 at
`de13bd769540642f89d6fb2e0492d466a58ae32b`. A new source-only workspace passes at
BlueWake `3392854c8daaf5d7900cd034694fabe18b699a24`, selecting maintained runtime
`18ba3b642588a33b9e8eac4aba7f713bb8d3d778` and translator
`b8b534591cba8ca7cd43943a655ee6e2591cf5de`. The expected base-source digest matches.
The full run uses the matching locally built app-only shell and fresh local
training; final iOS module compilation is interrupted for low disk space. Its final assembly and
provenance must be inspected before marking that run complete. Keep that
workspace pinned to `3392854` when resuming it. The cumulative branch now
contains a later host/runtime Pictobox repair; do not silently substitute its
source or shell into the paused workspace or claim the older full-build result
as clean-build acceptance of the newer revision.

| Route | Evidence / remaining gate |
| --- | --- |
| Apple Silicon Mac → iOS source-only | Actual CLI source generation passes at `3392854`; USA rev-0 disc and exact source/dependency identities verified |
| Apple Silicon Mac → iOS full personal IPA | Earlier `95adeed` / CLI 0.2.8 complete assembly passes. At `3392854` / CLI 0.2.9, fresh 23,000-retrace training passes and final compilation is interrupted for low disk space. Final assembly, signing/install, device save/reload/upgrade and matched performance remain open |
| macOS player app | Direct BlueWake builder now produces a fresh personal app with save/reload evidence. PadMint Mac remains planned; the direct builder does not establish adapter support |
| Windows / tvOS / Linux | No complete PadMint adapter acceptance; use each platform's explicitly documented source route or preview boundary |

The exact-source iOS app-only input passes the repository content gate and
PadMint audit without translated game code. It remains local while public
releases are paused. Personal assemblies and generated profiles remain private.
The source-only route must not receive the full mode's `--app` argument.
A supplied app shell must match the intended source/module compatibility;
never infer compatibility from the IPA filename alone.

The [reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md) is the
single current platform and acceptance matrix. Player instructions are in
[Build your own BlueWake](BUILD_YOUR_OWN.md). Older design and implementation
snapshots below explain the history; they do not override current evidence.

## What exists

The initial design was reviewed on 2026-09-28 against these source snapshots;
implementation evidence follows below:

- [PadMint](https://github.com/chrissotraidis/padmint) at
  `96611e4f1d582e2fbd0bbb72c158f08abae27509`: README and project rules; no executable
  builder yet.
- [KartPad](https://github.com/chrissotraidis/kartpad) at
  `e1908b0f3230ecf5f0cd0c84f2e66fe2afe442c4`: Python builder with compatibility
  profiles, dependency bootstrap, source-aware cache keys, validated extraction,
  translation checks, provenance, and deterministic IPA packaging.
- BlueWake: [`scripts/builder/build.sh`](../scripts/builder/build.sh) orchestrates
  a shell [profile](../scripts/builder/profiles/bluewake.sh), from disc validation
  through translation, mods, compilation, signing, and local packaging.

A private PadMint implementation now wraps both backends through a Python CLI
([PadMint PR #1](https://github.com/chrissotraidis/padmint/pull/1)). It checks the
reviewed source revision and clean checkout, hashes the disc, locks the checkout,
relays progress, cancels nested processes, and writes a local build record.
It validates IPA structure and provenance before reporting packaging success.

A real BlueWake source-only integration passed at revision `36b8488`: seven
stage start/completion pairs, the expected generated-source digest, and a clean
unchanged checkout afterward. Twenty-six synthetic PadMint tests passed. Source preflight and full builds
reuse the same workspace, including when the job count changes. Different
backend commits still use separate workspaces, so those builds can require a
full rebuild; cross-revision cache reuse is future work.
This establishes source generation through the shared runner. Full-build,
packaging and hardware acceptance are separate checks; it does not yet prove
the complete PadMint-to-iPad workflow. The CLI is the current interface; a Mac
GUI remains future work.

Reuse these implementations through adapters. Do not rewrite KartPad's working
pipeline or move game-specific translation into PadMint merely to make the
two projects look alike. Confirm upstream licensing before copying code and
retain required notices. KartPad's release policy is separate; it does not
override BlueWake's source-only publication policy.

## Small first version

A command-line frontend is sufficient initially: select the game and disc,
explain the supported revision and expected expensive steps, show live progress,
and finish with the personal output and signing instructions. A later Mac app
can consume the same event stream. No network service, remote compilation,
automatic artifact upload, plugin marketplace, or bespoke build scheduler is
needed.

PadMint owns argument validation, process execution, progress, cancellation,
build records, and the selected adapter. Each adapter owns disc validation,
dependency pins, translation, patches, training, compilation, and output checks.
Invoke an adapter as a subprocess with an argument array; do not interpolate
disc paths into shell command strings. Only explicitly supported adapters from
pinned trusted sources may execute.

## Adapter contract, version 1

The following describes the proposed shared contract; it is not a claim that
all fields or commands are implemented today.

| Part | Minimum requirement |
| --- | --- |
| Identity | Schema version, stable profile ID and version, display name, supported host and target |
| Input | Accepted disc/container identities, executable hashes, clear rejection of unsupported revisions |
| Dependencies | Source URLs and exact commits or archive checksums; required tool versions |
| Options | Explicit typed options for mods, target CPU, job count, training and output location |
| Invocation | Adapter executable plus arguments; repository and private workspace passed explicitly |
| Stages | Stable stage IDs, dependency order, inputs, expected outputs and validation rule |
| Result | App/IPA location, hashes, provenance location, status and actionable error |

Keep the initial order sequential:

`preflight → dependencies → extract → translate → generate → mods → train-build
→ train-run → train-merge → compile → app → package`

An adapter may skip an inapplicable stage with a reason. Installation is a
separate explicit action after packaging/signing; ordinary builds must not
interrupt a connected device. Run only one writer per build workspace.

## Progress and cancellation

Write newline-delimited JSON to `logs/progress.jsonl` as work happens. A terminal
frontend reads it and prints human-readable summaries; a future UI reads the
same file. Keep compiler output in ordinary stage logs. The minimal event is:

```json
{"schema_version":1,"event":"stage_progress","stage":"compile","elapsed_seconds":420,"completed":120,"total":754,"unit":"objects"}
```

Also emit `stage_started`, `stage_completed`, `stage_skipped`, `stage_failed`,
`build_completed` and `build_cancelled`. Include an exit code and a relative log
path on failure. Completed/total are optional: an indeterminate spinner with
elapsed time is better than an invented percentage. Compilation counts come
from the actual build tool. Stage elapsed time and total build elapsed time
must be distinguishable.

Estimate remaining time only when enough comparable local measurements exist;
label estimates and reset them if the configuration changes. The previous
83-minute unprofiled build is historical evidence on one Mac, not a promise
for the two-pass training workflow. Measure that workflow before documenting a
duration.

Ctrl-C stops the process group, retains completed work and logs, and records
cancellation without marking an unfinished stage successful. Restarting the
same command validates reusable outputs and resumes. No application restart or
device replacement is required merely to resume compilation.

## Cache and resume rules

A `done` file or output directory alone is insufficient proof of validity.
Use a small JSON stage record containing the input key, output hashes, and
successful validation result. Write it atomically after completion.

- Extraction key: profile version and complete input-image hash; verify disc
  identity and executable hashes before reuse.
- Translation/mod key: extracted executable identities, translator and generator
  revisions, patch hashes, selected mod configuration and adapter version.
- Training key: generated source digest, instrumentation/compiler version,
  training-route version, selected mods and training inputs. Store counts only
  in the player's private workspace.
- Compilation key: source and dependency fingerprints, target, SDK/compiler,
  flags, selected mods and optimization-profile hash.
- Packaging key: exact compiled outputs, app resources, notices and provenance.

Keep extraction reusable across ordinary app updates. Changing mods or PGO
must invalidate dependent compilation. Turning mods off must select a clean
base source tree, not retain variant files from a previous run. Validate files
again after an interrupted operation. Avoid deleting or rewriting a different
configuration's workspace to make a new build fit.

## BlueWake local optimization flow

LLVM profile-guided optimization (PGO) records how often code runs, then uses
that information during compilation. The release workflow must create these
records from each player's disc on their Mac:

1. Validate the supported disc and generate the base and selected mod variants.
2. Build an instrumented macOS training host and compatible game module from
   those local outputs. Instrument the relevant game, runtime and host code;
   do not silently substitute the developer's private game-function profile.
3. Train from a new local save created through boot, or an explicitly selected
   copy of the player's own save. Never depend on a maintainer save path or
   alter the player's original save. Automate a versioned route and include
   rendered training for renderer code that headless runs do not exercise.
4. Verify the route reached its expected scenes and exited cleanly. Confirm
   nonempty profile data and relevant game-function coverage, then merge with
   the matching LLVM tool. A successful process launch is not a successful
   training run.
5. Compile the optimized iOS module and host against the local profile. Treat
   profile/source mismatches as visible failures, not warnings to suppress.
6. Package the private IPA with provenance recording training and profile
   hashes. Cache the verified profile for compatible subsequent builds.

`scripts/builder/train_local_pgo.py` now provides the experimental local trainer;
`scripts/pgo_host_train.sh` is its compatibility wrapper. The legacy
`scripts/pgo_host.sh` and `scripts/pgo_composite_hot.py` remain advanced
development utilities with old build-layout assumptions. The player flow uses
the new trainer and does not depend on a maintainer save.

Acceptance requires a fresh workspace with no developer profiles or saves,
successful local training, then matched Outset and Windfall hardware tests.
Compare scene, settings, device, thermal state, FPS, frame time and CPU use with
the accepted developer build. The earlier Outset measurements were 26.0 FPS
without profiles, 27.5 with runtime/host profiles, and 29.9 with all three
developer profiles. They establish the missing performance requirement; they
do not prove a new training route will meet it. Runtime/host profile provenance
also remains an audit item; a heuristic scan is not provenance evidence.

## Patches and local assets

BlueWake's [`scripts/mods/build_mods.sh`](../scripts/mods/build_mods.sh) already
generates widescreen, Better Wind Waker, and combined code variants from the
player's disc. Preserve this path and pin/hash all required patch tooling.
The patched disc and translated variant code stay local. Documentation must
explain which resulting disc the app imports when a mod requires changed data.
Texture packs remain a separate user import with their own source and terms;
they must not be silently bundled into an app or this source repository.

Public source must exclude decompiled game source/patch material retained in
the private archive, saves, extracted assets, translated game modules and
private optimization profiles. Review the surviving patch/tool sources on
their own merits; calling a file a patch is not an audit result.

## Provenance and publication

Record schema/adapter versions, source/dependency revisions and dirty status,
input and generated-source hashes, selected mods and patch hashes, toolchain,
target/flags, training route/profile hashes and output hashes. Use hashes and
identifiers in shareable provenance; omit usernames, absolute paths, device
identifiers, signing identities and save contents. Full local logs may contain
paths, so exporting diagnostics must produce a sanitized copy.

A successfully packaged personal IPA is still a personal build. BlueWake's
public release check must reject it. Public source artifacts require the
repository's publication audit and notices; packaging checks do not replace
that audit. The pipeline must not upload generated code, profile data or IPA
files as CI artifacts or telemetry.

## Implementation order and handoff acceptance

1. BlueWake: complete and validate local PGO; add honest progress to its existing
   pipeline and fix configuration-sensitive cache reuse.
2. PadMint: implement one command-line runner and progress reader around the
   BlueWake adapter. Reuse suitable KartPad cache/provenance/packaging helpers
   only after license and interface review.
3. KartPad: adapt its existing Python builder to the same event/result contract;
   retain game-specific profile validation and translation behavior.
4. Validate unsupported disc rejection, paths containing spaces, cancellation
   and resume, mod/profile cache invalidation, output privacy and first-run
   dependency setup. Run one complete local build from an empty workspace.
5. Publish instructions only for commands that actually exist and label any
   unverified platform or performance path. Update this handoff as each proposed
   component becomes implemented and tested.

The immediate blockers are the local training route and measured optimized
performance, not the future graphical interface. Preserve the existing public
entrypoint `scripts/ios/build_device.sh` as a compatibility wrapper when PadMint
becomes the shared runner.
