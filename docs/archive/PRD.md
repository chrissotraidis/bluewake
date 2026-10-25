# BlueWake Product Requirements Document

**Product:** BlueWake

**Game:** *The Legend of Zelda: The Wind Waker* for GameCube

**Initial revision:** `GZLE01`, USA revision 0

**Platforms:** Apple Silicon macOS first; shared iOS/iPadOS product second

**Platform order amended 2026-09-23 by the user:** iPadOS is the active target
now, tested in the iOS simulator one device at a time before any hardware run.
Scope, gates and the definition of done are unchanged. Record:
[status/IPADOS_REORIENTATION_2026-09-23.md](status/IPADOS_REORIENTATION_2026-09-23.md).
Evidence as of 2026-09-24 night (details in [status/CURRENT.md](status/CURRENT.md)): the
iPad simulator app imports the user's disc on first run, boots through the title into
controllable Outset play with SunPad touch controls, a hardware keyboard or a controller, saves
and reloads through the game's own pause menu (acceptance script), plays the game's stereo mix
(FR-011 channel loss fixed: the console SRAM is now emulated, FR-009), and runs Outset and its
interiors at 58-60 retraces a second, the village at 59.9 when the host is cool. Graphics are
checked against Dolphin from shared inputs (FR-010 corpus: title, menus, text, the walk, swim,
village, sun glare via EFB depth peeks). Open: a hardware run (needs signing), the rest of the
coverage catalog, and speed headroom for fanless devices.

**Document status:** Governing implementation specification

**Prepared:** 2026-08-21

**Companion:** [GOAL_LOOP.md](../GOAL_LOOP.md)

## 1. Document authority

This PRD converts BlueWake's feasibility research into an executable product
program. It governs implementation order, architecture decisions, data safety,
test evidence, and the definition of done.

The earlier documents remain evidence and history:

- `FEASIBILITY_REPORT.md` records the 2026-08-09 source-native assessment.
- `FEASIBILITY_REASSESSMENT_2026-08-13.md` records the second opinion that
  admitted a bounded static-recompilation prototype.
- `IMPLEMENTATION_PLAN.md` contains useful phase detail for both candidate
  routes.
- `docs/research/` preserves dated inspections and experiments.

When those files conflict with this PRD, this PRD controls current work. New
measured evidence may update baselines and choose among the route options in
this PRD, but a bot must record the evidence and decision. Reducing product
scope, platforms, safety rules, acceptance coverage, or definition of done
requires explicit user approval; an autonomous agent may not weaken them.

The companion goal loop is the operating procedure. The PRD says what must be
built and proved; the loop says how an autonomous agent repeatedly advances it.

## 2. Product mandate

Build a native Apple application that runs the user's legally obtained
GameCube Wind Waker data on:

1. Apple Silicon macOS;
2. iPhone; and
3. iPad.

BlueWake must reach authentic, complete game behavior, not a renderer demo,
synthetic scene, memory-seeded snapshot, or collection of disconnected tests.
It must boot normally, accept input, render and play audio correctly, preserve
saves, survive lifecycle transitions, sustain the original game speed, and be
validated from new game through ending plus representative optional content.

The project is allowed to use static recompilation, reconstructed source, a
compatibility runtime, clean subsystem replacements, or a route-level pivot
between them. It is not allowed to hide missing behavior behind unsupported
claims. Persistence is expected; a failed architecture gate redirects work
rather than ending the overall BlueWake goal.

## 3. Claim-safe product description

Until the final architecture and all release gates are proven, use:

> BlueWake is an Apple-platform port project for running a user-owned copy of
> the original GameCube Wind Waker through ahead-of-time translated or
> reconstructed game code and native Apple host components. Development is in
> progress; complete gameplay, performance, and compatibility are not implied
> until the documented gates pass.

If the selected runtime retains Dolphin-derived services or interpreter
fallback, do not call the result “emulation-free.” A precise description is:

> Native Apple application with ahead-of-time translated game-code regions, a
> GameCube compatibility runtime, no runtime PowerPC JIT, and measured
> interpreter fallback where necessary.

That wording describes development builds only. The release acceptance corpus
requires zero observed interpreter-executed guest instructions; the final
description must state the audited route actually shipped.

If the final route becomes fully source-native over Aurora, that description
may be revised only after the artifact is audited.

## 4. Current evidence baseline — 2026-08-21

The following snapshot was refreshed from primary repositories and ignored
local `ref/` checkouts. It is a planning baseline, not a permanent pin.

| Project | Revision inspected | Verified role/status |
|---|---|---|
| `zeldaret/tww` | `03d27aa14389e648f51df2bc055ee3d53a3b67f2` | Active CC0 matching decompilation; requires the user's disc and contains no game assets or assembly. |
| Official TWW progress page | fetched 2026-08-21 | 73.81988% matched code; 31,514/39,324 matched functions (80.13936%); matched functions are 12,720/13,763 in the DOL and 18,794/25,561 across RELs. The served page does not prove that its data is from the exact checkout SHA above. |
| DolRecomp | `528a204fbf68fccf7353bff9202714fa6a009dcf` | GPLv3 CPU translator; current CI passes. Supports DOL and recursive REL folders, exact FP semantics by default, portable C, and LLVM 19/20 objects for x86-64 and generic ARMv8-A/Cortex-A57; current module ABI is v4 and lockstep/PGO tooling exists. |
| ModernGekko | `cc6b1327325d27ab1519daabce51dde47424705d` | GPL/Dolphin-derived runtime and tools; current `moderngekko-port` still generates only from `sys/main.dol`. Current head fails all CI builds, including macOS on a missing `Core/SavestateLayout.h`; latest observed green revision is `4adb92bbf66af78201470d0721d2d7a809ffbcdc`. Never float this pin. |
| RecompCore pinned by ModernGekko | `5c3611e2bcb03d1578274c956bc3d7b913527ec2` | Static-recomp runtime contains REL descriptors, active-section discovery, and runtime↔linked address translation. This pin embeds DolRecomp `f0a86be...`, not standalone head. |
| ModernGekko-Template | `fd940c5147c3f4bbc28a997c8853e08b3f5b6115` | Current bootstrap reference pins standalone DolRecomp `528a204` and the red ModernGekko `cc6b132`; it is not a known-green combination until BlueWake reproduces and validates it. |
| Dusklight | `5f0f3d4ebe6a866b09ff4b304dd90ca9642b5a50` | Shipped Twilight Princess source-port precedent; repository game source declares CC0 while Aurora and other linked dependencies retain their own licenses; current CI passes. |
| Aurora | `8b690b60e699c92e3327886ebd84cf7f05c5d36c` | MIT GameCube SDK/GX compatibility layer; current CI passes and it explicitly targets macOS/iOS/tvOS/Android through SDL3/Dawn/Metal-capable paths. |
| SunPad | local `e43f0ea6b797e5110787171957c9dc3c6213269c` | Required Apple host, touch, menu, importer, controller, diagnostics, packaging, and safety reference. |
| `sp00nznet/gcrecomp` | `67491ae68d29567d41d09ef99989893f94751cbf` | MIT experimental CPU/runtime/GX/audio toolkit; Windows/D3D11 first, with several graphics/audio paths still in progress and no public CI found. |
| `sp00nznet/ww` | `6098083bc3b53ba26ab929792c5b9ebae306560d` | Active Wind Waker-specific experiment using gcrecomp; renders authentic assets and contains useful CFG/jump-table/patch/watchdog work. Its README labels the demonstrated screenshot path “the renderer, not the game,” and source inspection shows forced/snapshot-driven process construction. Informative only. |
| GameCubeRecompiled | `d380537add5aa4116c94c3513b8e3a7dd89f2331` | Experimental CC0 Rust recompilation/runtime research; donor and comparison, not a proven product base. |

### 4.1 Current decompilation interpretation

The current decomp report leaves 7,810 functions not byte-matched. That does
not mean all 7,810 have no source: non-matching but behaviorally useful source
and empty stubs are different categories. The prior source census found the
critical boot/player/framework spine substantially stronger than the aggregate
percentage, while NPCs, menus/maps, and JAudio1/JAZelAudio carried the largest
gaps. The autonomous program must regenerate that census at its pinned TWW
revision rather than reuse the August 9 counts as current truth. The official
progress page's displayed linked-code percentage is backed by an older 2025
record and must not be described as a fresh 2026 linked metric.

The upstream full build reports 416 files. The consistent interpretation is
one main DOL plus 415 REL outputs. BlueWake still derives and explains the
authoritative topology from its own verified disc manifest.

### 4.2 Current static-recompilation interpretation

ModernGekko/RecompCore is the leading first execution candidate because:

- SunPad demonstrates the Apple host/no-runtime-PowerPC-JIT integration;
- DolRecomp can generate DOL and batches of RELs;
- the current module ABI includes REL module/section metadata; and
- the pinned RecompCore resolves runtime-loaded REL addresses into AOT-linked
  ranges.

DolRecomp's new direct AArch64 LLVM object path is a high-priority spike because
it may materially reduce portable-C compile size and runtime overhead. It must
pass exact-semantics, ABI, lockstep, and generated-artifact audits before
selection.

ModernGekko's current head is red, so BlueWake must begin from a tested green
pin or repair the regression on a controlled branch. It is not turnkey for
Wind Waker. BlueWake must build the composite module,
solve active-REL discovery cost, port/reconcile SunPad's Apple patches, and
measure interpreter fallback, module size, correctness, and speed.

Pin topology is part of the architecture: ModernGekko `cc6b132` pins
RecompCore `5c3611e`, which pins DolRecomp `f0a86be`; the latest observed green
ModernGekko `4adb92b` pins a different RecompCore revision. Standalone
DolRecomp `528a204` uses module ABI v4. BlueWake may combine a runtime and
translator from different trees only after a dedicated module-ABI and CPU-state
compatibility suite passes.

`gcrecomp` and `sp00nznet/ww` are useful for CFG recovery, function discovery,
Wind-Waker-specific OS/GX observations, and failure instrumentation. They do
not presently satisfy BlueWake's Apple or authenticity gates. Their committed
Dolphin memory captures and capture-derived process construction must never be
copied into BlueWake or used as normal initialization.

### 4.3 Baseline evidence record

The snapshot used `git ls-remote <url> refs/heads/main` plus shallow ignored
checkouts and `git log -1 --format='%H %cI %s'`. Progress was fetched from the
official TWW progress endpoint/page on 2026-08-21. Observed upstream checks:

- DolRecomp green: <https://github.com/ExpansionPak/DolRecomp/actions/runs/32462788867>
- ModernGekko red head: <https://github.com/ExpansionPak/ModernGekko/actions/runs/32443239784>
- Aurora green: <https://github.com/encounter/aurora/actions/runs/32393046084>
- Dusklight green: <https://github.com/TwilitRealm/dusklight/actions/runs/32456067327>

These are upstream observations, not locally reproduced BlueWake builds. P0/P2
must reproduce selected configure/build/tests and archive their commands before
promoting any implementation pin.

## 5. Goals

### G1 — authentic macOS game

Produce a self-contained Apple Silicon macOS application that:

- validates and privately prepares `GZLE01`;
- starts through the original boot flow;
- reaches title, file select, new game, and normal gameplay;
- supports keyboard and GameController input;
- renders and plays audio accurately;
- saves, quits, and reloads without corruption;
- sustains original 30 FPS game speed; and
- passes the full compatibility campaign in this PRD.

### G2 — shared iPhone/iPad product

After the macOS runtime gate, host the same core in a shared iOS target with
idiom-specific layouts. It must inherit SunPad's proven mobile interaction
language: the three-dot menu, editable touch controls, touch/controller
coexistence, staged Files import, lifecycle handling, and guided diagnostics.

### G3 — reproducible and resumable engineering

Make every generated output, dependency patch, failure, test route, and gate
status reproducible. Another agent must be able to resume after interruption
without rediscovering the current blocker.

### G4 — exhaustive compatibility, not milestone theatre

Compilation, link, installation, a live process, a logo, title screen, or one
playable room are intermediate evidence. Completion requires broad system and
content acceptance plus start-to-ending proof.

## 6. Non-goals for the first release

- Wind Waker HD or Wii U support.
- PAL/Japanese/demo revisions before `GZLE01` is complete.
- A 60 FPS game patch. Authentic 30 FPS/speed is the release baseline.
- Mods, randomizers, texture packs, cheats, netplay, or achievements.
- App Store acceptance guarantees.
- Bundling or downloading Nintendo game data.
- Preserving an architecture for its own sake after evidence shows it is the
  wrong product route.

These may become later roadmap items only after the release definition of done.

## 7. Users and core journeys

### 7.1 Owner/player

The player owns a supported GameCube disc image, installs BlueWake, imports or
selects the image, and plays with a controller or mobile touch interface. They
can understand failures, export a privacy-safe diagnostic, preserve saves
through app updates, and remove replaceable game data without deleting saves.

### 7.2 Developer/agent

The developer prepares private input, reproduces a failing gate, inspects
symbols/traces/reference repos, applies the smallest fix, runs tiered
regressions, records evidence, and advances to the next unmet gate.

### 7.3 Tester

The tester selects a named scenario from the coverage manifest, runs it on a
specified platform/input/form factor, records deterministic milestones and
visible/audio observations, and marks the scenario pass/fail without exposing
private game or save data.

## 8. Product principles

1. **Authentic path first.** Debug warps and synthetic fixtures accelerate
   testing but never replace normal boot/progression acceptance.
2. **One measured blocker at a time.** Do not paper over unknown behavior with
   broad stubs or manual state injection.
3. **Preserve user data.** Game data is replaceable; saves, settings, layouts,
   and controller preferences are not.
4. **Evidence outranks optimism.** Every gate has an artifact and measurable
   exit condition.
5. **Reuse behavior deliberately.** SunPad and Dusklight are donors, not files
   to copy blindly.
6. **Schedule follows evidence.** No phase has a promised week count.
7. **A route may fail; the product goal persists.** Capture the failure, pivot
   at a stable boundary, and continue.

## 9. Architecture strategy

### 9.1 Route A — AOT retail-code execution

Initial route:

```text
user-owned GZLE01 image
  -> private verified extraction
  -> main.dol + authoritative REL inventory
  -> DolRecomp generation and BlueWake normalization
  -> composite arm64 module + DOL/REL metadata
  -> ModernGekko/RecompCore compatibility runtime
  -> BlueWake runtime adapter
  -> macOS host, then shared iPhone/iPad host
```

Route A retains original retail behavior wherever AOT translation covers it.
Runtime PowerPC JIT is prohibited on every Apple target. Interpreter fallback
is allowed as an instrumented correctness mechanism, then reduced if it is hot
or performance-limiting.

### 9.2 Route B — reconstructed source over Aurora

Long-term/pivot route:

```text
zeldaret/tww reconstructed source
  -> BlueWake portability/ABI pass
  -> statically linked REL source or equivalent lifecycle model
  -> Aurora + required clean host services
  -> JAudio1/JAZelAudio host integration
  -> BlueWake runtime adapter
  -> macOS host, then shared iPhone/iPad host
```

Route B is the more maintainable and claim-clean architecture when sufficient
behavioral source exists. It may require functionally equivalent reconstruction
for missing source. Exact upstream matching is preferred when practical because
it is the strongest binary-level correctness check, but BlueWake product
progress does not require every source function to be byte-identical if
behavioral equivalence is independently tested.

### 9.3 Wind Waker research lane — always active, never product proof

Pin `sp00nznet/ww`, `sp00nznet/gcrecomp`, TWW hacking documentation, and other
qualified donors under ignored `ref/`. Selectively rederive or adapt:

- false CFG-entry detection and function-boundary correction;
- jump-table recovery;
- address-keyed generated patch preservation;
- dispatch/stall watchdogs;
- REL data materialization tests;
- process/profile layout knowledge;
- symbol-map ingestion; and
- trace/lockstep comparison tools.

Every imported algorithm needs a source/license/provenance review and a
BlueWake-owned test. Do not adopt the Windows/D3D11 runtime, tracked captures,
snapshot warm start, forced frame dispatch, hard-coded camera, or manual
process tree as the Apple product path.

### 9.4 Diagnostic hybrid — allowed

The routes may share:

- symbols, maps, types, and module manifests;
- input recordings and expected progression milestones;
- Dolphin differential traces produced from the user's own disc;
- frame-state digests, audio/event markers, and save hashes;
- clean host service implementations at documented APIs; and
- test fixtures that contain no proprietary content.

### 9.5 Fine-grained live-memory hybrid — prohibited by default

Do not arbitrarily call between host-endian reconstructed C++ objects and a
big-endian guest-memory `CPUState` implementation. Their layouts, pointers,
globals, and calling contracts differ. A mixed subsystem is allowed only if a
stable narrow API boundary and explicit marshalling contract are first proved.

### 9.6 Route decision rules

Continue Route A while failures remain bounded translator, packaging,
registration, fallback, or compatibility-runtime work with credible progress.

Trigger a formal route review when any occurs:

- hot execution remains interpreter-dominated after targeted translation work;
- REL lifecycle requires pervasive runtime surgery rather than a bounded seam;
- module size, compile/link time, memory, or iOS signing is impractical;
- authentic initialization requires capture-derived state;
- systemic corruption survives reduced reproducers; or
- physical Apple performance has no measured headroom.

At review, choose one of:

1. repair a bounded Route A subsystem;
2. replace an entire subsystem behind a stable API;
3. adopt a cleaner translator/runtime donor;
4. pivot the whole execution route to Route B; or
5. wait only for a truly external requirement while advancing independent work.

Never interpret a Route A stop as permission to abandon the overall PRD.

## 10. Required repository architecture

The implementation should converge on this public/private split:

```text
apple/
  shared/             BlueWake-owned input, settings, diagnostics contracts
  macos/              desktop host and packaging
  ios/                shared iPhone/iPad host and idiom-specific layouts
cmake/                toolchain and dependency integration
config/               public revision metadata and non-secret manifests
docs/                 PRD, loop, gates, decisions, compatibility reports
patches/              ordered reviewable upstream patch snapshots
scripts/              bootstrap, prepare, build, test, package, audit
src/
  runtime/            selected execution-core adapter
  module/             composite DOL/REL generator and metadata
  platform/           platform-neutral product services
tests/                synthetic/unit/integration/UI tests
ref/                  ignored public reference checkouts
local-research/       ignored traces, extracts, logs, private manifests
generated/            ignored translated/derived game code and objects
build*/               ignored build outputs
```

### 10.1 Public source allowed

- Original BlueWake code and tests.
- Properly licensed clean-room or reconstructed source.
- Reviewable patches against public dependencies.
- Public revision metadata, hashes where legally appropriate, and schemas.
- Synthetic fixtures created for tests.

### 10.2 Private/ignored material required

- Disc images and extracted game files.
- Original DOL/REL binaries.
- Generated translated game code or derived modules.
- Saves, screenshots, video, audio, traces, and state captures from gameplay.
- User paths, device identifiers, signing material, provisioning profiles,
  credentials, and private diagnostic logs.

Ignored generated source and local development packages are different from
public distribution authorization. A Route A package may necessarily contain
translated Nintendo code; it is private/local and self-signable by default.
Public hosting requires an explicit user publishing decision after separate
copyright/provenance, GPL/corresponding-source, and Apple-distribution review.
Compiled Route B artifacts require the same copyright/provenance review even
when their source dependencies are permissively licensed.

### 10.3 Prohibited sources

- Leaked Nintendo source or SDK material.
- Illicitly acquired game images or keys.
- Memory dumps committed by third parties when provenance or redistribution is
  unsafe.
- Code copied from incompatible sources without a documented license review.

Public reference repositories may be downloaded into ignored `ref/`. Each
dependency used in the product must have an exact SHA, URL, license, role,
local patch list, verification date, and test evidence in a machine-readable
lock file. The lock recursively records submodule SHAs, translator/runtime
module ABI and CPU-state ABI versions, and compatibility evidence.

Before reusing SunPad code, the dependency lock and a component-transfer
manifest must record its reproducible origin, exact SHA, branch/dirty state,
license at file/component level, copied/adapted files, excluded game-specific
behavior, and BlueWake regression coverage. A clean BlueWake bootstrap must
not depend on the absolute local SunPad path.

Because `sp00nznet/ww` tracks game-derived `dolphin_*.bin` captures, acquire it
with a sparse/provenance-reviewed checkout that excludes those blobs whenever
possible. If a full ignored reference clone is needed for audit, quarantine and
remove the captures immediately. They may never be vendored, packaged,
redistributed, or used for BlueWake initialization.

## 11. Functional requirements

### FR-001 — supported input

- Release 1 supports `GZLE01` USA revision 0 only.
- Release 1 import accepts uncompressed `.iso`/`.gcm`. RVZ, WIA, WBFS, CISO,
  NFS, GCZ, and other formats are later-compatible only after their decoder,
  identity, staging, free-space, and rollback tests pass.
- The importer must identify disc ID, revision, expected size/structure, and a
  pinned cryptographic identity before activation.
- Unsupported revisions must receive a specific, non-destructive error.
- The authoritative DOL/REL inventory comes from the verified disc, not a
  hard-coded assumption.
- The existing 415-versus-416 documentation conflict must be resolved by a
  generated manifest that explains loose RELs, `RELS.arc` members, any profile
  module, duplicate IDs, and excluded non-executable entries.

### FR-002 — private preparation pipeline

Preparation must be one command from a validated input and must:

1. import/select the source as a copy, validate before copying, and stage under
   an ignored UUID directory;
2. re-hash the staged source, then extract without modifying it;
3. hash every executable input;
4. enumerate DOL sections and all REL headers, sections, imports, relocations,
   prologs, epilogs, unresolved handlers, and sizes;
5. generate a public-safe summary with no paths or game bytes;
6. translate or prepare the selected route;
7. validate the complete required-output manifest;
8. stop the runtime only when activation is ready;
9. atomically replace/move the active `GameData` directory;
10. persist active metadata only after the swap succeeds; and
11. restart the previous active data after a failed swap.

Cancellation or failure must leave the previous active data and all saves
unchanged. Launch cleanup must safely remove abandoned UUID staging directories
without touching the active root. Same-filename reimport is a required case.

Before copy/extraction, estimate and verify space for the source copy, staging,
prepared output, rollback, and safety margin. Mobile imports become app-owned
copies; do not depend indefinitely on a transient security-scoped URL. macOS
may offer an in-place security-scoped bookmark only with renewal/error UX and
the same preparation/rollback contract. Rebase app-container paths after UUID
changes and exclude replaceable large game data from device/cloud backup while
keeping user saves/settings eligible according to documented policy.

### FR-003 — reproducible dependency bootstrap

- Bootstrap public dependencies only; never download game data.
- Recreate exact ignored checkouts from the lock file.
- Apply ordered patches idempotently.
- Detect already-applied patches with reverse checks.
- Abort on unknown dependency changes rather than overwriting them.
- Cache downloads and builds without making the cache authoritative.
- Record compilers, SDKs, deployment targets, CMake options, and tool hashes.
- Never promote a floating or merely newest revision. Record upstream CI, then
  reproduce configure/build/tests locally; a red head such as the current
  ModernGekko revision is research input, not an implementation pin.

### FR-004 — complete translation/source inventory

For Route A, record per DOL/REL image:

- executable ranges and instruction count;
- unknown or unsupported instructions;
- fallback sites with reason and symbol;
- SMC/code-retirement candidates;
- unresolved direct/indirect targets;
- relocation coverage and cross-module imports;
- generated source/object size;
- compile time and peak memory; and
- dispatcher coverage.

For Route B, record per object/module:

- matching/equivalent/non-matching state;
- sourced functions versus empty stubs;
- exact-match percentage where available;
- portability status;
- compile/link state; and
- tests/content that exercise it.

No image or object may disappear from the inventory merely to turn a gate
green.

### FR-005 — composite DOL/REL module

Route A must generate one deterministic module or attached descriptor that:

- namespaces generated symbols across every image;
- covers each executable address exactly once;
- includes sorted DOL and REL code/chunk/SMC ranges;
- includes REL module ID, version, section table, file size, executable
  sections, and build-time linked ranges;
- supports direct and indirect dispatch into and out of REL code;
- exposes stable hashes for netplay/reproducibility-style comparison even if
  BlueWake has no netplay feature; and
- passes ABI validation and synthetic representative dispatch tests.

### FR-006 — dynamic REL lifecycle

The runtime must correctly handle:

- link and unlink;
- repeated load/unload/reload;
- guest-address reuse by another module;
- multiple executable sections;
- prolog, epilog, unresolved handler, constructors, and destructors;
- REL-to-DOL and REL-to-REL imports;
- indirect calls and function pointers;
- active mapping invalidation; and
- save/load or state restoration if the selected runtime exposes it.

The existing descriptor-by-descriptor RAM scan must first be measured. If it
does not scale, BlueWake must register actual sections at `OSLink`/`OSUnlink`,
observe the active-module list, or add an explicit runtime mapping API. Per
frame work must not scale as all compiled RELs multiplied by all guest RAM.

### FR-007 — authentic initialization

Normal boot may use the user's disc/extraction and deterministic host settings.
It may not use:

- captured Dolphin RAM or register state;
- copied process-object templates;
- manual scene/process tree population;
- hard-coded gameplay heap pointers from a reference run;
- forced title/stage/actor spawn as the acceptance path; or
- a bypass that replaces the game's progression state machine.

Debug warps, host hooks, and synthetic RELs are allowed for diagnosis and
regression isolation, but every promoted milestone must also pass through
normal boot.

### FR-008 — CPU/runtime contract

- Apple targets must execute arm64 code.
- Runtime PowerPC JIT or writable-executable code is prohibited.
- Interpreter fallback must emit counters by address, symbol, module, reason,
  and hotness.
- Interpreter fallback is a bring-up/lockstep oracle, not a release execution
  path. The release candidate must record **zero interpreter-executed guest
  instructions across the complete required macOS, Simulator, physical-device,
  chapter, and full-game corpus**. Every known fallback site must be translated,
  reconstructed, or replaced behind a validated stable boundary. Any observed
  fallback blocks release and triggers translation or route review.
- Exceptions, interrupts, timing, retrace, memory aliases, caches, atomics,
  floating-point edge cases, paired singles, and SMC retirement must be
  correct enough for the content campaign.
- Idle-loop handling must be detected from behavior or configuration, not a
  Sunshine-specific guest address.
- A fallback path may establish correctness, but hot fallback is a tracked
  debt and performance gate.

### FR-009 — GameCube services

The selected execution route must provide correct behavior for all services
Wind Waker reaches, including:

- OS threads, contexts, synchronization, alarms, message queues, interrupts,
  arenas, heaps, time, and retrace;
- DVD/FST, async reads, archives, compression, cancellation, and errors;
- GX/VI, EFB operations, TEV/indirect textures, display lists, texture formats,
  shader/pipeline caching, and presentation;
- DSP/AI/ARAM and JAudio1/JAZelAudio behavior;
- PAD, analog triggers, rumble, and controller disconnect;
- CARD creation, read/write, free-space/error behavior, and `.gci`
  compatibility where feasible;
- reset, pause, and shutdown semantics; and
- any GBA/EXI feature reached by normal play, with explicit supported or
  benign-unavailable behavior.

Silent no-op stubs are forbidden for required behavior. A temporary stub must
log a unique ID, increment telemetry, list the content it blocks, and remain in
the blocker ledger.

### FR-010 — graphics

- Render through Metal on Apple platforms, either directly or through the
  selected compatibility layer.
- Match original scene composition, geometry, textures, materials, lighting,
  fog, alpha, EFB effects, particles, UI, and cutscenes.
- Build a canonical visual corpus spanning boot, title, sea, interiors,
  weather, dungeons, bosses, UI, Picto Box, heat haze, mirrors/reflections,
  day/night, and credits.
- Compare private reference captures with aligned screenshots, frame digests,
  or GPU captures; do not commit copyrighted images.
- Shader compilation and pipeline-cache behavior must not cause unbounded
  runtime stalls or cache growth.

### FR-011 — audio

- Wind Waker uses JAudio1 and JAZelAudio; do not assume Dusklight's JAudio2
  layer can be copied.
- Reuse only compatible DSP/ADPCM/device abstractions after source review.
- Validate music, ambience, streamed audio, UI sounds, combat, voices,
  positional effects, reverb, cutscene sync, pause/resume, interruption, and
  the Wind Waker conducting sequences.
- Detect silence, underrun, buffer drift, incorrect pitch, channel loss,
  deadlock, and long-session desynchronization.
- Audio may be temporarily absent for early boot milestones but is mandatory
  before representative compatibility promotion.

### FR-012 — save and data ownership

- Keep replaceable game data separate from saves, settings, controller
  mappings, touch layouts, diagnostics, and caches.
- Removing/reimporting game data must never delete saves or preferences.
- Destructive removal requires confirmation naming the exact `GameData` scope.
  It may delete only the replaceable active game-data root. Pre/post hashes
  must prove saves, phone/tablet layouts, controller mappings, and unrelated
  preferences are unchanged.
- Persist relative logical roots rather than stale app-container UUID paths.
- Flush saves explicitly on game request, app background, and user-requested
  data operations; request a best-effort flush on termination signals where
  available.
- Treat termination-time flushing as best effort only. Correctness must come
  from game-requested writes, transactional storage, background transitions,
  and recovery; iOS termination callbacks are not assumed.
- Prefer a completion signal over a fixed delay.
- Hash saves before/after risky operations and preserve a recoverable backup
  during migrations.
- Test save interchange with a reference implementation when format-compatible.

### FR-013 — macOS application

- Native Apple Silicon `.app`, self-contained and free of Homebrew/workspace
  runtime paths.
- Desktop-appropriate import/selection UX, controller and keyboard input,
  settings, diagnostic export, fullscreen/windowed behavior, and clean quit.
- Ad-hoc signing for development; documented Developer ID path may follow.
- Normal boot, gameplay, saves, audio, representative content, and original
  speed must pass before the mobile product phase begins.

### FR-014 — shared iOS/iPadOS application

- One shared runtime and application target with explicit iPhone and iPad
  idiom behavior.
- Landscape-first game presentation with safe-area handling.
- `CAMetalLayer` host and lifecycle adapter must remain thin and independent
  of the selected core.
- Files-based disc import with security-scoped access and background staging.
- Controller gameplay must work before touch-only completion is claimed.
- Support background/foreground, audio interruption, route changes, thermal
  state, Low Power Mode, app-container rebasing, and in-place updates.
- Use current `UIScene` lifecycle unless an evidence-backed compatibility
  decision documents why the legacy app-delegate topology is required.
- Combine scene/application activity, menus/import, and audio interruptions in
  one idempotent desired-pause state machine. Events before core creation must
  be retained; retries must be bounded; foreground must reconcile controllers
  and clear stale input; audio deactivation/reactivation and route recovery
  must be verified; resume should keep the same runtime process when safe.
- Simulator and physical-device evidence are separate.

### FR-015 — SunPad visual and interaction inheritance

SunPad is the required behavioral reference for the mobile shell.

BlueWake must provide:

- a safe-area-anchored top-right ellipsis button using a native menu;
- menu state/checkmarks rebuilt after changes;
- settings for resolution, aspect ratio, diagnostics/performance display,
  controller mapping, touch behavior, game data, saves, and problem reports;
- explicit persistence and restart warnings for restart-required settings;
- main stick, camera/C-stick, D-pad, A/B/X/Y/Z, Start, L, and R controls;
- accessibility labels and stable UI identifiers;
- global and per-control scaling, opacity, edit/drag mode, reset, and
  safe-area clamping;
- grouped D-pad editing: directions move, resize, and reset as one visual
  object while retaining independent hit regions and rolling-direction input;
- independent phone/tablet defaults and schema versions;
- sparse saved-layout semantics: an absent control retains its form-factor
  fallback rather than moving to zero;
- automatic controller/touch coexistence rules and optional touch hiding;
- release of all touch state during pause, interruption, view transition,
  controller handoff, and layout editing; and
- contextual Wind Waker mappings for targeting, items, camera, sailing,
  Wind Waker conducting, Picto aiming, and menus.

Do not inherit Sunshine-specific FLUDD controls, water animation, analog-R
sliders, 60 FPS toggles, idle addresses, or game-specific file manifests.

Opening a blocking menu, picker, problem-report flow, or layout editor must
pause the game through a pause-reason set. Closing one surface may resume only
when no background, audio-interruption, import, or other pause reason remains.
Entering layout editing must clear touch state and suppress gameplay actions
from editable controls until editing ends.

### FR-016 — normalized input contract

Define a BlueWake-owned complete input snapshot containing:

- signed main-stick and camera-stick axes;
- independent digital L/R click bits and analog L/R trigger pressure;
- button bitset;
- connected/source state;
- optional gyro and pointer values; and
- monotonic sequence/timestamp for diagnostics without recording raw history.

Touch and physical-controller states stay separate and merge predictably.
Brief rising edges must survive until the core polls them. Strongest-magnitude
axes and maximum trigger pressure may win unless gameplay testing proves a
different merge rule. Every runtime implements only a narrow adapter from this
contract; do not make the Apple UI depend on a ModernGekko FIFO.

Ordinary touch L/R buttons emit the digital click plus full analog pressure
unless Wind-Waker-specific testing selects another documented behavior. If a
serialized pipe adapter is selected, it must use dynamically sized encoding,
respect the transport's atomic-write limit, and advance button-edge state only
after the full message succeeds.

### FR-017 — controller lifecycle

- Treat `GCController.controllers` as connection authority on mobile.
- Reconcile launch, connect, disconnect, stale callbacks, sleep/wake, and
  foreground resume into stable player slots.
- Zero-initialize every complete state snapshot.
- Clear held state on disconnect, remap, pause, and core restart.
- Support controller-to-touch handoff without stuck inputs.
- Validate mappings as unique one-to-one assignments; assigning an in-use
  physical button swaps mappings predictably. Provide reset-to-default and
  clear held state after every mapping change.
- Validate standard extended gamepads and document best-effort GC adapters.
- Rumble/haptics must be bounded and stop on disconnect/background.

### FR-018 — diagnostics and support

The three-dot menu must expose a guided “Report a Problem” flow and “Share
Diagnostic Log.” Diagnostics must contain:

- ISO-8601 timestamps and session/correlation ID;
- BlueWake build, dependency lock hash, route, module hash, and game revision;
- platform, hardware model/class, OS, thermal, Low Power, and memory state;
- FPS, emulated speed, frame time, CPU, resident/peak memory, and top-thread
  samples where available;
- current stage, room, scene, story checkpoint, and save transition markers;
- active REL ID/name/sections, link/unlink events, relocations, resolver misses,
  fallback sites/counts, SMC retirement, and last safe guest PC/LR;
- graphics/backend counters, shader/pipeline events, audio underruns, and
  lifecycle/controller events;
- aggregated/deduplicated warning and error signatures; and
- current and previous session summaries.

Privacy and retention requirements:

- byte-bound active logging with rotation/ring behavior;
- bound unique warning/error signatures and all field lengths; summarize
  dropped kinds rather than growing cardinality without limit;
- configurable but bounded retained sessions;
- redact container, home, temp, and source paths;
- exclude disc bytes, extracted assets, save contents, screenshots, signing
  data, device identifiers, and raw input streams;
- preview what will be shared; and
- tests proving redaction and bounds.

Write crash/fatal, lifecycle, REL link/unlink, import activation, save
transition, and core-restart breadcrumbs durably enough to survive a crash.
The guided report asks three short questions—problem, area/context, and
frequency—adds a correlation ID and bounded current/previous technical
sessions, then offers Files/share sheet or a prefilled GitHub issue. Cancel
shares nothing.

SunPad rotates session files but does not currently byte-cap the active
session. BlueWake must add the missing cap.

### FR-019 — settings and migration

- Version every persisted schema.
- Separate product settings, runtime settings, phone layout, tablet layout,
  controller mapping, and active-game-data metadata.
- Migrations must be idempotent, backed up, and tested from every shipped
  version.
- Unknown future fields must not destroy known data.
- Reset actions must name their exact scope.

### FR-020 — packaging and repository audit

Every distributable must be checked for:

- correct platform, architecture, deployment target, and bundle identity;
- prohibited dynamic/JIT entitlements or writable-executable behavior;
- unexpected local/Homebrew paths;
- game data, saves, logs, screenshots, credentials, keys, provisioning, or
  personal paths;
- missing license/notice/SBOM entries;
- archive integrity and deterministic packaging where practical; and
- source/patch reproducibility for applicable licenses.

Mobile packaging also verifies `UIFileSharingEnabled`,
`LSSupportsOpeningDocumentsInPlace`, intended document types, and the absence
of host-absolute paths in runtime/module manifests.

## 12. Non-functional requirements

### NFR-001 — performance

The baseline is original 30 FPS game behavior and 100% emulated/game speed.
Measure separately:

- presented FPS;
- emulated speed ratio;
- median, p95, p99, and worst frame time;
- stutter duration/count;
- AOT versus interpreter work;
- CPU by dispatcher, mapping, graphics, DSP/audio, DVD, and host UI;
- working set, peak, allocation growth, and jetsam margin;
- shader/pipeline compilation; and
- sustained thermal/energy behavior.

Release requires representative field, sailing, dungeon, boss, cutscene, and
UI workloads plus a multi-hour session with no progressive collapse.

P6 must calibrate and lock exact budgets in `docs/archive/status/PERFORMANCE.md` before
P8 device promotion. Unless measured reference behavior justifies a stricter
content-specific exception, release thresholds are:

- expected 30 FPS presentation with mean emulated speed at least 99.5% and
  1st-percentile speed at least 95% during active gameplay;
- gameplay frame-time p95 at most 36.7 ms and p99 at most 50 ms, excluding
  separately labeled original/reference loading transitions;
- no BlueWake-caused hitch over 100 ms more than once per ten active minutes;
- zero interpreter-executed guest instructions across the acceptance corpus;
- no monotonic memory growth, jetsam, or less than 25% measured headroom below
  the process/device limit on the minimum mobile classes;
- no sustained `serious` or `critical` thermal state during a 60-minute
  canonical workload; and
- three-hour soak with no progressive speed, memory, audio, or mapping
  degradation.

If reference behavior itself violates a generic threshold, record an aligned
reference/BlueWake comparison and a scenario-specific budget. Failure to meet
locked budgets on the minimum devices triggers route/performance review.

### NFR-002 — reliability

- No S0/S1 crash, hang, save-loss, systemic visual corruption, or audio
  deadlock in the acceptance campaign.
- Repeated room/stage/REL transitions must not leak mappings or stale state.
- Fifty mobile background/foreground cycles, including mid-cutscene and
  controller changes, must preserve a recoverable state.
- Long-session test must include save, reload, and return to title.

### NFR-003 — reproducibility

- Clean checkout plus dependency lock plus private verified input must
  reproduce generated manifests and builds.
- Generated artifacts carry tool/input/config hashes.
- CI runs all game-data-free tests. Private runners may run authentic-data
  gates without uploading inputs or derived artifacts.

### NFR-004 — observability

Every failure must be localizable to a gate, content ID, subsystem, build,
input revision, and deterministic signature. Logs must be useful without
private data.

### NFR-005 — maintainability

- Apple shell depends on a narrow runtime adapter.
- Game-specific patches are named, documented, tested, and minimal.
- Upstream patches are exportable and replayable.
- No generated source is manually edited without a generator-owned patch
  mechanism and regeneration test.

### NFR-006 — accessibility and UX

- Touch targets, labels, safe areas, contrast, and Dynamic Type for native
  menus must be reviewed.
- Controls must remain usable on supported phone and tablet sizes.
- Controller-only, touch-only, and mixed paths must reach every required menu
  and gameplay action.

### NFR-007 — blocker severity

Gate IDs use `P#`; blocker severity uses `S#` and must not be conflated:

- `S0` — data loss/corruption, privacy/security/provenance breach, or a change
  that could distribute prohibited material. Stop affected work immediately.
- `S1` — deterministic crash/hang, authentic-progression blocker, save failure,
  systemic graphics/audio/input failure, observed release-corpus interpreter
  execution, or unmet minimum-device performance.
- `S2` — major but bounded content/subsystem defect with a viable workaround
  outside required acceptance, significant regression, or supportability gap.
- `S3` — minor visual/audio/UI polish or optional-content defect that does not
  violate another release requirement.

Release requires zero open `S0` or `S1` blocker and explicit disposition of
every `S2`/`S3` item.

## 13. Delivery phases and gates

Phases are evidence-ordered, not calendar promises. A phase may run for many
iterations. The autonomous loop always attacks the earliest unmet gate unless
an independent task can safely advance without masking it.

### P0 — governance, workspace, and reference lock

Deliver:

- this PRD and goal loop accepted as governing documents;
- dependency/reference lock with all current SHAs and licenses;
- documented minimum supported macOS, iOS/iPadOS versions and minimum/current
  Mac, iPhone, and iPad device classes for development and release evidence;
- public/private/generated artifact policy;
- expanded ignore and repository audit rules;
- build/test evidence schemas and gate ledger;
- documented route decision record; and
- baseline game-data-free CI.

Gate P0 passes when a clean checkout can bootstrap public dependencies without
game data, reproduce the environment report, run repository safety checks, and
leave no unknown dependency edits. Changing the support policy later marks
affected platform/performance evidence stale.

### P1 — authoritative input and topology

Deliver:

- non-destructive `GZLE01` validation;
- private extraction/staging pipeline;
- full DOL/REL manifest resolving the count discrepancy;
- hashes and provenance for private inputs;
- public-safe topology summary; and
- synthetic tests for malformed/unsupported inputs.

Gate P1 passes when two clean preparations of the same input produce identical
manifests and no Nintendo-derived artifact is tracked.

### P2 — translator/source census and route spike

Deliver:

- current DolRecomp build/tests on Apple Silicon;
- DOL plus every REL translation audit;
- refreshed TWW matching/source/stub census;
- small comparative spikes for gcrecomp/other credible donors where useful;
- unsupported instruction/relocation/fallback/SMC ledger;
- generated size and build-cost measurements; and
- first route decision update.

Gate P2 passes when every executable image is accounted for and every failure
has a stable signature, classification, and owner. No unexplained omission is
allowed.

### P3 — selected execution artifact and REL correctness

Route A deliverables:

- deterministic composite module generator;
- combined dispatcher and metadata;
- synthetic DOL/REL fixtures covering imports, multiple sections, lifecycle,
  reload, address reuse, indirect calls, and malformed inputs;
- ABI/address coverage verifier; and
- module size/link/memory report.

Route B replacement deliverables:

- deterministic source/object inventory for the DOL and all REL modules;
- a native-source link or explicit staged link plan that preserves every
  module and reports every missing function;
- guest-layout/pointer/endianness/ABI portability tests;
- static or equivalent REL lifecycle model with synthetic reload/address-reuse
  tests;
- generated/public/private data-boundary audit; and
- source/object size, link time, peak memory, and unresolved-behavior report.

Gate P3 passes for the route selected by a recorded decision when all its
synthetic tests pass, regeneration is deterministic, and size/link/memory
thresholds recorded in the gate ledger are met. Non-applicable route-specific
criteria are marked `SUPERSEDED_BY_DECISION` and linked to their replacement;
they are never silently skipped.

### P4 — no-runtime-PowerPC-JIT macOS boot

Deliver:

- minimal macOS runtime host and logging;
- enforced no-runtime-PowerPC-JIT configuration;
- authentic boot through original initialization;
- native/interpreter/REL/timing diagnostics; and
- deterministic boot regression.

Milestones within P4:

1. entry point and original low-level initialization;
2. original logo flow;
3. authentic title screen;
4. file-select interaction;
5. original new-game intro;
6. Outset loaded by original scene/actor code;
7. controllable Link;
8. one indoor/outdoor or room transition; and
9. save, quit, and reload.

Gate P4 passes only at milestone 9 without captured state, forced scene
construction, or runtime PowerPC JIT. This authorizes deeper product work; it
does not prove compatibility.

### P5 — representative runtime compatibility

Deliver a campaign that covers at minimum:

- Outset Island and interiors;
- sea traversal and island transition;
- Forsaken Fortress;
- one complete dungeon and boss;
- inventory, item use, pause, map, sea chart, messages, and file UI;
- save/load before and after a major story flag;
- cutscenes and any video path;
- field, interior, battle, cutscene, and conducting audio;
- death/retry and game over; and
- repeated REL load/unload/address reuse.

Gate P5 passes when the campaign completes without save corruption and a formal
route review classifies every remaining failure as bounded content-specific
work, with no missing architectural subsystem. Audio and input must be active;
“silent prototype” is no longer sufficient.

### P6 — macOS product baseline

Deliver:

- self-contained signed development `.app`;
- native import/settings/diagnostic UX;
- keyboard and controller mappings;
- save backup/export and safe game-data removal;
- package/repository audits;
- canonical macOS compatibility/performance report; and
- start-to-ending candidate build for extended testing.

Gate P6 passes when representative compatibility, packaging, updates, saves,
diagnostics, and sustained original speed all pass on Apple Silicon macOS.

### P7 — shared Apple mobile shell

Deliver:

- BlueWake-owned runtime adapter;
- iOS target hosting the P6 core;
- SunPad-derived visual language and three-dot menu;
- staged Files import and save/data separation;
- normalized touch/controller input and controller lifecycle;
- settings migration and guided diagnostics;
- independent phone/tablet touch defaults; and
- iPhone/iPad Simulator test hosts with synthetic fixtures.

Gate P7 passes when the shell, importer, settings, layouts, lifecycle events,
diagnostics, normal boot, deterministic smoke route, and representative
multi-area gameplay routes pass on both iPhone and iPad Simulator. This proves
Simulator functional integration, not physical gameplay, thermals, ergonomics,
or speed.

### P8 — physical iPhone and iPad gameplay

Deliver on at least one supported physical iPhone and iPad:

- install and in-place update without save loss;
- independent clean import, new game, save creation, restart, and early
  progression on each form factor;
- separate cross-platform boot to and continuation from an authenticated
  macOS save;
- controller gameplay and transition;
- touch-only gameplay and all required actions;
- background/foreground and interruption recovery;
- controller disconnect/reconnect and touch handoff;
- save hash preservation across restart;
- diagnostics export; and
- performance, memory, thermals, and energy report; and
- dated hands-on visual, audible, touch-ergonomics, controller, gameplay, and
  lifecycle acceptance, recorded separately from automated/process evidence.

Gate P8 passes only when both form factors sustain original game speed through
representative workloads. Simulator evidence cannot substitute.

### P9 — full-game compatibility campaign

Deliver:

- a generated coverage catalog of every stage, room, REL, actor profile,
  dungeon, boss, mandatory cutscene, story checkpoint, item class, UI mode,
  audio mode, and save transition present in the supported input;
- named automated or manual scenarios mapped to that catalog;
- a clean new-game-to-ending run;
- representative optional islands, side quests, minigames, collectibles, and
  unusual failure/retry branches;
- multi-hour soak, repeated warp, and lifecycle campaigns;
- regression replay for every fixed S0/S1 failure; and
- unresolved-content report containing no release blockers.

Minimum full-game/input matrix:

- macOS: one clean new-game-to-ending controller playthrough;
- physical iPhone: one clean touch-only new-game-to-ending playthrough;
- physical iPad: one clean touch-only new-game-to-ending playthrough;
- mobile controller: one clean new-game-to-ending playthrough on at least one
  form factor plus chapter-by-chapter controller regression on the other;
- iPhone and iPad Simulator: normal boot and representative routes from every
  story chapter, covering each required subsystem/content class; and
- all platforms: targeted replay of every S0/S1 regression and every
  platform-specific feature.

Automated routes may assist, but dated hands-on checks remain required for
visual/audio correctness, touch ergonomics, controller completeness, and
lifecycle behavior. Simulator coverage never substitutes for physical full
playthroughs.

Gate P9 passes when required coverage is complete on macOS and the mobile
matrix, with no S0/S1 issue and no unexplained gap.

### P10 — release candidate and handoff

Deliver:

- reproducible macOS and unsigned/self-signable iOS packages;
- clean source release with required corresponding source/patches;
- SBOM, licenses, provenance, privacy, support, build, install, and dumping
  guidance;
- artifact audits and download-back hashes where hosted;
- final compatibility/performance report; and
- explicit known limitations.

Gate P10 passes only when packaged artifacts reproduce the tested candidates
and their claims match the evidence. Private/local packaging is the default;
public hosting is outside this gate unless separately authorized and reviewed.

## 14. Test architecture

### 14.1 Tier A — game-data-free on every change

- parsers with synthetic DOL/REL fixtures;
- relocation and dispatch coverage;
- module metadata/ABI validation;
- input mixer, edge latching, mappings, and controller slots;
- touch layout, sparse persistence, safe-area, and migration logic;
- diagnostic redaction, aggregation, bounds, and schema;
- manifest/path safety and repository/package audits;
- settings and save migration with synthetic files; and
- CMake/build-script syntax and deterministic generator tests.

### 14.2 Tier B — private preparation/compile

- exact input validation;
- extraction and topology manifest;
- complete translation/source census;
- generated module compile/link;
- missing target, fallback, relocation, SMC, and size reports; and
- repeatability from a clean generated directory.

### 14.3 Tier C — deterministic runtime smoke

- original boot milestones;
- title/file select;
- fixed new-game input sequence;
- first controllable frame;
- one transition;
- save/reload; and
- expected scene, room, REL, dispatch, audio, and frame-state markers.

### 14.4 Tier D — subsystem differential tests

Use an unmodified reference run from the user's own input as an oracle:

- OS/time/interrupt traces;
- GX state and selected frame digests;
- audio event/buffer fingerprints;
- DVD request ordering/results;
- REL link/unlink/section mappings;
- actor/profile/state milestones;
- save-file hashes and interpreted fields; and
- controller response at deterministic checkpoints.

Reference artifacts remain private and ignored. Do not make a reference
emulator's internal memory dump part of BlueWake initialization.

### 14.5 Tier E — progression regression

Maintain short, medium, and long routes:

- **smoke:** boot to control and one transition;
- **subsystem:** one route for each graphics/audio/save/REL/input feature;
- **chapter:** story checkpoint to checkpoint;
- **soak:** repeated transitions and extended play; and
- **full:** new game to ending.

Automated inputs must synchronize on game milestones rather than wall-clock
delays wherever possible. A route that used a debug warp must have a normal
progression counterpart before release.

### 14.6 Tier F — Apple UI and Simulator

Run both current iPhone and iPad Simulator profiles plus smallest/largest
supported screen classes:

- first run and unsupported input;
- successful/cancelled/failed staged import, same-filename reimport,
  interrupted extraction, manifest failure, failed activation swap, and
  stale-staging recovery;
- confirmed removal of only `GameData`, with pre/post hashes proving saves,
  settings, mappings, and phone/tablet layouts unchanged;
- menu state and restart-required settings;
- every touch control, layout edit, reset, opacity/scale, and accessibility ID;
- phone/tablet preference separation and migrations;
- controller simulation where meaningful;
- synthetic Simulator controllers must not falsely trigger physical-controller
  touch hiding;
- resign-active/background/foreground/memory-warning hooks plus actual core
  pause, bounded startup-window retry, same-process resume, renewed frames,
  neutral input, and audio-session reactivation;
- diagnostic generation/share preview; and
- rotation/safe-area/screenshot regressions.

After shell tests pass, run normal-boot and checkpointed gameplay routes across
multiple stages, rooms, sea transitions, combat, UI, saves, and REL sets on
both iPhone and iPad Simulator. Simulator gameplay expands functional coverage;
it still cannot certify physical-device performance or ergonomics.

Simulator does not certify thermals, jetsam margin, Metal driver behavior,
real GameController lifecycle, audio interruption, haptics, sustained speed,
or physical touch ergonomics.

### 14.7 Tier G — physical devices

Test at least:

- minimum supported iPhone class;
- representative current iPhone;
- minimum supported iPad class;
- representative current iPad;
- controller-only and touch-only;
- wired/wireless controller variants where available;
- Low Power Mode and thermal load;
- audio route/interruption;
- repeated background/foreground;
- in-place update with established save/settings/layout; and
- an extended authentic gameplay session.

Compilation, signing, installation, launch, PID, and log activity are reported
separately from hands-on gameplay acceptance.

## 15. Compatibility coverage model

Do not rely on a hand-written list alone. Generate the coverage universe from
the supported input and TWW symbols, then map human-readable scenarios to it.

Each coverage record includes:

- content ID and friendly name;
- stage/room/layer and story prerequisites;
- active RELs and actor profiles;
- graphics/audio/input/save features touched;
- entry method: normal progression, save fixture, or debug warp;
- expected milestones/digests;
- platforms/form factors tested;
- last passing build and evidence path; and
- open failures.

Coverage categories must include:

- boot, logos, title, file select, new-game intro, credits;
- every required island, sea sector, interior, cave, dungeon, and boss;
- mandatory story actors/events and representative optional actors;
- sailing, combat, targeting, stealth, crawl, climb, swim, items, conducting,
  Picto Box, salvage, shops, mail, maps/charts, and minigames;
- day/night, weather, particles, EFB effects, cutscenes, and UI overlays;
- every item/inventory/message/save/game-over mode;
- audio across music, ambience, streams, SFX, voices, reverb, pause, and
  transitions;
- death, retry, save/reload, suspend/resume, controller reconnect, and data
  reimport; and
- long-session and repeated REL address-reuse behavior.

“Every REL translated” and “every REL observed active” are separate metrics.

## 16. Evidence and gate ledger

Every gate result must record:

- gate/scenario ID;
- status: `NOT_STARTED`, `ACTIVE`, `PASS`, `FAIL`, `BLOCKED`, or `STALE`;
- BlueWake commit and dependency lock hash;
- platform/device/OS/build configuration;
- private input identity alias, never its path;
- exact command or test route;
- start/end time and exit code;
- key metrics and artifact hashes;
- failure signature and blocker link;
- measured facts, inferences, and unresolved questions; and
- next smallest action.

Evidence becomes stale when a relevant runtime, translator, dependency,
platform SDK, input revision, or test oracle changes. Stale evidence remains
history but cannot pass a current gate.

## 17. Risk register

| Risk | Detection | Primary response |
|---|---|---|
| Composite module becomes impractically large/slow | P2/P3 size, link, memory metrics | Function-aware generation, split build, LTO/PGO, alternate translator, or Route B review. |
| REL discovery/mapping does not scale | Per-link/per-frame mapping profile | Direct link/unlink registration and active-section cache. |
| Interpreter fallback dominates | Address/symbol hotness counters | Translator fix, bounded native hook, source reconstruction, or route review. |
| Authentic initialization needs missing behavior | Normal-boot trace diverges before milestone | Differential trace, implement missing service, reconstruct subsystem; never seed captured state. |
| Graphics mismatch | Canonical visual/digest differences | Narrow GX/EFB/vertex/shader correction using licensed donors and reference behavior. |
| JAudio1 incomplete | Silence, drift, deadlock, event mismatch | Port/reconstruct JAudio1 behavior over proven device/DSP boundary; dedicated tests. |
| Save corruption | Hash/schema/reference mismatch | Transactional writes, backups, fault injection, reference interchange. |
| Mobile performance/thermal collapse | P8 sustained profiles | Remove hot fallback, optimize dispatcher/mapping/renderer, reduce safe resolution; reassess minimum device. |
| Generated code/manual patch drift | Regeneration changes or lost edit | Generator-owned patch manifests and clean regeneration CI. |
| Upstream churn | Lock drift or patch conflicts | Pins, update branch, patch rebase report, evidence invalidation. |
| License/provenance failure | SBOM/audit or unclear donor | Quarantine donor, replace cleanly, legal review before distribution. |
| Autonomous loop stalls | Repeated identical signature/no evidence gain | Anti-stall rule, smaller reproducer, new instrumentation, donor review, or route pivot. |

## 18. Autonomous-agent authority and limits

Within this product goal, the implementation agent may:

- download public open-source repositories and documented dependencies;
- create ignored reference checkouts and build caches;
- inspect the user-provided game locally without redistributing it;
- add tests, instrumentation, code, build tools, patches, and documentation;
- build/run local apps and Simulators;
- make coherent checkpoint commits when authorized by its execution context;
  and
- maintain the gate/blocker/evidence ledgers.

The agent must not:

- acquire game data or leaked material on the user's behalf;
- commit private/derived content, saves, captures, credentials, or signing
  material;
- destroy or replace live user data without an inspected recoverable backup;
- weaken tests or silence failures merely to advance a gate;
- publish, release, push, open PRs, or contact third parties without the
  authority available in its task context;
- misstate compilation, Simulator, or process evidence as gameplay/device
  acceptance; or
- repeat an identical failed action indefinitely.

Physical hardware, signing identities, and hands-on visible/audio acceptance
remain user/external gates when they cannot be automated safely.

## 19. Definition of done

BlueWake is complete only when all are true:

1. Every applicable P0–P10 gate and every mandatory replacement gate passes
   with current, reproducible evidence; any `SUPERSEDED_BY_DECISION` criterion
   links to the recorded decision and passing replacement.
2. A clean checkout plus locked public dependencies and supported user input
   reproduce the tested applications.
3. macOS, physical iPhone, and physical iPad boot normally without runtime PPC
   JIT or captured initialization state.
4. The full platform/input matrix in P9 completes with required dungeons,
   bosses, cutscenes, and hands-on acceptance.
5. The compatibility catalog covers required content and representative
   optional systems with no unexplained gap.
6. Controller-only and touch-only mobile play can perform every required
   action and menu operation.
7. Saves, settings, controller mappings, separate phone/tablet layouts, and
   active-data metadata survive migration, failed import, reload, app restart,
   backgrounding, in-place app update, controller changes, and game-data
   replacement; no corruption or unintended reset is observed.
8. Graphics and audio pass canonical comparison and long-session checks.
9. Original game speed is sustained across representative workloads on the
   documented minimum devices, with measured thermal and memory margin.
10. Package and repository audits find no game data, saves, logs, credentials,
    signing material, personal paths, or unexpected dependencies.
11. Licenses, corresponding source obligations, SBOM, provenance, privacy,
    install/build/dumping/support docs, and known limitations are complete.
12. Public claims describe exactly what was tested and do not overstate
    compatibility, performance, or architecture.

## 20. Primary references

- BlueWake reassessment:
  `docs/archive/FEASIBILITY_REASSESSMENT_2026-08-13.md`
- Wind Waker decompilation: <https://github.com/zeldaret/tww>
- Current official progress page: <https://zeldaret.github.io/tww/>
- DolRecomp: <https://github.com/ExpansionPak/DolRecomp>
- ModernGekko: <https://github.com/ExpansionPak/ModernGekko>
- ModernGekko Template:
  <https://github.com/ExpansionPak/ModernGekko-Template>
- RecompCore: <https://github.com/ExpansionPak/RecompCore>
- gcrecomp: <https://github.com/sp00nznet/gcrecomp>
- Wind Waker gcrecomp experiment: <https://github.com/sp00nznet/ww>
- GameCubeRecompiled:
  <https://github.com/KaiserGranatapfel/GameCubeRecompiled>
- Dusklight: <https://github.com/TwilitRealm/dusklight>
- Aurora: <https://github.com/encounter/aurora>
- Decomp Toolkit: <https://github.com/encounter/decomp-toolkit>
- Wind Waker hacking documentation:
  <https://github.com/LagoLunatic/WW-Hacking-Docs>
- Wind Waker Randomizer, useful for mature content/logic knowledge:
  <https://github.com/LagoLunatic/wwrando>
- SunPad local behavioral reference for this planning snapshot:
  `the SunPad checkout` at
  `e43f0ea6b797e5110787171957c9dc3c6213269c`; implementation must replace the
  absolute path with the dependency-lock provenance/transfer contract above.

All revision-dependent claims must be refreshed when the dependency lock
changes. Reference repositories are inputs to engineering judgment, not proof
that BlueWake itself passes a gate.
